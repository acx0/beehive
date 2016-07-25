#ifndef XBEE_COMMUNICATION_ENDPOINT_H
#define XBEE_COMMUNICATION_ENDPOINT_H

#include <chrono>
#include <stdexcept>

#include "communication_endpoint.h"
#include "xbee_s1.h"

class xbee_communication_endpoint : public communication_endpoint
{
public:
    xbee_communication_endpoint();

    virtual uint64_t get_address();
    virtual void transmit_frame(const std::vector<uint8_t> &payload);
    virtual std::shared_ptr<uart_frame> receive_frame();

private:
    static const std::chrono::milliseconds FRAME_READER_SLEEP_DURATION;

    xbee_s1 xbee;
};

#endif
