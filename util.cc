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
