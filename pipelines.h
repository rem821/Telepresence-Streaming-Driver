//
// Created by standa on 28.8.24.
//
#pragma once

#include <iostream>

enum Codec {
    JPEG, VP8, VP9, H264, H265
};

enum VideoMode {
    STEREO, MONO
};

struct StreamingConfig {
    std::string ip{};
    int portLeft{};
    int portRight{};
    Codec codec{}; //TODO: Implement different codecs
    int encodingQuality{};
    int bitrate{}; //TODO: Implement rate control
    int horizontalResolution{}, verticalResolution{}; //TODO: Restrict to specific supported resolutions
    VideoMode videoMode{};
    int fps{};
};


#ifdef JETSON

inline std::ostringstream GetJpegStreamingPipeline(const StreamingConfig &streamingConfig, int sensorId) {
    int port = sensorId == 0 ? streamingConfig.portLeft : streamingConfig.portRight;

    std::ostringstream oss;
    oss << "nvarguscamerasrc aeantibanding=AeAntibandingMode_Off ee-mode=EdgeEnhancement_Off tnr-mode=NoiseReduction_Off saturation=1.5 sensor-id=" << sensorId
        << " ! " << "video/x-raw(memory:NVMM),width=(int)" << streamingConfig.horizontalResolution << ",height=(int)" << streamingConfig.verticalResolution
        << ",framerate=(fraction)" << streamingConfig.fps << "/1,format=(string)NV12"
        << " ! identity name=nvarguscamerasrc_identity"
        << " ! nvvidconv flip-method=none"
        << " ! identity name=nvvidconv_identity"
        << " ! nvjpegenc quality=" << streamingConfig.encodingQuality << " idct-method=ifast"
        << " ! identity name=jpegenc_identity"
        << " ! rtpjpegpay"
        << " ! identity name=rtpjpegpay_identity"
        << " ! udpsink host=" << streamingConfig.ip << " sync=false port=" << port;
    return oss;
}

inline std::ostringstream GetH264StreamingPipeline(const StreamingConfig &streamingConfig, int sensorId) {
    int port = sensorId == 0 ? streamingConfig.portLeft : streamingConfig.portRight;

    std::ostringstream oss;
    oss << "nvarguscamerasrc aeantibanding=AeAntibandingMode_Off ee-mode=EdgeEnhancement_Off tnr-mode=NoiseReduction_Off saturation=1.5 sensor-id=" << sensorId
        << " ! " << "video/x-raw(memory:NVMM),width=(int)" << streamingConfig.horizontalResolution << ",height=(int)" << streamingConfig.verticalResolution
        << ",framerate=(fraction)" << streamingConfig.fps << "/1,format=(string)NV12"
        << " ! identity name=nvarguscamerasrc_identity"
        << " ! nvvidconv flip-method=none"
        << " ! identity name=nvvidconv_identity"
        << " ! nvv4l2h264enc insert-sps-pps=1 bitrate=10000000 preset-level=3"
        << " ! identity name=jpegenc_identity"
        << " ! rtph264pay"
        << " ! identity name=rtpjpegpay_identity"
        << " ! udpsink host=" << streamingConfig.ip << " sync=false port=" << port;
    return oss;
}

inline std::ostringstream GetJpegReceivingPipeline(const StreamingConfig &streamingConfig, int sensorId) { return std::ostringstream{}; }

inline std::ostringstream GetH264ReceivingPipeline(const StreamingConfig &streamingConfig, int sensorId) { return std::ostringstream{}; }

#else

inline std::ostringstream GetJpegStreamingPipeline(const StreamingConfig &streamingConfig, int sensorId) {
    int port = sensorId == 0 ? streamingConfig.portLeft : streamingConfig.portRight;

    std::ostringstream oss;
    oss << "videotestsrc pattern=" << 0 <<
        " ! " << "video/x-raw,width=(int)" << streamingConfig.horizontalResolution << ",height=(int)" << streamingConfig.verticalResolution << ",framerate=(fraction)"
        << streamingConfig.fps << "/1,format=(string)NV12" <<
        " ! identity name=nvarguscamerasrc_identity"
        " ! clockoverlay"
        " ! identity name=nvvidconv_identity"
        " ! jpegenc quality=" << streamingConfig.encodingQuality <<
        " ! identity name=jpegenc_identity"
        " ! rtpjpegpay"
        " ! identity name=rtpjpegpay_identity"
        " ! udpsink host=" << streamingConfig.ip << " sync=false port=" << port;

    return oss;
}

inline std::ostringstream GetJpegReceivingPipeline(const StreamingConfig &streamingConfig, int sensorId) {
    int port = sensorId == 0 ? streamingConfig.portLeft : streamingConfig.portRight;

    std::ostringstream oss;
    oss << "udpsrc port=" << port << " "
        "! application/x-rtp,encoding-name=JPEG,payload=26 ! identity name=udpsrc_identity "
        "! rtpjpegdepay ! identity name=rtpjpegdepay_identity "
        "! jpegdec ! video/x-raw,format=RGB ! identity name=jpegdec_identity "
        "! identity ! identity name=queue_identity "
        "! videoconvert ! identity name=videoconvert_identity "
        "! identity ! identity name=videoflip_identity "
        "! fpsdisplaysink sync=false";
    return oss;
}

inline std::ostringstream GetH264StreamingPipeline(const StreamingConfig &streamingConfig, int sensorId) {
    int port = sensorId == 0 ? streamingConfig.portLeft : streamingConfig.portRight;

    std::ostringstream oss;
    oss << "videotestsrc pattern=" << 0 <<
        " ! " << "video/x-raw,width=(int)" << streamingConfig.horizontalResolution << ",height=(int)" << streamingConfig.verticalResolution << ",framerate=(fraction)"
        << streamingConfig.fps << "/1,format=(string)I420" <<
        " ! identity name=nvarguscamerasrc_identity"
        " ! clockoverlay"
        " ! videoflip method=vertical-flip"
        " ! identity name=nvvidconv_identity"
        " ! openh264enc"
        " ! identity name=jpegenc_identity"
        " ! rtph264pay aggregate-mode=none"
        " ! identity name=rtpjpegpay_identity"
        " ! udpsink host=" << streamingConfig.ip << " sync=false port=" << port;
    return oss;
}

inline std::ostringstream GetH264ReceivingPipeline(const StreamingConfig &streamingConfig, int sensorId) {
    int port = sensorId == 0 ? streamingConfig.portLeft : streamingConfig.portRight;

    std::ostringstream oss;
    oss << "udpsrc port=" << port << " " <<
        "! application/x-rtp, media=video, clock-rate=90000, payload=96 ! identity name=udpsrc_identity "
        "! rtph264depay ! identity name=rtpjpegdepay_identity "
        "! avdec_h264 ! identity name=jpegdec_identity "
        "! queue ! identity name=queue_identity "
        "! videoconvert ! identity name=videoconvert_identity "
        "! identity ! identity name=videoflip_identity "
        "! fpsdisplaysink sync=false";
    return oss;
}

#endif
