#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "beehive_config.h"
#include "beehive_message.h"
#include "connection_tuple.h"
#include "logger.h"
#include "message_segment.h"
#include "port_manager.h"
#include "reliable_channel.h"
#include "threadsafe_blocking_queue.h"
#include "threadsafe_unordered_map.h"
#include "tx_request_64_frame.h"
#include "uart_frame.h"

// TODO: clean up domain socket fds
class channel_manager
{
public:
    channel_manager(const beehive_config &config,
        std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<std::vector<uint8_t>>>>
            write_queue);

    void set_local_address(uint64_t address);
    // TODO: no point of returning bool here?
    bool try_create_passive_socket(int client_socket_fd, uint16_t listen_port);
    bool try_create_active_socket(
        int client_socket_fd, uint64_t destination_address, uint16_t destination_port);
    void process_stream_segment(
        connection_tuple connection_key, std::shared_ptr<message_segment> segment);

private:
    static uint32_t get_next_socket_suffix();

    void incoming_connection_handler(connection_tuple connection_key,
        std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<message_segment>>> segment_queue);
    // TODO: difference between client_socket_fd and control_socket_fd? same?
    void passive_socket_manager(int client_socket_fd, uint16_t listen_port);
    void active_socket_manager(
        int control_socket_fd, uint64_t destination_address, uint16_t destination_port);
    void payload_read_handler(int listen_socket_fd, connection_tuple connection_key);
    void payload_write_handler(
        int control_socket_fd, int listen_socket_fd, connection_tuple connection_key);

    static uint32_t socket_suffix;
    static std::mutex socket_suffix_lock;

    const std::string channel_path_prefix;
    uint64_t local_address;
    std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<std::vector<uint8_t>>>> write_queue;
    // TODO: separate segment maps for payload vs control segments?, separate state/connection
    // management into separate class?
    threadsafe_unordered_map<uint16_t,
        std::shared_ptr<threadsafe_blocking_queue<std::pair<uint64_t, uint16_t>>>>
        connection_requests;
    threadsafe_unordered_map<connection_tuple,
        std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<message_segment>>>,
        connection_tuple_hasher>
        segment_queue_map;
    port_manager _port_manager;
};

#endif
