
#include <iostream>
#include <string_view>
#include <vector>

// Varjo includes
#include <Varjo.h>
#include <Varjo_events.h>
#include <Varjo_mr.h>

// Internal includes
#include "D3D11MultiLayerView.hpp"
#include "D3D11Renderer.hpp"
#include "DataStreamer.hpp"
#include "Globals.hpp"
#include "frame.pb.h"

class ProtobufFrameSerializer {
public:
    bool serializeFrame(const VarjoExamples::DataStreamer::Frame& frame);

    const std::vector<uint8_t>& getSerializedData() const;

private:
    Frame m_protoframe;
    Intrinsics m_intrinsics;
    std::vector<uint8_t> m_protobuf_data;
};
