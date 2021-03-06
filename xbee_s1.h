#ifndef XBEE_S1_H
#define XBEE_S1_H

#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <iomanip>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <serial/serial.h>

#include "at_command_frame.h"
#include "at_command.h"
#include "at_command_response_frame.h"
#include "logger.h"
#include "uart_frame.h"
#include "util.h"

/*
 * notes:
 *  - xbee will discard frames if destination address is not broadcast or xbee's SH + SL
 */

// TODO: for tx request writes -> can check api_frame type index and if tx_request, check if frame_id > 0 ? read ack frame : don't

class xbee_s1
{
public:
    static const std::string DEFAULT_DEVICE;
    static const uint64_t ADDRESS_UNKNOWN;
    static const uint64_t BROADCAST_ADDRESS;
    static const uint32_t FACTORY_DEFAULT_BAUD;
    static const uint32_t MIN_BAUD_TWO_STOP_BITS;
    static const uint32_t DEFAULT_BAUD;
    static const uint32_t DEFAULT_SERIAL_TIMEOUT_MS;    // primarily controls how long a read in the underlying serial library will wait until it times out
    static const uint32_t DEFAULT_GUARD_TIME_S;
    static const uint32_t DEFAULT_COMMAND_MODE_TIMEOUT_S;
    static const uint32_t CTS_LOW_RETRIES;  // how many times to retry frame writes to serial line when CTS (clear to send) is low
    static const uint32_t MAX_INVALID_FRAME_READS;  // TODO: haven't pinpointed why this happens, but sleeping seems to 'fix' corrupt frame reads
    static const std::chrono::milliseconds CTS_LOW_SLEEP;   // how long to sleep when CTS is low
    static const std::chrono::milliseconds INVALID_FRAME_READ_BACKOFF_SLEEP;    // how long to backoff for if MAX_INVALID_FRAME_READS is hit
    static const std::chrono::milliseconds RX_PACKET_SERIAL_READ_THRESHOLD;     // max duration to poll when checking for a single rx packet
    static const std::chrono::milliseconds AT_COMMAND_RESPONSE_SERIAL_READ_THRESHOLD;   // at commands can have a larger delay between request and response frames
    static const std::chrono::microseconds SERIAL_READ_BACKOFF_SLEEP;   // initial sleep duration when checking serial line for bytes available
    static const char *const COMMAND_SEQUENCE;

    xbee_s1(const std::string &device);
    xbee_s1(const std::string &device, uint32_t baud);
    bool reset_firmware_settings();
    bool initialize();
    bool configure_firmware_settings();

    bool read_and_set_address();
    uint64_t get_address() const;
    void write_frame(const std::vector<uint8_t> &payload);
    std::shared_ptr<at_command_response_frame> write_at_command_frame(std::shared_ptr<at_command_frame> command,
        const std::chrono::milliseconds &read_timeout = AT_COMMAND_RESPONSE_SERIAL_READ_THRESHOLD,
        const std::chrono::microseconds &initial_read_backoff = SERIAL_READ_BACKOFF_SLEEP);
    std::shared_ptr<uart_frame> read_frame();
    std::shared_ptr<uart_frame> write_and_read_frame(const std::vector<uint8_t> &payload);
    bool read_configuration_registers();

private:
    static const std::map<uint8_t, uint32_t> baud_config_map;

    void configure_stop_bits();
    bool test_at_command_mode();
    bool enable_api_mode();
    bool enable_64_bit_addressing();
    bool restore_defaults();
    bool enable_strict_802_15_4_mode();
    bool configure_baud();
    bool write_to_non_volatile_memory();
    std::string execute_command(const at_command &command, bool exit_command_mode = true);
    bool enter_command_mode();
    bool read_ieee_source_address(uint64_t &address);
    template <typename T>
        bool read_configuration_register(const std::string &at_command_str, const std::string &command_description, T &register_value);
    bool try_serial_read(std::function<void()> read_operation, const std::chrono::milliseconds &read_timeout = RX_PACKET_SERIAL_READ_THRESHOLD,
        const std::chrono::microseconds &initial_read_backoff = SERIAL_READ_BACKOFF_SLEEP);
    bool try_serial_write(std::function<void()> write_operation);

    void unlocked_write_string(const std::string &str);
    std::string unlocked_read_line();
    void unlocked_write_frame(const std::vector<uint8_t> &payload);
    std::shared_ptr<uart_frame> unlocked_read_frame(
        const std::chrono::milliseconds &read_timeout = RX_PACKET_SERIAL_READ_THRESHOLD,
        const std::chrono::microseconds &initial_read_backoff = SERIAL_READ_BACKOFF_SLEEP);
    std::shared_ptr<uart_frame> unlocked_write_and_read_frame(const std::vector<uint8_t> &payload,
        const std::chrono::milliseconds &read_timeout = RX_PACKET_SERIAL_READ_THRESHOLD,
        const std::chrono::microseconds &initial_read_backoff = SERIAL_READ_BACKOFF_SLEEP);

    // note: although serial library employs its own line access protection, access_lock is used to ensure multi-frame read/write methods are atomic
    std::mutex access_lock;
    uint64_t address;
    serial::Serial serial;
};

#endif
