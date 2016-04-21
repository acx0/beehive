#include "port_manager.h"

const uint16_t port_manager::MAX_LISTEN_PORT = std::numeric_limits<uint16_t>::max() / 2;
const uint16_t port_manager::MAX_EPHEMERAL_PORT = std::numeric_limits<uint16_t>::max();

port_manager::port_manager()
    : mt(rd()), dist(MAX_LISTEN_PORT + 1, MAX_EPHEMERAL_PORT)
{
}

bool port_manager::is_listen_port(int port_int)
{
    return 0 <= port_int && port_int <= MAX_EPHEMERAL_PORT;
}

bool port_manager::try_open_listen_port(uint16_t port)
{
    std::lock_guard<std::mutex> lock(listen_access_lock);
    if (used_listen_ports.count(port))
    {
        return false;
    }

    used_listen_ports.insert(port);
    return true;
}

bool port_manager::try_get_random_ephemeral_port(uint16_t &port)
{
    std::lock_guard<std::mutex> lock(ephemeral_access_lock);
    if (used_ephemeral_ports.size() == MAX_EPHEMERAL_PORT - MAX_LISTEN_PORT)
    {
        return false;
    }

    uint16_t random_port = 0;

    do
    {
        random_port = dist(mt);
    }
    while (used_ephemeral_ports.count(random_port));

    port = random_port;
    used_ephemeral_ports.insert(port);

    return true;
}

void port_manager::release_port(uint16_t port)
{
    if (port <= MAX_LISTEN_PORT)
    {
        std::lock_guard<std::mutex> lock(listen_access_lock);
        used_listen_ports.erase(port);
    }
    else
    {
        std::lock_guard<std::mutex> lock(ephemeral_access_lock);
        used_ephemeral_ports.erase(port);
    }
}

bool port_manager::is_open(uint16_t port) const
{
    if (port <= MAX_LISTEN_PORT)
    {
        std::lock_guard<std::mutex> lock(listen_access_lock);
        return used_listen_ports.count(port);
    }
    else
    {
        std::lock_guard<std::mutex> lock(ephemeral_access_lock);
        return used_ephemeral_ports.count(port);
    }
}
