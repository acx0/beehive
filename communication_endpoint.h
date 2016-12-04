#ifndef COMMUNICATION_ENDPOINT_H
#define COMMUNICATION_ENDPOINT_H

#include <cstdint>
#include <memory>
#include <vector>

#include "uart_frame.h"

class communication_endpoint
{
public:
    virtual ~communication_endpoint()
    {
    }

    virtual uint64_t get_address() = 0;
    virtual void transmit_frame(const std::vector<uint8_t> &payload) = 0;
    // TODO: change signature to return failure status as bool and frame payload as out param?
    virtual std::shared_ptr<uart_frame> receive_frame() = 0;
};

#endif
