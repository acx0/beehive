#ifndef RELIABLE_CHANNEL_H
#define RELIABLE_CHANNEL_H

#include <deque>
#include <functional>
#include <mutex>
#include <vector>

#include "message_segment.h"

class reliable_channel
{
public:
    message_segment read();
    void write(const message_segment &segment);

private:
    std::deque<message_segment> segment_stream;
    std::mutex access_lock;
};

#endif
