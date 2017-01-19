#include "datagram_socket_manager.h"

// TODO: calculate this based on uart_frame::MAX_FRAME_SIZE (add this)
// TODO: move into message_segment, other places will need this constant
const size_t datagram_socket_manager::MAX_MESSAGE_LENGTH = 91;
uint32_t datagram_socket_manager::socket_suffix = 0;
std::mutex datagram_socket_manager::socket_suffix_lock;

datagram_socket_manager::datagram_socket_manager(const beehive_config &config,
    std::shared_ptr<threadsafe_blocking_queue<std::shared_ptr<std::vector<uint8_t>>>> write_queue)
    : dgram_path_prefix(config.get_dgram_path_prefix()), write_queue(write_queue)
{
}

bool datagram_socket_manager::try_create_passive_socket(int control_socket_fd, uint16_t listen_port)
{
    if (!_port_manager.try_open_listen_port(listen_port))
    {
        LOG_ERROR("port in use");
        beehive_message::send_message(control_socket_fd, beehive_message::USED);
        return false;
    }

    auto segment_queue = segment_queue_map[listen_port]
        = std::make_shared<threadsafe_blocking_queue<datagram_segment>>();
    std::string communication_socket_path = dgram_path_prefix + "/" + std::to_string(listen_port)
        + "/" + std::to_string(get_next_socket_suffix());

    int listen_socket_fd
        = util::create_passive_abstract_domain_socket(communication_socket_path, SOCK_SEQPACKET);
    if (listen_socket_fd == -1)
    {
        destroy_socket(listen_port);
        LOG_ERROR("error creating communication socket ", communication_socket_path);
        beehive_message::send_message(control_socket_fd, beehive_message::FAILED);
        return false;
    }

    beehive_message::send_message(control_socket_fd,
        beehive_message::OK + beehive_message::SEPARATOR + communication_socket_path);
    std::thread socket_manager(&datagram_socket_manager::passive_socket_manager, this,
        control_socket_fd, listen_socket_fd, listen_port, segment_queue);
    socket_manager.detach();
    return true;
}

bool datagram_socket_manager::try_create_active_socket(int control_socket_fd)
{
    std::thread socket_manager(
        &datagram_socket_manager::active_socket_manager, this, control_socket_fd);
    socket_manager.detach();
    return true;
}

void datagram_socket_manager::process_segment(
    uint64_t source_address, std::shared_ptr<message_segment> segment)
{
    if (!_port_manager.is_open(segment->get_destination_port()))
    {
        LOG("discarding frame destined for port ", +segment->get_destination_port());
        return;
    }

    auto segment_queue = segment_queue_map.get_or_add(segment->get_destination_port(),
        std::make_shared<threadsafe_blocking_queue<datagram_segment>>());
    segment_queue->push(datagram_segment{source_address, segment});
}

uint32_t datagram_socket_manager::get_next_socket_suffix()
{
    std::lock_guard<std::mutex> lock(socket_suffix_lock);
    return socket_suffix++;
}

void datagram_socket_manager::passive_socket_manager(int control_socket_fd, int listen_socket_fd,
    uint16_t listen_port,
    std::shared_ptr<threadsafe_blocking_queue<datagram_segment>> segment_queue)
{
    LOG("starting passive_socket_manager thread for port ", +listen_port);

    int communication_socket_fd = util::accept_connection(listen_socket_fd);
    if (communication_socket_fd == -1)
    {
        destroy_socket(listen_port);
        LOG_ERROR(listen_socket_fd, ": error accepting communication connection");
        return;
    }

    LOG("client connected to communication socket");

    auto running = std::make_shared<bool>(true);    // TODO: cleaner way to share state?
    std::thread payload_read_handler(&datagram_socket_manager::payload_read_handler, this,
        communication_socket_fd, segment_queue, running);
    std::thread payload_write_handler(&datagram_socket_manager::payload_write_handler, this,
        communication_socket_fd, listen_port, running);

    while (*running)
    {
        std::string control_message = beehive_message::read_message(control_socket_fd);
        if (control_message.empty())
        {
            break;
        }

        // TODO: move away from using beehive_message::is_message?
        if (beehive_message::is_message(beehive_message::CLOSE, control_message))
        {
            break;
        }
    }

    *running = false;
    payload_read_handler.join();
    payload_write_handler.join();
    destroy_socket(listen_port);
}

void datagram_socket_manager::active_socket_manager(int control_socket_fd)
{
    LOG("starting active_socket_manager thread for fd ", control_socket_fd);

    uint16_t source_port;
    if (!_port_manager.try_get_random_ephemeral_port(source_port))
    {
        // TODO: unique error for port exhaustion ?
        beehive_message::send_message(control_socket_fd, beehive_message::FAILED);
        return;
    }

    auto segment_queue = segment_queue_map[source_port]
        = std::make_shared<threadsafe_blocking_queue<datagram_segment>>();
    std::string communication_socket_path = dgram_path_prefix + "/" + std::to_string(source_port)
        + "/" + std::to_string(get_next_socket_suffix());
    int listen_socket_fd
        = util::create_passive_abstract_domain_socket(communication_socket_path, SOCK_SEQPACKET);
    if (listen_socket_fd == -1)
    {
        destroy_socket(source_port);    // TODO: can use RAII for this? create class for sock object
        LOG_ERROR("error creating communication socket");
        return;
    }

    beehive_message::send_message(control_socket_fd,
        beehive_message::OK + beehive_message::SEPARATOR + communication_socket_path);
    // TODO: accept calls should eventually have timeout + cleanup as defensive measure
    int communication_socket_fd = util::accept_connection(listen_socket_fd);
    if (communication_socket_fd == -1)
    {
        destroy_socket(source_port);
        LOG_ERROR(listen_socket_fd, ": error accepting communication connection");
        return;
    }

    LOG("client connected to communication socket");
    auto running = std::make_shared<bool>(true);
    std::thread payload_read_handler(&datagram_socket_manager::payload_read_handler, this,
        communication_socket_fd, segment_queue, running);
    std::thread payload_write_handler(&datagram_socket_manager::payload_write_handler, this,
        communication_socket_fd, source_port, running);

    while (*running)
    {
        std::string control_message = beehive_message::read_message(control_socket_fd);
        if (control_message.empty())
        {
            break;
        }

        // TODO: move away from using beehive_message::is_message?
        if (beehive_message::is_message(beehive_message::CLOSE, control_message))
        {
            break;
        }
    }

    *running = false;
    payload_read_handler.join();
    payload_write_handler.join();
    destroy_socket(source_port);
    // TODO: close listen_socket_fd
}

void datagram_socket_manager::destroy_socket(uint16_t port)
{
    segment_queue_map.erase(port);
    _port_manager.release_port(port);
}

void datagram_socket_manager::payload_read_handler(int communication_socket_fd,
    std::shared_ptr<threadsafe_blocking_queue<datagram_segment>> segment_queue,
    std::shared_ptr<bool> running)
{
    while (*running)
    {
        datagram_segment datagram;
        // TODO: make timeout static, can make this larger, 1 second?
        if (!segment_queue->timed_wait_and_pop(datagram, std::chrono::milliseconds(250)))
        {
            continue;
        }

        std::vector<uint8_t> buffer;
        util::pack_value_as_bytes(std::back_inserter(buffer), datagram.source_address);
        util::pack_value_as_bytes(std::back_inserter(buffer), datagram.segment->get_source_port());
        buffer.insert(buffer.end(), datagram.segment->get_message().begin(),
            datagram.segment->get_message().end());

        // TODO: will this have to be configured to be nonblocking?
        // TODO: send() can send less than requested number of bytes, write util method to handle
        // this
        // TODO: can use write() instead of send if not using last arg -
        // http://www.gnu.org/software/libc/manual/html_node/Sending-Data.html
        if (send(communication_socket_fd, buffer.data(), buffer.size(), 0) == -1)
        {
            // TODO: check errno here or just fail completely? could have count of how many
            // consecutive fails have occured and only then exit
        }
    }
}

void datagram_socket_manager::payload_write_handler(
    int communication_socket_fd, uint16_t source_port, std::shared_ptr<bool> running)
{
    while (*running)
    {
        // TODO: configure nonblocking socket or use MSG_DONTWAIT?
        int error;
        std::vector<uint8_t> buffer;
        ssize_t bytes_read = util::nonblocking_recv(communication_socket_fd, buffer,
            sizeof(uint64_t) + sizeof(uint16_t) + MAX_MESSAGE_LENGTH, error);

        if (bytes_read == 0)
        {
            break;
        }
        else if (bytes_read == -1)
        {
            if (error == EAGAIN)
            {
                // TODO: add to beehive_config?
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
                continue;
            }

            break;
        }
        else if (static_cast<uint64_t>(bytes_read) < sizeof(uint64_t) + sizeof(uint16_t))
        {
            continue;
        }

        uint64_t destination_address = util::unpack_bytes_to_width<uint64_t>(std::begin(buffer));
        uint16_t destination_port = util::unpack_bytes_to_width<uint16_t>(std::begin(buffer) + 8);

        auto payload = std::vector<uint8_t>(std::begin(buffer), std::begin(buffer) + bytes_read);
        auto segment = std::make_shared<message_segment>(source_port, destination_port, 0,
            message_segment::type::datagram_segment, message_segment::flag::none, payload);
        uart_frame frame(std::make_shared<tx_request_64_frame>(destination_address, *segment));
        write_queue->push(std::make_shared<std::vector<uint8_t>>(frame));
    }
}
