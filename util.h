#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/tokenizer.hpp>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#include <unistd.h>

namespace util
{
    std::string get_escaped_string(const std::string &str);
    std::string strip_newline(const std::string &str);
    std::string get_frame_hex(const std::vector<uint8_t> &frame, bool show_prefix = false);
    void sleep(unsigned int seconds);   // TODO: use chrono?
    int create_passive_domain_socket(const std::string &name);
    int create_active_domain_socket(const std::string &name);
    int accept_connection(int socket_fd);
    bool try_configure_nonblocking_receive_timeout(int socket_fd);
    std::vector<std::string> split(const std::string &str, const std::string &separator);

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

    template <typename T>
    std::string to_hex_string(T i)
    {
        std::ostringstream oss;
        oss << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
        return oss.str();
    }
}

#endif
