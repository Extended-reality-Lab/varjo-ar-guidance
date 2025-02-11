// Copyright 2019-2021 Varjo Technologies Oy. All rights reserved.

#include "AppLogic.hpp"

#include <opencv2/opencv.hpp> 

using namespace VarjoExamples;
using namespace std;
using namespace cv;

// globals tracking the contents of both headset eyes to prevent them resetting data before we can read it
Mat leftEyeImage, rightEyeImage;
cv::Mat XYZ; // Depth Map

string str_DistFromMouse = "Collecting information from mouse..."; // stores distance from mouse to objects in

//---------------------------------------------------------------------------

AppLogic::~AppLogic()
{
    // Free data stremer resources
    m_streamer.reset();

    // Shutdown the varjo session. Can't check errors anymore after this.
    LOG_DEBUG("Shutting down Varjo session..");
    varjo_SessionShutDown(m_session);
    m_session = nullptr;
}

bool AppLogic::init()
{
    // Initialize the varjo session.
    LOG_DEBUG("Initializing Varjo session..");
    m_session = varjo_SessionInit();
    if (CHECK_VARJO_ERR(m_session) != varjo_NoError) {
        LOG_ERROR("Creating Varjo session failed.");
        return false;
    }

    // Create data streamer instance from DepthManager child class
    m_streamer = std::make_unique<DepthManager>(m_session, std::bind(&AppLogic::onFrameReceived, this, std::placeholders::_1));

    // Check if Mixed Reality features are available.
    varjo_Bool mixedRealityAvailable = varjo_False;
    varjo_SyncProperties(m_session);
    CHECK_VARJO_ERR(m_session);
    if (varjo_HasProperty(m_session, varjo_PropertyKey_MRAvailable)) {
        mixedRealityAvailable = varjo_GetPropertyBool(m_session, varjo_PropertyKey_MRAvailable);
    }

    // Handle mixed reality availability
    m_colorStreamFormat = m_streamer->getFormat(varjo_StreamType_DistortedColor);

    varjo_SessionSetPriority(m_session, 0);
    m_streamer->setDelayedBufferHandlingEnabled(false);

    // Data stream: YUV
    const auto streamType = varjo_StreamType_DistortedColor;
    const auto streamFormat = m_colorStreamFormat;
    const auto streamChannels = varjo_ChannelFlag_Left | varjo_ChannelFlag_Right;
    varjo_ChannelFlag currentChannels = varjo_ChannelFlag_None;

    if (m_streamer->isStreaming(streamType, streamFormat, currentChannels)) {
        if (currentChannels == streamChannels) {
            // If running stream channels match, match we want to stop it
            m_streamer->stopDataStream(streamType, streamFormat);
            m_streamer->startDataStream(streamType, streamFormat, varjo_ChannelFlag_None);
            }
    } else {
        // No streams running, just start our color data stream
        m_streamer->startDataStream(streamType, streamFormat, streamChannels);
    }

    m_initialized = true;

    return true;
}

void AppLogic::onFrameReceived(const DataStreamer::Frame& frame)
{
    const auto& streamFrame = frame.metadata.streamFrame;

    std::lock_guard<std::mutex> streamLock(m_frameDataMutex); // TODO: Doesn't need to be a switch since I deleted all options. Also is a stream
    if (frame.metadata.channelIndex == varjo_ChannelIndex_First) {
                m_frameData.metadata = streamFrame.metadata.distortedColor;
            }
            m_frameData.colorFrames[static_cast<size_t>(frame.metadata.channelIndex)] = frame;
}


// Return outputs of mouse click on image in specified window
// TODO: Set up to return proper values instead of the randomized current output
void onMouseCV(int action, int x, int y, int, void*)
{
    float depth = XYZ.at<float>(y, x);
    float depth_converted = depth * -10.56818019;
        
    if (action == cv::EVENT_LBUTTONDOWN) {
        std::cout << "Depth at (" << x << ", " << y << "): " << depth << " pixels" << std::endl;
        std::cout << "That's " << depth_converted * 12 << " inches or " << depth_converted << " feet" << std::endl;
    }

    if (action == cv::EVENT_MOUSEMOVE) {
        str_DistFromMouse = "(" + std::to_string(x) + "," + std::to_string(y) + ") = " + std::to_string(depth_converted * 12) + " inches or " + std::to_string(depth_converted) + " feet";
    }
}

void AppLogic::update()
{
    // Handle delayed data stream buffers
    m_streamer->handleDelayedBuffers();

    // Get latest frame data
    FrameData frameData{};
    {
        std::lock_guard<std::mutex> streamLock(m_frameDataMutex);

        frameData.metadata = m_frameData.metadata;

        // Move frame datas
        for (size_t ch = 0; ch < m_frameData.colorFrames.size(); ch++) {
            frameData.colorFrames[ch] = std::move(m_frameData.colorFrames[ch]);
            m_frameData.colorFrames[ch] = std::nullopt;
        }
    }

    // Get latest color frame
    for (size_t ch = 0; ch < frameData.colorFrames.size(); ch++) {
        if (frameData.colorFrames[ch].has_value()) {
            const auto& colorFrame = frameData.colorFrames[ch].value();

            // Skip conversions if the stream has only metadata.
            if (colorFrame.metadata.bufferMetadata.byteSize == 0) {
                continue;
            }

            const auto w = colorFrame.metadata.bufferMetadata.width;
            const auto h = colorFrame.metadata.bufferMetadata.height;
            const auto rowStride = w * 4;

            // Convert to RGBA in full res (this conversion is pixel to pixel, no scaling allowed)
            std::vector<uint8_t> bufferRGBA(rowStride * h);
            DataStreamer::convertToR8G8B8A(colorFrame.metadata.bufferMetadata, colorFrame.data.data(), bufferRGBA.data());

            const auto depthOut = "depthOut";
            cv::namedWindow(depthOut);
            cv::setMouseCallback(depthOut, onMouseCV);

            Mat rgba_mat = cv::Mat(h, w, CV_8UC4, bufferRGBA.data());

            // TODO: Get all 3 functioning in init
            m_streamer->getCalibDat();
            m_streamer->getSGBMDat();
            m_streamer->getUndistortMapDat(w, h);

            if (ch==0){ // Left Eye
                cv::cvtColor(rgba_mat, leftEyeImage, cv::COLOR_RGBA2BGR);
            }
            else if (ch==1){ // Right Eye
                cv::cvtColor(rgba_mat, rightEyeImage, cv::COLOR_RGBA2BGR);
            }
                
            if (!leftEyeImage.empty() && !rightEyeImage.empty() && ch == 1){
                // Actually called in update since we need to update it every frame
                m_streamer->getDepthMap(leftEyeImage, rightEyeImage);

                XYZ = m_streamer->depthMap.clone(); // This is to get onmousecv to work. Lazy because it's only here for debugging. Delete this when done

                // Creates and normalizes a representation of the depth display that's more readable and displayable
                cv::Mat depthDisplay;
                m_streamer->floatDisp.convertTo(depthDisplay, CV_8U, 255.0 / 16); // Normalize
                cv::applyColorMap(depthDisplay, depthDisplay, cv::COLORMAP_JET); // converts disparity map to color for a more readable stream

                // output onmouse depth details
                cv::putText(depthDisplay, str_DistFromMouse, cv::Point(0,h-30), cv::FONT_HERSHEY_COMPLEX , 0.5, CV_RGB(0, 0, 0), 4); //text outline
                cv::putText(depthDisplay, str_DistFromMouse, cv::Point(0,h-30), cv::FONT_HERSHEY_COMPLEX , 0.5, CV_RGB(255, 255, 255), 1); // onscreen text with depth info

                cv::imshow(depthOut, depthDisplay);
                cv::waitKey(1);
            }
        }
    }
}
