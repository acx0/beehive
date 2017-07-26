#include "util.h"

std::string util::prefix_null_terminator(const std::string &str)
{
    return '\0' + str;
}

std::string util::get_escaped_string(const std::string &str)
{
    std::ostringstream oss;

    for (auto c : str)
    {
        switch (c)
        {
            case '\r':
                oss << "\\r";
                break;
            case '\n':
                oss << "\\n";
                break;
            default:
                oss << c;
                break;
        }
    }

    return oss.str();
}

std::string util::strip_newline(const std::string &str)
{
    std::ostringstream oss;

    for (auto c : str)
    {
        switch (c)
        {
            case '\r':
            case '\n':
                break;
            default:
                oss << c;
                break;
        }
    }

    return oss.str();
}

// TODO: add option to enable colour with escape seqs?
std::string util::get_frame_hex(const std::vector<uint8_t> &frame, bool show_prefix)
{
    std::ostringstream oss;

    // TODO: refactor this? (used in util::to_hex_string as well, overload for vector?)
    for (std::vector<uint8_t>::size_type i = 0; i < frame.size(); ++i)
    {
        if (show_prefix)
        {
            oss << "0x";    // TODO: std::showbase ?
        }

        // +frame[i] promotes to type printable as number so that value isn't printed as char
        oss << std::setfill('0') << std::setw(2) << std::hex << +frame[i];

        if (i < frame.size() - 1)
        {
            oss << " ";
        }
    }

    return oss.str();
}

void util::sleep(unsigned int seconds)
{
    LOG("sleeping ", seconds, "s ... ");
    ::sleep(seconds);
}

// prefixed null terminator instructs socket to be created in abstract namespace (not mapped to
// filesystem)
int util::create_passive_abstract_domain_socket(const std::string &name, int type)
{
    return create_passive_domain_socket(prefix_null_terminator(name), type);
}

int util::create_passive_domain_socket(const std::string &name, int type)
{
    int socket_fd;
    if ((socket_fd = socket(AF_UNIX, type, 0)) == -1)
    {
        perror("socket");
        return -1;
    }

    LOG("created passive socket: [", name, "] -> ", socket_fd);

    sockaddr_un socket;
    socket.sun_family = AF_UNIX;
    name.copy(socket.sun_path, name.size());

    socklen_t length = name.size() + sizeof(socket.sun_family);
    if (bind(socket_fd, reinterpret_cast<const sockaddr *>(&socket), length) == -1)
    {
        perror("bind");    // TODO: get message string and log it with LOG()
        return -1;
    }

    if (listen(socket_fd, SOMAXCONN) == -1)
    {
        perror("listen");
        return -1;
    }

    return socket_fd;
}

int util::create_active_abstract_domain_socket(const std::string &name, int type)
{
    return create_active_domain_socket(prefix_null_terminator(name), type);
}

int util::create_active_domain_socket(const std::string &name, int type)
{
    int socket_fd;
    if ((socket_fd = socket(AF_UNIX, type, 0)) == -1)
    {
        perror("socket");
        return -1;
    }

    sockaddr_un remote_socket;
    remote_socket.sun_family = AF_UNIX;
    name.copy(remote_socket.sun_path, name.size());
    socklen_t length = name.size() + sizeof(remote_socket.sun_family);

    if (connect(socket_fd, reinterpret_cast<const sockaddr *>(&remote_socket), length) == -1)
    {
        perror("connect");
        return -1;
    }

    return socket_fd;
}

int util::accept_connection(int socket_fd)
{
    int request_socket_fd;
    sockaddr_un request_socket;
    socklen_t length = sizeof(request_socket);

    LOG(socket_fd, ": waiting for accept request");
    if ((request_socket_fd
            = accept(socket_fd, reinterpret_cast<sockaddr *>(&request_socket), &length))
        == -1)
    {
        perror("accept");    // TODO: use logger
        return -1;
    }

    LOG(socket_fd, ": received accept request -> ", request_socket_fd);
    return request_socket_fd;
}

// TODO: make timeout configurable -> in beehive_config
bool util::try_configure_nonblocking_receive_timeout(int socket_fd)
{
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 25000;    // 25ms timeout

    // TODO: O_NONBLOCK vs SO_RCVTIMEO vs MSG_DONTWAIT ?
    //  - seems O_NONBLOCK and MSG_DONTWAIT only configure/temporarily set nonblocking behaviour
    //  without specifying timeout whereas SO_RCVTIMEO enforces timeout value
    //  - might be preferrable to not enforce timeout value, otherwise client is bound to library
    //  defined timeout value
    return setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != -1;
}

// TODO: is this copy-elided?
std::vector<std::string> util::split(const std::string &str, const std::string &separator)
{
    std::vector<std::string> tokens;
    boost::char_separator<char> sep(separator.c_str());
    boost::tokenizer<boost::char_separator<char>> tokenizer(str, sep);

    for (auto &i : tokenizer)
    {
        tokens.push_back(i);
    }

    return tokens;
}

bool util::retry(std::function<bool()> function, uint32_t retries)
{
    for (uint32_t i = 0; i < retries; ++i)
    {
        if (function())
        {
            return true;
        }
    }

    return false;
}

bool util::try_parse_uint32_t(const std::string &str, uint32_t &out)
{
    return static_cast<bool>(std::istringstream(str) >> out);
}

ssize_t util::send(int socket_fd, const std::vector<uint8_t> &buffer)
{
    int error;
    return send(socket_fd, buffer, error);
}

ssize_t util::send(int socket_fd, const std::vector<uint8_t> &buffer, int &error)
{
    if (buffer.empty())
    {
        LOG_ERROR(__PRETTY_FUNCTION__, ": empty buffer");
        return -1;
    }

    size_t bytes_left = buffer.size();
    size_t buffer_bytes_sent = 0;

    // TODO: static number of attempts?
    for (int attempts_left = 5; attempts_left > 0 && buffer_bytes_sent != buffer.size();
         --attempts_left)
    {
        ssize_t bytes_sent = ::send(socket_fd, buffer.data() + buffer_bytes_sent, bytes_left, 0);
        error = errno;

        if (bytes_sent == -1)
        {
            char buf[256];
            // TODO: strerror_r return value needs to be checked, getting garbage output
            auto ret = strerror_r(error, buf, sizeof(buf));
            LOG_ERROR("strerror_r return: ", ret);
            LOG_ERROR(__PRETTY_FUNCTION__, ": ", buf);
            return -1;
        }

        buffer_bytes_sent += bytes_sent;
        bytes_left -= bytes_sent;
    }

    // consider partial send a failure
    if (buffer_bytes_sent != buffer.size())
    {
        LOG_ERROR(__PRETTY_FUNCTION__, ": partial send");
        return -1;
    }

    return buffer_bytes_sent;
}

ssize_t util::recv(int socket_fd, std::vector<uint8_t> &buffer, size_t buffer_length)
{
    int error;
    return recv(socket_fd, buffer, buffer_length, error);
}

ssize_t util::recv(int socket_fd, std::vector<uint8_t> &buffer, size_t buffer_length, int &error)
{
    if (buffer_length == 0)
    {
        return -1;
    }

    buffer.resize(buffer_length);
    ssize_t bytes_read = ::recv(socket_fd, buffer.data(), buffer.size(), 0);
    error = errno;

    if (bytes_read == 0)
    {
        LOG_ERROR(__PRETTY_FUNCTION__, ": client connection closed");
        return 0;
    }
    else if (bytes_read == -1)
    {
        // TODO: to have this method also support nonblocking socket, only print error if not
        // EAGAIN/EWOULDBLOCK
        char buf[256];
        strerror_r(error, buf, sizeof(buf));
        LOG_ERROR(__PRETTY_FUNCTION__, ": ", buf);
        return -1;
    }

    buffer.resize(bytes_read);
    return bytes_read;
}

// TODO:
// blocking recv
//     blocking socket    : util::recv
//     nonblocking socket : util::recv?     TODO: test
//         - would calling recv with flags=0 be nonblocking?
//         - util::recv would have to be modified to not treat EAGAIN as errror
// nonblocking recv
//     blocking socket    : util::nonblocking_recv
//     nonblocking socket : util::nonblocking_recv? TODO: test
//         - should work?
// TODO: how to enforce that a socket with nonblocking recv settings is configured?
//  - use getsockopt/fcntl to check and throw exception if not?
ssize_t util::nonblocking_recv(
    int socket_fd, std::vector<uint8_t> &buffer, size_t buffer_length, int &error)
{
    if (buffer_length == 0)
    {
        return -1;
    }

    buffer.resize(buffer_length);
    ssize_t bytes_read = ::recv(socket_fd, buffer.data(), buffer.size(), MSG_DONTWAIT);
    error = errno;

    if (bytes_read == 0)
    {
        LOG_ERROR(__PRETTY_FUNCTION__, ": client connection closed");
        return 0;
    }
    else if (bytes_read == -1)
    {
        // EAGAIN == EWOULDBLOCK on most systems (see errno(3), recv(2)), but not required by POSIX
        if (error != EAGAIN && error != EWOULDBLOCK)
        {
            char buf[256];
            strerror_r(error, buf, sizeof(buf));
            LOG_ERROR(__PRETTY_FUNCTION__, ": ", buf);
        }

        return -1;
    }

    buffer.resize(bytes_read);
    return bytes_read;
}
