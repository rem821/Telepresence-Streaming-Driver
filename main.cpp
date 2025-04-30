#include <iostream>
#include <csignal>
#include <chrono>
#include <gst/gst.h>
#include <thread>
#include <mutex>
#include "logging.h"
#include "pipelines.h"

StreamingConfig DEFAULT_STREAMING_CONFIG = {"192.168.1.100", 8554, 8556, Codec::JPEG, 85, 400000, 1920, 1080, VideoMode::STEREO, 60};
std::vector<GstElement *> pipelines = {nullptr, nullptr};
std::mutex pipelines_mutex;

void StopPipeline(GstElement *pipeline) {
    std::cout << "Stopping the pipeline!\n";
    gst_element_set_state(pipeline, GST_STATE_NULL);
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
    const auto msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (msg != nullptr) {
        gst_message_unref(msg);
    }
    gst_message_unref(msg);
    gst_object_unref(bus);
    StopPipeline(pipeline);
}

void RunCameraStreamingPipeline(const int sensorId, const StreamingConfig &streamingConfig) {
    std::ostringstream oss;

    switch (streamingConfig.codec) {
        case JPEG:
            oss = GetJpegStreamingPipeline(streamingConfig, sensorId);
            break;
        case VP8:
            break;
        case VP9:
            break;
        case H264:
            oss = GetH264StreamingPipeline(streamingConfig, sensorId);
            break;
        case H265:
            break;
    }

    {
        std::lock_guard<std::mutex> lock(pipelines_mutex);
        pipelines[sensorId] = gst_parse_launch(oss.str().c_str(), nullptr);
    }

    const auto pipeline = pipelines[sensorId];
    const std::string side = sensorId == 0 ? "left" : "right";
    const std::string pipelineName = "pipeline_" + side;
    gst_element_set_name(pipeline, pipelineName.c_str());

    GstElement *camsrc_ident = gst_bin_get_by_name(GST_BIN(pipeline), "camsrc_ident");
    GstElement *vidconv_ident = gst_bin_get_by_name(GST_BIN(pipeline), "vidconv_ident");
    GstElement *enc_ident = gst_bin_get_by_name(GST_BIN(pipeline), "enc_ident");
    GstElement *rtppay_ident = gst_bin_get_by_name(GST_BIN(pipeline), "rtppay_ident");

    g_signal_connect(camsrc_ident, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);
    g_signal_connect(vidconv_ident, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);
    g_signal_connect(enc_ident, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);
    g_signal_connect(rtppay_ident, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);

    SetPipelineToPlayingState(pipeline, "camera streaming pipeline");
}

void RunCombinedCameraStreamingPipeline(const StreamingConfig &streamingConfig) {
    std::ostringstream oss;
    oss = GetCombinedJpegStreamingPipeline(streamingConfig);
    pipelines[0] = gst_parse_launch(oss.str().c_str(), nullptr);
    
    gst_element_set_name(pipelines[0], "combined_pipeline");
    
    GstElement *nvarguscamerasrc_identity = gst_bin_get_by_name(GST_BIN(pipelines[0]), "nvarguscamerasrc_identity");
    GstElement *nvvidconv_identity = gst_bin_get_by_name(GST_BIN(pipelines[0]), "nvvidconv_identity");
    GstElement *jpegenc_identity = gst_bin_get_by_name(GST_BIN(pipelines[0]), "jpegenc_identity");
    GstElement *rtpjpegpay_identity = gst_bin_get_by_name(GST_BIN(pipelines[0]), "rtpjpegpay_identity");

    g_signal_connect(nvarguscamerasrc_identity, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);
    g_signal_connect(nvvidconv_identity, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);
    g_signal_connect(jpegenc_identity, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);
    g_signal_connect(rtpjpegpay_identity, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);

    std::cout << "Parsed pipeline: " << oss.str().c_str() << std::endl;

    SetPipelineToPlayingState(pipelines[0], "combined camera streaming pipeline");
}

void RunReceivingPipeline(const int sensorId, const StreamingConfig &streamingConfig) {
    std::ostringstream oss;

    switch (streamingConfig.codec) {
        case JPEG:
            oss = GetJpegReceivingPipeline(streamingConfig, sensorId);
            break;
        case VP8:
            break;
        case VP9:
            break;
        case H264:
            oss = GetH264ReceivingPipeline(streamingConfig, sensorId);
            break;
        case H265:
            break;
    }

    {
        std::lock_guard<std::mutex> lock(pipelines_mutex);
        pipelines[sensorId] = gst_parse_launch(oss.str().c_str(), nullptr);
    }

    const auto pipeline = pipelines[sensorId];
    const std::string side = sensorId == 0 ? "left" : "right";
    const std::string pipelineName = "pipeline_" + side;
    gst_element_set_name(pipeline, pipelineName.c_str());

    GstElement *udpsrc_ident = gst_bin_get_by_name(GST_BIN(pipeline), "udpsrc_ident");
    GstElement *rtpdepay_ident = gst_bin_get_by_name(GST_BIN(pipeline), "rtpdepay_ident");
    GstElement *dec_ident = gst_bin_get_by_name(GST_BIN(pipeline), "dec_ident");
    GstElement *queue_ident = gst_bin_get_by_name(GST_BIN(pipeline), "queue_ident");
    GstElement *vidconv_ident = gst_bin_get_by_name(GST_BIN(pipeline), "vidconv_ident");
    GstElement *vidflip_ident = gst_bin_get_by_name(GST_BIN(pipeline), "vidflip_ident");

    g_signal_connect(udpsrc_ident, "handoff", G_CALLBACK(OnIdentityHandoffReceiving), nullptr);
    g_signal_connect(rtpdepay_ident, "handoff", G_CALLBACK(OnIdentityHandoffReceiving), nullptr);
    g_signal_connect(dec_ident, "handoff", G_CALLBACK(OnIdentityHandoffReceiving), nullptr);
    g_signal_connect(queue_ident, "handoff", G_CALLBACK(OnIdentityHandoffReceiving), nullptr);
    g_signal_connect(vidconv_ident, "handoff", G_CALLBACK(OnIdentityHandoffReceiving), nullptr);
    g_signal_connect(vidflip_ident, "handoff", G_CALLBACK(OnIdentityHandoffReceiving), nullptr);

    SetPipelineToPlayingState(pipeline, "receiving pipeline");
}

int RunCameraStreaming(const StreamingConfig &streamingConfig) {
#ifdef COMBINED_STREAMING
    if(streamingConfig.videoMode == VideoMode::STEREO) {
        RunCombinedCameraStreamingPipeline(streamingConfig);
    } else {
        RunCameraStreamingPipeline(0, streamingConfig);
    }
#else
    std::thread cameraPipelineThread0(RunCameraStreamingPipeline, 0, streamingConfig);
    if (streamingConfig.videoMode == VideoMode::STEREO) {
        std::thread cameraPipelineThread1(RunCameraStreamingPipeline, 1, streamingConfig);
        cameraPipelineThread1.join();
    }

    cameraPipelineThread0.join();
#endif
    
    return 0;
}

int RunReceiving(const StreamingConfig &streamingConfig) {
    std::thread receivingPipelineThread0(RunReceivingPipeline, 0, streamingConfig);
    if (streamingConfig.videoMode == VideoMode::STEREO) {
        std::thread receivingPipelineThread1(RunReceivingPipeline, 1, streamingConfig);
        receivingPipelineThread1.join();
    }

    receivingPipelineThread0.join();

    return 0;
}

int RunBoth(const StreamingConfig &streamingConfig) {
    std::thread streamingThread(RunCameraStreaming, streamingConfig);
    std::thread receivingThread(RunReceiving, streamingConfig);
    streamingThread.join();
    receivingThread.join();

    return 0;
}

Codec GetCodecFromArg(std::string &codecString) {
    if (codecString == "JPEG") {
        return Codec::JPEG;
    } else if (codecString == "VP8") {
        return Codec::VP8;
    } else if (codecString == "VP9") {
        return Codec::VP9;
    } else if (codecString == "H264") {
        return Codec::H264;
    } else if (codecString == "H265") {
        return Codec::H265;
    } else {
        throw std::invalid_argument("Invalid codec passed!");
    }
}

VideoMode GetVideoModeFromArg(std::string &videoModeString) {
    if (videoModeString == "stereo") {
        return VideoMode::STEREO;
    } else if (videoModeString == "mono") {
        return VideoMode::MONO;
    } else {
        throw std::invalid_argument("Invalid video mode passed!");
    }
}

void SignalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received. Will be stopping " << pipelines.size() << " pipelines!\n";

    std::lock_guard<std::mutex> lock(pipelines_mutex);
    for (auto pipeline: pipelines) {
        StopPipeline(pipeline);
    }

    exit(signum);
}

int main(int argc, char *argv[]) {
    std::vector<std::string> argList(argv + 1, argv + argc);

    StreamingConfig streamingConfig;
    if (argList.size() == 10) {
        streamingConfig.ip = argList[0];
        streamingConfig.portLeft = std::stoi(argList[1]);
        streamingConfig.portRight = std::stoi(argList[2]);
        streamingConfig.codec = GetCodecFromArg(argList[3]);
        streamingConfig.encodingQuality = std::stoi(argList[4]);
        streamingConfig.bitrate = std::stoi(argList[5]);
        streamingConfig.horizontalResolution = std::stoi(argList[6]);
        streamingConfig.verticalResolution = std::stoi(argList[7]);
        streamingConfig.videoMode = GetVideoModeFromArg(argList[8]);
        streamingConfig.fps = std::stoi(argList[9]);

        std::cout << "Telepresence streaming driver has received the following configuration arguments:"
                  << "\n  1. Destination IP: " << streamingConfig.ip
                  << "\n  2. Port Left: " << streamingConfig.portLeft
                  << "\n  3. Port Right: " << streamingConfig.portRight
                  << "\n  4. Codec: " << streamingConfig.codec
                  << "\n  5. Encoding Quality: " << streamingConfig.encodingQuality << "%"
                  << "\n  6. Target Bitrate: " << streamingConfig.bitrate
                  << "\n  7. Horizontal Resolution: " << streamingConfig.horizontalResolution
                  << "\n  8. Vertical Resolution: " << streamingConfig.verticalResolution
                  << "\n  9. Video Mode (stereo/mono): " << streamingConfig.videoMode
                  << "\n  10. FPS: " << streamingConfig.fps << "\n";
    } else if (argList.empty()) {
        streamingConfig = DEFAULT_STREAMING_CONFIG;
        std::cout << "Telepresence streaming driver has been initialized with the default configuration arguments\n";
    } else {
        std::cerr << "Incorrect number of input parameters!"
                     "\nFor the correct usage pass the configuration arguments in the following order:"
                     "\n  1. Destination IP"
                     "\n  2. Port Left"
                     "\n  3. Port Right"
                     "\n  4. Codec"
                     "\n  5. Encoding Quality [%]"
                     "\n  6. Target Bitrate"
                     "\n  7. Horizontal Resolution"
                     "\n  8. Vertical Resolution"
                     "\n  9. Video Mode (stereo/mono)"
                     "\n  10. FPS \n";
        return 1;
    }

    gst_init(nullptr, nullptr);
    gst_debug_set_default_threshold(GST_LEVEL_ERROR);

    signal(SIGTERM, SignalHandler);

#ifdef STREAMING
    return RunCameraStreaming(streamingConfig);
#else
    return RunReceiving(streamingConfig);
#endif
}
