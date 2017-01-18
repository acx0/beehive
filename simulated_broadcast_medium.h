#ifndef SIMULATED_BROADCAST_MEDIUM_H
#define SIMULATED_BROADCAST_MEDIUM_H

#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>

#include "beehive_config.h"
#include "frame_data.h"
#include "logger.h"
#include "rx_packet_64_frame.h"
#include "threadsafe_unordered_map.h"
#include "tx_request_64_frame.h"
#include "uart_frame.h"
#include "util.h"
#include "xbee_s1.h"

// TODO: can add connectivity graph later to simulate different topologies
// TODO: handle node disconnect/expiration
class simulated_broadcast_medium
{
public:
    simulated_broadcast_medium(uint32_t packet_loss_percent);

    static std::vector<uint8_t> read_frame(int socket_fd);

    bool start();
    void node_traffic_forwarder(uint64_t node_address, int node_socket_fd);

private:
    uint32_t packet_loss_percent;
    std::mt19937 mt;
    std::uniform_int_distribution<uint32_t> dist;
    threadsafe_unordered_map<uint64_t, int> node_sockets;
};

#endif
