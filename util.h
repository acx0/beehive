#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
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

    // TODO: use iterators as input instead?
    // unpack byte vector of size n into single value of width n bytes, MSB first
    template <typename T>
    T unpack_bytes_to_width(const std::vector<uint8_t> &bytes)
    {
        if (bytes.size() != sizeof(T))
        {
            // TODO: exception
            std::cerr << "byte vector size does not match size of template type" << std::endl;
            return T();
        }

        T unpacked_value = 0;
        for (size_t i = 0; i < bytes.size(); ++i)
        {
            unpacked_value += static_cast<T>(bytes[i]) << (sizeof(uint8_t) * (bytes.size() - i - 1) * 8);
        }

        return unpacked_value;
    }

    // pack value of width n bytes into byte vector, MSB first
    template <typename T>
    void pack_value_as_bytes(std::vector<uint8_t> &frame, T value)
    {
        auto width = sizeof(T);
        T mask = 0xff;

        for (int i = width - 1; i >= 0; --i)
        {
            auto shift = sizeof(uint8_t) * i * 8;
            frame.push_back((value & (mask << shift)) >> shift);
        }
    }
}

#endif
