// The purpose of this copy is to generate timer data into 3 arrays for time to generate a point cloud. It's not concerned with anything but tracking progress of numbers for me

#include "AppLogic.hpp"

#include <Varjo_mr_experimental.h> // enables point cloud construction

#include <opencv2/opencv.hpp> 

#include <iostream>
#include <cstdint>
#include <utility> // return a pair of values

#include <iostream>// write to file
#include <fstream> // write to file

#include "half.h"
#include <chrono>
#include <ctime>

#include <opencv2/highgui.hpp> // visualize point cloud

CONST int TRACKERSIZE = 5;
int arrayTracker = 0;

std::chrono::time_point<std::chrono::system_clock> startLatancy, endLatancy, startgenTime, endGenTime; // Latancy = time from eyes gen to cloud gen. GenTime is total time taken to make point clouds

// Track what the name says. Does it for the amount of tracker size plus 1 more space where they'll each output the average value
int cloudDensity[TRACKERSIZE+1];
float cloudLatancy[TRACKERSIZE+1];
float timeToGenerate[TRACKERSIZE+1];

int numEyes = 0; // counter for amount of times passthrough cams generate data before we get a point cloud
bool isTrackingLatancy = 0;

using namespace VarjoExamples;
using namespace std;
using namespace cv;

// globals tracking the contents of both headset eyes to prevent them resetting data before we can read it
Mat leftEyeImage, rightEyeImage;
cv::Mat XYZ; // Depth Map

varjo_PointCloudSnapshotId snapshotId;
varjo_PointCloudDeltaContent varjoPointCloud;
varjo_PointCloudSnapshotContent varjoCloudContent;

string str_DistFromMouse = "Collecting information from mouse..."; // stores distance from mouse to objects in

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

    // enable point cloud construction from Varjo lighter
    varjo_MRSetReconstruction(m_session, true);
    snapshotId = varjo_MRBeginPointCloudSnapshot(m_session); // schedules new snapshot. I only want 1 for now but eventually this'll have to be under update as well as init

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

void unpack16bitints(varjo_PointCloudPoint* points, size_t pointCount, int w, int h, Mat leftEye, Mat rightEye) {

    static size_t cntr = 0;
    if (!leftEye.empty() && !rightEye.empty()){
        imwrite("leftEye" + std::to_string(cntr) + ".bmp", leftEye);
        imwrite("rightEye" + std::to_string(cntr) + ".bmp", rightEye);
    }
    ofstream myFile("VarjoCloudOut" + std::to_string(cntr++) + ".pcd");

    // Generate point cloud header
    myFile << "VERSION .7" << endl;
    myFile << "FIELDS x y z" << endl;
    myFile << "SIZE 4 4 4" << endl;
    myFile << "TYPE F F F" << endl;
    myFile << "COUNT 1 1 1" << endl;
    myFile << "WIDTH " << pointCount << endl;// set by system
    myFile << "HEIGHT " << 1 << endl; // set by system
    myFile << "VIEWPOINT 0 0 0 1 0 0 0" << endl;
    myFile << "POINTS " << pointCount << endl; // set by system
    myFile << "DATA ascii" << endl;

    // Generate point cloud points
    for (size_t i = 0; i < pointCount; ++i) {
        // Extract the two 16-bit float16 values (16 bits each)
        uint16_t intX = (points[i].positionXY >> 16) & 0xFFFF;  // High 16 bits
        uint16_t intY = points[i].positionXY & 0xFFFF;          // Low 16 bits
        uint16_t intZ = (points[i].positionZradius) & 0xFFFF;
        uint16_t intR = (points[i].normalZcolorR >> 16) & 0xFFFF;
        uint16_t intB = (points[i].colorBG >> 16) & 0xFFFF;  // High 16 bits
        uint16_t intG = points[i].colorBG & 0xFFFF;

        

        // Convert the float16 values to float
        float floatX = FLOAT16(intX);
        float floatY = FLOAT16(intY);
        float floatZ = FLOAT16(intZ);
        float floatR = FLOAT16(intR);
        float floatB = FLOAT16(intB);
        float floatG = FLOAT16(intG);
        
        //std::cout << "Point " << i << " X: " << posX << ", Y: " << posY << std::endl;
        myFile << floatX << " " << floatY << " " << floatZ << endl;
    }
    
    cout << "file written" << endl;
    myFile.close();
    
}

void outputArraysFloat(float inputarr[]){
    float average = 0;
    for (int i=0; i<TRACKERSIZE; i++){
        cout << inputarr[i] << ", ";
        average += inputarr[i];
    }
    average /= TRACKERSIZE;
    cout << "AVERAGE OUTPUT:" << average;
    cout << endl << endl;
}

void outputArraysInt(int inputarr[]){
    int average = 0;
    for (int i=0; i<TRACKERSIZE; i++){
        cout << inputarr[i] << ", ";
        average += inputarr[i];
    }
    average /= TRACKERSIZE;
    cout << "AVERAGE OUTPUT:" << average;
    cout << endl << endl;
}


void AppLogic::update()
{
    // Handle delayed data stream buffers
    m_streamer->handleDelayedBuffers();
    startgenTime = std::chrono::system_clock::now();

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
            //m_streamer->getUndistortMapDat(w, h);

            cv::Rect ROI;
            cv::Mat rL, rR;
    
            Size imgSize(w, h);
    
            m_streamer->storedCalibData.M1.row(0) *= w;
            m_streamer->storedCalibData.M1.row(1) *= h;
            m_streamer->storedCalibData.M2.row(0) *= w;
            m_streamer->storedCalibData.M2.row(1) *= h;
    
            cv::Mat kNew = getOptimalNewCameraMatrix(m_streamer->storedCalibData.M1, m_streamer->storedCalibData.D1, imgSize, 1, imgSize, &ROI);
    
            cv::omnidir::stereoRectify(m_streamer->storedCalibData.R, m_streamer->storedCalibData.T, rL, rR);

            if (ch==0){ // Left Eye
                cv::cvtColor(rgba_mat, leftEyeImage, cv::COLOR_RGBA2BGR);
                startLatancy = std::chrono::system_clock::now();
            }
            else if (ch==1){ // Right Eye
                cv::cvtColor(rgba_mat, rightEyeImage, cv::COLOR_RGBA2BGR);
            }

            cout << "Time in nanoseconds: " << varjoCloudContent.timestamp << endl;
            cout << varjo_MRGetPointCloudSnapshotStatus(m_session, snapshotId)  << endl;

            if (varjo_MRGetPointCloudSnapshotStatus(m_session, snapshotId) == 2 ){ // if the current point cloud snapshot is ready
                
                endLatancy = std::chrono::system_clock::now();
                std::chrono::duration<double> elapsed_seconds_latancy = endLatancy - startLatancy;
                std::time_t end_time_latancy = std::chrono::system_clock::to_time_t(endLatancy);
                std::cout << "finished computation for latancy at " << std::ctime(&end_time_latancy)
              << "elapsed time: " << elapsed_seconds_latancy.count() << "s\n";

                endGenTime = std::chrono::system_clock::now();
                std::chrono::duration<double> elapsed_seconds = endGenTime - startgenTime;
                std::time_t end_time = std::chrono::system_clock::to_time_t(endGenTime);
                std::cout << "finished computation for generate point cloud time at " << std::ctime(&end_time)
              << "elapsed time: " << elapsed_seconds.count() << "s\n";

              timeToGenerate[arrayTracker] = elapsed_seconds.count();
              cloudLatancy[arrayTracker] = elapsed_seconds_latancy.count();
              isTrackingLatancy = 0; // reset checker for cam latancy

                cout << "snapshot is ready " << varjo_MRGetPointCloudSnapshotStatus(m_session, snapshotId)  << endl; // alternative way of querying snapshot
                
                varjo_MRGetPointCloudSnapshotContent(m_session, snapshotId, &varjoCloudContent); // save current snapshot content to varjoCloudContent
                cloudDensity[arrayTracker] = varjoCloudContent.pointCount;

                //unpack16bitints(varjoCloudContent.points, varjoCloudContent.pointCount, w, h, leftEyeImage, rightEyeImage);

                startgenTime = std::chrono::system_clock::now();
                if (arrayTracker + 1 != TRACKERSIZE){
                    arrayTracker++;

                    varjo_MRReleasePointCloudSnapshot(m_session, snapshotId); // release now unused snapshot
                    snapshotId = varjo_MRBeginPointCloudSnapshot(m_session); // request new snapshot
                    cout << "New snapshot requested" << endl;
                }
                else{
                    // calculate averages of outputs here
                    cout << "time to generate a point cloud from fresh snapshot: ";
                    outputArraysFloat(timeToGenerate);
                    cout << "Density per cloud generated: ";
                    outputArraysInt(cloudDensity);
                    cout << "time to generate a point cloud the eye view: ";
                    outputArraysFloat(cloudLatancy);
                    exit(1);
                }

            }
            else{
                cout << "snapshot is not ready." << endl;
            }

                
            if (!leftEyeImage.empty() && !rightEyeImage.empty() && ch == 1){
                Mat leftU, rightU, disMap;
                Mat img1Rec, img2Rec, pointCloud;

                cv::omnidir::undistortImage(leftEyeImage, leftU, m_streamer->storedCalibData.M1, m_streamer->storedCalibData.D1, m_streamer->storedCalibData.XI1, cv::omnidir::RECTIFY_PERSPECTIVE, kNew, leftEyeImage.size(), m_streamer->storedCalibData.R);
                cv::omnidir::undistortImage(rightEyeImage, rightU, m_streamer->storedCalibData.M2, m_streamer->storedCalibData.D2, m_streamer->storedCalibData.XI2, cv::omnidir::RECTIFY_PERSPECTIVE, kNew, rightEyeImage.size(), m_streamer->storedCalibData.R);

                // This one is modded. We should compare to unmodded version they might know what they're doing
                // My flag is also not a recommended flag. Point cloud could very well come out really ugly
                cv::omnidir::stereoReconstructModSGBM(leftU, rightU, m_streamer->storedCalibData.M1, m_streamer->storedCalibData.D1, m_streamer->storedCalibData.XI1, m_streamer->storedCalibData.M2, m_streamer->storedCalibData.D2, m_streamer->storedCalibData.XI2, m_streamer->storedCalibData.R, m_streamer->storedCalibData.T, cv::omnidir::RECTIFY_PERSPECTIVE, 
                m_streamer->storedSGBMData.numDisparity, 5, disMap, img1Rec, img2Rec, leftU.size(), kNew, pointCloud, 1, m_streamer->storedSGBMData.minDisp, m_streamer->storedSGBMData.p1, m_streamer->storedSGBMData.p2, m_streamer->storedSGBMData.disp12MaxDiff, m_streamer->storedSGBMData.uniquenessRatio, m_streamer->storedSGBMData.speckleWindowSize, m_streamer->storedSGBMData.speckleRange, m_streamer->storedSGBMData.preFilterCap);

                
                //cout << "We are about to try and write the pointcloud" << endl;
                //cv::FileStorage file("pointCloud.csv", cv::FileStorage::WRITE);
                //file << "pointCloud" << pointCloud;
                //file.release();
                //cv::imwrite("pointCloud.pcd", pointCloud);
                //cout << "point cloud written" << endl;

                cv::imshow("leftEye", leftU);
                cv::waitKey(1);
                cv::imshow(depthOut, rightU);
                cv::waitKey(1);
            }
        }
    }
}
