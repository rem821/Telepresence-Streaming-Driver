//
// Created by standa on 24.1.24.
//
#pragma once
#include <fstream>
#include <map>
#include <vector>
#include <exception>
#ifdef JETSON
#include <experimental/filesystem>
#else
#include <filesystem>
#endif

constexpr bool BENCHMARK = false;
constexpr unsigned int SAMPLES = 1000;

inline std::map<std::string, std::vector<long> > timestampsCamera;
inline std::map<std::string, std::vector<long> > timestampsStreaming;
inline std::map<std::string, std::vector<long> > timestampsStreamingFiltered;
inline std::map<std::string, std::vector<long> > timestampsCameraStreaming;
inline std::map<std::string, std::vector<long> > timestampsCameraStreamingFiltered;
inline int64_t cameraStreamingFrameId = -1;
inline std::map<std::string, std::vector<long> > timestampsReceiving;
inline std::map<std::string, std::vector<long> > timestampsReceivingFiltered;

inline bool finishing = false;

inline void SaveLogFilesStreaming() {
    std::ofstream cameraPipeline0File, cameraPipeline1File, streamingPipeline0File, streamingPipeline1File;
    cameraPipeline0File.open("cameraPipeline0Log.txt", std::ios::trunc);
    cameraPipeline1File.open("cameraPipeline1Log.txt", std::ios::trunc);
    streamingPipeline0File.open("streamingPipeline0Log.txt", std::ios::trunc);
    streamingPipeline1File.open("streamingPipeline1Log.txt", std::ios::trunc);

    std::cout << "Will be writing log containing " << timestampsStreaming["pipeline_8554"].size() << " records\n";
    for (int i = 0; i < timestampsStreaming["pipeline_8554"].size(); i = i + 3) {
        streamingPipeline0File <<
                timestampsStreamingFiltered["pipeline_8554"][i] << "," <<
                timestampsStreamingFiltered["pipeline_8554"][i + 1] << "," <<
                timestampsStreamingFiltered["pipeline_8554"][i + 2] << "\n";
    }

    for (int i = 0; i < timestampsStreaming["pipeline_8556"].size(); i = i + 3) {
        streamingPipeline1File <<
                timestampsStreamingFiltered["pipeline_8556"][i] << "," <<
                timestampsStreamingFiltered["pipeline_8556"][i + 1] << "," <<
                timestampsStreamingFiltered["pipeline_8556"][i + 2] << "\n";
    }

    for (int i = 0; i < timestampsCamera["pipeline_camera_0"].size(); i = i + 2) {
        cameraPipeline0File <<
                timestampsCamera["pipeline_camera_0"][i] << "," <<
                timestampsCamera["pipeline_camera_0"][i + 1] << "\n";
    }

    for (int i = 0; i < timestampsCamera["pipeline_camera_1"].size(); i = i + 2) {
        cameraPipeline1File <<
                timestampsCamera["pipeline_camera_1"][i] << "," <<
                timestampsCamera["pipeline_camera_1"][i + 1] << "\n";
    }

    cameraPipeline0File.close();
    cameraPipeline1File.close();
    streamingPipeline0File.close();
    streamingPipeline1File.close();
    std::cout << "Log files written! \n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    throw std::exception();
}

inline void SaveLogFilesReceiving() {
    std::ofstream receivingPipeline0File, receivingPipeline1File;
    receivingPipeline0File.open("receivingPipeline0Log.txt", std::ios::trunc);
    receivingPipeline1File.open("receivingPipeline0Log.txt", std::ios::trunc);

    std::cout << "Will be writing log containing " << timestampsStreaming["pipeline_8554"].size() << " records\n";
    for (int i = 0; i < timestampsStreaming["pipeline_8554"].size(); i = i + 6) {
        receivingPipeline0File <<
                timestampsReceiving["pipeline_8554"][i] << "," <<
                timestampsReceiving["pipeline_8554"][i + 1] << "," <<
                timestampsReceiving["pipeline_8554"][i + 2] << "," <<
                timestampsReceiving["pipeline_8554"][i + 3] << "," <<
                timestampsReceiving["pipeline_8554"][i + 5] << "," <<
                timestampsReceiving["pipeline_8554"][i + 5] << "," <<
                timestampsReceiving["pipeline_8554"][i + 6] << "\n";
    }

    for (int i = 0; i < timestampsStreaming["pipeline_8556"].size(); i = i + 6) {
        receivingPipeline1File <<
                timestampsReceiving["pipeline_8556"][i] << "," <<
                timestampsReceiving["pipeline_8556"][i + 1] << "," <<
                timestampsReceiving["pipeline_8556"][i + 2] << "," <<
                timestampsReceiving["pipeline_8556"][i + 3] << "," <<
                timestampsReceiving["pipeline_8556"][i + 5] << "," <<
                timestampsReceiving["pipeline_8556"][i + 5] << "," <<
                timestampsReceiving["pipeline_8556"][i + 6] << "\n";
    }

    receivingPipeline0File.close();
    receivingPipeline1File.close();
    std::cout << "Log files written! \n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    throw std::exception();
}

static void OnIdentityHandoffCamera(const GstElement *identity, GstBuffer *buffer, gpointer data) {
    if (finishing) { return; }
    using namespace std::chrono;

    const auto tp = std::chrono::time_point_cast<microseconds>(steady_clock::now());
    const auto tmp = std::chrono::duration_cast<microseconds>(tp.time_since_epoch());
    const auto timeMicro = tmp.count();

    const std::string pipelineName = identity->object.parent->name;
    timestampsCamera[pipelineName].emplace_back(timeMicro);

    if (std::string(identity->object.name) == "nvvidconv_identity") {
        const unsigned long d = timestampsCamera[pipelineName].size();

        std::cout << pipelineName << ": frame - " << GstBufferGetFrameIdMeta(buffer) <<
                " nvvidconv: " << timestampsCamera[pipelineName].back() - timestampsCamera[pipelineName][d - 2] << "\n";
    }
}

static void OnIdentityHandoffStreaming(const GstElement *identity, GstBuffer *buffer, gpointer data) {
    if (finishing) { return; }
    using namespace std::chrono;

    const auto tp = std::chrono::time_point_cast<microseconds>(steady_clock::now());
    const auto tmp = std::chrono::duration_cast<microseconds>(tp.time_since_epoch());
    const auto timeMicro = tmp.count();

    const std::string pipelineName = identity->object.parent->name;
    if (std::string(identity->object.name) == "shmsrc_identity") {
        if (!timestampsStreaming[pipelineName].empty()) {
            timestampsStreamingFiltered[pipelineName].emplace_back(timestampsStreaming[pipelineName][0]);
            timestampsStreamingFiltered[pipelineName].emplace_back(timestampsStreaming[pipelineName][1]);
            timestampsStreamingFiltered[pipelineName].emplace_back(timestampsStreaming[pipelineName].back());

            const unsigned long d = timestampsStreamingFiltered[pipelineName].size();
            std::cout << pipelineName << ": frame - " << GstBufferGetFrameIdMeta(buffer) <<
                    " jpegenc: " << timestampsStreamingFiltered[pipelineName][d - 2] - timestampsStreamingFiltered[pipelineName][d - 3] <<
                    ", rtpjpegpay: " << timestampsStreamingFiltered[pipelineName].back() - timestampsStreamingFiltered[pipelineName][d - 2] <<
                    "\n";
        }
        timestampsStreaming[pipelineName].clear();
    }

    timestampsStreaming[pipelineName].emplace_back(timeMicro);

    if (timestampsStreamingFiltered[pipelineName].size() > SAMPLES && BENCHMARK) {
        finishing = true;
        SaveLogFilesStreaming();
    }
}

static void OnIdentityHandoffCameraStreaming(const GstElement *identity, GstBuffer *buffer, gpointer data) {
    if (finishing) { return; }
    using namespace std::chrono;

    const auto tp = std::chrono::time_point_cast<microseconds>(steady_clock::now());
    const auto tmp = std::chrono::duration_cast<microseconds>(tp.time_since_epoch());
    const auto timeMicro = tmp.count();

    const std::string pipelineName = identity->object.parent->name;
    if (
        std::string(identity->object.name) == "nvarguscamerasrc_identity" &&
        !timestampsCameraStreaming[pipelineName].empty() &&
        cameraStreamingFrameId >= 0
    ) {
        timestampsCameraStreamingFiltered[pipelineName].emplace_back(timestampsCameraStreaming[pipelineName][0]);
        timestampsCameraStreamingFiltered[pipelineName].emplace_back(timestampsCameraStreaming[pipelineName][1]);
        timestampsCameraStreamingFiltered[pipelineName].emplace_back(timestampsCameraStreaming[pipelineName][2]);
        timestampsCameraStreamingFiltered[pipelineName].emplace_back(timestampsCameraStreaming[pipelineName].back());

        const unsigned long d = timestampsCameraStreamingFiltered[pipelineName].size();
        std::cout << pipelineName << ": frame - " << 0 <<//GstBufferGetFrameIdMeta(buffer) <<
                " jpegenc: " << timestampsCameraStreamingFiltered[pipelineName][d - 2] - timestampsCameraStreamingFiltered[pipelineName][d - 3] <<
                ", rtpjpegpay: " << timestampsCameraStreamingFiltered[pipelineName].back() - timestampsCameraStreamingFiltered[pipelineName][d - 2] <<
                "\n";

        timestampsCameraStreaming[pipelineName].clear();
        cameraStreamingFrameId = -1;
    }

    //cameraStreamingFrameId = GstBufferGetFrameIdMeta(buffer);
    timestampsCameraStreaming[pipelineName].emplace_back(timeMicro);

    if (timestampsCameraStreamingFiltered[pipelineName].size() > SAMPLES && BENCHMARK) {
        finishing = true;
        //SaveLogFilesStreaming();
    }
}

static void OnIdentityHandoffReceiving(const GstElement *identity, GstBuffer *buffer, gpointer data) {
    if (finishing) { return; }
    using namespace std::chrono;

    const auto tp = std::chrono::time_point_cast<microseconds>(steady_clock::now());
    const auto tmp = std::chrono::duration_cast<microseconds>(tp.time_since_epoch());
    const auto timeMicro = tmp.count();

    const std::string pipelineName = identity->object.parent->name;
    timestampsReceiving[pipelineName].emplace_back(timeMicro);

    if (std::string(identity->object.name) == "videoflip_identity") {
        if (!timestampsReceiving[pipelineName].empty()) {
            const unsigned long s = timestampsReceiving[pipelineName].size();

            timestampsReceivingFiltered[pipelineName].emplace_back(timestampsReceiving[pipelineName][s - 6]);
            timestampsReceivingFiltered[pipelineName].emplace_back(timestampsReceiving[pipelineName][s - 5]);
            timestampsReceivingFiltered[pipelineName].emplace_back(timestampsReceiving[pipelineName][s - 4]);
            timestampsReceivingFiltered[pipelineName].emplace_back(timestampsReceiving[pipelineName][s - 3]);
            timestampsReceivingFiltered[pipelineName].emplace_back(timestampsReceiving[pipelineName][s - 2]);
            timestampsReceivingFiltered[pipelineName].emplace_back(timestampsReceiving[pipelineName][s - 1]);

            const unsigned long d = timestampsReceivingFiltered[pipelineName].size();
            std::cout << pipelineName <<
                    ": frame - " << GstBufferGetFrameIdMeta(buffer) <<
                    ": rtpjpegdepay: " << timestampsReceivingFiltered[pipelineName][d - 5] - timestampsReceivingFiltered[pipelineName][d - 6] <<
                    ", jpegdec: " << timestampsReceivingFiltered[pipelineName][d - 4] - timestampsReceivingFiltered[pipelineName][d - 5] <<
                    ", queue: " << timestampsReceivingFiltered[pipelineName][d - 3] - timestampsReceivingFiltered[pipelineName][d - 4] <<
                    ", videoconvert: " << timestampsReceivingFiltered[pipelineName][d - 2] - timestampsReceivingFiltered[pipelineName][d - 3] <<
                    ", videoflip: " << timestampsReceivingFiltered[pipelineName][d - 1] - timestampsReceivingFiltered[pipelineName][d - 2] << "\n";
        }
        timestampsReceiving[pipelineName].clear();
    }

    if (timestampsReceivingFiltered[pipelineName].size() > SAMPLES && BENCHMARK) {
        finishing = true;
        SaveLogFilesReceiving();
    }
}
