#include "rx_packet_64_frame.h"

const size_t rx_packet_64_frame::SOURCE_ADDRESS_OFFSET
    = frame_data::API_IDENTIFIER_OFFSET + sizeof(frame_data::api_identifier_value);
const size_t rx_packet_64_frame::RSSI_OFFSET = SOURCE_ADDRESS_OFFSET + sizeof(source_address);
const size_t rx_packet_64_frame::OPTIONS_OFFSET = RSSI_OFFSET + sizeof(rssi);
const size_t rx_packet_64_frame::RF_DATA_OFFSET = OPTIONS_OFFSET + sizeof(options);
// TODO: rf_data can't be empty, xbee discards packet (double check)
const size_t rx_packet_64_frame::MIN_FRAME_DATA_LENGTH = RF_DATA_OFFSET;

std::shared_ptr<rx_packet_64_frame> rx_packet_64_frame::parse_frame(
    std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end)
{
    auto size = static_cast<size_t>(std::distance(begin, end));
    if (size < MIN_FRAME_DATA_LENGTH)
    {
        return nullptr;
    }

    if (begin[frame_data::API_IDENTIFIER_OFFSET] != api_identifier::rx_packet_64)
    {
        return nullptr;
    }

    // unpack 8 byte address from bytes 1-8
    uint64_t source_address = util::unpack_bytes_to_width<uint64_t>(begin + SOURCE_ADDRESS_OFFSET);
    uint8_t rssi = begin[RSSI_OFFSET];
    uint8_t options = begin[OPTIONS_OFFSET];
    std::vector<uint8_t> rf_data;

    if (size > RF_DATA_OFFSET)
    {
        rf_data = std::vector<uint8_t>(begin + RF_DATA_OFFSET, end);
    }

    return std::make_shared<rx_packet_64_frame>(source_address, rssi, options, rf_data);
}

rx_packet_64_frame::rx_packet_64_frame(
    uint64_t source_address, uint8_t rssi, uint8_t options, std::vector<uint8_t> rf_data)
    : frame_data(api_identifier::rx_packet_64), source_address(source_address), rssi(rssi),
      options(options), rf_data(rf_data)
{
}

uint64_t rx_packet_64_frame::get_source_address() const
{
    return source_address;
}

const std::vector<uint8_t> &rx_packet_64_frame::get_rf_data() const
{
    return rf_data;
}

bool rx_packet_64_frame::is_broadcast_frame() const
{
    return (options & (1 << options_bit::address_broadcast))
        || (options & (1 << options_bit::pan_broadcast));
}

rx_packet_64_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(api_identifier_value);
    util::pack_value_as_bytes(
        std::back_inserter(frame), source_address);    // pack 8 byte address as 8 single bytes
    frame.push_back(rssi);
    frame.push_back(options);
    frame.insert(frame.end(), rf_data.begin(), rf_data.end());

    return frame;
}
