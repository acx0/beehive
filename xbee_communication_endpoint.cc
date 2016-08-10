#include "xbee_communication_endpoint.h"

xbee_communication_endpoint::xbee_communication_endpoint()
{
    if (!xbee.initialize())
    {
        // TODO: throw specific exception
        throw std::exception();
    }
}

uint64_t xbee_communication_endpoint::get_address()
{
    return xbee.get_address();
}

void xbee_communication_endpoint::transmit_frame(const std::vector<uint8_t> &payload)
{
    xbee.write_frame(payload);
}

std::shared_ptr<uart_frame> xbee_communication_endpoint::receive_frame()
{
    return xbee.read_frame();
}
