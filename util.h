#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <sstream>
#include <string>

#include <unistd.h>

namespace util
{
    std::string get_escaped_string(const std::string &str);
    std::string strip_newline(const std::string &str);
    void sleep(unsigned int seconds);
};

#endif
