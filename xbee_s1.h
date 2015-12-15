#ifndef XBEE_S1_H
#define XBEE_S1_H

#include <exception>
#include <iostream>
#include <string>

#include "serial/serial.h"

#include "at_command.h"
#include "util.h"
#include "xbee_config.h"

class xbee_s1
{
public:
    // setting timeout <= 525 seems to cause serial reads to sometimes return nothing on odroid
    static constexpr uint32_t DEFAULT_TIMEOUT_MS = 700;
    static constexpr uint32_t DEFAULT_GUARD_TIME_S = 1;
    static constexpr uint32_t DEFAULT_COMMAND_MODE_TIMEOUT_S = 10;
    static constexpr const char *COMMAND_SEQUENCE = "+++";

    xbee_s1();

    bool enable_api_mode();

private:
    std::string execute_command(const at_command &command);
    bool enter_command_mode();
    void write_string(const std::string &str);
    std::string read_line();

    xbee_config config;
    serial::Serial serial;
};

#endif
