#include "tx_status_frame.h"

// api_identifier + frame_id + status
const size_t tx_status_frame::MIN_FRAME_DATA_LENGTH = 3;

tx_status_frame::tx_status_frame(const std::vector<uint8_t> &frame)
    : frame_data(api_identifier::tx_status)
{
    if (frame[0] != api_identifier::tx_status)
    {
        // TODO: exception
        std::cerr << "not a tx_status frame" << std::endl;
    }

    if (frame.size() < MIN_FRAME_DATA_LENGTH)
    {
        // TODO: exception
        std::cerr << "invalid frame size" << std::endl;
    }

    frame_id = frame[1];
    status = frame[2];
}

tx_status_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(api_identifier);
    frame.push_back(frame_id);
    frame.push_back(status);

    return frame;
}
