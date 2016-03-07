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
    std::clog << "using port: " << config.port << ", baud: " << config.baud << std::endl;
}

xbee_s1::xbee_s1(uint32_t baud)
    : address(ADDRESS_UNKNOWN), serial(config.port, baud, serial::Timeout::simpleTimeout(DEFAULT_TIMEOUT_MS))
{
    std::clog << "using port: " << config.port << ", baud: " << baud << std::endl;
}

bool xbee_s1::reset_firmware_settings()
{
    if (!test_at_command_mode())
    {
        return false;
    }

    std::string response = execute_command(at_command(at_command::RESTORE_DEFAULTS));
    if (response != at_command::RESPONSE_SUCCESS)
    {
        std::cerr << "firmware reset failed" << std::endl;
        return false;
    }

    // note: once WR command is sent, no additional characters should be sent until after "OK" is received
    // hence why we disable CN from being sent after WR is issued
    response = execute_command(at_command(at_command::WRITE), false);
    if (response != at_command::RESPONSE_SUCCESS)
    {
        std::cerr << "could not write to non-volatile memory, sleeping to prevent additional characters from being sent" << std::endl;
        util::sleep(DEFAULT_COMMAND_MODE_TIMEOUT_S);
        return false;
    }

    // explicitly exit command mode if write is successful
    response = execute_command(at_command(at_command::EXIT_COMMAND_MODE), false);
    if (response != at_command::RESPONSE_SUCCESS)
    {
        std::cerr << "could not exit command mode, sleeping for " << DEFAULT_COMMAND_MODE_TIMEOUT_S << "s" << std::endl;
        util::sleep(DEFAULT_COMMAND_MODE_TIMEOUT_S);
    }

    std::clog << "settings successfully written to non-volatile memory" << std::endl;
    return true;
}

bool xbee_s1::initialize()
{
    return read_ieee_source_address();
}

bool xbee_s1::configure_firmware_settings()
{
    return enable_api_mode() && enable_64_bit_addressing() && read_ieee_source_address()
        && enable_strict_802_15_4_mode() && configure_baud() && write_to_non_volatile_memory();
}

bool xbee_s1::test_at_command_mode()
{
    // send empty "AT" query to see if we get an "OK" response
    std::string response = execute_command(at_command(at_command::REGISTER_QUERY));
    if (response != at_command::RESPONSE_SUCCESS)
    {
        std::cerr << "at command mode test failed, baud mismatch?" << std::endl;
        return false;
    }

    std::clog << "at command mode test succeeded" << std::endl;
    return true;
}

bool xbee_s1::enable_api_mode()
{
    // put device in API mode 1 (enabled without escape characters)
    std::string response = execute_command(at_command(at_command::API_ENABLE, "1"));
    if (response != at_command::RESPONSE_SUCCESS)
    {
        std::cerr << "could not enter api mode" << std::endl;
        return false;
    }

    std::clog << "api mode successfully enabled" << std::endl;
    return true;
}

bool xbee_s1::enable_64_bit_addressing()
{
    // enable 64 bit addressing mode by setting 16 bit address to 0xffff
    auto response = write_at_command_frame(std::make_shared<at_command_frame>(
        at_command::SOURCE_ADDRESS_16_BIT, std::vector<uint8_t>{ 0xff, 0xff }));
    if (response == nullptr)
    {
        return false;
    }

    if (response->get_status() != at_command_response_frame::status::ok)
    {
        std::cerr << "could not enable 64 bit addressing mode" << std::endl;
        return false;
    }

    std::clog << "64 bit addressing mode successfully enabled" << std::endl;
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
        std::cerr << "could not read serial number high, status: " << +response->get_status() << std::endl;
        return false;
    }

    auto value = response->get_value();
    if (value.size() != sizeof(uint32_t))
    {
        std::cerr << "invalid at_command_response value size for serial number high" << std::endl;
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
        std::cerr << "could not read serial number low, status: " << +response->get_status() << std::endl;
        return false;
    }

    value = response->get_value();
    if (value.size() != sizeof(uint32_t))
    {
        std::cerr << "invalid at_command_response value size for serial number low" << std::endl;
        return false;
    }

    address.insert(address.end(), value.begin(), value.end());
    this->address = util::unpack_bytes_to_width<uint64_t>(address);

    // TODO: do iomanip settings have to be reverted? use ostringstream for now
    std::ostringstream oss;
    oss << "ieee source address: 0x" << std::setfill('0') << std::setw(16) << std::hex << +this->address;
    std::cout << oss.str() << std::endl;

    return true;
}

bool xbee_s1::enable_strict_802_15_4_mode()
{
    // enable strict 802.15.4 mode (no Digi headers, no acks) by setting MAC mode to 1
    auto response = write_at_command_frame(std::make_shared<at_command_frame>(
        at_command::MAC_MODE, std::vector<uint8_t>{ 0x01 }));
    if (response == nullptr)
    {
        return false;
    }

    if (response->get_status() != at_command_response_frame::status::ok)
    {
        std::cerr << "could not enable strict 802.15.4 mode" << std::endl;
        return false;
    }

    std::clog << "strict 802.15.4 mode successfully enabled" << std::endl;
    return true;
}

bool xbee_s1::configure_baud()
{
    // 7 = 115200 bps
    auto response = write_at_command_frame(std::make_shared<at_command_frame>(
        at_command::INTERFACE_DATA_RATE, std::vector<uint8_t>{ 0x07 }));
    if (response == nullptr)
    {
        return false;
    }

    // note: new baud takes effect after "OK" is sent
    if (response->get_status() != at_command_response_frame::status::ok)
    {
        std::cerr << "could not configure target baud" << std::endl;
        return false;
    }

    // update serial object to interface at newly configured baud
    serial.setBaudrate(115200);
    std::clog << "target baud successfully configured" << std::endl;

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
        std::cerr << "could not write to non-volatile memory" << std::endl;
        return false;
    }

    std::clog << "settings successfully written to non-volatile memory" << std::endl;
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

    write_string(command);
    response = read_line();
    if (response.empty())
    {
        std::cerr << "empty response from device" << std::endl;
    }

    if (exit_command_mode)
    {
        write_string(at_command(at_command::EXIT_COMMAND_MODE));
        std::string exit_response = read_line();

        if (exit_response != at_command::RESPONSE_SUCCESS)
        {
            std::cerr << "could not exit command mode, sleeping for " << DEFAULT_COMMAND_MODE_TIMEOUT_S << "s" << std::endl;
            util::sleep(DEFAULT_COMMAND_MODE_TIMEOUT_S);
        }
    }

    return response;
}

bool xbee_s1::enter_command_mode()
{
    // note: sleep is required before AND after input of COMMAND_SEQUENCE
    util::sleep(DEFAULT_GUARD_TIME_S);
    write_string(COMMAND_SEQUENCE);
    util::sleep(DEFAULT_GUARD_TIME_S);
    std::string response = read_line();

    if (response != at_command::RESPONSE_SUCCESS)
    {
        std::cerr << "could not enter command mode" << std::endl;
        return false;
    }

    return true;
}

void xbee_s1::write_string(const std::string &str)
{
    size_t bytes_written;

    try
    {
        bytes_written = serial.write(str);
    }
    catch (const serial::PortNotOpenedException &e)
    {
        std::clog << "write_string: caught PortNotOpenedException: " << e.what() << std::endl;
        return;
    }
    catch (const serial::SerialException &e)
    {
        std::clog << "write_string: caught SerialException: " << e.what() << std::endl;
        return;
    }
    catch (const serial::IOException &e)
    {
        std::clog << "write_string: caught IOException: " << e.what() << std::endl;
        return;
    }

    if (bytes_written != str.size())
    {
        std::cerr << "could not write string (expected: " << str.size() << ", wrote: " << bytes_written << " bytes)" << std::endl;
        return;
    }

    std::clog << "write [" << util::get_escaped_string(str) << "] (" << bytes_written << " bytes)" << std::endl;
}

// TODO: return status
void xbee_s1::write_frame(const std::vector<uint8_t> &payload)
{
    size_t bytes_written;

    try
    {
        bytes_written = serial.write(payload);
    }
    catch (const serial::PortNotOpenedException &e)
    {
        std::clog << "write_frame: caught PortNotOpenedException: " << e.what() << std::endl;
        return;
    }
    catch (const serial::SerialException &e)
    {
        std::clog << "write_frame: caught SerialException: " << e.what() << std::endl;
        return;
    }
    catch (const serial::IOException &e)
    {
        std::clog << "write_frame: caught IOException: " << e.what() << std::endl;
        return;
    }

    if (bytes_written != payload.size())
    {
        std::cerr << "could not write frame (expected: " << payload.size() << ", wrote: " << bytes_written << " bytes)" << std::endl;
        return;
    }

    std::clog << "write [" << util::get_frame_hex(payload) << "] (" << bytes_written << " bytes)" << std::endl;
}

std::shared_ptr<at_command_response_frame> xbee_s1::write_at_command_frame(std::shared_ptr<at_command_frame> command)
{
    write_frame(uart_frame(command));
    auto response = read_frame();

    if (response == nullptr)
    {
        std::cerr << "could not read response to " << command->get_at_command() << " command, is API mode (1) enabled?" << std::endl;
        return nullptr;
    }

    // TODO: have casting done in derived frame_data classes? better way to do this?
    if (response->get_api_identifier() != frame_data::api_identifier::at_command_response)
    {
        std::cerr << "response frame is not an at_command_response" << std::endl;
        return nullptr;
    }

    return std::static_pointer_cast<at_command_response_frame>(response->get_data());
}

std::string xbee_s1::read_line()
{
    std::string result;
    size_t bytes_read;

    try
    {
        bytes_read = serial.readline(result);
    }
    catch (const serial::PortNotOpenedException &e)
    {
        std::clog << "read_line: caught PortNotOpenedException: " << e.what() << std::endl;
        return std::string();
    }
    catch (const serial::SerialException &e)
    {
        std::clog << "read_line: caught SerialException: " << e.what() << std::endl;
        return std::string();
    }
    catch (const serial::IOException &e)
    {
        std::clog << "read_line: caught IOException: " << e.what() << std::endl;
        return std::string();
    }

    std::clog << "read  [" << util::get_escaped_string(result) << "] (" << bytes_read << " bytes)" << std::endl;

    return util::strip_newline(result);
}

std::shared_ptr<uart_frame> xbee_s1::read_frame()
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
        std::clog << "read_frame: caught PortNotOpenedException: " << e.what() << std::endl;
        return nullptr;
    }
    catch (const serial::SerialException &e)
    {
        std::clog << "read_frame: caught SerialException: " << e.what() << std::endl;
        return nullptr;
    }
    catch (const serial::IOException &e)
    {
        std::clog << "read_frame: caught IOException: " << e.what() << std::endl;
        return nullptr;
    }

    if (bytes_read_head != HEADER_LENGTH_END_POSITION)
    {
        if (bytes_read_head != 0)
        {
            std::cerr << "could not read frame delimiter + length" << std::endl;
        }

        return nullptr;
    }

    if (frame_head[0] != uart_frame::FRAME_DELIMITER)
    {
        std::cerr << "frame_head did not contain valid start delimiter" << std::endl;
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
        std::clog << "read_frame: caught PortNotOpenedException: " << e.what() << std::endl;
        return nullptr;
    }
    catch (const serial::SerialException &e)
    {
        std::clog << "read_frame: caught SerialException: " << e.what() << std::endl;
        return nullptr;
    }

    if (bytes_read_tail != length + 1)
    {
        std::cerr << "could not read rest of frame (expected: " << length + 1 << ", read: " << bytes_read_tail << " bytes)" << std::endl;
        return nullptr;
    }

    std::clog << "read  [" << util::get_frame_hex(frame_head) << " " << util::get_frame_hex(frame_tail) << "] ("
        << bytes_read_head + bytes_read_tail << " bytes)" << std::endl;
    uint8_t received_checksum = frame_tail[length];
    frame_tail.resize(length);  // truncate checksum byte
    uint8_t calculated_checksum = uart_frame::compute_checksum(frame_tail);

    if (calculated_checksum != received_checksum)
    {
        std::cerr << "invalid checksum" << std::endl;
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
            std::cerr << "invalid api identifier value" << std::endl;
            return nullptr;
    }
}
