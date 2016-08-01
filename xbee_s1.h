#ifndef XBEE_S1_H
#define XBEE_S1_H

#include <chrono>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "serial/serial.h"

#include "at_command_frame.h"
#include "at_command.h"
#include "at_command_response_frame.h"
#include "logger.h"
#include "uart_frame.h"
#include "util.h"
#include "xbee_config.h"

/*
 * notes:
 *  - xbee will discard frames if destination address is not broadcast or xbee's SH + SL
 */

// TODO: for tx request writes -> can check api_frame type index and if tx_request, check if frame_id > 0 ? read ack frame : don't

class xbee_s1
{
public:
    static const uint64_t ADDRESS_UNKNOWN;
    static const uint64_t BROADCAST_ADDRESS;
    static const uint32_t DEFAULT_SERIAL_TIMEOUT_MS;
    static const uint32_t DEFAULT_GUARD_TIME_S;
    static const uint32_t DEFAULT_COMMAND_MODE_TIMEOUT_S;
    static const uint32_t CTS_LOW_RETRIES;  // how many times to retry frame writes to serial line when CTS (clear to send) is low
    static const uint32_t CTS_LOW_SLEEP_MS; // how long to sleep when CTS is low
    static const uint32_t MAX_INVALID_FRAME_READS;  // TODO: haven't pinpointed why this happens, but sleeping seems to 'fix' corrupt frame reads
    static const uint32_t INVALID_FRAME_READ_SLEEP_MS;
    static const char *const COMMAND_SEQUENCE;

    xbee_s1();
    xbee_s1(uint32_t baud);
    bool reset_firmware_settings();
    bool initialize();
    bool configure_firmware_settings();

    uint64_t get_address() const;
    void write_frame(const std::vector<uint8_t> &payload);
    std::shared_ptr<at_command_response_frame> write_at_command_frame(std::shared_ptr<at_command_frame> command);
    std::shared_ptr<uart_frame> read_frame();
    std::shared_ptr<uart_frame> write_and_read_frame(const std::vector<uint8_t> &payload);

private:
    bool test_at_command_mode();
    bool enable_api_mode();
    bool enable_64_bit_addressing();
    bool read_ieee_source_address();
    bool enable_strict_802_15_4_mode();
    bool configure_baud();
    bool write_to_non_volatile_memory();
    std::string execute_command(const at_command &command, bool exit_command_mode = true);
    bool enter_command_mode();

    void unlocked_write_string(const std::string &str);
    std::string unlocked_read_line();
    void unlocked_write_frame(const std::vector<uint8_t> &payload);
    std::shared_ptr<uart_frame> unlocked_read_frame();
    std::shared_ptr<uart_frame> unlocked_write_and_read_frame(const std::vector<uint8_t> &payload);

    // note: although serial library employs its own line access protection, access_lock is used to ensure multi-frame read/write methods are atomic
    std::mutex access_lock;
    uint64_t address;
    xbee_config config;
    serial::Serial serial;
};

#endif
