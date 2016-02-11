#include "frame_data.h"

const uint8_t frame_data::FRAME_ID_DISABLE_RESPONSE_FRAME = 0;
uint8_t frame_data::next_frame_id = 1;
std::mutex frame_data::frame_id_lock;

frame_data::frame_data(uint8_t api_identifier_value)
    : api_identifier_value(api_identifier_value)
{
}

uint8_t frame_data::get_next_frame_id()
{
    std::lock_guard<std::mutex> lock(frame_id_lock);

    // skip 0 as it has special meaning
    if (next_frame_id == FRAME_ID_DISABLE_RESPONSE_FRAME)
    {
        next_frame_id = 1;
    }

    return next_frame_id++;
}
