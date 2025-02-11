#pragma once

#include "DepthManager.hpp"

namespace VarjoExamples{

    void DepthManager::getCalibDat(){
        cv::FileStorage int_fs("../../src/CamParams/intrinsics.yml", cv::FileStorage::READ);
        cv::FileStorage ext_fs("../../src/CamParams/extrinsics.yml", cv::FileStorage::READ);

        int_fs["M1"] >> storedCalibData.M1;
        int_fs["M2"] >> storedCalibData.M2;
        int_fs["D1"] >> storedCalibData.D1;
        int_fs["D2"] >> storedCalibData.D2;
        int_fs["Skew1"] >> storedCalibData.Skew1;
        int_fs["Skew2"] >> storedCalibData.Skew2;
        int_fs["XI1"] >> storedCalibData.XI1;
        int_fs["XI2"] >> storedCalibData.XI2;
        ext_fs["R"] >> storedCalibData.R;
        ext_fs["T"] >> storedCalibData.T;
        ext_fs["Q"] >> storedCalibData.Q;

        int_fs.release();
        ext_fs.release();
    }

    void DepthManager::getSGBMDat(){
        cv::FileStorage sgbmDat("../../src/CamParams/sgbmdat.yml", cv::FileStorage::READ);

        sgbmDat["NUMDISPARITY"] >> storedSGBMData.numDisparity;
        sgbmDat["BLOCKSIZE"] >> storedSGBMData.blockSize;
        sgbmDat["MINDISPARITY"] >> storedSGBMData.minDisp;
        sgbmDat["P1"] >> storedSGBMData.p1;
        sgbmDat["P2"] >> storedSGBMData.p2;
        sgbmDat["DISP12MAXDIFF"] >> storedSGBMData.disp12MaxDiff;
        sgbmDat["UNIQUENESSRATIO"] >> storedSGBMData.uniquenessRatio;
        sgbmDat["SPECKLEWINDOWSIZE"] >> storedSGBMData.speckleWindowSize;
        sgbmDat["SPECKLERANGE"] >> storedSGBMData.speckleRange;
        sgbmDat["PREFILTERCAP"] >> storedSGBMData.preFilterCap;

        sgbmDat.release();
    }

    void DepthManager::getUndistortMapDat(float w, float h){
        cv::Rect ROI;
        cv::Mat rL, rR;

        Size imgSize(w, h);

        storedCalibData.M1.row(0) *= w;
        storedCalibData.M1.row(1) *= h;
        storedCalibData.M2.row(0) *= w;
        storedCalibData.M2.row(1) *= h;

        cv::Mat kNew = getOptimalNewCameraMatrix(storedCalibData.M1, storedCalibData.D1, imgSize, 1, imgSize, &ROI);

        cv::omnidir::stereoRectify(storedCalibData.R, storedCalibData.T, rL, rR);

        // This outputs map1L, map2L, map1R, map2R. All necessary for remap()
        cv::omnidir::initUndistortRectifyMap(storedCalibData.M1, storedCalibData.D1, storedCalibData.XI1, rL, kNew, imgSize, CV_32FC1, map1L, map2L, cv::omnidir::RECTIFY_PERSPECTIVE);
        cv::omnidir::initUndistortRectifyMap(storedCalibData.M2, storedCalibData.D2, storedCalibData.XI2, rR, kNew, imgSize, CV_32FC1, map1R, map2R, cv::omnidir::RECTIFY_PERSPECTIVE);
    }

    void DepthManager::getDepthMap(Mat leftEyeImg, Mat rightEyeImg){
        cv::Ptr<cv::StereoSGBM> stereo = cv::StereoSGBM::create(storedSGBMData.minDisp, storedSGBMData.numDisparity, storedSGBMData.blockSize, 
                                                                storedSGBMData.p1, storedSGBMData.p2, storedSGBMData.disp12MaxDiff, storedSGBMData.preFilterCap, 
                                                                storedSGBMData.uniquenessRatio, storedSGBMData.speckleWindowSize, storedSGBMData.speckleRange, false);

        cv::Mat leftUndistorted, rightUndistorted;
        cv::Rect ROI;
        cv::Mat dispMap;

        cv::remap(leftEyeImg, leftUndistorted, map1L, map2L, INTER_LINEAR);
        cv::remap(rightEyeImg, rightUndistorted, map1R, map2R, INTER_LINEAR);

        //TODO: Look into why this makes depth manager sad :c
        //cv::Mat leftROI = leftUndistorted(ROI);
        //cv::Mat rightROI = rightUndistorted(ROI);

        stereo->compute(leftUndistorted, rightUndistorted, dispMap);

        floatDisp;
        dispMap.convertTo(floatDisp, CV_32F, 1.0/16);

        reprojectImageTo3D(floatDisp, depthMap, storedCalibData.Q, false);
    }
}