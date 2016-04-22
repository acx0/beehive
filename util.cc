#include "util.h"

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

std::string util::get_frame_hex(const std::vector<uint8_t> &frame, bool show_prefix)
{
    std::ostringstream oss;

    for (std::vector<uint8_t>::size_type i = 0; i < frame.size(); ++i)
    {
        if (show_prefix)
        {
            oss << "0x";
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
    std::clog << "<sleeping " << seconds << "s ... ";
    std::clog.flush();
    ::sleep(seconds);
    std::clog << "done>" << std::endl;
}

int util::create_passive_domain_socket(const std::string &name)
{
    std::clog << "creating passive socket: [" << name << "]" << std::endl;

    int socket_fd;
    if ((socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return -1;
    }

    sockaddr_un socket;
    socket.sun_family = AF_UNIX;
    name.copy(socket.sun_path, name.size());

    std::clog << "binding socket" << std::endl;
    socklen_t length = name.size() + sizeof(socket.sun_family);
    if (bind(socket_fd, (sockaddr *)&socket, length) == -1)
    {
        perror("bind");
        return -1;
    }

    std::clog << "marking socket as passive" << std::endl;
    if (listen(socket_fd, SOMAXCONN) == -1)
    {
        perror("listen");
        return -1;
    }

    return socket_fd;
}

int util::create_active_domain_socket(const std::string &name)
{
    int socket_fd;
    if ((socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
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

    std::clog << socket_fd << ": waiting for accept request" << std::endl;
    if ((request_socket_fd = accept(socket_fd, (sockaddr *)&request_socket, &length)) == -1)
    {
        perror("accept");
        return -1;
    }

    return request_socket_fd;
}

bool util::try_configure_nonblocking_receive_timeout(int socket_fd)
{
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 25000;

    return setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != -1;
}

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
