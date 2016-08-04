#include "tx_status_frame.h"

const size_t tx_status_frame::MIN_FRAME_DATA_LENGTH = sizeof(api_identifier_value) + sizeof(frame_id) + sizeof(status);

std::shared_ptr<tx_status_frame> tx_status_frame::parse_frame(std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end)
{
    if (end - begin < MIN_FRAME_DATA_LENGTH)
    {
        return nullptr;
    }

    if (begin[0] != api_identifier::tx_status)
    {
        return nullptr;
    }

    uint8_t frame_id = begin[1];
    uint8_t status = begin[2];

    return std::make_shared<tx_status_frame>(frame_id, status);
}

tx_status_frame::tx_status_frame(uint8_t frame_id, uint8_t status)
    : frame_data(api_identifier::tx_status), frame_id(frame_id), status(status)
{
}

tx_status_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(api_identifier_value);
    frame.push_back(frame_id);
    frame.push_back(status);

    return frame;
}
