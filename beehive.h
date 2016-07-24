#ifndef BEEHIVE_H
#define BEEHIVE_H

#include <chrono>
#include <cstdint>
#include <ios>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "beehive_message.h"
#include "channel_manager.h"
#include "connection_tuple.h"
#include "datagram_socket_manager.h"
#include "logger.h"
#include "message_segment.h"
#include "rx_packet_64_frame.h"
#include "threadsafe_unordered_map.h"
#include "tx_request_64_frame.h"
#include "uart_frame.h"
#include "util.h"
#include "xbee_s1.h"

struct neighbour_info
{
    uint64_t address;
    std::chrono::system_clock::time_point timestamp;
};

class beehive
{
public:
    beehive();

    void run();

private:
    static void log_segment(const connection_tuple &key, std::shared_ptr<message_segment> segment);
    static bool try_parse_ieee_address(const std::string &str, uint64_t &address);
    static bool try_parse_port(int client_socket_fd, const std::string &str, uint16_t &port);

    void request_handler();
    void frame_processor();
    // TODO: frame reads/writes should be explicitly scheduled
    //  - note: xbee_s1::DEFAULT_SERIAL_TIMEOUT_MS will determine the longest duration a single frame read will take, will need to take this
    //      into consideration when trying to fairly schedule reads/writes
    void frame_reader();
    void frame_writer();
    void neighbour_discoverer();
    void process_neighbour_discovery_message(uint64_t source_address, std::shared_ptr<message_segment> segment);

    static const std::string BEEHIVE_SOCKET_PATH;
    static const std::chrono::milliseconds FRAME_READER_SLEEP_DURATION;

    xbee_s1 xbee;
    // TODO: bound frame_writer_queue to a fixed size?
    std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<std::vector<uint8_t>>>> frame_writer_queue;
    threadsafe_blocking_queue<std::shared_ptr<uart_frame>> frame_processor_queue;
    channel_manager _channel_manager;
    datagram_socket_manager _datagram_socket_manager;
    threadsafe_unordered_map<uint64_t, neighbour_info> neighbours;
};

#endif
