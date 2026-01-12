#include <atomic>
#include <iostream>
#include <csignal>
#include <chrono>
#include <gst/gst.h>
#include <thread>
#include <mutex>
#include "json.hpp"
#include "logging.h"
#include "pipelines.h"

using json = nlohmann::json;

StreamingConfig DEFAULT_STREAMING_CONFIG = {
    "192.168.1.100", 8554, 8556, Codec::JPEG, 85, 400000, 1920, 1080, VideoMode::STEREO, 60
};
std::vector<GstElement *> pipelines = {nullptr, nullptr};
std::mutex pipelines_mutex;

std::mutex cfg_mutex;
StreamingConfig desired_cfg = {};
std::atomic<uint64_t> cfg_version{0};
std::atomic<bool> stop_requested{false};

void StopPipeline(GstElement *pipeline) {
    if (pipeline == nullptr) { return; };
    std::cout << "Stopping the pipeline!\n";

    // Set pipeline to NULL state
    gst_element_set_state(pipeline, GST_STATE_NULL);

    // Wait for state change to complete (with 5 second timeout)
    GstState state, pending;
    GstStateChangeReturn ret = gst_element_get_state(pipeline, &state, &pending, 5 * GST_SECOND);

    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to stop pipeline cleanly\n";
    } else if (ret == GST_STATE_CHANGE_ASYNC) {
        std::cerr << "Pipeline stop timed out (still in progress)\n";
    }

    gst_object_unref(pipeline);
}

void SetPipelineToPlayingState(GstElement *pipeline, const std::string &name) {
    const auto ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Unable to set the pipeline to the playing state." << std::endl;
        StopPipeline(pipeline);
        return;
    }

    std::cout << name.c_str() << " playing." << std::endl;

    const auto bus = gst_element_get_bus(pipeline);
    const auto msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                                static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (msg != nullptr) {
        gst_message_unref(msg);
    }

    gst_object_unref(bus);
    StopPipeline(pipeline);
}

GstElement *BuildCameraPipeline(int sensorId, const StreamingConfig &streamingConfig) {
    std::ostringstream oss;

    switch (streamingConfig.codec) {
        case JPEG: oss = GetJpegStreamingPipeline(streamingConfig, sensorId);
            break;
        case H264: oss = GetH264StreamingPipeline(streamingConfig, sensorId);
            break;
        case H265: oss = GetH265StreamingPipeline(streamingConfig, sensorId);
            break;
        case VP8:
        case VP9:
        default:
            throw std::runtime_error("Unsupported codec in this build");
    }

    const std::string side = sensorId == 0 ? "left" : "right";
    const std::string pipelineStr = oss.str();

    std::cout << "=== Building Pipeline for Camera " << sensorId << " (" << side << ") ===\n";
    std::cout << pipelineStr << "\n";
    std::cout << "=== End Pipeline ===\n";

    GstElement *pipeline = gst_parse_launch(pipelineStr.c_str(), nullptr);
    gst_element_set_name(pipeline, ("pipeline_" + side).c_str());

    GstElement *camsrc_ident = gst_bin_get_by_name(GST_BIN(pipeline), "camsrc_ident");
    GstElement *vidconv_ident = gst_bin_get_by_name(GST_BIN(pipeline), "vidconv_ident");
    GstElement *enc_ident = gst_bin_get_by_name(GST_BIN(pipeline), "enc_ident");
    GstElement *rtppay_ident = gst_bin_get_by_name(GST_BIN(pipeline), "rtppay_ident");

    g_signal_connect(camsrc_ident, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);
    g_signal_connect(vidconv_ident, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);
    g_signal_connect(enc_ident, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);
    g_signal_connect(rtppay_ident, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);

    return pipeline;
}

void RunCameraStreamingPipelineDynamic(int sensorId) {
    uint64_t seen_version = 0;

    while (!stop_requested.load()) {
        StreamingConfig cfg;
        {
            std::lock_guard<std::mutex> lk(cfg_mutex);
            cfg = desired_cfg;
            seen_version = cfg_version.load(std::memory_order_relaxed);
            if (seen_version == 0) continue;
        }

        GstElement *pipeline = nullptr;
        try {
            pipeline = BuildCameraPipeline(sensorId, cfg);
        } catch (const std::exception &e) {
            std::cerr << "Build failed: " << e.what() << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        {
            // publish for SignalHandler / debugging
            std::lock_guard<std::mutex> lock(pipelines_mutex);
            pipelines[sensorId] = pipeline;
        }

        if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
            std::cerr << "Unable to set pipeline PLAYING\n";
            StopPipeline(pipeline);
            continue;
        }

        GstBus *bus = gst_element_get_bus(pipeline);
        bool rebuild = false;

        while (!stop_requested.load() && !rebuild) {
            // 100ms poll so updates can be noticed
            GstMessage *msg = gst_bus_timed_pop_filtered(
                bus,
                100 * GST_MSECOND,
                (GstMessageType) (GST_MESSAGE_ERROR | GST_MESSAGE_EOS)
            );

            if (msg) {
                gst_message_unref(msg);
                rebuild = true; // EOS/ERROR -> rebuild (or exit if you prefer)
            }

            if (cfg_version.load(std::memory_order_relaxed) != seen_version) {
                rebuild = true; // config changed
            }
        }

        gst_object_unref(bus);
        StopPipeline(pipeline);

        {
            std::lock_guard<std::mutex> lock(pipelines_mutex);
            pipelines[sensorId] = nullptr;
        }

        // Give camera hardware time to fully release before rebuilding
        // Argus camera service needs time to clean up resources
        if (rebuild && !stop_requested.load()) {
            std::cout << "Waiting for camera " << sensorId << " to fully release...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

int RunCameraStreaming() {
    std::cout << "Streaming driver running; waiting for updates on stdin\n";
    std::thread t0(RunCameraStreamingPipelineDynamic, 0);
    std::thread t1(RunCameraStreamingPipelineDynamic, 1);

    t0.join();
    t1.join();
    return 0;
}

Codec GetCodecFromString(const std::string &codecString) {
    if (codecString == "JPEG") return Codec::JPEG;
    if (codecString == "VP8") return Codec::VP8;
    if (codecString == "VP9") return Codec::VP9;
    if (codecString == "H264") return Codec::H264;
    if (codecString == "H265") return Codec::H265;
    throw std::invalid_argument("Invalid codec passed!");
}

VideoMode GetVideoModeFromString(const std::string &videoModeString) {
    if (videoModeString == "stereo") return VideoMode::STEREO;
    if (videoModeString == "mono") return VideoMode::MONO;
    throw std::invalid_argument("Invalid video mode passed!");
}

StreamingConfig ConfigFromJson(const json &c) {
    StreamingConfig out;
    out.ip = c.at("ip").get<std::string>();
    out.portLeft = c.at("portLeft").get<int>();
    out.portRight = c.at("portRight").get<int>();
    out.codec = GetCodecFromString(c.at("codec").get<std::string>());
    out.encodingQuality = c.at("encodingQuality").get<int>();
    out.bitrate = c.at("bitrate").get<int>();
    out.horizontalResolution = c.at("horizontalResolution").get<int>();
    out.verticalResolution = c.at("verticalResolution").get<int>();
    out.videoMode = GetVideoModeFromString(c.at("videoMode").get<std::string>());
    out.fps = c.at("fps").get<int>();
    return out;
}

std::string CodecToString(Codec codec) {
    switch (codec) {
        case JPEG: return "JPEG";
        case VP8: return "VP8";
        case VP9: return "VP9";
        case H264: return "H264";
        case H265: return "H265";
        default: return "UNKNOWN";
    }
}

std::string VideoModeToString(VideoMode mode) {
    switch (mode) {
        case STEREO: return "STEREO";
        case MONO: return "MONO";
        default: return "UNKNOWN";
    }
}

void DumpConfig(const StreamingConfig &cfg) {
    std::cout << "=== Configuration Dump ===\n";
    std::cout << "  IP Address: " << cfg.ip << "\n";
    std::cout << "  Port Left: " << cfg.portLeft << "\n";
    std::cout << "  Port Right: " << cfg.portRight << "\n";
    std::cout << "  Codec: " << CodecToString(cfg.codec) << "\n";
    std::cout << "  Encoding Quality: " << cfg.encodingQuality << "\n";
    std::cout << "  Bitrate: " << cfg.bitrate << "\n";
    std::cout << "  Resolution: " << cfg.horizontalResolution << "x" << cfg.verticalResolution << "\n";
    std::cout << "  Video Mode: " << VideoModeToString(cfg.videoMode) << "\n";
    std::cout << "  FPS: " << cfg.fps << "\n";
    std::cout << "==========================\n";
}

void SignalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received. Will be stopping " << pipelines.size() <<
            " pipelines!\n";

    std::lock_guard<std::mutex> lock(pipelines_mutex);
    for (auto pipeline: pipelines) {
        StopPipeline(pipeline);
    }

    exit(signum);
}

void ControlLoop() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        try {
            json msg = json::parse(line);
            const std::string cmd = msg.value("cmd", "");

            if (cmd == "update") {
                StreamingConfig cfg = ConfigFromJson(msg.at("config"));
                {
                    std::lock_guard<std::mutex> lk(cfg_mutex);
                    desired_cfg = cfg;
                    cfg_version.fetch_add(1, std::memory_order_relaxed);
                }
                std::cout << "Config updated (version " << cfg_version.load() << ")\n";
                DumpConfig(cfg);
            } else if (cmd == "stop") {
                stop_requested.store(true);
                break;
            }
        } catch (const std::exception &e) {
            std::cerr << "Bad control message: " << e.what() << "\n";
        }
    }

    stop_requested.store(true);
}

int main(int argc, char *argv[]) {
    std::vector<std::string> argList(argv + 1, argv + argc);

    gst_init(nullptr, nullptr);
    gst_debug_set_default_threshold(GST_LEVEL_ERROR);

    signal(SIGTERM, SignalHandler);

    std::thread ctrl(ControlLoop);
    int rc = RunCameraStreaming();

    stop_requested.store(true);
    ctrl.join();

    return rc;
}
