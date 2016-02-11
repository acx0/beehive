#include "reliable_channel.h"

message_segment reliable_channel::read()
{
    return message_segment(std::vector<uint8_t>(message_segment::MIN_SEGMENT_LENGTH, 0x00));
}

void reliable_channel::write(const message_segment &segment)
{
    segment_stream.push_back(segment);
}
