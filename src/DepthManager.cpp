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

    void DepthManager::getDispDat(){
        cv::FileStorage dispDat("../../src/CamParams/dispDat.yml", cv::FileStorage::READ);

        dispDat["NUMDISPARITY"] >> storedDispData.numDisparity;
        dispDat["BLOCKSIZE"] >> storedDispData.blockSize;
        dispDat["MINDISPARITY"] >> storedDispData.minDisp;
        dispDat["P1"] >> storedDispData.p1;
        dispDat["P2"] >> storedDispData.p2;
        dispDat["DISP12MAXDIFF"] >> storedDispData.disp12MaxDiff;
        dispDat["UNIQUENESSRATIO"] >> storedDispData.uniquenessRatio;
        dispDat["SPECKLEWINDOWSIZE"] >> storedDispData.speckleWindowSize;
        dispDat["SPECKLERANGE"] >> storedDispData.speckleRange;
        dispDat["PREFILTERCAP"] >> storedDispData.preFilterCap;

        dispDat["LAMBDA"] >> storedDispData.lambda;
        dispDat["SIGMA"] >> storedDispData.sigma;

        dispDat.release();
    }

    void DepthManager::getUndistortMapDat(float w, float h){
        cv::Rect ROI;
        cv::Mat rL, rR;

        Size imgSize(w, h);

        // Scale camera matricies before use
        storedCalibData.M1.row(0) *= w;
        storedCalibData.M1.row(1) *= h;
        storedCalibData.M2.row(0) *= w;
        storedCalibData.M2.row(1) *= h;

        cv::Mat kNew = getOptimalNewCameraMatrix(storedCalibData.M1, storedCalibData.D1, imgSize, 1, imgSize, &ROI);

        left_matcher = cv::StereoSGBM::create(storedDispData.minDisp, storedDispData.numDisparity, storedDispData.blockSize, 
            storedDispData.p1, storedDispData.p2, storedDispData.disp12MaxDiff, storedDispData.preFilterCap, 
            storedDispData.uniquenessRatio, storedDispData.speckleWindowSize, storedDispData.speckleRange, false);

        cv::omnidir::stereoRectify(storedCalibData.R, storedCalibData.T, rL, rR);

        // This outputs map1L, map2L, map1R, map2R. All necessary for remap()
        cv::omnidir::initUndistortRectifyMap(storedCalibData.M1, storedCalibData.D1, storedCalibData.XI1, rL, kNew, imgSize, CV_32FC1, map1L, map2L, cv::omnidir::RECTIFY_PERSPECTIVE);
        cv::omnidir::initUndistortRectifyMap(storedCalibData.M2, storedCalibData.D2, storedCalibData.XI2, rR, kNew, imgSize, CV_32FC1, map1R, map2R, cv::omnidir::RECTIFY_PERSPECTIVE);
        cout << "stereorectification complete" << kNew << endl;
    }

    void DepthManager::getDepthMap(Mat leftEyeImg, Mat rightEyeImg){
        if (!map1L.empty() && !map1R.empty()){
            cv::Mat leftUndistorted, rightUndistorted, left_for_matcher, right_for_matcher;

            cv::remap(leftEyeImg, leftUndistorted, map1L, map2L, INTER_LINEAR);
            cv::remap(rightEyeImg, rightUndistorted, map1R, map2R, INTER_LINEAR);
    
            cv::Mat left_disp, right_disp;
            
            // Downscale matcher images for performance boost
            resize(leftUndistorted ,left_for_matcher ,Size(),0.5,0.5);
            resize(rightUndistorted,right_for_matcher,Size(),0.5,0.5);

            // Compute left and right disparity map using values inputed into storedDispData struct
            Ptr<DisparityWLSFilter> disparityFilter = createDisparityWLSFilter(left_matcher);
            Ptr<StereoMatcher> right_matcher = createRightMatcher(left_matcher);

           left_matcher -> compute(left_for_matcher, right_for_matcher, left_disp);
           right_matcher -> compute(right_for_matcher, left_for_matcher, right_disp);

           disparityFilter->setLambda(storedDispData.lambda); // typical value is 8000
           disparityFilter->setSigmaColor(storedDispData.sigma); //Typical values range from 0.8 to 2.0.

           cv::Mat filtered_disp;
           disparityFilter->filter(left_disp, leftUndistorted, filtered_disp, right_disp, Rect(), rightUndistorted);

           // for debugging purposes while I'm fixing the disparity map
           Mat raw_disp_vis;
           getDisparityVis(left_disp,raw_disp_vis,1);
           namedWindow("raw disparity", WINDOW_AUTOSIZE);
           imshow("raw disparity", raw_disp_vis);
           Mat filtered_disp_vis;
           getDisparityVis(filtered_disp,filtered_disp_vis,1);
           imshow("Filtered disparity map", filtered_disp_vis);
           waitKey(1);

           Mat floatDisp;
           filtered_disp.convertTo(floatDisp, CV_32F, 1.0); // normally divided by 16. This leads to poor visibility on depth map
           // Creates and normalizes a representation of the depth display that's more readable and displayable
           cv::Mat depthDisplay;
           floatDisp.convertTo(depthDisplay, CV_8U, 255.0 / 16); // Normalize
           cv::applyColorMap(depthDisplay, depthDisplay, cv::COLORMAP_JET); // Reconverts image to color

           reprojectImageTo3D(floatDisp, depthMap, storedCalibData.Q, false);
        }
        else{
            cout << "ERROR: Maps for remapping not found. Must run getUndistortMapData() before running function: getDepthMap()" << endl;
        }

    }
}