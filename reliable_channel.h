#ifndef RELIABLE_CHANNEL_H
#define RELIABLE_CHANNEL_H

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "connection_tuple.h"
#include "logger.h"
#include "message_segment.h"
#include "threadsafe_blocking_queue.h"
#include "tx_request_64_frame.h"
#include "uart_frame.h"
#include "util.h"
#include "xbee_s1.h"

// TODO: make note of sender requiring SO_RCVTIMEO set (can we check for this?) or just set it
// within class?
//      - use getsockopt to check for this -> exception if not set
//  - might be neater to have reader()/writer() thread logic inside this class as public members
// TODO: need to verify: when sender requests CLOSE, send FIN only after current payload is sent?
// currently might be stopping loop early causing retransmission queue not to be fully retransmitted
// TODO: are we using a monotonic/steady clock?
// TODO: don't need to use timer... can manage ticks ourself?
//  - is boost deadline_timer safe from clock skew? -> boost steady_timer ?

// TODO: create structure similar to transmission control block (tcp) to hold connection state? ->
// reliable_channel_state ?

class reliable_channel
{
public:
    reliable_channel(connection_tuple connection_key, int communication_socket_fd,
        std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<std::vector<uint8_t>>>>
            write_queue,
        std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<message_segment>>>
            incoming_segment_queue);

    void start_sending();
    void start_receiving();
    void request_channel_close();
    void received_fin();

private:
    void retransmit_timed_out_unacked_segments();
    void retransmitter(const boost::system::error_code & /*e*/, boost::asio::deadline_timer *timer);
    void retransmit_timer();
    bool in_window(uint16_t sequence_number) const;
    bool in_previous_window(uint16_t sequence_number) const;
    void send_segments_in_window();
    void try_advance_window_base();
    void listen_for_acks();
    void receive_segments_in_window();

    struct timestamp_compare
    {
        bool operator()(const std::pair<std::chrono::system_clock::time_point, uint16_t> &lhs,
            const std::pair<std::chrono::system_clock::time_point, uint16_t> &rhs)
        {
            return lhs.first > rhs.first;
        }
    };

    mutable std::mutex access_lock;
    connection_tuple connection_key;
    int communication_socket_fd;
    // TODO: outbound_frame_queue ?
    std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<std::vector<uint8_t>>>> write_queue;
    std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<message_segment>>>
        incoming_segment_queue;

    // TODO: define sequence_number_t, etc -> in common header file?
    uint16_t window_base;
    uint16_t next_sequence_number;
    uint16_t window_size;    // note: must be <= uint16_t::max() / 2

    bool sequence_number_wrapped;
    bool channel_close_requested;
    bool sending;
    bool fin_received;

    int retransmission_timeout_int_ms;    // TODO: type?
    std::chrono::milliseconds retransmission_timeout;
    std::chrono::milliseconds segment_queue_read_timeout;    // TODO: make static

    std::map<uint16_t, std::shared_ptr<message_segment>> segment_buffer;
    std::priority_queue<std::pair<std::chrono::system_clock::time_point, uint16_t>,
        std::vector<std::pair<std::chrono::system_clock::time_point, uint16_t>>,
        timestamp_compare>
        retransmit_queue;    // TODO: rewrite to use map?
};

#endif
