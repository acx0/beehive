#include "xbee_s1.h"

const uint64_t xbee_s1::ADDRESS_UNKNOWN = 0xffffffffffffffff;
const uint64_t xbee_s1::BROADCAST_ADDRESS = 0xffff;
// setting timeout <= 525 seems to cause serial reads to sometimes return nothing on odroid
const uint32_t xbee_s1::DEFAULT_TIMEOUT_MS = 700;
const uint32_t xbee_s1::DEFAULT_GUARD_TIME_S = 1;
const uint32_t xbee_s1::DEFAULT_COMMAND_MODE_TIMEOUT_S = 10;
const uint8_t xbee_s1::HEADER_LENGTH_END_POSITION = 3;   // delimiter + 2 length bytes
const uint8_t xbee_s1::API_IDENTIFIER_INDEX = 0;
const char *const xbee_s1::COMMAND_SEQUENCE = "+++";

// TODO: check for exceptions when initializing serial object
xbee_s1::xbee_s1()
    : address(ADDRESS_UNKNOWN), serial(config.port, config.baud, serial::Timeout::simpleTimeout(DEFAULT_TIMEOUT_MS))
{
    LOG("using port: ", config.port, ", baud: ", config.baud);
}

xbee_s1::xbee_s1(uint32_t baud)
    : address(ADDRESS_UNKNOWN), serial(config.port, baud, serial::Timeout::simpleTimeout(DEFAULT_TIMEOUT_MS))
{
    LOG("using port: ", config.port, ", baud: ", baud);
}

bool xbee_s1::reset_firmware_settings()
{
    std::lock_guard<std::mutex> lock(access_lock);
    if (!test_at_command_mode())
    {
        return false;
    }

    std::string response = execute_command(at_command(at_command::RESTORE_DEFAULTS));
    if (response != at_command::RESPONSE_SUCCESS)
    {
        LOG_ERROR("firmware reset failed");
        return false;
    }

    // note: once WR command is sent, no additional characters should be sent until after "OK" is received
    // hence why we disable CN from being sent after WR is issued
    response = execute_command(at_command(at_command::WRITE), false);
    if (response != at_command::RESPONSE_SUCCESS)
    {
        LOG_ERROR("could not write to non-volatile memory, sleeping to prevent additional characters from being sent");
        util::sleep(DEFAULT_COMMAND_MODE_TIMEOUT_S);
        return false;
    }

    // explicitly exit command mode if write is successful
    response = execute_command(at_command(at_command::EXIT_COMMAND_MODE), false);
    if (response != at_command::RESPONSE_SUCCESS)
    {
        LOG_ERROR("could not exit command mode, sleeping for ", DEFAULT_COMMAND_MODE_TIMEOUT_S, "s");
        util::sleep(DEFAULT_COMMAND_MODE_TIMEOUT_S);
    }

    LOG("settings successfully written to non-volatile memory");
    return true;
}

bool xbee_s1::initialize()
{
    std::lock_guard<std::mutex> lock(access_lock);

    // use two stop bits to increase success rate of frame reads/writes at higher baud
    serial.setStopbits(serial::stopbits_two);
    return read_ieee_source_address();
}

bool xbee_s1::configure_firmware_settings()
{
    std::lock_guard<std::mutex> lock(access_lock);
    return enable_api_mode() && enable_64_bit_addressing() && read_ieee_source_address()
        && enable_strict_802_15_4_mode() && configure_baud() && write_to_non_volatile_memory();
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
        LOG_ERROR("could not enter api mode");
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

bool xbee_s1::read_ieee_source_address()
{
    std::vector<uint8_t> address;
    auto response = write_at_command_frame(std::make_shared<at_command_frame>(at_command::SERIAL_NUMBER_HIGH));
    if (response == nullptr)
    {
        return false;
    }

    if (response->get_status() != at_command_response_frame::status::ok)
    {
        LOG_ERROR("could not read serial number high, status: ", +response->get_status());
        return false;
    }

    auto value = response->get_value();
    if (value.size() != sizeof(uint32_t))
    {
        LOG_ERROR("invalid at_command_response value size for serial number high");
        return false;
    }

    address.insert(address.end(), value.begin(), value.end());
    response = write_at_command_frame(std::make_shared<at_command_frame>(at_command::SERIAL_NUMBER_LOW));
    if (response == nullptr)
    {
        return false;
    }

    if (response->get_status() != at_command_response_frame::status::ok)
    {
        LOG_ERROR("could not read serial number low, status: ", +response->get_status());
        return false;
    }

    value = response->get_value();
    if (value.size() != sizeof(uint32_t))
    {
        LOG_ERROR("invalid at_command_response value size for serial number low");
        return false;
    }

    address.insert(address.end(), value.begin(), value.end());
    this->address = util::unpack_bytes_to_width<uint64_t>(address);

    // TODO: do iomanip settings have to be reverted? use ostringstream for now
    std::ostringstream oss;
    oss << "ieee source address: 0x" << std::setfill('0') << std::setw(16) << std::hex << +this->address;
    LOG(oss.str());

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
    // 7 = 115200 bps
    auto response = write_at_command_frame(std::make_shared<at_command_frame>(at_command::INTERFACE_DATA_RATE, std::vector<uint8_t>{ 0x07 }));
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
    serial.setBaudrate(115200);
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
        LOG_ERROR("could not write to non-volatile memory");
        return false;
    }

    LOG("settings successfully written to non-volatile memory");
    return true;
}

uint64_t xbee_s1::get_address() const
{
    return address;
}

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
    size_t bytes_written;

    try
    {
        bytes_written = serial.write(payload);
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
    // read up to length header first to determine how many more bytes to read from serial line
    std::vector<uint8_t> frame_head;    // contains start delimiter to length bytes
    size_t bytes_read_head;

    try
    {
        bytes_read_head = serial.read(frame_head, HEADER_LENGTH_END_POSITION);
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

    if (bytes_read_head != HEADER_LENGTH_END_POSITION)
    {
        if (bytes_read_head != 0)
        {
            LOG_ERROR("could not read frame delimiter + length");
        }

        return nullptr;
    }

    if (frame_head[0] != uart_frame::FRAME_DELIMITER)
    {
        LOG_ERROR("frame_head did not contain valid start delimiter");
        return nullptr;
    }

    uint16_t length = 0;
    length += frame_head[1] << (sizeof(uint8_t) * 8);
    length += frame_head[2];

    // note: in API mode 2 (enabled with escape characters) length field doesn't account for escaped chars; checksum can be escaped
    std::vector<uint8_t> frame_tail;    // contains api identifier to checksum bytes
    size_t bytes_read_tail;

    try
    {
        bytes_read_tail = serial.read(frame_tail, length + 1);    // payload + checksum
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

    LOG("read  [", util::get_frame_hex(frame_head), " ", util::get_frame_hex(frame_tail), "] (", bytes_read_head + bytes_read_tail, " bytes)");
    uint8_t received_checksum = frame_tail[length];
    frame_tail.resize(length);  // truncate checksum byte
    uint8_t calculated_checksum = uart_frame::compute_checksum(frame_tail);

    if (calculated_checksum != received_checksum)
    {
        LOG_ERROR("invalid checksum");
        return nullptr;
    }

    switch (frame_tail[API_IDENTIFIER_INDEX])
    {
        case frame_data::api_identifier::rx_packet_64:
            return std::make_shared<uart_frame>(frame_head[1], frame_head[2],
                std::make_shared<rx_packet_64_frame>(frame_tail), received_checksum);

        case frame_data::api_identifier::at_command_response:
            return std::make_shared<uart_frame>(frame_head[1], frame_head[2],
                std::make_shared<at_command_response_frame>(frame_tail), received_checksum);

        case frame_data::api_identifier::tx_status:
            return std::make_shared<uart_frame>(frame_head[1], frame_head[2],
                std::make_shared<tx_status_frame>(frame_tail), received_checksum);

        default:
            LOG_ERROR("invalid api identifier value");
            return nullptr;
    }
}

std::shared_ptr<uart_frame> xbee_s1::unlocked_write_and_read_frame(const std::vector<uint8_t> &payload)
{
    unlocked_write_frame(payload);
    return unlocked_read_frame();
}
