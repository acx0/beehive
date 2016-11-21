#include "xbee_s1.h"

const std::string xbee_s1::DEFAULT_DEVICE = "/dev/ttyUSB0";
const uint64_t xbee_s1::ADDRESS_UNKNOWN = 0xffffffffffffffff;
const uint64_t xbee_s1::BROADCAST_ADDRESS = 0xffff;
const uint32_t xbee_s1::FACTORY_DEFAULT_BAUD = 9600;
const uint32_t xbee_s1::MIN_BAUD_TWO_STOP_BITS = 115200;
const uint32_t xbee_s1::DEFAULT_BAUD = 9600;
const uint32_t xbee_s1::DEFAULT_SERIAL_TIMEOUT_MS = 2000;
const uint32_t xbee_s1::DEFAULT_GUARD_TIME_S = 1;
const uint32_t xbee_s1::DEFAULT_COMMAND_MODE_TIMEOUT_S = 10;
const uint32_t xbee_s1::CTS_LOW_RETRIES = 100;
const uint32_t xbee_s1::MAX_INVALID_FRAME_READS = 20;
const std::chrono::milliseconds xbee_s1::CTS_LOW_SLEEP(5);
const std::chrono::milliseconds xbee_s1::INVALID_FRAME_READ_BACKOFF_SLEEP(200);
const std::chrono::milliseconds xbee_s1::SERIAL_READ_THRESHOLD(10);
const std::chrono::microseconds xbee_s1::SERIAL_READ_BACKOFF_SLEEP(100);
const char *const xbee_s1::COMMAND_SEQUENCE = "+++";

const std::map<uint8_t, uint32_t> xbee_s1::baud_config_map
    {
        { 0, 1200 },
        { 1, 2400 },
        { 2, 4800 },
        { 3, 9600 },
        { 4, 19200 },
        { 5, 38400 },
        { 6, 57600 },
        { 7, 115200 }
    };

xbee_s1::xbee_s1(const std::string &device)
    : xbee_s1(device, FACTORY_DEFAULT_BAUD)
{
}

// TODO: check for exceptions when initializing serial object
xbee_s1::xbee_s1(const std::string &device, uint32_t baud)
    : address(ADDRESS_UNKNOWN), serial(device, baud, serial::Timeout::simpleTimeout(DEFAULT_SERIAL_TIMEOUT_MS))
{
    LOG("using port: ", device, ", baud: ", baud);
}

bool xbee_s1::reset_firmware_settings()
{
    std::lock_guard<std::mutex> lock(access_lock);

    std::vector<uint32_t> baud_attempts;
    for (auto &entry : baud_config_map)
    {
        // don't bother with lower/unused baud rates to save time
        if (entry.second >= FACTORY_DEFAULT_BAUD)
        {
            baud_attempts.push_back(entry.second);
        }
    }

    bool api_mode_configured = false;
    for (auto &baud_attempt : baud_attempts)
    {
        LOG("attempting api mode configuration at ", baud_attempt, " baud");
        serial.setBaudrate(baud_attempt);

        // enable API mode for configuration since AT command mode can be flaky
        if (enable_api_mode())
        {
            api_mode_configured = true;
            break;
        }

        LOG_ERROR("connection attempt at ", baud_attempt, " baud failed");
    }

    if (!api_mode_configured)
    {
        return false;
    }

    if (!restore_defaults())
    {
        return false;
    }

    serial.setBaudrate(FACTORY_DEFAULT_BAUD);

    if (!enable_api_mode())
    {
        return false;
    }

    if (!write_to_non_volatile_memory())
    {
        return false;
    }

    // note: we don't bother disabling api mode here
    return true;
}

bool xbee_s1::initialize()
{
    std::lock_guard<std::mutex> lock(access_lock);
    configure_stop_bits();

    if (read_and_set_address())
    {
        LOG("ieee source address: ", util::to_hex_string(address));
        return true;
    }

    return false;
}

bool xbee_s1::configure_firmware_settings()
{
    std::lock_guard<std::mutex> lock(access_lock);
    configure_stop_bits();
    return enable_api_mode() && enable_64_bit_addressing() && enable_strict_802_15_4_mode()
        && configure_baud() && write_to_non_volatile_memory();
}

void xbee_s1::configure_stop_bits()
{
    // use two stop bits to increase success rate of frame reads/writes at higher baud
    if (serial.getBaudrate() >= MIN_BAUD_TWO_STOP_BITS)
    {
        LOG("using two stop bits");
        serial.setStopbits(serial::stopbits_two);
    }
}

bool xbee_s1::test_at_command_mode()
{
    // send empty "AT" query to see if we get an "OK" response
    std::string response = execute_command(at_command(at_command::REGISTER_QUERY));
    if (response != at_command::RESPONSE_SUCCESS)
    {
        LOG_ERROR("at command mode test failed, baud mismatch?");
        return false;
    }

    LOG("at command mode test succeeded");
    return true;
}

bool xbee_s1::enable_api_mode()
{
    // put device in API mode 1 (enabled without escape characters)
    std::string response = execute_command(at_command(at_command::API_ENABLE, "1"));
    if (response != at_command::RESPONSE_SUCCESS)
    {
        LOG_ERROR("could not configure api mode");
        return false;
    }

    LOG("api mode successfully enabled");
    return true;
}

bool xbee_s1::enable_64_bit_addressing()
{
    // enable 64 bit addressing mode by setting 16 bit address to 0xffff
    auto response = write_at_command_frame(std::make_shared<at_command_frame>(at_command::SOURCE_ADDRESS_16_BIT, std::vector<uint8_t>{ 0xff, 0xff }));
    if (response == nullptr)
    {
        return false;
    }

    if (response->get_status() != at_command_response_frame::status::ok)
    {
        LOG_ERROR("could not enable 64 bit addressing mode");
        return false;
    }

    LOG("64 bit addressing mode successfully enabled");
    return true;
}

bool xbee_s1::read_ieee_source_address(uint64_t &out_address)
{
    uint32_t serial_number_high;
    if (!read_configuration_register(at_command::SERIAL_NUMBER_HIGH, "serial number high", serial_number_high))
    {
        return false;
    }

    uint32_t serial_number_low;
    if (!read_configuration_register(at_command::SERIAL_NUMBER_LOW, "serial number low", serial_number_low))
    {
        return false;
    }

    out_address = (static_cast<uint64_t>(serial_number_high) << sizeof(uint32_t) * 8) + serial_number_low;
    return true;
}

template <typename T>
bool xbee_s1::read_configuration_register(const std::string &at_command_str, const std::string &command_description, T &register_value)
{
    auto response = write_at_command_frame(std::make_shared<at_command_frame>(at_command_str));
    if (response == nullptr)
    {
        return false;
    }

    if (response->get_status() != at_command_response_frame::status::ok)
    {
        LOG_ERROR("could not read ", command_description, ", status: ", +response->get_status());
        return false;
    }

    auto value = response->get_value();
    if (value.size() != sizeof(T))
    {
        LOG_ERROR("invalid at_command_response value size for ", command_description);
        return false;
    }

    register_value = util::unpack_bytes_to_width<T>(value.begin());
    return true;
}

bool xbee_s1::read_configuration_registers()
{
    // TODO: create global mapping of at_command_str -> command description
    uint64_t address;
    if (!read_ieee_source_address(address))
    {
        return false;
    }

    uint8_t api_mode;
    if (!read_configuration_register(at_command::API_ENABLE, "api mode", api_mode))
    {
        return false;
    }

    uint16_t address_16;
    if (!read_configuration_register(at_command::SOURCE_ADDRESS_16_BIT, "16 bit source address", address_16))
    {
        return false;
    }

    uint32_t baud;
    if (!read_configuration_register(at_command::INTERFACE_DATA_RATE, "interface data rate", baud))
    {
        return false;
    }

    uint8_t mac_mode;
    if (!read_configuration_register(at_command::MAC_MODE, "mac mode", mac_mode))
    {
        return false;
    }

    uint16_t pan_id;
    if (!read_configuration_register(at_command::PERSONAL_AREA_NETWORK_ID, "personal area network id", pan_id))
    {
        return false;
    }

    uint8_t channel;
    if (!read_configuration_register(at_command::CHANNEL, "channel", channel))
    {
        return false;
    }

    uint8_t xbee_retries;
    if (!read_configuration_register(at_command::XBEE_RETRIES, "xbee retries", xbee_retries))
    {
        return false;
    }

    uint8_t power_level;
    if (!read_configuration_register(at_command::POWER_LEVEL, "power level", power_level))
    {
        return false;
    }

    uint8_t cca_threshold;
    if (!read_configuration_register(at_command::CCA_THRESHOLD, "clear channel assessment threshold", cca_threshold))
    {
        return false;
    }

    LOG("[ATSH+ATSL] ieee source address: ", util::to_hex_string(address));
    LOG("[ATAP] api mode: ", util::to_hex_string(api_mode));
    LOG("[ATMY] 16 bit source address: ", util::to_hex_string(address_16));
    uint32_t baud_value = baud_config_map.find(baud) == baud_config_map.end()
        ? baud
        : baud_config_map.find(baud)->second;
    LOG("[ATBD] interface data rate: ", util::to_hex_string(baud), " => ", baud_value, " b/s");
    LOG("[ATMM] mac mode: ", util::to_hex_string(mac_mode));
    LOG("[ATID] personal area network id: ", util::to_hex_string(pan_id));
    LOG("[ATCH] channel: ", util::to_hex_string(channel), " => TODO MHz");
    LOG("[ATRR] xbee retries: ", util::to_hex_string(xbee_retries));
    LOG("[ATPL] power level: ", util::to_hex_string(power_level), " => TODO dBm");
    LOG("[ATCA] clear channel assessment threshold: ", util::to_hex_string(cca_threshold), " => TODO dBm");

    return true;
}

bool xbee_s1::restore_defaults()
{
    auto response = write_at_command_frame(std::make_shared<at_command_frame>(at_command::RESTORE_DEFAULTS));
    if (response == nullptr)
    {
        return false;
    }

    if (response->get_status() != at_command_response_frame::status::ok)
    {
        LOG_ERROR("could not restore configuration to factory default");
        return false;
    }

    LOG("restored configuration to factory default");
    return true;
}

bool xbee_s1::enable_strict_802_15_4_mode()
{
    // enable strict 802.15.4 mode (no Digi headers, no acks) by setting MAC mode to 1
    auto response = write_at_command_frame(std::make_shared<at_command_frame>(at_command::MAC_MODE, std::vector<uint8_t>{ 0x01 }));
    if (response == nullptr)
    {
        return false;
    }

    if (response->get_status() != at_command_response_frame::status::ok)
    {
        LOG_ERROR("could not enable strict 802.15.4 mode");
        return false;
    }

    LOG("strict 802.15.4 mode successfully enabled");
    return true;
}

bool xbee_s1::configure_baud()
{
    bool use_literal_baud = true;
    uint32_t baud;
    std::vector<uint8_t> baud_config_value;

    for (auto &entry : baud_config_map)
    {
        if (entry.second == DEFAULT_BAUD)
        {
            use_literal_baud = false;
            baud = entry.second;
            baud_config_value.push_back(entry.first);
            break;
        }
    }

    if (use_literal_baud)
    {
        if (DEFAULT_BAUD <= baud_config_map.end()->second)
        {
            LOG_ERROR("literal baud value must be greater than ", +baud_config_map.end()->second);
            return false;
        }

        // baud value is interpreted as actual baud rate
        baud = DEFAULT_BAUD;
        util::pack_value_as_bytes(std::back_inserter(baud_config_value), DEFAULT_BAUD);
    }

    auto response = write_at_command_frame(std::make_shared<at_command_frame>(at_command::INTERFACE_DATA_RATE, baud_config_value));
    if (response == nullptr)
    {
        return false;
    }

    // note: new baud takes effect after "OK" is sent
    if (response->get_status() != at_command_response_frame::status::ok)
    {
        LOG_ERROR("could not configure target baud");
        return false;
    }

    // update serial object to interface at newly configured baud
    serial.setBaudrate(baud);
    configure_stop_bits();
    LOG("target baud successfully configured");

    return true;
}

bool xbee_s1::write_to_non_volatile_memory()
{
    auto response = write_at_command_frame(std::make_shared<at_command_frame>(at_command::WRITE));
    if (response == nullptr)
    {
        return false;
    }

    if (response->get_status() != at_command_response_frame::status::ok)
    {
        // note: documentation specifies that once WR command is sent, no additional characters should be sent until
        // after "OK" is received (referring to response received in AT command mode) - we assume the same applies
        // to a WR command issued as an AT command frame so a sleep is introduced after failure as a safegaurd
        LOG_ERROR("could not write to non-volatile memory, sleeping to prevent additional characters from being sent");
        util::sleep(DEFAULT_COMMAND_MODE_TIMEOUT_S);
        return false;
    }

    LOG("settings successfully written to non-volatile memory");
    return true;
}

bool xbee_s1::read_and_set_address()
{
    return read_ieee_source_address(this->address);
}

uint64_t xbee_s1::get_address() const
{
    return address;
}

// TODO: rename to indicate this uses AT/"+++" mode?
std::string xbee_s1::execute_command(const at_command &command, bool exit_command_mode)
{
    std::string response;
    if (!enter_command_mode())
    {
        return response;
    }

    unlocked_write_string(command);
    response = unlocked_read_line();
    if (response.empty())
    {
        LOG_ERROR("empty response from device");
    }

    if (exit_command_mode)
    {
        unlocked_write_string(at_command(at_command::EXIT_COMMAND_MODE));
        std::string exit_response = unlocked_read_line();

        if (exit_response != at_command::RESPONSE_SUCCESS)
        {
            LOG_ERROR("could not exit command mode, sleeping for ", DEFAULT_COMMAND_MODE_TIMEOUT_S, "s");
            util::sleep(DEFAULT_COMMAND_MODE_TIMEOUT_S);
        }
    }

    return response;
}

bool xbee_s1::enter_command_mode()
{
    // note: sleep is required before AND after input of COMMAND_SEQUENCE
    util::sleep(DEFAULT_GUARD_TIME_S);
    unlocked_write_string(COMMAND_SEQUENCE);
    util::sleep(DEFAULT_GUARD_TIME_S);
    std::string response = unlocked_read_line();

    if (response != at_command::RESPONSE_SUCCESS)
    {
        LOG_ERROR("could not enter command mode");
        return false;
    }

    return true;
}

void xbee_s1::unlocked_write_string(const std::string &str)
{
    size_t bytes_written;

    try
    {
        bytes_written = serial.write(str);
    }
    catch (const serial::PortNotOpenedException &e)
    {
        LOG_ERROR("write_string: caught PortNotOpenedException: ", e.what());
        return;
    }
    catch (const serial::SerialException &e)
    {
        LOG_ERROR("write_string: caught SerialException: ", e.what());
        return;
    }
    catch (const serial::IOException &e)
    {
        LOG_ERROR("write_string: caught IOException: ", e.what());
        return;
    }

    if (bytes_written != str.size())
    {
        LOG_ERROR("could not write string (expected: ", str.size(), ", wrote: ", bytes_written, " bytes)");
        return;
    }

    LOG("write [", util::get_escaped_string(str), "] (", bytes_written, " bytes)");
}

// TODO: return status
void xbee_s1::write_frame(const std::vector<uint8_t> &payload)
{
    std::lock_guard<std::mutex> lock(access_lock);
    unlocked_write_frame(payload);
}

std::shared_ptr<at_command_response_frame> xbee_s1::write_at_command_frame(std::shared_ptr<at_command_frame> command)
{
    auto response = unlocked_write_and_read_frame(uart_frame(command));
    if (response == nullptr)
    {
        LOG_ERROR("could not read response to ", command->get_at_command(), " command, is API mode (1) enabled?");
        return nullptr;
    }

    // TODO: have casting done in derived frame_data classes? better way to do this?
    if (response->get_api_identifier() != frame_data::api_identifier::at_command_response)
    {
        LOG_ERROR("response frame is not an at_command_response");
        return nullptr;
    }

    return std::static_pointer_cast<at_command_response_frame>(response->get_data());
}

std::string xbee_s1::unlocked_read_line()
{
    std::string result;
    size_t bytes_read;

    try
    {
        bytes_read = serial.readline(result);
    }
    catch (const serial::PortNotOpenedException &e)
    {
        LOG_ERROR("read_line: caught PortNotOpenedException: ", e.what());
        return std::string();
    }
    catch (const serial::SerialException &e)
    {
        LOG_ERROR("read_line: caught SerialException: ", e.what());
        return std::string();
    }
    catch (const serial::IOException &e)
    {
        LOG_ERROR("read_line: caught IOException: ", e.what());
        return std::string();
    }

    LOG("read  [", util::get_escaped_string(result), "] (", bytes_read, " bytes)");

    return util::strip_newline(result);
}

std::shared_ptr<uart_frame> xbee_s1::read_frame()
{
    std::lock_guard<std::mutex> lock(access_lock);
    return unlocked_read_frame();
}

std::shared_ptr<uart_frame> xbee_s1::write_and_read_frame(const std::vector<uint8_t> &payload)
{
    std::lock_guard<std::mutex> lock(access_lock);
    return unlocked_write_and_read_frame(payload);
}

void xbee_s1::unlocked_write_frame(const std::vector<uint8_t> &payload)
{
    size_t bytes_written = 0;

    try
    {
        util::retry([&]
            {
                if (serial.getCTS())
                {
                    bytes_written = serial.write(payload);
                    return true;
                }
                else
                {
                    std::this_thread::sleep_for(CTS_LOW_SLEEP);
                    return false;
                }
            }, CTS_LOW_RETRIES);

        if (bytes_written == 0)
        {
            return;
        }
    }
    catch (const serial::PortNotOpenedException &e)
    {
        LOG_ERROR("write_frame: caught PortNotOpenedException: ", e.what());
        return;
    }
    catch (const serial::SerialException &e)
    {
        LOG_ERROR("write_frame: caught SerialException: ", e.what());
        return;
    }
    catch (const serial::IOException &e)
    {
        LOG_ERROR("write_frame: caught IOException: ", e.what());
        return;
    }

    if (bytes_written != payload.size())
    {
        LOG_ERROR("could not write frame (expected: ", payload.size(), ", wrote: ", bytes_written, " bytes)");
        return;
    }

    LOG("write [", util::get_frame_hex(payload), "] (", bytes_written, " bytes)");
}

std::shared_ptr<uart_frame> xbee_s1::unlocked_read_frame()
{
    static uint32_t invalid_frame_reads = 0;

    // read up to length header first to determine how many more bytes to read from serial line
    std::vector<uint8_t> frame;
    size_t bytes_read_head;

    try
    {
        /*
         * Ideally we'd check serial.available() once to see if there are bytes available to be read before incurring the potential read
         * timeout cost of serial.read() when there aren't bytes available, but it seems available() has a bit of a delay.
         *
         * After some testing it looks like the average response for available() to reflect the presence of bytes on the serial line is 2ms
         * and 3-4ms for available() to reflect the correct value. So far there hasn't been a case where calling serial.read(), after available()
         * returned a non-zero value other than the full expected frame size, resulted in a failed frame read. Therefore we only wait until a
         * presence of bytes is detected and then try to read a frame.
         *
         * An effort is made to reduce the amount of calls made to the serial interface as it seems that writing too many frames to the xbee
         * can cause the device to lock up and stop responding in certain situations. So far it seems this lockup has only been triggered by
         * repeated AT command frame writes and hasn't been reproducable by having two xbees constantly transmitting to each other.
         *
         * The only solution in the case of the xbee locking up seems to be removing and reinserting the USB to serial adapter or using the
         * physical reset button if the adapter has one.
         *
         * TODO: ensure beehive can handle adapter being removed and reinserted
         *  - expose state to indicate that device is no longer responding
         */

        size_t bytes_available = 0;
        auto start_time = std::chrono::high_resolution_clock::now();
        auto backoff_duration = SERIAL_READ_BACKOFF_SLEEP;

        while ((bytes_available = serial.available()) == 0)
        {
            if (std::chrono::high_resolution_clock::now() - start_time >= SERIAL_READ_THRESHOLD)
            {
                return nullptr;
            }

            // TODO: bounded exponential backoff between calls? reset backoff duration upon successful frame read
            std::this_thread::sleep_for(backoff_duration);
            backoff_duration *= 2;
        }

        bytes_read_head = serial.read(frame, uart_frame::HEADER_LENGTH);
    }
    catch (const serial::PortNotOpenedException &e)
    {
        LOG_ERROR("read_frame: caught PortNotOpenedException: ", e.what());
        return nullptr;
    }
    catch (const serial::SerialException &e)
    {
        LOG_ERROR("read_frame: caught SerialException: ", e.what());
        return nullptr;
    }
    catch (const serial::IOException &e)
    {
        LOG_ERROR("read_frame: caught IOException: ", e.what());
        return nullptr;
    }

    if (bytes_read_head != uart_frame::HEADER_LENGTH)
    {
        if (bytes_read_head != 0)
        {
            LOG_ERROR("could not read frame delimiter + length");
        }

        return nullptr;
    }

    if (frame[uart_frame::FRAME_DELIMITER_OFFSET] != uart_frame::FRAME_DELIMITER)
    {
        ++invalid_frame_reads;

        if (invalid_frame_reads >= MAX_INVALID_FRAME_READS)
        {
            std::this_thread::sleep_for(INVALID_FRAME_READ_BACKOFF_SLEEP);
            invalid_frame_reads = 0;
        }

        return nullptr;
    }

    // note:
    //  - in API mode 2 (enabled with escape characters) length field doesn't account for escaped chars
    //  - checksum can be escaped
    uint16_t length = util::unpack_bytes_to_width<uint16_t>(frame.begin() + uart_frame::LENGTH_MSB_OFFSET);
    size_t bytes_read_tail;

    try
    {
        bytes_read_tail = serial.read(frame, length + 1);    // payload + checksum
    }
    catch (const serial::PortNotOpenedException &e)
    {
        LOG_ERROR("read_frame: caught PortNotOpenedException: ", e.what());
        return nullptr;
    }
    catch (const serial::SerialException &e)
    {
        LOG_ERROR("read_frame: caught SerialException: ", e.what());
        return nullptr;
    }

    if (bytes_read_tail != length + 1)
    {
        LOG_ERROR("could not read rest of frame (expected: ", length + 1, ", read: ", bytes_read_tail, " bytes)");
        return nullptr;
    }

    LOG("read  [", util::get_frame_hex(frame), "] (", frame.size(), " bytes)");
    uint8_t received_checksum = *(frame.end() - 1);
    uint8_t calculated_checksum = uart_frame::compute_checksum(frame.begin() + uart_frame::HEADER_LENGTH, frame.end() - 1); // ignore frame header and trailing checksum

    if (calculated_checksum != received_checksum)
    {
        LOG_ERROR("invalid checksum");
        return nullptr;
    }

    return uart_frame::parse_frame(frame.begin(), frame.end());
}

std::shared_ptr<uart_frame> xbee_s1::unlocked_write_and_read_frame(const std::vector<uint8_t> &payload)
{
    unlocked_write_frame(payload);
    return unlocked_read_frame();
}
