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
#include "xbee_s1.h"

// TODO: make note of sender requiring SO_RCVTIMEO set (can we check for this?) or just set it
// within class?
//  - might be neater to have reader()/writer() thread logic inside this class as public members
// TODO: make magic numbers const
// TODO: need to send fin to close connection
// TODO: look up fast retransmit technique
// TODO: need to verify: when sender requests CLOSE, send FIN only after current payload is sent?
// currently might be stopping loop early causing retransmission queue not to be fully retransmitted
// TODO: estimate RTT with capped max
// TODO: need to define connection state machine

template <typename TSequenceNumber>
class reliable_channel
{
public:
    reliable_channel(connection_tuple connection_key, int communication_socket_fd,
        std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<std::vector<uint8_t>>>>
            write_queue,
        std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<message_segment>>>
            incoming_segment_queue)
        : connection_key(connection_key), communication_socket_fd(communication_socket_fd),
          write_queue(write_queue), incoming_segment_queue(incoming_segment_queue), window_base(0),
          next_sequence_number(0), window_size(25), sequence_number_wrapped(false),
          channel_close_requested(false), sending(false), fin_received(false),
          retransmission_timeout_int_ms(500), retransmission_timeout(retransmission_timeout_int_ms),
          segment_queue_read_timeout(25)
    {
    }

    void start_sending()
    {
        sending = true;
        std::thread timer(&reliable_channel::retransmit_timer, this);

        // TODO: verify how often send loop is happening
        // TODO: might be race condition here, try opening channel, sleep and then send small
        // payload, might finish loop before actually tx-ing payload (or send channel_close_request
        // before segment_buffer has a chance to be populated?)
        while (!(segment_buffer.empty() && channel_close_requested))
        {
            // TODO: small sleep here?
            send_segments_in_window();
            listen_for_acks();
        }

        sending = false;    // TODO: does this need to be locked?

        // TODO: send fin here, wait for ack?
        // TODO: verify might have to swap src/dest?
        auto fin = message_segment::create_fin(
            connection_key.destination_port, connection_key.source_port);
        uart_frame frame(
            std::make_shared<tx_request_64_frame>(connection_key.source_address, *fin));

        // TODO: util::repeat
        for (int i = 0; i < 10; ++i)
        {
            write_queue->push(std::make_shared<std::vector<uint8_t>>(frame));
        }

        timer.join();
    }

    void start_receiving()
    {
        receive_segments_in_window();
    }

    void request_channel_close()
    {
        channel_close_requested = true;
    }

    // TODO: needed? need way for sender to indicate payload channel has closed as well as receiver
    // to receive fin and notify application layer
    //  -> will also need to send fin from receiver -> sender (when?)
    void received_fin()
    {
        fin_received = true;
    }

private:
    void retransmit_timed_out_unacked_segments()
    {
        std::lock_guard<std::mutex> lock(access_lock);
        auto now = std::chrono::system_clock::now();

        // TODO: re-write retransmit_queue to be a map, when ack is received, remove segment from
        // it, will speed up channel teardown
        // TODO: while (... && sending), no need to continue re-tx once tx complete
        while (!retransmit_queue.empty())
        {
            // TODO: make aliases vars instead of using .first, .second ?
            std::pair<std::chrono::system_clock::time_point, TSequenceNumber> segment_info
                = retransmit_queue.top();
            auto delta
                = std::chrono::duration_cast<std::chrono::milliseconds>(now - segment_info.first);

            if (delta < retransmission_timeout)
            {
                return;
            }

            // ACK for segment received, remove from retransmit queue
            if (segment_buffer.count(segment_info.second) == 0)
            {
                retransmit_queue.pop();
                continue;
            }

            retransmit_queue.pop();
            retransmit_queue.push(
                std::make_pair(std::chrono::system_clock::now(), segment_info.second));
            auto segment = segment_buffer[segment_info.second];
            uart_frame frame(
                std::make_shared<tx_request_64_frame>(connection_key.source_address, *segment));
            write_queue->push(std::make_shared<std::vector<uint8_t>>(frame));
        }
    }

    void retransmitter(const boost::system::error_code &/*e*/, boost::asio::deadline_timer *timer)
    {
        retransmit_timed_out_unacked_segments();

        if (sending)
        {
            timer->expires_at(timer->expires_at()
                + boost::posix_time::milliseconds(retransmission_timeout_int_ms));
            timer->async_wait(boost::bind(
                &reliable_channel::retransmitter, this, boost::asio::placeholders::error, timer));
        }
    }

    void retransmit_timer()
    {
        // TODO: can same io_service be used by multiple deadline_timers?
        boost::asio::io_service io;
        boost::asio::deadline_timer timer(
            io, boost::posix_time::milliseconds(retransmission_timeout_int_ms));
        timer.async_wait(boost::bind(
            &reliable_channel::retransmitter, this, boost::asio::placeholders::error, &timer));
        io.run();

        // TODO: need to prevent this thread from exiting? (no?) or keep ref to io_service to stop
        // it/cleanup?
    }

    bool in_window(TSequenceNumber sequence_number) const
    {
        TSequenceNumber upper = static_cast<TSequenceNumber>(window_base + window_size - 1);
        return (
            (window_base < upper && (window_base <= sequence_number && sequence_number <= upper))
            || (upper < window_base
                   && (sequence_number <= upper || window_base <= sequence_number)));
    }

    bool in_previous_window(TSequenceNumber sequence_number) const
    {
        auto previous_window_base = static_cast<TSequenceNumber>(window_base - window_size);
        TSequenceNumber upper
            = static_cast<TSequenceNumber>(previous_window_base + window_size - 1);
        return ((previous_window_base < upper
                    && (previous_window_base <= sequence_number && sequence_number <= upper))
            || (upper < previous_window_base
                    && (sequence_number <= upper || previous_window_base <= sequence_number)));

        // TODO: can factor out above logic so this method just invokes something like:
        // return in_window(static_cast<TSequenceNumber>(window_base - window_size), window_size,
        // sequence_number);
    }

    // segment_buffer is used to hold sent + un-ACK'd segments
    void send_segments_in_window()
    {
        bool sent_segments = false;

        // send as many segments as window size allows
        while (in_window(next_sequence_number))
        {
            int error;
            std::vector<uint8_t> buffer;
            ssize_t bytes_read = util::nonblocking_recv(
                communication_socket_fd, buffer, message_segment::MAX_SEGMENT_LENGTH, error);

            if (bytes_read == 0)
            {
                // TODO: have flag here, if this occurs, tear down channel ?
                sending = false;
                break;
            }
            else if (bytes_read == -1)
            {
                if (error == EAGAIN || error == EWOULDBLOCK)
                {
                    break;
                }

                sending = false;
                return;
            }

            buffer.resize(bytes_read);
            auto segment = std::make_shared<message_segment>(connection_key.destination_port,
                connection_key.source_port, next_sequence_number,
                message_segment::type::stream_segment, message_segment::flag::none, buffer);

            std::unique_lock<std::mutex> lock(access_lock);
            ++next_sequence_number;
            if (next_sequence_number == 0)
            {
                sequence_number_wrapped = true;
            }

            segment_buffer[segment->get_sequence_num()] = segment;
            retransmit_queue.push(
                std::make_pair(std::chrono::system_clock::now(), segment->get_sequence_num()));
            sent_segments = true;
            lock.unlock();

            // TODO: save entire frame in buffer?
            uart_frame frame(
                std::make_shared<tx_request_64_frame>(connection_key.source_address, *segment));
            write_queue->push(std::make_shared<std::vector<uint8_t>>(frame));
        }
    }

    // TODO: this is called by methods that already aqcuired lock
    //      - have way to indicate whether methods acquire lock vs don't?
    // tries to advance window_base to un-ACK'd segment with smallest sequence number (accounting
    // for wraparound)
    void try_advance_window_base()
    {
        if (segment_buffer.empty())
        {
            window_base = next_sequence_number;
            return;
        }

        auto entry = segment_buffer.lower_bound(window_base);
        if (entry != segment_buffer.end())
        {
            window_base = entry->first;
            return;
        }

        // if upper bound of window has wrapped around and window_base hasn't, lower_bound won't
        // return anything since now unacked_sequence_nummber may be less than window_base, in this
        // case check again from 0
        if (sequence_number_wrapped)
        {
            entry = segment_buffer.lower_bound(0);
            if (entry != segment_buffer.end())
            {
                window_base = entry->first;
                sequence_number_wrapped = false;    // reset wrapped flag
                return;
            }
        }
    }

    void listen_for_acks()
    {
        std::shared_ptr<message_segment> segment;

        // listen for ACKs (until failure)
        while (incoming_segment_queue->timed_wait_and_pop(segment, segment_queue_read_timeout))
        {
            if (in_window(segment->get_sequence_num()))
            {
                std::lock_guard<std::mutex> lock(access_lock);
                segment_buffer.erase(segment->get_sequence_num());

                if (segment->get_sequence_num() == window_base)
                {
                    try_advance_window_base();
                }
            }
        }
    }

    // TODO: locking needed in receiver? nothing else modifying segment_buffer, so not needed for
    // now
    //      - should lock all access to segment_buffer? - easier to refactor it to threadsafe_map?
    void receive_segments_in_window()
    {
        while (!fin_received)    // TODO: what other conditions should signal stop?
        {
            std::shared_ptr<message_segment> segment;
            if (!incoming_segment_queue->timed_wait_and_pop(segment, segment_queue_read_timeout))
            {
                // TODO: try reduce amount of busy wait?
                continue;
            }

            // TODO: handle fin packets here, will be written to same channel as payload bits ->
            // separate channels?
            if (segment->is_fin())
            {
                // TODO: can just break here
                // TODO: factor out code that reads writes to domain socket so if there are still
                // frames that haven't been written to domain socket after fin is received, we can
                // finish transfering them
                fin_received = true;
                continue;
            }

            // TODO: verify: next_sequence_number not used in receiver code?
            auto sequence_number = segment->get_sequence_num();
            if (in_window(sequence_number))
            {
                auto ack = message_segment::create_ack(
                    connection_key.destination_port, connection_key.source_port, sequence_number);
                uart_frame frame(
                    std::make_shared<tx_request_64_frame>(connection_key.source_address, *ack));
                write_queue->push(std::make_shared<std::vector<uint8_t>>(frame));

                // buffer segment if we haven't seen it before
                if (segment_buffer.count(sequence_number) == 0)
                {
                    segment_buffer[sequence_number] = segment;
                }

                // TODO: could buffer these and send as chunk?
                //  -> will be writing to domain socket, will buffering before sending make a
                //  difference?
                // send segments to application layer
                while (segment_buffer.count(window_base) != 0)
                {
                    auto payload = segment_buffer[window_base]->get_message();
                    if (util::send(communication_socket_fd, payload) == -1)
                    {
                        LOG_ERROR("channel corrupted");
                        // TODO: some way to signal client that IPC failure has occurred -> reliable
                        // channel corrupted?
                        // TODO: have beehive_context/state object that tracks these counters, i.e.
                        // beehive_state.increment_corrupted_channels
                    }

                    segment_buffer.erase(window_base++);
                }
            }
            else if (in_previous_window(sequence_number))
            {
                // TODO: Kurose excplitily defines this range, other source says to ack
                // 'anything outside window' ... will both work?

                auto ack = message_segment::create_ack(
                    connection_key.destination_port, connection_key.source_port, sequence_number);
                uart_frame frame(
                    std::make_shared<tx_request_64_frame>(connection_key.source_address, *ack));
                write_queue->push(std::make_shared<std::vector<uint8_t>>(frame));
            }
        }
    }

    struct timestamp_compare
    {
        bool operator()(
            const std::pair<std::chrono::system_clock::time_point, TSequenceNumber> &lhs,
            const std::pair<std::chrono::system_clock::time_point, TSequenceNumber> &rhs)
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

    TSequenceNumber window_base;
    TSequenceNumber next_sequence_number;
    TSequenceNumber window_size;    // note: must be <= TSequenceNumber::max() / 2

    bool sequence_number_wrapped;
    bool channel_close_requested;
    bool sending;
    bool fin_received;

    int retransmission_timeout_int_ms;    // TODO: type?
    std::chrono::milliseconds retransmission_timeout;
    std::chrono::milliseconds segment_queue_read_timeout;    // TODO: make static

    std::map<TSequenceNumber, std::shared_ptr<message_segment>> segment_buffer;
    std::priority_queue<std::pair<std::chrono::system_clock::time_point, TSequenceNumber>,
        std::vector<std::pair<std::chrono::system_clock::time_point, TSequenceNumber>>,
        timestamp_compare>
        retransmit_queue;    // TODO: rewrite to use map?
};

#endif
