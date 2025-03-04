#include "ZmqSender.hpp"

#include <string>

ZmqSender::ZmqSender(const std::string& port) {
    m_ctx = std::make_unique<zmq::context_t>(1);
    m_publisher = std::make_unique<zmq::socket_t>(*m_ctx, zmq::socket_type::pub);
    m_publisher->bind(port);
};

void ZmqSender::send_message(const std::vector<uint8_t>& message) {
    zmq::message_t msg(message.size());
    std::memcpy(msg.data(), message.data(), message.size());
    m_publisher->send(msg, zmq::send_flags::none);
}
