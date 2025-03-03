#pragma once

#include <DataStreamer.hpp> // parent class

#include <opencv2/opencv.hpp> // import .yaml files
#include <opencv2/ccalib/omnidir.hpp> // Omnidirectional rectification
#include <opencv2/calib3d/calib3d.hpp> // disparity map algorithms3
#include <opencv2/ximgproc/disparity_filter.hpp> // filtering algorithms

namespace VarjoExamples{
using namespace std;
using namespace cv;

class DepthManager: public DataStreamer{
    public:
        DepthManager(varjo_Session* session, const std::function<void(const Frame&)>& onFrameCallback)
            : DataStreamer(session, onFrameCallback){};
    public:
        struct CalibData{ // intrinsics and extrinsics of Varjo Headset
            Mat M1, M2; // Camera Matricies
            Mat D1, D2; // distortion params. 2 radial. 2 tengential
            Mat Skew1, Skew2; 
            Mat XI1, XI2; 
            Mat R; // Rotation
            Mat T; // Translation
            Mat Q;
        }; CalibData storedCalibData;

        struct DispData{ // inputs for sgbm matching
            // SGBM formula inputs
            int numDisparity;
            int blockSize; 
            int minDisp;
            int p1;
            int p2;
            int disp12MaxDiff;
            int uniquenessRatio;
            int speckleWindowSize;
            int speckleRange;
            int preFilterCap;
            // WLS formula inputs
            float lambda;
            int sigma;
        }; DispData storedDispData;

        cv::Ptr<cv::StereoSGBM> left_matcher;

        cv::Ptr<cv::StereoSGBM> stereo;

        Mat floatDisp; // For image output. Completely useless after debugging over. TODO: Delete
        cv::Mat depthMap;
        cv::Mat map1L, map2L, map1R, map2R; // for cv::remap func

        // pull hard-coded callibration data from yaml files
        void getCalibDat();
        
        // pull hard-coded sgbm callibration data from yaml files
        void getDispDat();

        // calculate map1L, 2L, 1R, 2R for use in remap function
        void getUndistortMapDat(float w, float h);

        // Runs SGBM algorithm with vals calculated from getSGBMData, gets dispmap, gets depthMap
        void getDepthMap(Mat leftEyeImg, Mat rightEyeImg);
    };
};