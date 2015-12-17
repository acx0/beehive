#ifndef UTIL_H
#define UTIL_H

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

namespace util
{
    std::string get_escaped_string(const std::string &str);
    std::string strip_newline(const std::string &str);
    std::string get_frame_hex(const std::vector<uint8_t> &frame, bool show_prefix = false);
    void sleep(unsigned int seconds);   // TODO: use chrono?
}

#endif
