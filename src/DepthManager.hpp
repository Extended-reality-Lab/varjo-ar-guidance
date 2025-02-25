#pragma once

#include <DataStreamer.hpp> // parent class
#include <opencv2/opencv.hpp> // import .yaml files
#include <opencv2/ccalib/omnidir.hpp> // Omnidirectional rectification
#include <opencv2/calib3d/calib3d.hpp> // disparity map algorithms

namespace VarjoExamples {
using namespace std;
using namespace cv;

class DepthManager : public DataStreamer {
public:
    // Constructor
    DepthManager(varjo_Session* session, const std::function<void(const Frame&)>& onFrameCallback);

public:
    // Intrinsic and extrinsic camera calibration data for Varjo Headset
    struct CalibData {
        Mat M1, M2; // Camera Matrices
        Mat D1, D2; // Distortion parameters (radial and tangential)
        Mat Skew1, Skew2;
        Mat XI1, XI2;
        Mat R; // Rotation matrix
        Mat T; // Translation vector
        Mat Q; // Q matrix for stereo vision
    };
    CalibData storedCalibData;

    // SGBM stereo matching parameters
    struct SGBMData {
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
    };
    SGBMData storedSGBMData;

    Mat floatDisp; // For image output (useful for debugging)
    cv::Mat depthMap; // Depth map output
    cv::Mat map1L, map2L, map1R, map2R; // Rectification maps for `cv::remap()`

private:
    // StereoSGBM object for disparity map calculation
    cv::Ptr<cv::StereoSGBM> stereo;

public:

    // Load intrinsic and extrinsic calibration data from YAML files
    void getCalibDat();

    // Load SGBM stereo matching parameters from YAML file
    void getSGBMDat();

    // Calculate rectification maps (map1L, map2L, map1R, map2R) for `cv::remap()`
    void getUndistortMapDat(float w, float h);

    // Compute disparity map and depth map using SGBM
    void getDepthMap(Mat leftEyeImg, Mat rightEyeImg);
};

} // namespace VarjoExamples
