// Copyright 2019-2021 Varjo Technologies Oy. All rights reserved.

#pragma once

#include "DepthManager.hpp"
#include "ProtobufFrameSerializer.hpp"
#include "ZmqSender.hpp"
#include "frame.pb.h"

//! Application logic class
class AppLogic
{
public:
    struct General {
        double frameTime{0.0f};   //!< Current frame time
        int64_t frameCount{0};    //!< Current frame count
        bool mrAvailable{false};  //!< Mixed reality available flag
    };
public:
    //! Constructor
    AppLogic() = default;

    //! Destruictor
    ~AppLogic();

    // Disable copy and assign
    AppLogic(const AppLogic& other) = delete;
    AppLogic(const AppLogic&& other) = delete;
    AppLogic& operator=(const AppLogic& other) = delete;
    AppLogic& operator=(const AppLogic&& other) = delete;

    //! Initialize application
    bool init();

    //! Update application
    void update();

    //! Return true is app has been initialized successfully
    bool isInitialized() const { return m_initialized; }

    //! Return data streamer instance
    VarjoExamples::DataStreamer& getStreamer() const { return *m_streamer; }

private:
    //! Handles new color stream frames from the data streamer (run in DataStreamer's worker thread)
    void onFrameReceived(const VarjoExamples::DataStreamer::Frame& frame);

private:
    bool m_initialized{false};                                   //!< App initialized flag
    varjo_Session* m_session{nullptr};

    std::unique_ptr<VarjoExamples::DepthManager> m_streamer;  //!< Data streamer instance
    ProtobufFrameSerializer m_protoframe;
    ZmqSender m_sender{"tcp://localhost:5555"};

    struct FrameData {
        std::optional<varjo_DistortedColorFrameMetadata> metadata;                     //!< Color stream metadata
        std::array<std::optional<VarjoExamples::DataStreamer::Frame>, 2> colorFrames;  //!< Color stream stereo frames
    };
    FrameData m_frameData;        //!< Latest frame data
    std::mutex m_frameDataMutex;  //!< Mutex for locking frame data

    varjo_TextureFormat m_colorStreamFormat{varjo_TextureFormat_INVALID};  //!< Texture format for color stream
};
