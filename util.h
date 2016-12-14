#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <functional>
#include <iomanip>
#include <ios>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <boost/tokenizer.hpp>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#include <errno.h>
#include <unistd.h>

#include "logger.h"

namespace util
{
    std::string prefix_null_terminator(const std::string &str);
    std::string get_escaped_string(const std::string &str);
    std::string strip_newline(const std::string &str);
    std::string get_frame_hex(const std::vector<uint8_t> &frame, bool show_prefix = false);
    void sleep(unsigned int seconds);   // TODO: use chrono?
    int create_passive_abstract_domain_socket(const std::string &name, int type);
    int create_passive_domain_socket(const std::string &name, int type);
    int create_active_abstract_domain_socket(const std::string &name, int type);
    int create_active_domain_socket(const std::string &name, int type);
    int accept_connection(int socket_fd);
    bool try_configure_nonblocking_receive_timeout(int socket_fd);
    std::vector<std::string> split(const std::string &str, const std::string &separator);
    bool retry(std::function<bool()> function, uint32_t retries);
    bool try_parse_uint32_t(const std::string &str, uint32_t &out);
    ssize_t nonblocking_recv(int socket_fd, std::vector<uint8_t> &buffer, size_t buffer_length, int &error);

    // unpack byte vector of size n into single value of width n bytes, MSB first, n = sizeof(T)
    template <typename T, typename Iterator>
    T unpack_bytes_to_width(Iterator begin)
    {
        auto width = sizeof(T);
        T unpacked_value = 0;

        // TODO: why doesn't 'auto i' match with typeid(sizeof(T)), because of i's use in begin[] ?
        for (decltype(sizeof(T)) i = 0; i < width; ++i)
        {
            unpacked_value += static_cast<T>(begin[i]) << (sizeof(uint8_t) * (width - i - 1) * 8);
        }

        return unpacked_value;
    }

    // pack value of width n bytes into byte vector, MSB first
    template <typename T>
    void pack_value_as_bytes(std::back_insert_iterator<std::vector<uint8_t>> inserter, T value)
    {
        auto width = sizeof(T);
        T mask = 0xff;

        for (int i = width - 1; i >= 0; --i)
        {
            auto shift = sizeof(uint8_t) * i * 8;
            inserter = (value & (mask << shift)) >> shift;
        }
    }

    // promotes to type printable as number so that value isn't printed as char
    template <typename T>
    auto promote_to_printable_integer_type(T i) -> decltype(+i)
    {
        return +i;
    }

    template <typename T>
    std::string to_hex_string(T i)
    {
        std::ostringstream oss;
        oss << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << promote_to_printable_integer_type(i);
        return oss.str();
    }
}

#endif
