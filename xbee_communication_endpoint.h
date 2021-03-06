#ifndef XBEE_COMMUNICATION_ENDPOINT_H
#define XBEE_COMMUNICATION_ENDPOINT_H

#include <cstdint>
#include <stdexcept>
#include <string>

#include "communication_endpoint.h"
#include "xbee_s1.h"

class xbee_communication_endpoint : public communication_endpoint
{
public:
    xbee_communication_endpoint(const std::string &device, uint32_t baud);

    virtual uint64_t get_address();
    virtual void transmit_frame(const std::vector<uint8_t> &payload);
    virtual std::shared_ptr<uart_frame> receive_frame();

private:
    xbee_s1 xbee;
};

#endif
