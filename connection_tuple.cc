#include "connection_tuple.h"

connection_tuple::connection_tuple(uint64_t source_address, uint16_t source_port,
    uint64_t destination_address, uint16_t destination_port)
    : source_address(source_address), source_port(source_port),
      destination_address(destination_address), destination_port(destination_port)
{
}

std::string connection_tuple::to_string() const
{
    std::ostringstream oss;

    oss << "(0x" << std::setfill('0') << std::setw(16) << std::hex << +source_address << ", ";
    oss << std::dec << +source_port << ", 0x" << std::setw(16) << std::hex << +destination_address
        << ", " << std::dec << +destination_port << ")";

    return oss.str();
}

bool connection_tuple::operator==(const connection_tuple &rhs) const
{
    return source_address == rhs.source_address && source_port == rhs.source_port
        && destination_address == rhs.destination_address
        && destination_port == rhs.destination_port;
}

size_t connection_tuple_hasher::operator()(const connection_tuple &ct) const
{
    size_t seed = 0;

    boost::hash_combine(seed, boost::hash_value(ct.source_address));
    boost::hash_combine(seed, boost::hash_value(ct.source_port));
    boost::hash_combine(seed, boost::hash_value(ct.destination_address));
    boost::hash_combine(seed, boost::hash_value(ct.destination_port));

    return seed;
}
