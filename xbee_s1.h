#ifndef XBEE_S1_H
#define XBEE_S1_H

#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "serial/serial.h"

#include "at_command_frame.h"
#include "at_command.h"
#include "at_command_response_frame.h"
#include "rx_packet_64_frame.h"
#include "tx_status_frame.h"
#include "uart_frame.h"
#include "util.h"
#include "xbee_config.h"

class xbee_s1
{
public:
    static const uint64_t ADDRESS_UNKNOWN;
    static const uint32_t DEFAULT_TIMEOUT_MS;
    static const uint32_t DEFAULT_GUARD_TIME_S;
    static const uint32_t DEFAULT_COMMAND_MODE_TIMEOUT_S;
    static const uint8_t HEADER_LENGTH_END_POSITION;
    static const uint8_t API_IDENTIFIER_INDEX;
    static const char *const COMMAND_SEQUENCE;

    xbee_s1();

    // TODO: move setup code into single method
    bool enable_api_mode();
    bool enable_64_bit_addressing();
    bool read_ieee_source_address();

    void write_frame(const std::vector<uint8_t> &payload);

    std::shared_ptr<at_command_response_frame> write_at_command_frame(std::shared_ptr<at_command_frame> command);
    std::unique_ptr<uart_frame> read_frame();
    // TODO: read_frames()

private:
    std::string execute_command(const at_command &command);
    bool enter_command_mode();
    void write_string(const std::string &str);
    std::string read_line();

    uint64_t address;
    xbee_config config;
    serial::Serial serial;
};

#endif
