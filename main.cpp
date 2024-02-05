#include <iostream>
#include <chrono>
#include <gst/gst.h>
#include <thread>
#include "meta.h"
#include "logging.h"

const std::string MONO_VIDEO_CAPS_NVMM = "video/x-raw(memory:NVMM),width=(int)1920,height=(int)1080,framerate=(fraction)30/1,format=(string)NV12";
const std::string MONO_VIDEO_CAPS = "video/x-raw,width=(int)1920,height=(int)1080,framerate=(fraction)30/1,format=(string)NV12";
const std::string DEST_IP = "192.168.1.100";

constexpr int PORT_LEFT = 8554;
constexpr int PORT_RIGHT = 8556;

void SetPipelineToPlayingState(GstElement *pipeline, const std::string &name) {
    const auto ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Unable to set the pipeline to the playing state." << std::endl;
        gst_object_unref(pipeline);
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
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void RunCombinedCameraStreamingPipeline(int sensorId, int port) {
    std::ostringstream oss;
#ifdef JETSON
    oss << "nvarguscamerasrc aeantibanding=AeAntibandingMode_Off ee-mode=EdgeEnhancement_Off tnr-mode=NoiseReduction_Off saturation=1.5 sensor-id=" << sensorId
            << " ! " << MONO_VIDEO_CAPS_NVMM << " ! identity name=nvarguscamerasrc_identity ! nvvidconv flip-method=vertical-flip ! " << MONO_VIDEO_CAPS <<
            " ! identity name=meta_identity"
            " ! identity name=nvvidconv_identity ! jpegenc quality=70 idct-method=ifast"
            " ! identity name=jpegenc_identity ! rtpjpegpay"
            " ! identity name=rtpjpegpay_identity ! udpsink host=" << DEST_IP << " sync=false port=" << port;
#else
    oss << "videotestsrc pattern=" << sensorId
            << " ! " << MONO_VIDEO_CAPS <<
            " ! identity name=nvarguscamerasrc_identity"
            " ! identity name=meta_identity"
            " ! clockoverlay"
            " ! identity name=nvvidconv_identity"
            " ! jpegenc quality=70 idct-method=ifast"
            " ! identity name=jpegenc_identity"
            " ! rtpjpegpay"
            " ! identity name=rtpjpegpay_identity"
            " ! udpsink host=" << DEST_IP << " sync=false port=" << port;
#endif

    const auto pipeline = gst_parse_launch(oss.str().c_str(), nullptr);
    const std::string pipelineName = "pipeline_camera_" + std::to_string(sensorId);
    gst_element_set_name(pipeline, pipelineName.c_str());

    GstElement *nvarguscamerasrc_identity = gst_bin_get_by_name(GST_BIN(pipeline), "nvarguscamerasrc_identity");
    GstElement *meta_identity = gst_bin_get_by_name(GST_BIN(pipeline), "meta_identity");
    GstElement *nvvidconv_identity = gst_bin_get_by_name(GST_BIN(pipeline), "nvvidconv_identity");
    GstElement *jpegenc_identity = gst_bin_get_by_name(GST_BIN(pipeline), "jpegenc_identity");
    GstElement *rtpjpegpay_identity = gst_bin_get_by_name(GST_BIN(pipeline), "rtpjpegpay_identity");

    g_signal_connect(nvarguscamerasrc_identity, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);
    g_signal_connect(meta_identity, "handoff", G_CALLBACK(OnIdentityHandoffMeta), nullptr);
    g_signal_connect(nvvidconv_identity, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);
    g_signal_connect(jpegenc_identity, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);
    g_signal_connect(rtpjpegpay_identity, "handoff", G_CALLBACK(OnIdentityHandoffCameraStreaming), nullptr);

    SetPipelineToPlayingState(pipeline, "camera streaming combined pipeline");
}

void RunCameraPipeline(int sensorId, const std::string &shmPath) {
    std::ostringstream oss;
#ifdef JETSON
    oss << "nvarguscamerasrc aeantibanding=AeAntibandingMode_Off ee-mode=EdgeEnhancement_Off tnr-mode=NoiseReduction_Off saturation=1.5 sensor-id=" << sensorId
            << " ! " << MONO_VIDEO_CAPS_NVMM << " ! identity name=nvarguscamerasrc_identity ! nvvidconv flip-method=vertical-flip ! " << MONO_VIDEO_CAPS <<
            " ! identity name=nvvidconv_identity ! shmsink wait-for-connection=false sync=true socket-path=" << shmPath;
#else
    oss << "videotestsrc pattern=" << sensorId
            << " ! " << MONO_VIDEO_CAPS <<
            " ! identity name=nvarguscamerasrc_identity"
            " ! clockoverlay"
            " ! identity name=nvvidconv_identity"
            " ! shmsink wait-for-connection=false sync=true socket-path="
            << shmPath;
#endif

    const auto pipeline = gst_parse_launch(oss.str().c_str(), nullptr);
    const std::string pipelineName = "pipeline_camera_" + std::to_string(sensorId);
    gst_element_set_name(pipeline, pipelineName.c_str());

    GstElement *nvarguscamerasrc_identity = gst_bin_get_by_name(GST_BIN(pipeline), "nvarguscamerasrc_identity");
    GstElement *nvvidconv_identity = gst_bin_get_by_name(GST_BIN(pipeline), "nvvidconv_identity");

    g_signal_connect(nvarguscamerasrc_identity, "handoff", G_CALLBACK(OnIdentityHandoffCamera), nullptr);
    g_signal_connect(nvvidconv_identity, "handoff", G_CALLBACK(OnIdentityHandoffCamera), nullptr);

    SetPipelineToPlayingState(pipeline, "camera src pipeline");
}

void RunCompositionPipeline(const std::string &shm0Path, const std::string &shm1Path, const std::string &outShmPath) {
    // std::ostringstream oss;
    // oss << "shmsrc is-live=true do-timestamp=true socket-path=" << shm0Path
    //     << " ! " << MONO_VIDEO_CAPS
    //     << " ! queue ! compositor name=comp sink_0::xpos=0 sink_0::ypos=0 sink_1::xpos=640 sink_1::ypos=0 ! " << STEREO_VIDEO_CAPS
    //     << " ! videoconvert ! shmsink wait-for-connection=false sync=true socket-path=" << outShmPath
    //     //<< " ! videoconvert ! fpsdisplaysink"
    //     << " shmsrc is-live=true do-timestamp=true socket-path=" << shm1Path
    //     << " ! " << MONO_VIDEO_CAPS
    //     << " ! queue ! comp.sink_1";
    //
    // auto *pipeline = gst_parse_launch(oss.str().c_str(), nullptr);
    //
    // SetPipelineToPlayingState(pipeline, "composition pipeline");
}

void RunStreamingPipeline(const std::string &shmPath, const int port) {
    std::ostringstream oss;
    oss << "shmsrc is-live=true do-timestamp=true socket-path=" << shmPath
            << " ! " << MONO_VIDEO_CAPS
            << " ! identity name=meta_identity ! identity name=shmsrc_identity ! jpegenc quality=70 idct-method=ifast ! identity name=jpegenc_identity ! rtpjpegpay ! identity name=rtpjpegpay_identity ! udpsink host="
            << DEST_IP << " sync=false port=" << port;

    auto *pipeline = gst_parse_launch(oss.str().c_str(), nullptr);
    const std::string pipelineName = "pipeline_" + std::to_string(port);
    gst_element_set_name(pipeline, pipelineName.c_str());

    GstElement *meta_identity = gst_bin_get_by_name(GST_BIN(pipeline), "meta_identity");
    GstElement *shmsrc_identity = gst_bin_get_by_name(GST_BIN(pipeline), "shmsrc_identity");
    GstElement *jpegenc_identity = gst_bin_get_by_name(GST_BIN(pipeline), "jpegenc_identity");
    GstElement *rtpjpegpay_identity = gst_bin_get_by_name(GST_BIN(pipeline), "rtpjpegpay_identity");

    g_signal_connect(meta_identity, "handoff", G_CALLBACK(OnIdentityHandoffMeta), nullptr);
    g_signal_connect(shmsrc_identity, "handoff", G_CALLBACK(OnIdentityHandoffStreaming), nullptr);
    g_signal_connect(jpegenc_identity, "handoff", G_CALLBACK(OnIdentityHandoffStreaming), nullptr);
    g_signal_connect(rtpjpegpay_identity, "handoff", G_CALLBACK(OnIdentityHandoffStreaming), nullptr);

    SetPipelineToPlayingState(pipeline, "streaming pipeline");
}

void RunReceivingPipeline(const int &port) {
    std::ostringstream oss;
    oss << "udpsrc port=" << port
            << " ! application/x-rtp,encoding-name=JPEG,payload=26 ! identity name=udpsrc_identity "
            "! rtpjpegdepay ! identity name=rtpjpegdepay_identity "
            "! jpegdec ! video/x-raw,format=RGB ! identity name=jpegdec_identity "
            "! identity ! identity name=queue_identity "
            "! videoconvert ! identity name=videoconvert_identity "
            "! videoflip method=vertical-flip ! identity name=videoflip_identity "
            "! fpsdisplaysink";

    auto *pipeline = gst_parse_launch(oss.str().c_str(), nullptr);
    const std::string pipelineName = "pipeline_" + std::to_string(port);
    gst_element_set_name(pipeline, pipelineName.c_str());

    GstElement *udpsrc_identity = gst_bin_get_by_name(GST_BIN(pipeline), "udpsrc_identity");
    GstElement *rtpjpegdepay_identity = gst_bin_get_by_name(GST_BIN(pipeline), "rtpjpegdepay_identity");
    GstElement *jpegdec_identity = gst_bin_get_by_name(GST_BIN(pipeline), "jpegdec_identity");
    GstElement *queue_identity = gst_bin_get_by_name(GST_BIN(pipeline), "queue_identity");
    GstElement *videoconvert_identity = gst_bin_get_by_name(GST_BIN(pipeline), "videoconvert_identity");
    GstElement *videoflip_identity = gst_bin_get_by_name(GST_BIN(pipeline), "videoflip_identity");

    g_signal_connect(udpsrc_identity, "handoff", G_CALLBACK(OnIdentityHandoffReceiving), nullptr);
    g_signal_connect(rtpjpegdepay_identity, "handoff", G_CALLBACK(OnIdentityHandoffReceiving), nullptr);
    g_signal_connect(jpegdec_identity, "handoff", G_CALLBACK(OnIdentityHandoffReceiving), nullptr);
    g_signal_connect(queue_identity, "handoff", G_CALLBACK(OnIdentityHandoffReceiving), nullptr);
    g_signal_connect(videoconvert_identity, "handoff", G_CALLBACK(OnIdentityHandoffReceiving), nullptr);
    g_signal_connect(videoflip_identity, "handoff", G_CALLBACK(OnIdentityHandoffReceiving), nullptr);

    SetPipelineToPlayingState(pipeline, "receiving pipeline");
}

int RunStreaming() {
#ifdef JETSON
    std::experimental::filesystem::remove_all("/tmp/cam0");
    std::experimental::filesystem::remove_all("/tmp/cam1");
    std::experimental::filesystem::remove_all("/tmp/camcomp");
#else
    std::filesystem::remove_all("/tmp/cam0");
    std::filesystem::remove_all("/tmp/cam1");
    std::filesystem::remove_all("/tmp/camcomp");
#endif

    const std::string shmCam0Path = "/tmp/cam0";
    std::cout << "Path for Camera 0 shm: " << shmCam0Path.c_str() << "\n";
    std::thread cameraPipelineThread0(RunCameraPipeline, 0, shmCam0Path);

    const std::string shmCam1Path = "/tmp/cam1";
    std::cout << "Path for Camera 1 shm: " << shmCam1Path.c_str() << "\n";
    //std::thread cameraPipelineThread1(RunCameraPipeline, 1, shmCam1Path);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::thread streamingPipelineThread0(RunStreamingPipeline, shmCam0Path, PORT_LEFT);
    //std::thread streamingPipelineThread1(RunStreamingPipeline, shmCam1Path, PORT_RIGHT);

    cameraPipelineThread0.join();
    //cameraPipelineThread1.join();
    streamingPipelineThread0.join();
    //streamingPipelineThread1.join();

    return 0;
}

int RunCombinedCameraStreaming() {
    std::thread cameraStreamingPipelineThread0(RunCombinedCameraStreamingPipeline, 0, PORT_LEFT);
    //std::thread cameraStreamingPipelineThread1(RunCombinedCameraStreamingPipeline, 1, PORT_RIGHT);

    cameraStreamingPipelineThread0.join();
    //cameraStreamingPipelineThread1.join();
}

int RunReceiving() {
    std::thread receivingPipelineThread0(RunReceivingPipeline, PORT_LEFT);
    //std::thread receivingPipelineThread1(RunReceivingPipeline, PORT_RIGHT);

    receivingPipelineThread0.join();
    //receivingPipelineThread1.join();

    return 0;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    gst_debug_set_default_threshold(GST_LEVEL_ERROR);


#ifdef JETSON
    return RunCombinedCameraStreaming();
#else
    return RunCombinedCameraStreaming();
#endif
}
