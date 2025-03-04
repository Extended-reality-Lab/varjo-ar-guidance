#include <memory>
#include <vector>

#include <zmq.hpp>

// Varjo includes
#include <Varjo.h>
#include <Varjo_events.h>
#include <Varjo_mr.h>

// Internal includes
#include "DataStreamer.hpp"
#include "Globals.hpp"
#include "frame.pb.h"

class ZmqSender {
public:
    ZmqSender(const std::string& port);

    void send_message(const std::vector<uint8_t>& proto_message);

private:
    std::unique_ptr<zmq::context_t> m_ctx;
    std::unique_ptr<zmq::socket_t> m_publisher;
};
