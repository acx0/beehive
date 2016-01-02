#include "rx_packet_64_frame.h"

// api_identifier + source_address (8 bytes) + rssi + options
// TODO: can rf_data be empty?
// TODO: replace this with sum of class member sizeof()s
const size_t rx_packet_64_frame::MIN_FRAME_DATA_LENGTH = 11;

rx_packet_64_frame::rx_packet_64_frame(const std::vector<uint8_t> &frame)
    : frame_data(api_identifier::rx_packet_64)
{
    if (frame[0] != api_identifier::rx_packet_64)
    {
        // TODO: exception
        std::cerr << "not an rx_packet_64 frame" << std::endl;
    }

    if (frame.size() < MIN_FRAME_DATA_LENGTH)
    {
        // TODO: exception
        std::cerr << "invalid frame size" << std::endl;
    }

    // unpack 8 byte address from bytes 1-8
    auto offset = 1;
    source_address = util::unpack_bytes_to_width<uint64_t>(std::vector<uint8_t>(frame.begin() + offset, frame.begin() + offset + 8));
    rssi = frame[9];
    options = frame[10];

    if (frame.size() > MIN_FRAME_DATA_LENGTH)
    {
        rf_data = std::vector<uint8_t>(frame.begin() + MIN_FRAME_DATA_LENGTH, frame.end());
    }
}

rx_packet_64_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(api_identifier);
    util::pack_value_as_bytes(frame, source_address);   // pack 8 byte address as 8 single bytes
    frame.push_back(rssi);
    frame.push_back(options);
    frame.insert(frame.end(), rf_data.begin(), rf_data.end());

    return frame;
}
