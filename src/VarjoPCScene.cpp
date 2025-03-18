#include "VarjoPCScene.hpp"

VarjoPCScene::VarjoPCScene(){
    struct varjo_Session* currentSession = nullptr; 
    snapshotId = NULL;
    varjoCloudDelta = {};
    varjoCloudContent = {};
    cout << "Initializer is done running" << endl;
}

// write point cloud as PLY file. Must be run after initializeCloud(). input will be file name. Do not include file extension in name
void VarjoPCScene::writeCloudPLY(const char* input) {
    if (!storedPointsData.pointsMap.empty()){

        std::string fileName = std::string(input) + ".ply";
        ofstream myFile(fileName);

        // Generate point cloud header
        myFile << "ply" << endl;
        myFile << "format ascii 1.0" << endl;
        myFile << "element vertex " << storedPointsData.pointsMap.size() << endl;
        myFile << "property float x" << endl;
        myFile << "property float y" << endl;
        myFile << "property float z" << endl;// set by system
        myFile << "property uint8 red" << endl; // set by system
        myFile << "property uint8 green" << endl;
        myFile << "property uint8 blue" << endl; // set by system
        myFile << "end_header" << endl;
    
        // Generate point cloud points
        for (const auto& [id, point]: storedPointsData.pointsMap) {
    
            // Varjo packs values 2 at a time into a 32 bit int. Unpacking
            uint16_t intX = (point.positionXY >> 16) & 0xFFFF;  // High 16 bits
            uint16_t intY = point.positionXY & 0xFFFF;          // Low 16 bits
            uint16_t intZ = (point.positionZradius) & 0xFFFF; // Note we're extracting what Varjo calls 'radius'. This is not a bug they got their data packed backwards
    
            uint16_t int16R = (point.normalZcolorR >> 16) &0xFFFF; // Varjo red color data is incorrectly stored under normalZ so we are shifting up instead of down here
            uint16_t int16G = (point.colorBG) &0xFFFF;
            uint16_t int16B = (point.colorBG >> 16) &0xFFFF;
        
            // Convert the float16 values to float
            float floatX = FLOAT16(intX);
            float floatY = FLOAT16(intY);
            float floatZ = FLOAT16(intZ);
    
            // normalizing 65K color values to 255 rgb vals
            int16R /= 255;
            int16G /= 255;
            int16B /= 255;
    
            uint8_t int8R = static_cast<uint8_t>(int16R);
            uint8_t int8B = static_cast<uint8_t>(int16B);
            uint8_t int8G = static_cast<uint8_t>(int16G);
    
            uint32_t rgb = ((uint32_t)int8R << 16 | (uint32_t)int8G << 8 | (uint32_t)int8B);
            
            myFile << floatX << " " << -floatY << " " << floatZ << " " << (unsigned)int8R << " " << (unsigned)int8G << " " << (unsigned)int8B << endl; // varjo points clouds return backwards on y-axis so floatY has been made negative here to rectify that
        }
        
        cout << "file written" << endl;
        myFile.close();
    }
    else{
        cerr << "ERROR: No points found. Please initialize point cloud before attempting to write it" << endl;
        return;
    }  
}

// write point cloud as PCD file. Must be run after initializeCloud(). input will be file name. Do not include file extension in name
void VarjoPCScene::writeCloudPCD(const char* input) { //TODO: Filename should be an input param

    if (!storedPointsData.pointsMap.empty()){

        std::string fileName = std::string(input) + ".pcd";

        ofstream myFile(fileName);

        // Generate point cloud header
        myFile << "VERSION .7" << endl;
        myFile << "FIELDS x y z rgb" << endl;
        myFile << "SIZE 4 4 4 4" << endl;
        myFile << "TYPE F F F U" << endl;
        myFile << "COUNT 1 1 1 1" << endl;
        myFile << "WIDTH " << storedPointsData.pointsMap.size() << endl;// set by system
        myFile << "HEIGHT " << 1 << endl; // set by system
        myFile << "VIEWPOINT 0 0 0 1 0 0 0" << endl;
        myFile << "POINTS " << storedPointsData.pointsMap.size() << endl; // set by system
        myFile << "DATA ascii" << endl;

        cout << "header written. Entering for loop" << endl;
    
        // Generate point cloud points
        for (const auto& [id, point]: storedPointsData.pointsMap) {
    
            // Varjo packs values 2 at a time into a 32 bit int. Unpacking
            uint16_t intX = (point.positionXY >> 16) & 0xFFFF;  // High 16 bits
            uint16_t intY = point.positionXY & 0xFFFF;          // Low 16 bits
            uint16_t intZ = (point.positionZradius) & 0xFFFF; // Note we're extracting what Varjo calls 'radius'. This is not a bug they got their data packed backwards
    
            uint16_t int16R = (point.normalZcolorR >> 16) &0xFFFF; // Varjo red color data is incorrectly stored under normalZ so we are shifting up instead of down here
            uint16_t int16G = (point.colorBG) &0xFFFF;
            uint16_t int16B = (point.colorBG >> 16) &0xFFFF;
        
            // Convert the float16 values to float
            float floatX = FLOAT16(intX);
            float floatY = FLOAT16(intY);
            float floatZ = FLOAT16(intZ);
    
            // normalizing 65K color values to 255 rgb vals
            int16R /= 255;
            int16G /= 255;
            int16B /= 255;
    
            uint8_t int8R = static_cast<uint8_t>(int16R);
            uint8_t int8B = static_cast<uint8_t>(int16B);
            uint8_t int8G = static_cast<uint8_t>(int16G);
    
            uint32_t rgb = ((uint32_t)int8R << 16 | (uint32_t)int8G << 8 | (uint32_t)int8B);
            
            myFile << floatX << " " << -floatY << " " << floatZ << " " << rgb << endl; // varjo points clouds return backwards on y-axis so floatY has been made negative here to rectify that
        }
        
        cout << "file written" << endl;
        myFile.close();
    }
    else{
        cerr << "ERROR: No points found. Please initialize point cloud before attempting to write it" << endl;
        return;
    }    
}

// create new point cloud. Will query Varjo headset for num of loops inputed for input "numFrames". WillOptimizeClouds will remove duplicate points
void VarjoPCScene::intializeCloud(struct varjo_Session* currentSession, int numFrames, bool willOptimizeCloud){

    varjo_MRSetReconstruction(currentSession, true);
    snapshotId = varjo_MRBeginPointCloudSnapshot(currentSession); // schedules new snapshot. I only want 1 for now but eventually this'll have to be under update as well as init

    int arrayTracker = 0;

    while(1){
        if (varjo_MRGetPointCloudSnapshotStatus(currentSession, snapshotId) == 2 ){ // if the current point cloud snapshot is ready
            
            varjo_MRGetPointCloudSnapshotContent(currentSession, snapshotId, &varjoCloudContent); // save current snapshot content to varjoCloudContent

            // Add all new points from current frame into points array. Update points count so writeCloud writes all points
            for (int i=storedPointsData.pointsMap.size(); i<varjoCloudContent.pointCount + storedPointsData.pointsMap.size(); i++){ // start at current size of points map. Add all new points
                uint16_t index = (varjoCloudContent.points[i].indexConfidence >> 8);
                uint16_t confidence = varjoCloudContent.points[i].indexConfidence & 0xFF;
                if (confidence > 7){ 
                    storedPointsData.pointsMap.insert({index, varjoCloudContent.points[i]});
                }
            }
            
            // if the program has not completed the current number of iterations, iterate and refresh
            if (arrayTracker != numFrames){
                arrayTracker++;

                varjo_MRGetPointCloudDelta(currentSession, snapshotId, &varjoCloudDelta);
                
                if (varjoCloudDelta.removedPointCount != 0){
                    for (int i=0; i<varjoCloudDelta.removedPointCount; i++){
                        cout<< &varjoCloudDelta.removedPointIds << endl;
                        cout<< varjoCloudDelta.removedPointIds << endl;
                        storedPointsData.pointsMap.erase(*varjoCloudDelta.removedPointIds); // make sure these points exist. I might need a check added later
                    }
                }

                varjo_MRPopPointCloudDelta(currentSession, snapshotId);

                // refresh point cloud so it's ready to build after first run
                varjo_MRReleasePointCloudSnapshot(currentSession, snapshotId); // release now unused snapshot
                snapshotId = varjo_MRBeginPointCloudSnapshot(currentSession); // request new snapshot
            }
            else{
                break;
            }
        }
        else{
            continue;
        }
    }
}