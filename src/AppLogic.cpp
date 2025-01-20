// Copyright 2019-2021 Varjo Technologies Oy. All rights reserved.

#include "AppLogic.hpp"

#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <set>
#include <thread>

#include <Varjo.h>
#include <Varjo_events.h>
#include <Varjo_mr.h>
#include <Varjo_d3d11.h>
#include <glm/gtx/matrix_decompose.hpp>

#include "D3D11MultiLayerView.hpp"

#include <stdio.h>
#include <string>
#include <iostream>
#include "../include_experimental/Varjo_mr_experimental.h" // For Meshing

#include <opencv2/core/core.hpp> // blur img
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/ccalib/omnidir.hpp> // Omnidirectional rectification
#include <opencv2/calib3d/calib3d.hpp> // disparity map algorithms
#include <opencv2/core/core_c.h>
#include <opencv2/core/types_c.h> // cvPoint2D64f datatype for calculating q matrix

// VarjoExamples namespace contains simple example wrappers for using Varjo API features.
// These are only meant to be used in SDK example applications. In your own application,
// use your own production quality integration layer.
using namespace VarjoExamples;
using namespace std;
using namespace cv;

// globals tracking the contents of both headset eyes. Necessary or they'll reset every frame
Mat leftEyeImage, rightEyeImage, GrayL, GrayR;
cv::Mat disMap;
cv::Mat kL, dL, xiL; // left cam intrinsics
cv::Mat kR, dR, xiR; // right cam intrinsics
cv::Mat R, T; // cam rotation and translation
cv::Mat XYZ; // Depth Map

bool trackbarsCreated = 0;
string str_DistFromMouse = "Collecting information from mouse..."; // stores distance from mouse to objects in

//---------------------------------------------------------------------------

AppLogic::~AppLogic()
{
    // Free camera manager resources
    m_camera.reset();

    // Free data stremer resources
    m_streamer.reset();

    // Free scene resources
    m_scene.reset();

    // Free view resources
    m_varjoView.reset();

    // Free renderer resources
    m_renderer.reset();

    // Shutdown the varjo session. Can't check errors anymore after this.
    LOG_DEBUG("Shutting down Varjo session..");
    varjo_SessionShutDown(m_session);
    m_session = nullptr;
}

bool AppLogic::init(GfxContext& context)
{
    // Initialize the varjo session.
    LOG_DEBUG("Initializing Varjo session..");
    m_session = varjo_SessionInit();
    if (CHECK_VARJO_ERR(m_session) != varjo_NoError) {
        LOG_ERROR("Creating Varjo session failed.");
        return false;
    }

    {
        // Get graphics adapter used by Varjo session
        auto dxgiAdapter = D3D11MultiLayerView::getAdapter(m_session);

        // Init graphics
        context.init(dxgiAdapter.Get());

        // Create D3D11 renderer instance.
        auto d3d11Renderer = std::make_unique<D3D11Renderer>(dxgiAdapter.Get());

        // Create varjo multi layer view
        m_varjoView = std::make_unique<D3D11MultiLayerView>(m_session, *d3d11Renderer);

        // Store as generic renderer
        m_renderer = std::move(d3d11Renderer);
    }

    // Create scene instance
    m_scene = std::make_unique<MRScene>(*m_renderer);

    // Create data streamer instance
    m_streamer = std::make_unique<DataStreamer>(m_session, std::bind(&AppLogic::onFrameReceived, this, std::placeholders::_1));

    // Create mixed reality camera manager instance
    m_camera = std::make_unique<CameraManager>(m_session);

    // Check if Mixed Reality features are available.
    varjo_Bool mixedRealityAvailable = varjo_False;
    varjo_SyncProperties(m_session);
    CHECK_VARJO_ERR(m_session);
    if (varjo_HasProperty(m_session, varjo_PropertyKey_MRAvailable)) {
        mixedRealityAvailable = varjo_GetPropertyBool(m_session, varjo_PropertyKey_MRAvailable);
    }

    // Handle mixed reality availability
    onMixedRealityAvailable(mixedRealityAvailable == varjo_True, false);

    if (mixedRealityAvailable == varjo_True) {
        LOG_INFO("Varjo Mixed Reality features available!");

        // Optionally reset camera at startup
        constexpr bool resetCameraAtStart = true;
        if (resetCameraAtStart) {
            // Reset camera properties to defaults
            m_camera->resetPropertiesToDefaults();

            // Set camera exposure and white balance to auto
            m_camera->setAutoMode(varjo_CameraPropertyType_ExposureTime);
            m_camera->setAutoMode(varjo_CameraPropertyType_WhiteBalance);
        }

    } else {
        LOG_WARNING("Varjo Mixed Reality features not available!");
    }

    m_initialized = true;

    return true;
}

void AppLogic::setVideoRendering(bool enabled)
{
    varjo_MRSetVideoRender(m_session, enabled ? varjo_True : varjo_False);
    if (CHECK_VARJO_ERR(m_session) == varjo_NoError) {
        LOG_INFO("Video rendering: %s", enabled ? "ON" : "OFF");
    }
    m_appState.options.videoRenderingEnabled = enabled;
}

void AppLogic::setState(const AppState& appState, bool force)
{
    // Store previous state and set new one
    const auto prevState = m_appState;
    m_appState = appState;

    // Client priority
    if (force || appState.options.clientPriority != prevState.options.clientPriority) {
        varjo_SessionSetPriority(m_session, appState.options.clientPriority);
        if (CHECK_VARJO_ERR(m_session) == varjo_NoError) {
            LOG_INFO("Client priority: %i", appState.options.clientPriority);
        }
    }
    // Check for mixed reality availability
    if (!m_appState.general.mrAvailable) {
        // Toggle video rendering off
        if (m_appState.options.videoRenderingEnabled) {
            setVideoRendering(false);
        }
        return;
    }

    // Video rendering
    if (force || appState.options.videoRenderingEnabled != prevState.options.videoRenderingEnabled) {
        setVideoRendering(appState.options.videoRenderingEnabled);
    }

    // Video depth estimation
    if (force || appState.options.VideoDepthEstimationEnabled != prevState.options.VideoDepthEstimationEnabled) {
        varjo_MRSetVideoDepthEstimation(m_session, appState.options.VideoDepthEstimationEnabled ? varjo_True : varjo_False);
        if (CHECK_VARJO_ERR(m_session) == varjo_NoError) {
            LOG_INFO("Video depth estimation: %s", appState.options.VideoDepthEstimationEnabled ? "ON" : "OFF");
        }
    }

    // Chroma keying
    if (force || appState.options.chromaKeyingEnabled != prevState.options.chromaKeyingEnabled) {
        varjo_MRSetChromaKey(m_session, appState.options.chromaKeyingEnabled ? varjo_True : varjo_False);
        if (CHECK_VARJO_ERR(m_session) == varjo_NoError) {
            LOG_INFO("Chroma keying: %s", appState.options.chromaKeyingEnabled ? "ON" : "OFF");
        }
    }

    // VR view offset
    if (force || appState.options.vrViewOffset != prevState.options.vrViewOffset) {
        varjo_MRSetVRViewOffset(m_session, appState.options.vrViewOffset);
        if (CHECK_VARJO_ERR(m_session) == varjo_NoError) {
            LOG_INFO("VR view offset: %.1f", appState.options.vrViewOffset);
        }
    }

    // VR rendering
    if (force || appState.options.renderVREnabled != prevState.options.renderVREnabled) {
        LOG_INFO("Rendering VR layer: %s", appState.options.renderVREnabled ? "ON" : "OFF");
    }

    // VR depth submit
    if (force || appState.options.submitVRDepthEnabled != prevState.options.submitVRDepthEnabled) {
        LOG_INFO("Submitting VR depth: %s", appState.options.submitVRDepthEnabled ? "ON" : "OFF");
    }

    // VR depth test range
    if (force || (appState.options.vrDepthTestRangeEnabled != prevState.options.vrDepthTestRangeEnabled) ||
        (appState.options.vrDepthTestRangeValue != prevState.options.vrDepthTestRangeValue)) {
        LOG_INFO("Depth test range: %s %.2f", (appState.options.vrDepthTestRangeEnabled ? "ON" : "OFF"), appState.options.vrDepthTestRangeValue);
    }

    // React MR events
    if (force || appState.options.reactToConnectionEvents != prevState.options.reactToConnectionEvents) {
        LOG_INFO("Handling connection events: %s", appState.options.reactToConnectionEvents ? "ON" : "OFF");
    }

    // VR background
    if (force || appState.options.drawVRBackgroundEnabled != prevState.options.drawVRBackgroundEnabled) {
        std::string warn = (appState.options.videoRenderingEnabled && appState.options.drawVRBackgroundEnabled) ? " (not visible with VST)" : "";
        LOG_INFO("Drawing VR background: %s %s", appState.options.drawVRBackgroundEnabled ? "ON" : "OFF", warn.c_str());
    }

    // VR ambient light
    if (force || appState.options.ambientLightTempK != prevState.options.ambientLightTempK ||
        appState.options.ambientLightGainRGB != prevState.options.ambientLightGainRGB) {
        LOG_INFO("VR ambient light (%dK): [%f, %f, %f]", appState.options.ambientLightTempK, appState.options.ambientLightGainRGB[0],
            appState.options.ambientLightGainRGB[1], appState.options.ambientLightGainRGB[2]);
    }

    // VR color correction
    if (force || appState.options.vrColorCorrectionEnabled != prevState.options.vrColorCorrectionEnabled) {
        LOG_INFO("Color correcting VR: %s", appState.options.vrColorCorrectionEnabled ? "ON" : "OFF");

        const auto reqType = varjo_StreamType_DistortedColor;
        const auto reqFormat = m_colorStreamFormat;
        const auto reqChannels = varjo_ChannelFlag_None;

        varjo_ChannelFlag runningChannels = varjo_ChannelFlag_None;
        if (appState.options.vrColorCorrectionEnabled) {
            if (m_streamer->isStreaming(reqType, reqFormat, runningChannels)) {
                LOG_INFO("Already streaming color stream..");
            } else {
                LOG_INFO("Starting metadata only color stream..");
                m_streamer->startDataStream(reqType, reqFormat, reqChannels);
            }
        } else {
            if (m_streamer->isStreaming(reqType, reqFormat, runningChannels)) {
                if (runningChannels == reqChannels) {
                    LOG_INFO("Stop metadata only color stream..");
                    m_streamer->stopDataStream(reqType, reqFormat);
                } else {
                    LOG_INFO("Keep streaming color with data..");
                }
            } else {
                // Not streaming, nothing to do.
            }
        }
    }

    // Cubemap config
    if (force || appState.options.cubemapMode != prevState.options.cubemapMode) {
        auto ret = varjo_Lock(m_session, varjo_LockType_EnvironmentCubemap);
        CHECK_VARJO_ERR(m_session);
        if (ret == varjo_True) {
            varjo_EnvironmentCubemapConfig cubemapConfig;
            cubemapConfig.mode = appState.options.cubemapMode;
            varjo_MRSetEnvironmentCubemapConfig(m_session, &cubemapConfig);

            if (CHECK_VARJO_ERR(m_session) == varjo_NoError) {
                LOG_INFO("Cubemap mode: %s", (appState.options.cubemapMode == varjo_EnvironmentCubemapMode_AutoAdapt) ? "Auto Adapt" : "Fixed 6500K");
            }

            varjo_Unlock(m_session, varjo_LockType_EnvironmentCubemap);
            CHECK_VARJO_ERR(m_session);
        } else {
            LOG_ERROR("Could not change cubemap config.");
        }
    }

    // Data stream buffer handling
    if (force || appState.options.delayedBufferHandlingEnabled != prevState.options.delayedBufferHandlingEnabled) {
        m_streamer->setDelayedBufferHandlingEnabled(appState.options.delayedBufferHandlingEnabled);
        LOG_INFO("Buffer handling: %s", appState.options.delayedBufferHandlingEnabled ? "DELAYED" : "IMMEDIATE");
    }

    if (force || appState.options.undistortEnabled != prevState.options.undistortEnabled) {
        LOG_INFO("Color stream undistortion: %s", appState.options.undistortEnabled ? "ENABLED" : "DISABLED");
    }

    // Data stream: YUV
    if (force || appState.options.dataStreamColorEnabled != prevState.options.dataStreamColorEnabled) {
        const auto streamType = varjo_StreamType_DistortedColor;
        const auto streamFormat = m_colorStreamFormat;
        const auto streamChannels = varjo_ChannelFlag_Left | varjo_ChannelFlag_Right;
        varjo_ChannelFlag currentChannels = varjo_ChannelFlag_None;

        if (appState.options.dataStreamColorEnabled) {
            if (m_streamer->isStreaming(streamType, streamFormat, currentChannels)) {
                if (currentChannels == streamChannels) {
                    // If running stream channels match, match we want to stop it
                    m_streamer->stopDataStream(streamType, streamFormat);
                    if (appState.options.vrColorCorrectionEnabled) {
                        // If color correction is enabled, start stream with no data channels to
                        // keep getting camera metadata for exposure adjustment
                        LOG_INFO("Switching to metadata only color stream..");
                        m_streamer->startDataStream(streamType, streamFormat, varjo_ChannelFlag_None);
                    }
                } else {
                    // When color correction is enabled, stop stream and start new stream with data channels
                    LOG_INFO("Switching to color stream with data channels..");
                    assert(appState.options.vrColorCorrectionEnabled);
                    m_streamer->stopDataStream(streamType, streamFormat);
                    m_streamer->startDataStream(streamType, streamFormat, streamChannels);
                }
            } else {
                // No streams running, just start our color data stream
                m_streamer->startDataStream(streamType, streamFormat, streamChannels);
            }
        } else {
            if (m_streamer->isStreaming(streamType, streamFormat, currentChannels)) {
                if (currentChannels == streamChannels) {
                    // If running stream channels match, match we want to stop it
                    m_streamer->stopDataStream(streamType, streamFormat);
                    if (appState.options.vrColorCorrectionEnabled) {
                        // If color correction is enabled, start stream with no data channels to
                        // keep getting camera metadata for exposure adjustment
                        LOG_INFO("Switching to metadata only color stream..");
                        m_streamer->startDataStream(streamType, streamFormat, varjo_ChannelFlag_None);
                    }
                } else {
                    // When color correction is enabled, stop stream and start new stream with data channels
                    LOG_INFO("Switching to color stream with data channels..");
                    assert(appState.options.vrColorCorrectionEnabled);
                    m_streamer->stopDataStream(streamType, streamFormat);
                    m_streamer->startDataStream(streamType, streamFormat, streamChannels);
                }
            } else {
                // No streams running, just ignore
            }

            // Reset textures if disabled
            {
                std::lock_guard<std::mutex> streamLock(m_frameDataMutex);
                m_frameData.colorFrames = {};
                m_scene->updateColorFrame(0, {0, 0}, 0, 0, nullptr);
                m_scene->updateColorFrame(1, {0, 0}, 0, 0, nullptr);
            }
        }

        // Write stream status back to state
        m_appState.options.dataStreamColorEnabled = m_streamer->isStreaming(streamType, streamFormat);
    }

    // Data stream: Cubemap
    if (force || appState.options.dataStreamCubemapEnabled != prevState.options.dataStreamCubemapEnabled) {
        const auto streamType = varjo_StreamType_EnvironmentCubemap;
        const auto streamFormat = varjo_TextureFormat_RGBA16_FLOAT;
        const auto streamChannels = varjo_ChannelFlag_First;

        if (appState.options.dataStreamCubemapEnabled) {
            if (!m_streamer->isStreaming(streamType, streamFormat)) {
                m_streamer->startDataStream(streamType, streamFormat, streamChannels);
            }
        } else {
            if (m_streamer->isStreaming(streamType, streamFormat)) {
                m_streamer->stopDataStream(streamType, streamFormat);
            }

            // Reset textures if disabled
            {
                std::lock_guard<std::mutex> streamLock(m_frameDataMutex);
                m_frameData.cubemapFrame = {};
                m_scene->updateHdrCubemap(0, 0, 0, nullptr);
            }
        }

        // Write stream status back to state
        m_appState.options.dataStreamCubemapEnabled = m_streamer->isStreaming(streamType, streamFormat);
    }

    // Frame rate limiter
    if (force || (appState.options.vrLimitFrameRate != prevState.options.vrLimitFrameRate)) {
        LOG_INFO("Frame rate limiter: %s", (appState.options.vrLimitFrameRate ? "ON" : "OFF"));
    }
}

void AppLogic::onFrameReceived(const DataStreamer::Frame& frame)
{
    const auto& streamFrame = frame.metadata.streamFrame;

    std::lock_guard<std::mutex> streamLock(m_frameDataMutex);
    switch (streamFrame.type) {
        case varjo_StreamType_DistortedColor: {
            // We update color stream metadata from first channel only
            if (frame.metadata.channelIndex == varjo_ChannelIndex_First) {
                m_frameData.metadata = streamFrame.metadata.distortedColor;
            }
            m_frameData.colorFrames[static_cast<size_t>(frame.metadata.channelIndex)] = frame;
        } break;
        case varjo_StreamType_EnvironmentCubemap: {
            // We update cube frame only from first channel only (actually there is no second)
            if (frame.metadata.channelIndex == varjo_ChannelIndex_First) {
                m_frameData.cubemapFrame = frame;
                m_frameData.cubemapMetadata = streamFrame.metadata.environmentCubemap;
            }
        } break;
        default: {
            LOG_ERROR("Unsupported stream type: %d", streamFrame.type);
            assert(false);
        } break;
    }
}

// Return outputs of mouse click on image in specified window
void onMouseCV(int action, int x, int y, int, void*)
{
    float depth = XYZ.at<float>(y, x);
    float depth_converted = depth * -10.56818019; // TODO: Incorrect conversion
        
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
    // Check for new mixed reality events
    checkEvents();

    // Handle delayed data stream buffers
    m_streamer->handleDelayedBuffers();

    // Sync frame
    m_varjoView->syncFrame();

    // Update frame time
    m_appState.general.frameTime += m_varjoView->getDeltaTime();
    m_appState.general.frameCount = m_varjoView->getFrameNumber();

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

        frameData.cubemapFrame = std::move(m_frameData.cubemapFrame);
        m_frameData.cubemapFrame = std::nullopt;

        frameData.cubemapMetadata = m_frameData.cubemapMetadata;
    }

    // Update scene
    MRScene::UpdateParams updateParams{};

    // Determine current cubemap mode. If cubemap stream is not enabled, we default to the fixed 6500K mode to allow VR color correction.
    varjo_EnvironmentCubemapMode cubemapMode = varjo_EnvironmentCubemapMode_Fixed6500K;

    if (m_appState.options.dataStreamCubemapEnabled && frameData.cubemapMetadata.has_value()) {
        cubemapMode = frameData.cubemapMetadata.value().mode;
    }

    // Get latest color adjustments. These will take effect only if cubemap is not using auto adaptation.
    if (cubemapMode != varjo_EnvironmentCubemapMode_AutoAdapt) {
        updateParams.cameraParams.simulateBrightness = true;

        if (m_appState.options.vrColorCorrectionEnabled && m_appState.options.videoRenderingEnabled) {
            if (frameData.metadata.has_value()) {
                const auto& metadata = *frameData.metadata;
                updateParams.cameraParams.exposureEV = metadata.ev;
                updateParams.cameraParams.cameraCalibrationConstant = metadata.cameraCalibrationConstant;
                updateParams.cameraParams.wbNormalizationData = metadata.wbNormalizationData;
            }
        }

        updateParams.lighting.ambientLight = m_appState.options.ambientLightGainRGB;
    }

    m_scene->update(m_varjoView->getFrameTime(), m_varjoView->getDeltaTime(), m_varjoView->getFrameNumber(), updateParams);

    // Get latest cubemap frame.
    if (frameData.cubemapFrame.has_value()) {
        const auto& cubemapFrame = frameData.cubemapFrame.value();
        m_scene->updateHdrCubemap(cubemapFrame.metadata.bufferMetadata.width, cubemapFrame.metadata.bufferMetadata.format,
            cubemapFrame.metadata.bufferMetadata.rowStride, cubemapFrame.data.data());
    }

    // Get latest color frame
    for (size_t ch = 0; ch < frameData.colorFrames.size(); ch++) {
        if (frameData.colorFrames[ch].has_value()) {
            const auto& colorFrame = frameData.colorFrames[ch].value();

            // Skip conversions if the stream has only metadata.
            if (colorFrame.metadata.bufferMetadata.byteSize == 0) {
                continue;
            }

            // NOTICE! This code is only to demonstrate how color camera frames can be accessed,
            // converted to RGB colorspace, and rectified and projected for e.g. computer vision purposes.
            // This is not intended to be an example of how to render the video-pass-through image!
            // Conversions and undistortion on CPU are very slow and are for demonstration purposes only.

            // Convert datastream YUV frame to RGBA
            if (m_appState.options.undistortEnabled) {
                // We can downsample here to any resolution
                constexpr int c_downScaleFactor = 4;
                const auto w = colorFrame.metadata.bufferMetadata.width / c_downScaleFactor;
                const auto h = colorFrame.metadata.bufferMetadata.height / c_downScaleFactor;
                const auto rowStride = w * 4;

                // Projection for rectified image. In case no projection matrix is provided, uniform projection will be used.
                std::optional<varjo_Matrix> projection = m_varjoView->getLayer(0).getProjection(static_cast<int>(ch));
                // std::optional<varjo_Matrix> projection = std::nullopt;

                // Convert to rectified RGBA in lower resolution
                std::vector<uint8_t> bufferRGBA(rowStride * h);
                DataStreamer::convertDistortedYUVToRectifiedRGBA(colorFrame.metadata.bufferMetadata, colorFrame.data.data(), glm::ivec2(w, h),
                    bufferRGBA.data(), colorFrame.metadata.extrinsics, colorFrame.metadata.intrinsics, projection);

                // Update frame data
                m_scene->updateColorFrame(static_cast<int>(ch), glm::ivec2(w, h), varjo_TextureFormat_R8G8B8A8_UNORM, rowStride, bufferRGBA.data());
            } else {
                const auto w = colorFrame.metadata.bufferMetadata.width;
                const auto h = colorFrame.metadata.bufferMetadata.height;
                const auto rowStride = w * 4;

                // Convert to RGBA in full res (this conversion is pixel to pixel, no scaling allowed)
                std::vector<uint8_t> bufferRGBA(rowStride * h);
                DataStreamer::convertToR8G8B8A(colorFrame.metadata.bufferMetadata, colorFrame.data.data(), bufferRGBA.data());

                // Update frame data
                m_scene->updateColorFrame(static_cast<int>(ch), glm::ivec2(w, h), varjo_TextureFormat_R8G8B8A8_UNORM, rowStride, bufferRGBA.data());

                const auto depthOut = "depthOut";
                cv::namedWindow(depthOut);
                cv::setMouseCallback(depthOut, onMouseCV);

                Mat rgba_mat = cv::Mat(h, w, CV_8UC4, bufferRGBA.data());
                Size imgSize(w, h);

                
                int numChannels = rgba_mat.channels();

                int numDisparity = 4;
                int blockSize = 5;
                int min_disp = 0;
                int p1 = 2;
                int p2 = 20;
                int disp12MaxDiff = 30;
                int uniquenessRatio = 0;
                int speckleWindowSize = 50;
                int speckleRange = 2;
                int prefilterCap = 5;
                
                if (trackbarsCreated == 0){
                    cv::createTrackbar("numDisparity", depthOut, &numDisparity, 20);
                    cv::createTrackbar("blockSize", depthOut, &blockSize, 100);
                    cv::createTrackbar("min_disp", depthOut, &min_disp, 100);
                    cv::createTrackbar("p1", depthOut, &p1, 100);
                    cv::createTrackbar("p2", depthOut, &p2, 300);
                    cv::createTrackbar("disp12MaxDiff", depthOut, &disp12MaxDiff, 100);
                    cv::createTrackbar("uniquenessRatio", depthOut, &uniquenessRatio, 100);
                    cv::createTrackbar("specklewindowsize", depthOut, &speckleWindowSize, 100);
                    cv::createTrackbar("speckleRange", depthOut, &speckleRange, 100);
                    cv::createTrackbar("prefiltercap", depthOut, &prefilterCap, 100);
                    trackbarsCreated = 1;
                }

                numDisparity = cv::getTrackbarPos("numDisparity", depthOut);
                blockSize = cv::getTrackbarPos("blockSize", depthOut);
                min_disp = cv::getTrackbarPos("min_disp", depthOut);
                p1 = cv::getTrackbarPos("p1", depthOut);
                p2 = cv::getTrackbarPos("p2", depthOut);
                disp12MaxDiff = cv::getTrackbarPos("disp12MaxDiff", depthOut);
                uniquenessRatio = cv::getTrackbarPos("uniquenessRatio", depthOut);
                speckleWindowSize = cv::getTrackbarPos("specklewindowsize", depthOut);
                speckleRange = cv::getTrackbarPos("speckleRange", depthOut);
                prefilterCap = cv::getTrackbarPos("prefiltercap", depthOut);

                numDisparity = std::max(1, numDisparity * 16);
                blockSize = (blockSize % 2 == 0) ? blockSize + 1 : blockSize;
                min_disp *= -1;
                uniquenessRatio = uniquenessRatio;
                p1 = p1 * numChannels * blockSize * blockSize; //8 to 32
                p2 = p2 * numChannels * blockSize * blockSize; // 32 to 56*/
                

                if (ch==0){ // Left Eye
                    cv::cvtColor(rgba_mat, leftEyeImage, cv::COLOR_RGBA2BGR);
                    cv::cvtColor(leftEyeImage, GrayL, cv::COLOR_BGR2GRAY);
                    GrayL.convertTo(GrayL, CV_8U);

                    kL = (cv::Mat_<double>(3,3) <<
                    (0.431721 * w),    0,                   (0.502108 * w),
                    0,                 (0.431721 * h),      (0.509123 * h),
                    0,                 0,                   1 
                    );

                    dL = (cv::Mat_<double>(1,4) << 
                    -0.152559,
                    0.024209,
                    -0.000175,
                    -0.000107
                    );

                    xiL = (cv::Mat_<double>(1,1) << 0.085909);

                }
                if (ch==1){ // Right Eye
                    cv::cvtColor(rgba_mat, rightEyeImage, cv::COLOR_RGBA2BGR);
                    cv::cvtColor(rightEyeImage, GrayR, cv::COLOR_BGR2GRAY);
                    GrayR.convertTo(GrayR, CV_8U);

                    kR = (cv::Mat_<double>(3,3) <<
                    (0.423736 * w),    0,                   (0.503964 * w),
                    0,                 (0.423736 * h),      (0.507224 * h),
                    0,                 0,                   1 
                    );

                    dR = (cv::Mat_<double>(1,4) << 
                    -0.152976,
                    0.023694,
                    -0.000302,
                    -0.000316
                    );

                    xiR = (cv::Mat_<double>(1,1) << 0.068839);

                    R = (cv::Mat_<double>(3, 3) << // manually calculated, corrected rotation matrix
                        0.99999604094504943076, 
                        0.0023173339275301227727,
                        0.001755938946078424719,
                        -0.0023099358690568524827,
                        0.99998704268073523836,
                        -0.0046550356300614758439,
                        -0.0017673258267597134926,
                        0.0046515232975054745946,
                        0.99998750952989840899
                    );

                    T = (cv::Mat_<double>(3, 1) <<  // image transposed left-right so we have to keep this in right camera setup
                        -0.062976, 
                        0.000323, 
                        -0.000423
                    );

                }
                
                if (!GrayL.empty() && !GrayR.empty() && ch == 1){
                    cv::Ptr<cv::StereoSGBM> stereo = cv::StereoSGBM::create(min_disp, numDisparity, blockSize, p1, p2, disp12MaxDiff, prefilterCap, uniquenessRatio, speckleWindowSize, speckleRange, false);

                    cv::Mat leftU, rightU, rL, rR;
                    cv::Matx33f Knew;
                    cv::Mat map1L, map2L, map1R, map2R, QQ;
                    cv::Rect ROI;

                    cv::Mat kNew = getOptimalNewCameraMatrix(kR, dR, imgSize, 1, imgSize, &ROI);

                    cv::omnidir::stereoRectify(R, T, rL, rR);

                    cv::omnidir::initUndistortRectifyMap(kL, dL, xiL, rL, kNew, imgSize, CV_32FC1, map1L, map2L, cv::omnidir::RECTIFY_PERSPECTIVE);
                    cv::omnidir::initUndistortRectifyMap(kR, dR, xiR, R, kNew, imgSize, CV_32FC1, map1R, map2R, cv::omnidir::RECTIFY_PERSPECTIVE);

                    //TODO: Using full color instead of grayscale halves our framerate. Worth it for slightly more detail?
                    cv::remap(leftEyeImage, leftU, map1L, map2L, INTER_LINEAR);
                    cv::remap(rightEyeImage, rightU, map1R, map2R, INTER_LINEAR);

                    cv::Mat leftROI = leftU(ROI);
                    cv::Mat rightROI = rightU(ROI);

                    stereo->compute(leftROI, rightROI, disMap);

                    // src: https://docs.carnegierobotics.com/docs/cookbook/overview.html
                    double q[] =
                    {
                        1, 0, 0, -0.502108,
                        0, 1, 0, -0.509123,
                        0, 0, 0, 0.431721,
                        0, 0, -15.87851034, -0.02947051519
                    };
                    cv::Mat Q = Mat(4, 4, CV_64F, q);

                    // Convert disparity map into depth map
                    Mat floatDisp;
                    disMap.convertTo(floatDisp, CV_32F, 1.0/16);
                    reprojectImageTo3D(floatDisp, XYZ, Q, false);

                    // Creates and normalizes a representation of the depth display that's more readable and displayable
                    cv::Mat depthDisplay;
                    floatDisp.convertTo(depthDisplay, CV_8U, 255.0 / 16); // Normalize
                    cv::applyColorMap(depthDisplay, depthDisplay, cv::COLORMAP_JET); // Reconverts image to color

                    // output current disparity map
                    cv::putText(depthDisplay, str_DistFromMouse, cv::Point(0,h-30), cv::FONT_HERSHEY_COMPLEX , 0.5, CV_RGB(0, 0, 0), 4); //text outline
                    cv::putText(depthDisplay, str_DistFromMouse, cv::Point(0,h-30), cv::FONT_HERSHEY_COMPLEX , 0.5, CV_RGB(255, 255, 255), 1); // onscreen text with depth info

                    cv::imshow(depthOut, depthDisplay);
                    cv::waitKey(1);
                }
            }
        }
    }

    // Early exit if no frame submit
    if (!m_appState.options.renderVREnabled) {
        // Invalidate frame by submitting empty frame on the first call, then no op
        m_varjoView->invalidateFrame();
        return;
    }

    // Limit frame rate
    if (m_appState.options.vrLimitFrameRate) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 45));
    }

    // Begin frame
    m_varjoView->beginFrame();

    // Render layer
    {
        // Get layer for rendering
        constexpr int layerIndex = 0;
        auto& layer = m_varjoView->getLayer(layerIndex);

        // Setup render params
        MultiLayerView::Layer::SubmitParams submitParams{};
        submitParams.submitColor = m_appState.options.renderVREnabled;
        submitParams.submitDepth = m_appState.options.submitVRDepthEnabled;
        submitParams.depthTestEnabled = m_appState.options.VideoDepthEstimationEnabled;
        submitParams.depthTestRangeEnabled = m_appState.options.vrDepthTestRangeEnabled;
        submitParams.depthTestRangeLimits = {
            0.0, submitParams.depthTestRangeEnabled ? std::max(static_cast<double>(m_appState.options.vrDepthTestRangeValue), 0.0) : 0.0};
        submitParams.chromaKeyEnabled = m_appState.options.chromaKeyingEnabled;
        submitParams.alphaBlend = m_appState.options.videoRenderingEnabled || !m_appState.options.drawVRBackgroundEnabled;

        // Begin layer rendering
        layer.begin(submitParams);

        // Clear frame
        const glm::vec4 opaqueBg(0.15f, 0.5f, 0.6f, 1.0f);
        const glm::vec4 transpBg(0.0f, 0.0f, 0.0f, 0.0f);
        const glm::vec4 clearColor = submitParams.alphaBlend ? transpBg : opaqueBg;
        layer.clear(MultiLayerView::Layer::ClearParams(clearColor));

        // Render frame
        layer.renderScene(*m_scene);

        // End layer rendering
        layer.end();
    }

    // Submit varjo frame with rendered layers
    m_varjoView->endFrame();
}

void AppLogic::onMixedRealityAvailable(bool available, bool forceSetState)
{
    m_appState.general.mrAvailable = available;

    if (available) {
        // Update stuff here if needed

        // Get format for color stream
        m_colorStreamFormat = m_streamer->getFormat(varjo_StreamType_DistortedColor);

        if (m_appState.options.reactToConnectionEvents && !m_appState.options.videoRenderingEnabled) {
            LOG_INFO("Enabling video rendering on MR available event..");
            setVideoRendering(true);
        }

    } else {
        LOG_ERROR("Mixed Reality features not available.");

        // Update stuff here if needed
        if (m_appState.options.reactToConnectionEvents && m_appState.options.videoRenderingEnabled) {
            LOG_INFO("Disabling video rendering on MR unavailable event..");
            setVideoRendering(false);
        }
    }

    // Force set state when MR becomes active
    if (forceSetState) {
        setState(m_appState, true);
    }

    // Update camera properties based on the mixed reality availability
    m_camera->enumerateCameraProperties(available);
}

void AppLogic::checkEvents()
{
    varjo_Bool ret = varjo_False;

    do {
        varjo_Event evt{};
        ret = varjo_PollEvent(m_session, &evt);
        CHECK_VARJO_ERR(m_session);

        if (ret == varjo_True) {
            switch (evt.header.type) {
                case varjo_EventType_MRDeviceStatus: {
                    switch (evt.data.mrDeviceStatus.status) {
                        // Occurs when Mixed Reality features are enabled
                        case varjo_MRDeviceStatus_Connected: {
                            LOG_INFO("EVENT: Mixed reality device status: %s", "Connected");
                            constexpr bool forceSetState = true;
                            onMixedRealityAvailable(true, forceSetState);
                        } break;
                        // Occurs when Mixed Reality features are disabled
                        case varjo_MRDeviceStatus_Disconnected: {
                            LOG_INFO("EVENT: Mixed reality device status: %s", "Disconnected");
                            constexpr bool forceSetState = false;
                            onMixedRealityAvailable(false, forceSetState);
                        } break;
                        default: {
                            // Ignore unknown status.
                        } break;
                    }
                } break;

                case varjo_EventType_MRCameraPropertyChange: {
                    varjo_CameraPropertyMode mode = varjo_MRGetCameraPropertyMode(m_session, evt.data.mrCameraPropertyChange.type);
                    varjo_CameraPropertyValue manualValue = varjo_MRGetCameraPropertyValue(m_session, evt.data.mrCameraPropertyChange.type);
                    std::string cameraPropStr = CameraManager::propertyTypeToString(evt.data.mrCameraPropertyChange.type);
                    std::string modeStr = CameraManager::propertyModeToString(mode);
                    std::string valStr = CameraManager::propertyValueToString(evt.data.mrCameraPropertyChange.type, manualValue);
                    LOG_INFO("EVENT: Camera prop changed: %s. mode: %s, value: %s", cameraPropStr.c_str(), modeStr.c_str(), valStr.c_str());

                    // Update any information related to the changed property
                    getCamera().onCameraPropertyChanged(evt.data.mrCameraPropertyChange.type);
                } break;

                case varjo_EventType_DataStreamStart: {
                    LOG_INFO("EVENT: Data stream started: id=%d", static_cast<int>(evt.data.dataStreamStart.streamId));
                } break;

                case varjo_EventType_DataStreamStop: {
                    LOG_INFO("EVENT: Data stream stopped: id=%d", static_cast<int>(evt.data.dataStreamStop.streamId));
                } break;

                case varjo_EventType_MRChromaKeyConfigChange: {
                    LOG_INFO("EVENT: Chroma key config changed");
                } break;

                default: {
                    // Ignore unknown event.
                } break;
            }
        }
    } while (ret);
}
