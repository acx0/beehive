#ifndef FRAME_PROCESSOR_H
#define FRAME_PROCESSOR_H

#include <cstdint>
#include <ios>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "connection_tuple.h"
#include "frame_data.h"
#include "message_segment.h"
#include "port_manager.h"
#include "reliable_channel.h"
#include "rx_packet_64_frame.h"
#include "threadsafe_blocking_queue.h"
#include "threadsafe_unordered_map.h"
#include "tx_request_64_frame.h"
#include "uart_frame.h"
#include "util.h"
#include "xbee_s1.h"

class frame_processor
{
public:
    void run();

private:
    static void log_segment(const connection_tuple &key, std::shared_ptr<message_segment> segment);
    static uint32_t get_next_socket_suffix();
    static std::string read_message(int socket_fd);
    static void send_message(int socket_fd, const std::string &message);
    static bool is_message(const std::string &message_type, const std::string &request);
    static bool try_parse_ieee_address(const std::string &str, uint64_t &address);
    static bool try_parse_port(const std::string &str, int &port);

    void frame_reader();
    void frame_writer();
    void channel_manager();
    void passive_socket_manager(int socket_fd, uint16_t listen_port);
    void active_socket_manager(int socket_fd, uint64_t destination_address, uint16_t destination_port);
    void connection_handler(connection_tuple connection_key, std::shared_ptr<message_segment> segment);
    void payload_read_handler(int listen_socket_fd, connection_tuple connection_key);
    void payload_write_handler(int control_socket_fd, int listen_socket_fd, connection_tuple connection_key);

    static const std::string BEEHIVE_SOCKET_PATH;
    static const std::string COMMUNICATION_PATH_PREFIX;
    static const size_t MAX_MESSAGE_SIZE;
    static const std::string MESSAGE_LISTEN;
    static const std::string MESSAGE_CONNECT;
    static const std::string MESSAGE_ACCEPT;
    static const std::string MESSAGE_CLOSE;
    static const std::string MESSAGE_INVALID;
    static const std::string MESSAGE_USED;
    static const std::string MESSAGE_FAILED;
    static const std::string MESSAGE_OK;
    static const std::string MESSAGE_SEPARATOR;

    static uint32_t socket_suffix;
    static std::mutex socket_suffix_lock;

    xbee_s1 xbee;
    threadsafe_blocking_queue<std::shared_ptr<std::vector<uint8_t>>> write_queue;
    threadsafe_unordered_map<uint16_t, std::shared_ptr<threadsafe_blocking_queue<std::pair<uint64_t, uint16_t>>>> connection_requests;
    threadsafe_unordered_map<connection_tuple, std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<message_segment>>>, connection_tuple_hasher> segment_queue_map;
    port_manager pm;
};

#endif
