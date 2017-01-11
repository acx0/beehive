#include "tx_status_frame.h"

const size_t tx_status_frame::FRAME_ID_OFFSET
    = frame_data::API_IDENTIFIER_OFFSET + sizeof(frame_data::api_identifier);
const size_t tx_status_frame::STATUS_OFFSET = FRAME_ID_OFFSET + sizeof(frame_id);
const size_t tx_status_frame::MIN_FRAME_DATA_LENGTH = STATUS_OFFSET + sizeof(status);

std::shared_ptr<tx_status_frame> tx_status_frame::parse_frame(
    std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end)
{
    if (static_cast<size_t>(std::distance(begin, end)) < MIN_FRAME_DATA_LENGTH)
    {
        return nullptr;
    }

    if (begin[frame_data::API_IDENTIFIER_OFFSET] != api_identifier::tx_status)
    {
        return nullptr;
    }

    uint8_t frame_id = begin[FRAME_ID_OFFSET];
    uint8_t status = begin[STATUS_OFFSET];

    return std::make_shared<tx_status_frame>(frame_id, status);
}

tx_status_frame::tx_status_frame(uint8_t frame_id, uint8_t status)
    : frame_data(api_identifier::tx_status), frame_id(frame_id), status_value(status)
{
}

tx_status_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(api_identifier_value);
    frame.push_back(frame_id);
    frame.push_back(status_value);

    return frame;
}
