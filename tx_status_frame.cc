#include "tx_status_frame.h"

// api_identifier + frame_id + status
const size_t tx_status_frame::MIN_FRAME_DATA_LENGTH = sizeof(api_identifier_value) + sizeof(frame_id) + sizeof(status);

tx_status_frame::tx_status_frame(std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end)
    : frame_data(api_identifier::tx_status)
{
    if (end - begin < MIN_FRAME_DATA_LENGTH)
    {
        // TODO: exception
        std::cerr << "invalid frame size" << std::endl;
    }

    if (begin[0] != api_identifier::tx_status)
    {
        // TODO: exception
        std::cerr << "not a tx_status frame" << std::endl;
    }

    frame_id = begin[1];
    status = begin[2];
}

tx_status_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(api_identifier_value);
    frame.push_back(frame_id);
    frame.push_back(status);

    return frame;
}
