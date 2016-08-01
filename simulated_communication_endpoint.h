#ifndef SIMULATED_COMMUNICATION_ENDPOINT_H
#define SIMULATED_COMMUNICATION_ENDPOINT_H

#include <cstdint>
#include <exception>
#include <iterator>
#include <limits>
#include <memory>
#include <random>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>

#include "communication_endpoint.h"
#include "simulated_broadcast_medium.h"
#include "uart_frame.h"
#include "util.h"

class simulated_communication_endpoint : public communication_endpoint
{
public:
    simulated_communication_endpoint();

    virtual uint64_t get_address();
    virtual void transmit_frame(const std::vector<uint8_t> &payload);
    virtual std::shared_ptr<uart_frame> receive_frame();

private:
    static uint64_t get_random_address();

    uint64_t address;
    int socket_fd;
};

#endif
