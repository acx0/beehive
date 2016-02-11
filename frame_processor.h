#ifndef FRAME_PROCESSOR_H
#define FRAME_PROCESSOR_H

#include <cstdint>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

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

    void test_write_messsage();

private:
    static void log_segment(const connection_tuple &key, const message_segment &segment);

    xbee_s1 xbee;
    std::unordered_map<connection_tuple, std::shared_ptr<reliable_channel>, connection_tuple_hasher> channel_map;
};

#endif
