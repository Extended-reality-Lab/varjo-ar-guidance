#include <Varjo_mr_experimental.h> // enables point cloud construction

#include <iostream>
#include <cstdint>
#include "half.h"

#include <iostream>// write to file
#include <fstream> // write to file
#include <vector> // create a vector of structs that can be resized at runtime

#include <map>

using namespace std;

class VarjoPCScene{
    public:
        struct PointsData{ // Tracks size of point cloud and points to be written at the end of our scan
            int frameWidth = 832;
            int frameHeight = 640;
            size_t pointCount = 0;
            vector<varjo_PointCloudPoint> points;
            map <int, varjo_PointCloudPoint> pointsMap; // int key should be the globally unique index from input struct
        }; PointsData storedPointsData;

        varjo_PointCloudSnapshotId snapshotId;
        varjo_PointCloudDeltaContent varjoCloudDelta;
        varjo_PointCloudSnapshotContent varjoCloudContent;

        VarjoPCScene();

        void intializeCloud(struct varjo_Session* currentSession,  int numFrames, bool willOptimizeCloud);

        void writeCloudPCD(const char* input);
        void writeCloudPLY(const char* input);
};