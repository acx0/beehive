#ifndef PORT_MANAGER_H
#define PORT_MANAGER_H

#include <cstdint>
#include <limits>
#include <mutex>
#include <random>

#include "threadsafe_set.h"

class port_manager
{
public:
    port_manager();

    static bool is_valid_port(int port_int);

    bool try_open_listen_port(uint16_t port);
    bool try_get_random_ephemeral_port(uint16_t &port);
    void release_port(uint16_t port);

private:
    static const uint16_t MAX_LISTEN_PORT;
    static const uint16_t MAX_EPHEMERAL_PORT;

    mutable std::mutex listen_access_lock;
    threadsafe_set<uint16_t> used_listen_ports;
    mutable std::mutex ephemeral_access_lock;
    threadsafe_set<uint16_t> used_ephemeral_ports;
    std::random_device rd;
    std::mt19937 mt;
    std::uniform_int_distribution<int> dist;
};

#endif
