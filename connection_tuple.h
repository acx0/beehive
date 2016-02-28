#ifndef CONNECTION_TUPLE_H
#define CONNECTION_TUPLE_H

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include <boost/functional/hash.hpp>

struct connection_tuple
{
    connection_tuple(uint64_t source_address, uint16_t source_port, uint64_t destination_address, uint16_t destination_port);

    std::string to_string() const;

    bool operator==(const connection_tuple &rhs) const;

    uint64_t source_address;
    uint16_t source_port;
    uint64_t destination_address;
    uint16_t destination_port;
    // TODO: message_type?
};

struct connection_tuple_hasher
{
    size_t operator()(const connection_tuple &ct) const;
};

#endif
