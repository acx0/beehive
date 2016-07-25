#include "xbee_communication_endpoint.h"

const std::chrono::milliseconds xbee_communication_endpoint::FRAME_READER_SLEEP_DURATION(25);   // TODO: test how small this can be without hogging lock

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
    auto frame = xbee.read_frame();
    if (frame == nullptr)
    {
        // note: sleep is needed to prevent xbee_s1::read_frame from keeping access lock held
        // TODO: better solution: explicitly schedule frame reads/writes?
        // TODO: std::this_thread::yield() didn't seem to work, try multiple yields?
        std::this_thread::sleep_for(FRAME_READER_SLEEP_DURATION);
    }

    return frame;
}
