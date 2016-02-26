#ifndef FRAME_PROCESSOR_H
#define FRAME_PROCESSOR_H

#include <condition_variable>
#include <cstdint>
#include <ios>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <boost/tokenizer.hpp>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "connection_tuple.h"
#include "frame_data.h"
#include "message_segment.h"
#include "reliable_channel.h"
#include "rx_packet_64_frame.h"
#include "tx_request_64_frame.h"
#include "util.h"
#include "xbee_s1.h"

class frame_processor
{
public:
    bool try_initialize_hardware();
    void run();

private:
    static void log_segment(const connection_tuple &key, const message_segment &segment);
    static uint32_t get_next_socket_suffix();
    static std::string read_message(int socket_fd);
    static void send_message(int socket_fd, const std::string &message);
    static bool is_message(const std::string &message_type, const std::string &request);

    void frame_reader();
    void frame_writer();
    void channel_manager();
    void passive_socket_manager(int socket_fd);
    void active_socket_manager(int socket_fd, uint64_t destination_address, uint8_t port);

    static const std::string BEEHIVE_SOCKET_PATH;
    static const std::string CONTROL_PATH_PREFIX;
    static const std::string COMMUNICATION_PATH_PREFIX;
    static const size_t MAX_MESSAGE_SIZE;
    static const std::string MESSAGE_LISTEN;
    static const std::string MESSAGE_CONNECT;
    static const std::string MESSAGE_ACCEPT;
    static const std::string MESSAGE_INVALID;
    static const std::string MESSAGE_USED;
    static const std::string MESSAGE_FAILED;
    static const std::string MESSAGE_OK;
    static const std::string MESSAGE_SEPARATOR;

    static uint32_t socket_suffix;
    static std::mutex socket_suffix_lock;

    xbee_s1 xbee;
    std::unordered_map<connection_tuple, std::shared_ptr<reliable_channel>, connection_tuple_hasher> channel_map;
    std::mutex write_lock;  // TODO: might want to factor out queue+locking code into separate class
    std::queue<std::shared_ptr<std::vector<uint8_t>>> write_queue;
    std::condition_variable condition;
    std::set<uint8_t> listen_ports;
};

#endif
