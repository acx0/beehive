#ifndef DATAGRAM_SOCKET_MANAGER_H
#define DATAGRAM_SOCKET_MANAGER_H

#include <chrono>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>

#include "beehive_config.h"
#include "beehive_message.h"
#include "message_segment.h"
#include "port_manager.h"
#include "threadsafe_blocking_queue.h"
#include "threadsafe_unordered_map.h"
#include "tx_request_64_frame.h"
#include "uart_frame.h"

struct datagram_segment
{
    uint64_t source_address;
    std::shared_ptr<message_segment> segment;
};

class datagram_socket_manager
{
public:
    datagram_socket_manager(const beehive_config &config, std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<std::vector<uint8_t>>>> write_queue);

    bool try_create_passive_socket(int control_socket_fd, uint16_t listen_port);
    bool try_create_active_socket(int control_socket_fd);
    void process_segment(uint64_t source_address, std::shared_ptr<message_segment> segment);

private:
    static uint32_t get_next_socket_suffix();

    void passive_socket_manager(int control_socket_fd, int listen_socket_fd, uint16_t listen_port, std::shared_ptr<threadsafe_blocking_queue<datagram_segment>> segment_queue);
    void active_socket_manager(int control_socket_fd);
    void destroy_socket(uint16_t port);
    void payload_read_handler(int communication_socket_fd, std::shared_ptr<threadsafe_blocking_queue<datagram_segment>> segment_queue, std::shared_ptr<bool> running);
    void payload_write_handler(int communication_socket_fd, uint16_t source_port, std::shared_ptr<bool> running);

    // TODO: defining locally for now since currently used for stack allocated buffer
    static const size_t MAX_MESSAGE_LENGTH;
    static uint32_t socket_suffix;
    static std::mutex socket_suffix_lock;

    const std::string dgram_path_prefix;;
    std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<std::vector<uint8_t>>>> write_queue;
    threadsafe_unordered_map<uint16_t, std::shared_ptr<threadsafe_blocking_queue<datagram_segment>>> segment_queue_map;
    port_manager _port_manager;
};

#endif
