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
    oss << "nvarguscamerasrc aeantibanding=AeAntibandingMode_Off ee-mode=EdgeEnhancement_Off tnr-mode=NoiseReduction_Off saturation=1.2 sensor-id=" << sensorId
        << " ! " << "video/x-raw(memory:NVMM),width=(int)" << streamingConfig.horizontalResolution << ",height=(int)" << streamingConfig.verticalResolution
        << ",framerate=(fraction)" << streamingConfig.fps << "/1,format=(string)NV12"
        << " ! identity name=nvarguscamerasrc_identity"
        << " ! nvvidconv flip-method=vertical-flip"
        << " ! identity name=nvvidconv_identity"
        << " ! nvjpegenc quality=" << streamingConfig.encodingQuality << " idct-method=ifast"
        << " ! identity name=enc_ident"
        << " ! rtpjpegpay"
        << " ! identity name=rtppay_ident"
        << " ! udpsink host=" << streamingConfig.ip << " sync=false port=" << port;
    return oss;
}

inline std::ostringstream GetCombinedJpegStreamingPipeline(const StreamingConfig &streamingConfig) {
    std::ostringstream oss;
    
    oss << "nvcompositor name=comp sink_0::ypos=0 sink_1::ypos=" << streamingConfig.verticalResolution
    	<< " ! video/x-raw(memory:NVMM), format=RGBA, width=" << streamingConfig.horizontalResolution << ", height=" << streamingConfig.verticalResolution * 2
    	<< " ! nvvidconv flip-method=vertical-flip ! video/x-raw(memory:NVMM), format=NV12, width=" << streamingConfig.horizontalResolution << ", height=" << streamingConfig.verticalResolution * 2
    	<< " ! identity name=nvvidconv_identity"
    	<< " ! nvjpegenc quality=" << streamingConfig.encodingQuality
    	<< " ! identity name=jpegenc_identity"
    	<< " ! rtpjpegpay"
    	<< " ! identity name=rtpjpegpay_identity"
    	<< " ! udpsink host=" << streamingConfig.ip << " sync=false port=" << streamingConfig.portLeft
    	<< " nvarguscamerasrc sensor-id=1 ! video/x-raw(memory:NVMM), width=" << streamingConfig.horizontalResolution << ", height=" << streamingConfig.verticalResolution << ", format=NV12, framerate=" << streamingConfig.fps << "/1" 
    	<< " ! identity name=nvarguscamerasrc_identity"
    	<< " ! comp.sink_0"
    	<< " nvarguscamerasrc sensor-id=0 ! video/x-raw(memory:NVMM), width=" << streamingConfig.horizontalResolution << ", height=" << streamingConfig.verticalResolution << ", format=NV12, framerate=" << streamingConfig.fps << "/1" 
    	<< " ! comp.sink_1";
    	
    return oss;
}

inline std::ostringstream GetH264StreamingPipeline(const StreamingConfig &streamingConfig, int sensorId) {
    int port = sensorId == 0 ? streamingConfig.portLeft : streamingConfig.portRight;

    std::ostringstream oss;
    oss << "nvarguscamerasrc aeantibanding=AeAntibandingMode_Off ee-mode=EdgeEnhancement_Off tnr-mode=NoiseReduction_Off saturation=1.5 sensor-id=" << sensorId
        << " ! " << "video/x-raw(memory:NVMM),width=(int)" << streamingConfig.horizontalResolution << ",height=(int)" << streamingConfig.verticalResolution
        << ",framerate=(fraction)" << streamingConfig.fps << "/1,format=(string)NV12"
	<< " ! identity name=nvarguscamerasrc_identity"
	<< " ! nvvidconv flip-method=vertical-flip"
        << " ! identity name=nvvidconv_identity"
        << " ! nvv4l2h264enc insert-sps-pps=1 bitrate=10000000 preset-level=3"
        << " ! identity name=enc_ident"
        << " ! rtph264pay"
        << " ! identity name=rtpjpegpay_identity"
	<< " ! rtpstreampay"
        << " ! tcpserversink host=" << "0.0.0.0" << " sync=false port=" << port;
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
            " ! identity name=camsrc_ident"
            " ! clockoverlay"
            " ! videoflip method=vertical-flip"
            " ! identity name=vidconv_ident"
            " ! jpegenc quality=" << streamingConfig.encodingQuality <<
            " ! identity name=enc_ident"
            " ! rtpjpegpay"
            " ! identity name=rtppay_ident"
            " ! udpsink host=" << streamingConfig.ip << " sync=false port=" << port;

    return oss;
}

inline std::ostringstream GetJpegReceivingPipeline(const StreamingConfig &streamingConfig, int sensorId) {
    int port = sensorId == 0 ? streamingConfig.portLeft : streamingConfig.portRight;

    std::ostringstream oss;
    oss << "udpsrc port=" << port << " "
            "! application/x-rtp,encoding-name=JPEG,payload=26 ! identity name=udpsrc_ident "
            "! rtpjpegdepay ! identity name=rtpdepay_ident "
            "! jpegdec ! video/x-raw,format=RGB ! identity name=dec_ident "
            "! identity ! identity name=queue_ident "
            "! videoconvert ! identity name=vidconv_ident "
            "! identity ! identity name=vidflip_ident "
            "! fpsdisplaysink sync=false";
    return oss;
}

inline std::ostringstream GetH264StreamingPipeline(const StreamingConfig &streamingConfig, int sensorId) {
    int port = sensorId == 0 ? streamingConfig.portLeft : streamingConfig.portRight;

    std::ostringstream oss;
    oss << "videotestsrc pattern=" << 0 <<
            " ! " << "video/x-raw,width=(int)" << streamingConfig.horizontalResolution << ",height=(int)" << streamingConfig.verticalResolution << ",framerate=(fraction)"
            << streamingConfig.fps << "/1" <<
            " ! identity name=camsrc_ident"
            " ! clockoverlay"
            " ! videoflip method=vertical-flip"
            " ! identity name=vidconv_ident"
            " ! openh264enc gop-size=1 bitrate=20000 ! h264parse config-interval=-1"
            " ! identity name=enc_ident"
            " ! rtph264pay aggregate-mode=none config-interval=-1"
            " ! identity name=rtppay_ident"
            " ! udpsink host=" << streamingConfig.ip << " sync=false port=" << port;
    return oss;
}

inline std::ostringstream GetH264ReceivingPipeline(const StreamingConfig &streamingConfig, int sensorId) {
    int port = sensorId == 0 ? streamingConfig.portLeft : streamingConfig.portRight;

    std::ostringstream oss;
    oss << "udpsrc port=" << port << " " <<
            "! application/x-rtp, media=video, clock-rate=90000, payload=96 ! identity name=udpsrc_ident "
            "! rtph264depay ! identity name=rtpdepay_ident "
            "! avdec_h264 ! identity name=dec_ident "
            "! queue ! identity name=queue_ident "
            "! videoconvert ! identity name=vidconv_ident "
            "! identity ! identity name=vidflip_ident "
            "! fpsdisplaysink sync=false";
    return oss;
}

#endif
