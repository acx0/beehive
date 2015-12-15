#ifndef XBEE_CONFIG_H
#define XBEE_CONFIG_H

#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class xbee_config
{
public:
    std::string port;
    uint32_t baud;

    xbee_config();

private:
    static constexpr const char *CONFIG_FILE = "xbee_config";
    static constexpr const char *PORT_KEY = "port";
    static constexpr const char *BAUD_KEY = "baud";
    static constexpr const char SEPARATOR = '=';
};

#endif
