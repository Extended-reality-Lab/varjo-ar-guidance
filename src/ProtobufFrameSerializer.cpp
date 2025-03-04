#include "ProtobufFrameSerializer.hpp"

#include "DataStreamer.hpp"

bool ProtobufFrameSerializer::serializeFrame(const VarjoExamples::DataStreamer::Frame& frame) {
    m_protoframe.set_height(frame.metadata.bufferMetadata.rowStride);
    m_protoframe.set_width(frame.metadata.bufferMetadata.height);

    std::string_view str_frame((const char*)frame.data.data(), frame.data.size());
    m_protoframe.set_pixels(str_frame);

    // Add extrinsics matrix values to the protobuf
    for (int i = 0; i < sizeof(frame.metadata.extrinsics.value); i++) {
        m_protoframe.add_extrinsics_matrix(frame.metadata.extrinsics.value[i]);
    }

    // Set intrinsic values in the protobuf
    m_intrinsics.set_principalpointx(frame.metadata.intrinsics.principalPointX);
    m_intrinsics.set_principalpointy(frame.metadata.intrinsics.principalPointY);
    m_intrinsics.set_focallengthx(frame.metadata.intrinsics.focalLengthX);
    m_intrinsics.set_focallengthy(frame.metadata.intrinsics.focalLengthY);

    // Add distortion coefficients to the protobuf
    for (int i = 0; i < sizeof(frame.metadata.intrinsics.distortionCoefficients); i++) {
        m_intrinsics.add_distortioncoefficients(frame.metadata.intrinsics.distortionCoefficients[i]);
    }

    *m_protoframe.mutable_intrinsics() = m_intrinsics;

    m_protoframe.set_ts(frame.metadata.timestamp);

    m_protobuf_data.resize(m_protoframe.ByteSizeLong());

    if (!m_protoframe.SerializeToArray(m_protobuf_data.data(), m_protobuf_data.size())) {
        std::cerr << "Failed to serialize protobuf message." << std::endl;
        return false;
    }

    return true;
}


bool ProtobufFrameSerializer::addDepthMap(const cv::Mat& depthMap) {

    if (depthMap.empty()) {
        std::cerr << "Depth map is empty!" << std::endl;
        return false;
    }
    std::vector<uchar> depthData;

    depthData.assign(depthMap.datastart, depthMap.dataend);


    m_protoframe.set_depthpixels(depthData.data(), depthData.size());

    return true;
}

const std::vector<uint8_t>& ProtobufFrameSerializer::getSerializedData() const { return m_protobuf_data; }
