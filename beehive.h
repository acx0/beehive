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
#include "logger.h"
#include "message_segment.h"
#include "rx_packet_64_frame.h"
#include "uart_frame.h"
#include "util.h"
#include "xbee_s1.h"

class beehive
{
public:
    beehive();

    void run();

private:
    static void log_segment(const connection_tuple &key, std::shared_ptr<message_segment> segment);
    static bool try_parse_ieee_address(const std::string &str, uint64_t &address);
    static bool try_parse_port(const std::string &str, int &port);

    void request_handler();
    void frame_processor();
    void frame_reader();
    void frame_writer();

    static const std::string BEEHIVE_SOCKET_PATH;
    static const std::chrono::milliseconds FRAME_READER_SLEEP_DURATION;

    xbee_s1 xbee;
    std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<std::vector<uint8_t>>>> frame_writer_queue;
    threadsafe_blocking_queue<std::shared_ptr<uart_frame>> frame_processor_queue;
    channel_manager channel_manager;
};

#endif
