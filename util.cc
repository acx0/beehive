#include "util.h"

std::string util::get_escaped_string(const std::string &str)
{
    std::stringstream ss;

    for (auto c : str)
    {
        switch (c)
        {
            case '\r':
                ss << "\\r";
                break;
            case '\n':
                ss << "\\n";
                break;
            default:
                ss << c;
                break;
        }
    }

    return ss.str();
}

std::string util::strip_newline(const std::string &str)
{
    std::stringstream ss;

    for (auto c : str)
    {
        switch (c)
        {
            case '\r':
            case '\n':
                break;
            default:
                ss << c;
                break;
        }
    }

    return ss.str();
}

void util::sleep(unsigned int seconds)
{
    std::clog << "<sleeping " << seconds << "s ... ";
    std::clog.flush();
    ::sleep(seconds);
    std::clog << "done>" << std::endl;
}
