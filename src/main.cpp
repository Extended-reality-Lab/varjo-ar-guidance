#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <atomic>
#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>

// Varjo includes
#include <Varjo.h>
#include <Varjo_events.h>
#include <Varjo_mr.h>

// Internal includes
#include "Globals.hpp"
#include "D3D11Renderer.hpp"
#include "D3D11MultiLayerView.hpp"
#include "DataStreamer.hpp"

#include "frame.pb.h"


using namespace VarjoExamples;

namespace
{
// Input actions
enum class InputAction {
    None,
    Quit,
    LockMarkers,
    IncreaseMarkerVolume,
    DecreaseMarkerVolume
};

const std::unordered_map<InputAction, std::string> c_inputNames = {
    {InputAction::None, "None"},
    {InputAction::Quit, "Quit"},
    {InputAction::LockMarkers, "Lock Markers"},
    {InputAction::IncreaseMarkerVolume, "Increase Marker Volume"},
    {InputAction::DecreaseMarkerVolume, "Decrease Marker Volume"},
};

const std::unordered_map<WORD, InputAction> s_inputActionMapping = {
    {VK_ESCAPE, InputAction::Quit},
    {VK_SPACE, InputAction::LockMarkers},
    {VK_UP, InputAction::IncreaseMarkerVolume},
    {VK_DOWN, InputAction::DecreaseMarkerVolume},
};

std::atomic_bool ctrlCPressed = false;

BOOL WINAPI ctrlHandler(DWORD /*dwCtrlType*/)
{
    ctrlCPressed = true;
    return TRUE;
}
}  // namespace

class TestClient
{
public:
    // Construct client app
    TestClient(varjo_Session* session)
        : m_session(session)
    {
        // Initialize D3D11 renderer and view
        auto dxgiAdapter = D3D11MultiLayerView::getAdapter(session);
        auto d3d11Renderer = std::make_unique<D3D11Renderer>(dxgiAdapter.Get());
        m_varjoView = std::make_unique<D3D11MultiLayerView>(session, *d3d11Renderer);
        m_renderer = std::move(d3d11Renderer);

        // Initialize DataStreamer
        m_streamer = std::make_unique<DataStreamer>(m_session, std::bind(&TestClient::onFrameReceived, this, std::placeholders::_1));
        m_streamer->startDataStream(varjo_StreamType_DistortedColor, varjo_TextureFormat_NV12, varjo_ChannelFlag_Left);

        // Check if Mixed Reality features are available
        varjo_Bool mixedRealityAvailable = varjo_False;
        varjo_SyncProperties(m_session);
        if (varjo_HasProperty(m_session, varjo_PropertyKey_MRAvailable)) {
            mixedRealityAvailable = varjo_GetPropertyBool(m_session, varjo_PropertyKey_MRAvailable);
        }

        if (mixedRealityAvailable == varjo_False) {
            LOG_ERROR("ERROR: Varjo Mixed Reality features not available!");
            exit(EXIT_FAILURE);
        }

        // Start video-see-through and depth estimation
        varjo_MRSetVideoRender(m_session, varjo_True);
        varjo_MRSetVideoDepthEstimation(m_session, varjo_True);

        

        // Initialize ZeroMQ context and publisher
        m_ctx = std::make_unique<zmq::context_t>(1);
        m_publisher = std::make_unique<zmq::socket_t>(*m_ctx, zmq::socket_type::pub);
        m_publisher->bind("tcp://*:5555");
    }

    // Destructor
    ~TestClient()
    {
        // Stop video-see-through
        varjo_MRSetVideoRender(m_session, varjo_False);
        varjo_MRSetVideoDepthEstimation(m_session, varjo_False);

        // Free resources
        m_varjoView.reset();
        m_renderer.reset();
        m_streamer->stopDataStream(varjo_StreamType_DistortedColor, varjo_TextureFormat_NV12);
    }

    // Disable copy and assign
    TestClient(const TestClient& other) = delete;
    TestClient(const TestClient&& other) = delete;
    TestClient& operator=(const TestClient& other) = delete;
    TestClient& operator=(const TestClient&& other) = delete;

    // Frame received callback
    void onFrameReceived(const DataStreamer::Frame& frame)
    {
        using namespace std::chrono;
        
        auto start_time = high_resolution_clock::now();
        // 
        // printf("height: %d\n", frame.metadata.bufferMetadata.height);
        // // // printf("Width: %d\n", frame.metadata.bufferMetadata.width);
        // printf("RowStride: %d\n", frame.metadata.bufferMetadata.rowStride);
        // //  printf("size: %d bytes\n", frame.data.size());
        

        m_protoframe.set_height(frame.metadata.bufferMetadata.rowStride);
        m_protoframe.set_width(frame.metadata.bufferMetadata.height);
        std::string_view str_frame((const char*)frame.data.data(), frame.data.size());
        m_protoframe.set_pixels(str_frame);


        if (!m_protoframe.IsInitialized()) {
            std::cerr << "Error: Message is not fully initialized!" << std::endl;
            return;
        } else {
            std::cout << "Message is initialized." << std::endl;
        }


        m_protobuf_data.resize(m_protoframe.ByteSizeLong());

        if (!m_protoframe.SerializeToArray(m_protobuf_data.data(), m_protobuf_data.size())) {
            std::cerr << "Failed to serialize protobuf message." << std::endl;
            return;
        }

 
        zmq::message_t zmq_msg{m_protobuf_data.data(), m_protobuf_data.size()};

        // Send the message to the publisher
        m_publisher->send(zmq_msg, zmq::send_flags::none);

        // std::cout << "Sent: " << message << std::endl;

        auto end_time = high_resolution_clock::now();
        auto total_duration = duration_cast<milliseconds>(end_time - start_time);  
        std::cout << "Operation took " << total_duration.count() << " milliseconds." << std::endl;

          ++msg_count;

    }

    // Main loop
    void run()
    {
        InputAction input = InputAction::None;

        while (true) {
            // Check for keyboard input
            input = checkInput();
            assert(c_inputNames.count(input));
            std::string inputName = c_inputNames.at(input);

            // Check for quit or Ctrl-C
            if (input == InputAction::Quit || ctrlCPressed) {
                LOG_INFO("Quitting main loop..");
                break;
            }

            // Sync frame
            m_varjoView->syncFrame();

            // Begin frame rendering
            m_varjoView->beginFrame();

            // Render layer
            {
                constexpr int layerIndex = 0;
                auto& layer = m_varjoView->getLayer(layerIndex);
                MultiLayerView::Layer::SubmitParams submitParams{};
                submitParams.submitColor = true;
                submitParams.submitDepth = true;
                submitParams.alphaBlend = true;
                submitParams.depthTestEnabled = false;
                submitParams.depthTestRangeEnabled = false;
                submitParams.depthTestRangeLimits = {0.0, -1.0};
                submitParams.chromaKeyEnabled = false;

                // Begin layer rendering
                layer.begin(submitParams);
                layer.clear();
                layer.end();
            }

            // End and submit frame
            m_varjoView->endFrame();
        }
    }

private:
    // Check for keyboard input
    InputAction checkInput()
    {
        HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
        INPUT_RECORD input;
        DWORD cnt = 0;
        while (GetNumberOfConsoleInputEvents(in, &cnt) && cnt > 0) {
            if (ReadConsoleInputA(in, &input, 1, &cnt) && cnt > 0 && input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown) {
                auto it = s_inputActionMapping.find(input.Event.KeyEvent.wVirtualKeyCode);
                if (it != s_inputActionMapping.end()) {
                    return it->second;
                }
            }
        }
        return InputAction::None;
    }

private:
    varjo_Session* m_session = nullptr;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<MultiLayerView> m_varjoView;
    std::unique_ptr<VarjoExamples::DataStreamer> m_streamer;

    // ZeroMQ context and publisher
    std::unique_ptr<zmq::context_t> m_ctx;
    std::unique_ptr<zmq::socket_t> m_publisher;
    std::vector<uint8_t> m_protobuf_data;
    Frame m_protoframe{};


    int msg_count = 0;
};

int main(int argc, char** argv)
{
    // Exit gracefully when Ctrl-C is pressed
    SetConsoleCtrlHandler(ctrlHandler, TRUE);

    LOG_INFO("Varjo RGB Test");

    // Initialize Varjo session
    LOG_INFO("Initializing varjo session..");
    varjo_Session* session = varjo_SessionInit();
    CHECK_VARJO_ERR(session);

    // Instantiate test client and start main loop
    LOG_INFO("Initializing client app..");
    auto client = std::make_unique<TestClient>(session);
    LOG_INFO("Running client app..");
    client->run();

    // Shutdown Varjo session
    LOG_INFO("Shutting down varjo session..");
    varjo_SessionShutDown(session);

    // Exit successfully
    LOG_INFO("Done!");
    return EXIT_SUCCESS;
}