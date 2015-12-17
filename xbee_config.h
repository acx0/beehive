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
    static const char *const CONFIG_FILE;
    static const char *const PORT_KEY;
    static const char *const BAUD_KEY;
    static const char SEPARATOR;
};

#endif
