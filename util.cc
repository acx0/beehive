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

// prefixed null terminator instructs socket to be created in abstract namespace (not mapped to filesystem)
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
    if (bind(socket_fd, (sockaddr *)&socket, length) == -1)
    {
        perror("bind"); // TODO: get message string and log it with LOG()
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

    if (connect(socket_fd, (sockaddr *)&remote_socket, length) == -1)
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
    if ((request_socket_fd = accept(socket_fd, (sockaddr *)&request_socket, &length)) == -1)
    {
        perror("accept");   // TODO: use logger
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

    return setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != -1;
}

// TODO: is this copy-elided?
std::vector<std::string> util::split(const std::string &str, const std::string &separator)
{
    std::vector<std::string> tokens;
    boost::char_separator<char> sep(separator.c_str());
    boost::tokenizer<boost::char_separator<char>> tokenizer(str, sep);

    for (auto i = tokenizer.begin(); i != tokenizer.end(); ++i)
    {
        tokens.push_back(*i);
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
    std::istringstream iss(str);
    iss >> out;

    return static_cast<bool>(iss);
}

// TODO: merge all socket code into util methods and verify return status of send/recv calls to ensure no data loss
ssize_t util::nonblocking_recv(int socket_fd, std::vector<uint8_t> &buffer, size_t buffer_length, int &error)
{
    if (buffer_length == 0)
    {
        return -1;
    }

    buffer.resize(buffer_length);
    ssize_t bytes_read = recv(socket_fd, buffer.data(), buffer.size(), MSG_DONTWAIT);
    error = errno;

    if (bytes_read == 0)
    {
        LOG_ERROR("client connection closed");
        return 0;
    }
    else if (bytes_read == -1)
    {
        if (error != EAGAIN)
        {
            perror("recv");     // TODO: get string and use LOG_ERROR()
        }

        return -1;
    }

    buffer.resize(bytes_read);
    return bytes_read;
}
