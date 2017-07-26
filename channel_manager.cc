#include "channel_manager.h"

uint32_t channel_manager::socket_suffix = 0;
std::mutex channel_manager::socket_suffix_lock;

channel_manager::channel_manager(const beehive_config &config,
    std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<std::vector<uint8_t>>>> write_queue)
    : channel_path_prefix(config.get_channel_path_prefix()), write_queue(write_queue)
{
}

// TODO: this should be stored in globally accessible context/state object
void channel_manager::set_local_address(uint64_t address)
{
    local_address = address;
}

bool channel_manager::try_create_passive_socket(int client_socket_fd, uint16_t listen_port)
{
    if (!_port_manager.try_open_listen_port(listen_port))
    {
        LOG_ERROR("port in use");
        beehive_message::send_message(client_socket_fd, beehive_message::USED);
        return false;
    }

    connection_requests[listen_port]
        = std::make_shared<threadsafe_blocking_queue<std::pair<uint64_t, uint16_t>>>();
    beehive_message::send_message(client_socket_fd, beehive_message::OK);

    std::thread socket_manager(
        &channel_manager::passive_socket_manager, this, client_socket_fd, listen_port);
    socket_manager.detach();
    return true;
}

bool channel_manager::try_create_active_socket(
    int client_socket_fd, uint64_t destination_address, uint16_t destination_port)
{
    std::thread socket_manager(&channel_manager::active_socket_manager, this, client_socket_fd,
        destination_address, destination_port);
    socket_manager.detach();
    return true;
}

void channel_manager::process_stream_segment(
    connection_tuple connection_key, std::shared_ptr<message_segment> segment)
{
    if (!_port_manager.is_open(segment->get_destination_port()))
    {
        LOG("discarding frame destined for port ", +segment->get_destination_port());
        return;
    }

    if (segment->is_syn())
    {
        // protect against multiple handlers being spawned on syn retransmit
        auto segment_queue
            = std::make_shared<threadsafe_blocking_queue<std::shared_ptr<message_segment>>>();
        if (segment_queue_map.try_add(connection_key, segment_queue))
        {
            std::thread request_handler(
                &channel_manager::incoming_connection_handler, this, connection_key, segment_queue);
            request_handler.detach();
        }
    }
    else
    {
        std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<message_segment>>> segment_queue;
        if (segment_queue_map.try_get(connection_key, segment_queue))
        {
            segment_queue->push(segment);
        }
    }
}

void channel_manager::incoming_connection_handler(connection_tuple connection_key,
    std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<message_segment>>> segment_queue)
{
    LOG("starting incoming_connection_handler thread");

    auto response = message_segment::create_synack(
        connection_key.destination_port, connection_key.source_port);
    uart_frame frame(
        std::make_shared<tx_request_64_frame>(connection_key.source_address, *response));
    bool ack_received = false;

    for (int i = 0; i < 5; ++i)
    {
        // TODO: have generic write_frame method in xbee so we don't have to explicitly create uart
        // frame every time?
        write_queue->push(std::make_shared<std::vector<uint8_t>>(frame));
        std::shared_ptr<message_segment> message;
        // TODO: make timeout confiugurable
        if (segment_queue->timed_wait_and_pop(message, std::chrono::milliseconds(500))
            && message->is_ack())
        {
            ack_received = true;
            break;
        }
    }

    // TODO: known issue: as of current behavior, sender may start sending traffic while receiver
    // (here, above) is still waiting for ack_received
    //  - ack here not necessarily needed? since acks sent for data transfer will suffice to confirm
    //  2 way communication
    if (!ack_received)
    {
        return;
    }

    auto request_queue = connection_requests[connection_key.destination_port];
    if (request_queue == nullptr)
    {
        LOG_ERROR("request_queue is null");
        return;
    }

    request_queue->push(std::make_pair(connection_key.source_address, connection_key.source_port));
}

uint32_t channel_manager::get_next_socket_suffix()
{
    std::lock_guard<std::mutex> lock(socket_suffix_lock);
    return socket_suffix++;
}

void channel_manager::passive_socket_manager(int client_socket_fd, uint16_t listen_port)
{
    LOG("starting passive_socket_manager thread for port ", +listen_port);
    auto request_queue = connection_requests[listen_port];

    while (true)
    {
        std::string request_message = beehive_message::read_message(client_socket_fd);
        if (request_message.empty())
        {
            return;
        }

        LOG(+listen_port, ": received request message: [", request_message, "]");

        if (beehive_message::is_message(beehive_message::ACCEPT, request_message))
        {
            LOG("waiting for client request on port ", +listen_port);
            auto client_info = request_queue->wait_and_pop();

            uint64_t source_address = client_info.first;
            uint16_t source_port = client_info.second;

            LOG("received request on port ", +listen_port, " from (dest: ",
                util::to_hex_string(source_address), ", port: ", +source_port, ")");
            std::string communication_socket_path = channel_path_prefix + "/"
                + util::to_hex_string(source_address) + "/" + std::to_string(source_port) + "/"
                + std::to_string(get_next_socket_suffix());

            int listen_socket_fd = util::create_passive_abstract_domain_socket(
                communication_socket_path, SOCK_STREAM);
            if (listen_socket_fd == -1)
            {
                LOG("error creating communication socket");
                continue;
            }

            connection_tuple connection_key(
                source_address, source_port, local_address, listen_port);
            beehive_message::send_message(client_socket_fd,
                beehive_message::OK + beehive_message::SEPARATOR + communication_socket_path);
            std::thread payload_read_handler(
                &channel_manager::payload_read_handler, this, listen_socket_fd, connection_key);
            payload_read_handler.detach();
        }
    }
}

void channel_manager::active_socket_manager(
    int control_socket_fd, uint64_t destination_address, uint16_t destination_port)
{
    LOG("starting active_socket_manager thread for fd ", control_socket_fd, " (dest: ",
        util::to_hex_string(destination_address), ", port: ", +destination_port, ")");

    uint16_t source_port;
    if (!_port_manager.try_get_random_ephemeral_port(source_port))
    {
        // TODO: unique error for port exhaustion ?
        beehive_message::send_message(control_socket_fd, beehive_message::FAILED);
        return;
    }

    // note: connection_tuple created based on expected response tuple
    connection_tuple connection_key(
        destination_address, destination_port, local_address, source_port);
    auto segment_queue = segment_queue_map[connection_key]
        = std::make_shared<threadsafe_blocking_queue<std::shared_ptr<message_segment>>>();

    // TODO: if destination != localhost -> 3 way handshake
    // TODO: localhost communication will need to bypass xbee hardware and just do domain socket ->
    // domain socket forwarding?
    if (destination_address != local_address)
    {
        auto segment = message_segment::create_syn(source_port, destination_port);
        uart_frame frame(std::make_shared<tx_request_64_frame>(destination_address, *segment));
        bool synack_received = false;

        for (int i = 0; i < 5; ++i)
        {
            write_queue->push(std::make_shared<std::vector<uint8_t>>(frame));
            std::shared_ptr<message_segment> response;
            // TODO: make timeout configurable
            if (segment_queue->timed_wait_and_pop(response, std::chrono::milliseconds(500))
                && response->is_synack())
            {
                synack_received = true;
                break;
            }
        }

        if (!synack_received)
        {
            // TODO: error
            return;
        }

        segment = message_segment::create_ack(source_port, destination_port);
        // TODO: hold shared_ptr to segment?
        frame = uart_frame(std::make_shared<tx_request_64_frame>(destination_address, *segment));
        write_queue->push(std::make_shared<std::vector<uint8_t>>(frame));
    }

    std::string communication_socket_path = channel_path_prefix + "/"
        + util::to_hex_string(destination_address) + "/" + std::to_string(destination_port) + "/"
        + std::to_string(get_next_socket_suffix());
    int listen_socket_fd
        = util::create_passive_abstract_domain_socket(communication_socket_path, SOCK_STREAM);
    if (listen_socket_fd == -1)
    {
        LOG_ERROR("error creating communication socket");
        return;
    }

    beehive_message::send_message(control_socket_fd,
        beehive_message::OK + beehive_message::SEPARATOR + communication_socket_path);
    std::thread payload_write_handler(&channel_manager::payload_write_handler, this,
        control_socket_fd, listen_socket_fd, connection_key);
    payload_write_handler.join();
    _port_manager.release_port(source_port);
}

void channel_manager::payload_read_handler(int listen_socket_fd, connection_tuple connection_key)
{
    LOG("starting payload_read_handler thread");

    int communication_socket_fd = util::accept_connection(listen_socket_fd);
    if (communication_socket_fd == -1)
    {
        LOG_ERROR(listen_socket_fd, ": error accepting communication connection");
        return;
    }

    LOG("client connected to communication socket");

    auto segment_queue = segment_queue_map[connection_key];
    if (segment_queue == nullptr)
    {
        // TODO: error msg/handling?
        LOG_ERROR("payload_read_handler: null segment_queue");
        return;
    }

    // TODO: will need to keep ref to channel to signal close
    auto channel = std::make_shared<reliable_channel>(
        connection_key, communication_socket_fd, write_queue, segment_queue);
    channel->start_receiving();

    // TODO: close/cleanup communication_socket_fd
}

void channel_manager::payload_write_handler(
    int control_socket_fd, int listen_socket_fd, connection_tuple connection_key)
{
    int communication_socket_fd = util::accept_connection(listen_socket_fd);
    if (communication_socket_fd == -1)
    {
        LOG_ERROR(listen_socket_fd, ": error accepting communication connection");
        return;
    }

    // TODO: trace this to see implications of configuring this socket as nonblocking
    LOG("client connected to communication socket");
    if (!util::try_configure_nonblocking_receive_timeout(communication_socket_fd))
    {
        // TODO: handle
        return;
    }

    auto segment_queue = segment_queue_map[connection_key];
    if (segment_queue == nullptr)
    {
        // TODO: handle
        // TODO: use __PRETTY_FUNCTION__
        LOG_ERROR("payload_write_handler: null segment_queue");
        return;
    }

    auto channel = std::make_shared<reliable_channel>(
        connection_key, communication_socket_fd, write_queue, segment_queue);
    std::thread reliable_sender(&reliable_channel::start_sending, channel);

    while (true)
    {
        std::string control_message = beehive_message::read_message(control_socket_fd);
        if (control_message.empty())
        {
            // TODO: should exit here?
            continue;
        }

        if (beehive_message::is_message(beehive_message::CLOSE, control_message))
        {
            // TODO: send fin to receiver
            // TODO: differentiate CLOSE vs SHUTDOWN?
            //  - close -> finish sending payload and then send fin
            //  - shutdown -> send immediate fin (disruptive disconnect)

            channel->request_channel_close();
            break;
        }
    }

    reliable_sender.join();
    // TODO: close/cleanup communication_socket_fd + others
}
