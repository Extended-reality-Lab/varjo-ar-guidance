// Copyright 2019-2021 Varjo Technologies Oy. All rights reserved.

#include "AppLogic.hpp"

#include <opencv2/opencv.hpp> 

#include <opencv2/ximgproc/disparity_filter.hpp> // filtering algorithms

#include <opencv2/ximgproc/disparity_filter.hpp> // filtering algorithms

using namespace VarjoExamples;
using namespace std;
using namespace cv;
using namespace cv::ximgproc;
using namespace cv::ximgproc;

// globals tracking the contents of both headset eyes to prevent them resetting data before we can read it
Mat leftEyeImage, rightEyeImage, grayL, grayR;
Mat leftEyeImage, rightEyeImage, grayL, grayR;
cv::Mat XYZ; // Depth Map

cv::Mat left_for_matcher, right_for_matcher;

cv::Mat left_for_matcher, right_for_matcher;

string str_DistFromMouse = "Collecting information from mouse..."; // stores distance from mouse to objects in

bool trackbarsCreated = false;

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

    // Initialize camera intrinsice and extrinsics data for Varjo headset
    m_streamer->getCalibDat();
    m_streamer->getDispDat();
    m_streamer->getUndistortMapDat(832, 640);

    // Initialize camera intrinsice and extrinsics data for Varjo headset
    m_streamer->getCalibDat();
    m_streamer->getSGBMDat();
    m_streamer->getUndistortMapDat(832, 640);

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
    float depth_converted = depth * 1;//-10.56818019;
    float depth_converted = depth * 1;//-10.56818019;
        
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
    for (size_t ch = 0; ch < frameData.colorFrames.size(); ch++) { // TODO: Slava doesn't want this to be a loop. Not necessary at all and probably slows things down
    for (size_t ch = 0; ch < frameData.colorFrames.size(); ch++) { // TODO: Slava doesn't want this to be a loop. Not necessary at all and probably slows things down
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

            if (ch==0){ // Left Eye
                cv::cvtColor(rgba_mat, leftEyeImage, cv::COLOR_RGBA2BGR);
                cv::cvtColor(leftEyeImage, grayL, COLOR_BGR2GRAY);
                cv::cvtColor(leftEyeImage, grayL, COLOR_BGR2GRAY);
            }
            else if (ch==1){ // Right Eye
                cv::cvtColor(rgba_mat, rightEyeImage, cv::COLOR_RGBA2BGR);
                cv::cvtColor(rightEyeImage, grayR, COLOR_BGR2GRAY);
            }
                
            if (!grayL.empty() && !grayR.empty() && ch == 1){

                // conditional here is probably pointless. Delete when I'm done fixing
                m_streamer->getDepthMap(leftEyeImage, rightEyeImage);
                cv::cvtColor(rightEyeImage, grayR, COLOR_BGR2GRAY);
            }

                int numChannels = 1;
                // The comments for the following are what you need to do if you remove the trackbars from the image:
                int numDisparity = 3; // multiply by 16
                int blockSize = 1;
                int min_disp = 0; // set negative
                int p1 = 87; // multiply by the following: numChannels * blockSize * blockSize
                int p2 = 315; // multiply by the following: numChannels * blockSize * blockSize
                int disp12MaxDiff = 0;
                int uniquenessRatio = 0;
                int speckleWindowSize = 0;
                int speckleRange = 0;
                int prefilterCap = 70;
                float lambda = 1.0;
                int lambda_int = 10;
                int sigma = 8000;
                
                if (trackbarsCreated == 0){
                    cv::createTrackbar("numDisparity", depthOut, &numDisparity, 40);
                    cv::createTrackbar("blockSize", depthOut, &blockSize, 100);
                    cv::createTrackbar("min_disp", depthOut, &min_disp, 300);
                    cv::createTrackbar("p1", depthOut, &p1, 300);
                    cv::createTrackbar("p2", depthOut, &p2, 600);
                    cv::createTrackbar("disp12MaxDiff", depthOut, &disp12MaxDiff, 100);
                    cv::createTrackbar("uniquenessRatio", depthOut, &uniquenessRatio, 100);
                    cv::createTrackbar("specklewindowsize", depthOut, &speckleWindowSize, 100);
                    cv::createTrackbar("speckleRange", depthOut, &speckleRange, 100);
                    cv::createTrackbar("prefiltercap", depthOut, &prefilterCap, 300);
                    cv::createTrackbar("lambda", depthOut, &lambda_int, 30);
                    cv::createTrackbar("sigma", depthOut, &sigma, 20000);
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
                lambda_int = cv::getTrackbarPos("lambda", depthOut);
                sigma = cv::getTrackbarPos("sigma", depthOut);

                numDisparity = std::max(1, numDisparity * 16);
                blockSize = (blockSize % 2 == 0) ? blockSize + 1 : blockSize;
                min_disp *= -1;
                uniquenessRatio = uniquenessRatio;
                p1 = p1 * numChannels * blockSize * blockSize; //8 to 32
                p2 = p2 * numChannels * blockSize * blockSize; // 32 to 56*/
                lambda = lambda_int/10; // trackbars must be ints. Converts integer input into usable float
                
            if (!grayL.empty() && !grayR.empty() && ch == 1){

                // rectification happens in init. It's not skipped here
                
                cv::Mat leftUndistorted, rightUndistorted;

                cv::remap(leftEyeImage, leftUndistorted, m_streamer->map1L, m_streamer->map2L, INTER_LINEAR);
                cv::remap(rightEyeImage, rightUndistorted, m_streamer->map1R, m_streamer->map2R, INTER_LINEAR);

                cv::Mat left_disp, right_disp;
                
               resize(leftUndistorted ,left_for_matcher ,Size(),0.5,0.5);
               resize(rightUndistorted,right_for_matcher,Size(),0.5,0.5);

                // Compute both eyes using left eye as reference
                Ptr<StereoSGBM> left_matcher = StereoSGBM::create(min_disp, numDisparity, blockSize, p1, p2, 
                                                                    disp12MaxDiff, prefilterCap, uniquenessRatio, 
                                                                    speckleWindowSize, speckleRange);
                Ptr<DisparityWLSFilter> disparityFilter = createDisparityWLSFilter(left_matcher);
                Ptr<StereoMatcher> right_matcher = createRightMatcher(left_matcher);

                //cvtColor(left_for_matcher,  left_for_matcher,  COLOR_BGR2GRAY);
                //cvtColor(right_for_matcher, right_for_matcher, COLOR_BGR2GRAY);

                // conditional here is probably pointless. Delete when I'm done fixing
                if (!left_matcher.empty() && !right_matcher.empty()){
                    left_matcher -> compute(left_for_matcher, right_for_matcher, left_disp);
                    right_matcher -> compute(right_for_matcher, left_for_matcher, right_disp);

                    disparityFilter->setLambda(lambda); // typical value is 8000
                    disparityFilter->setSigmaColor(sigma); //Typical values range from 0.8 to 2.0.

                    //cv::Rect roi(0, 0, left_disp.cols, left_disp.rows); // using unpopulated Rect() for filter() input seemingly has way cleaner filter outputs

                    //cout << left_disp.size() << " " << right_disp.size() << " " << leftEyeImage.size() << endl;
    
                    cv::Mat filtered_disp;
                    disparityFilter->filter(left_disp, leftUndistorted, filtered_disp, right_disp, Rect(), rightUndistorted);

                    ///*
                    Mat raw_disp_vis;
                    getDisparityVis(left_disp,raw_disp_vis,1);
                    namedWindow("raw disparity", WINDOW_AUTOSIZE);
                    imshow("raw disparity", raw_disp_vis);
                    Mat filtered_disp_vis;
                    getDisparityVis(filtered_disp,filtered_disp_vis,1);
                    imshow("Filtered disparity map", filtered_disp_vis);
                    waitKey(1);
                    //*/

                    Mat floatDisp;
                    filtered_disp.convertTo(floatDisp, CV_32F, 1.0); // normally divided by 16. This leads to poor visibility on depth map
                    // Creates and normalizes a representation of the depth display that's more readable and displayable
                    cv::Mat depthDisplay;
                    floatDisp.convertTo(depthDisplay, CV_8U, 255.0 / 16); // Normalize
                    cv::applyColorMap(depthDisplay, depthDisplay, cv::COLORMAP_JET); // Reconverts image to color

                    reprojectImageTo3D(floatDisp, m_streamer->depthMap, m_streamer->storedCalibData.Q, false);

                    XYZ = m_streamer->depthMap.clone(); // This is to get onmousecv to work and display text onto screen

                    // output onmouse depth details
                    cv::putText(XYZ, str_DistFromMouse, cv::Point(0,h-30), cv::FONT_HERSHEY_COMPLEX , 0.5, CV_RGB(0, 0, 0), 4); //text outline
                    cv::putText(XYZ, str_DistFromMouse, cv::Point(0,h-30), cv::FONT_HERSHEY_COMPLEX , 0.5, CV_RGB(255, 255, 255), 1); // onscreen text with depth info

                    cv::imshow(depthOut, XYZ);
                    cv::waitKey(1);


                }
                /*
                // Actually called in update since we need to update it every frame
                m_streamer->getDepthMap(grayL, grayR);

                XYZ = m_streamer->depthMap.clone(); // This is to get onmousecv to work and display text onto screen
                XYZ = m_streamer->depthMap.clone(); // This is to get onmousecv to work and display text onto screen

                // output onmouse depth details
                cv::putText(XYZ, str_DistFromMouse, cv::Point(0,h-30), cv::FONT_HERSHEY_COMPLEX , 0.5, CV_RGB(0, 0, 0), 4); //text outline
                cv::putText(XYZ, str_DistFromMouse, cv::Point(0,h-30), cv::FONT_HERSHEY_COMPLEX , 0.5, CV_RGB(255, 255, 255), 1); // onscreen text with depth info
                cv::putText(XYZ, str_DistFromMouse, cv::Point(0,h-30), cv::FONT_HERSHEY_COMPLEX , 0.5, CV_RGB(0, 0, 0), 4); //text outline
                cv::putText(XYZ, str_DistFromMouse, cv::Point(0,h-30), cv::FONT_HERSHEY_COMPLEX , 0.5, CV_RGB(255, 255, 255), 1); // onscreen text with depth info

                cv::imshow(depthOut, XYZ);
                cv::imshow(depthOut, XYZ);
                cv::waitKey(1);
                */
            }
        }
    }
}
