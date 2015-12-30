#include "xbee_s1.h"

// setting timeout <= 525 seems to cause serial reads to sometimes return nothing on odroid
const uint32_t xbee_s1::DEFAULT_TIMEOUT_MS = 700;
const uint32_t xbee_s1::DEFAULT_GUARD_TIME_S = 1;
const uint32_t xbee_s1::DEFAULT_COMMAND_MODE_TIMEOUT_S = 10;
const uint8_t xbee_s1::HEADER_LENGTH_END_POSITION = 3;   // delimiter + 2 length bytes
const uint8_t xbee_s1::API_IDENTIFIER_INDEX = 0;
const char *const xbee_s1::COMMAND_SEQUENCE = "+++";

xbee_s1::xbee_s1()
    : serial(config.port, config.baud, serial::Timeout::simpleTimeout(DEFAULT_TIMEOUT_MS))
{
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

// TODO: can also do this with at_command_frame once api mode is enabled
bool xbee_s1::enable_64_bit_addressing()
{
    // enable 64 bit addressing mode by setting 16 bit address to 0xFFFF
    std::string response = execute_command(at_command(at_command::SOURCE_ADDRESS_16_BIT, "FFFF"));
    if (response != at_command::RESPONSE_SUCCESS)
    {
        std::cerr << "could not enable 64 bit addressing mode" << std::endl;
        return false;
    }

    std::clog << "64 bit addressing mode successfully enabled" << std::endl;
    return true;
}

std::string xbee_s1::execute_command(const at_command &command)
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
        return response;
    }

    write_string(at_command(at_command::EXIT_COMMAND_MODE));
    std::string exit_response = read_line();

    if (exit_response != at_command::RESPONSE_SUCCESS)
    {
        std::cerr << "could not exit command mode, sleep for " << DEFAULT_COMMAND_MODE_TIMEOUT_S << "s" << std::endl;
        util::sleep(DEFAULT_COMMAND_MODE_TIMEOUT_S);
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
    size_t bytes_written = serial.write(str);

    if (bytes_written != str.size())
    {
        std::cerr << "could not write string (expected: " << str.size() << ", wrote: " << bytes_written << " bytes)" << std::endl;
        return;
    }

    std::clog << "write [" << util::get_escaped_string(str) << "] (" << bytes_written << " bytes)" << std::endl;
}

void xbee_s1::write_frame(const std::vector<uint8_t> &payload)
{
    size_t bytes_written = serial.write(payload);

    if (bytes_written != payload.size())
    {
        std::cerr << "could not write frame (expected: " << payload.size() << ", wrote: " << bytes_written << " bytes)" << std::endl;
        return;
    }

    std::clog << "write [" << util::get_frame_hex(payload) << "] (" << bytes_written << " bytes)" << std::endl;
}

std::string xbee_s1::read_line()
{
    std::string result;
    size_t bytes_read = serial.readline(result);
    std::clog << "read  [" << util::get_escaped_string(result) << "] (" << bytes_read << " bytes)" << std::endl;

    return util::strip_newline(result);
}

std::unique_ptr<uart_frame> xbee_s1::read_frame()
{
    // read up to length header first to determine how many more bytes to read from serial line
    std::vector<uint8_t> frame_head;    // contains start delimiter to length bytes
    size_t bytes_read = serial.read(frame_head, HEADER_LENGTH_END_POSITION);
    std::clog << "read  [" << util::get_frame_hex(frame_head) << "] (" << bytes_read << " bytes)" << std::endl;

    if (bytes_read != HEADER_LENGTH_END_POSITION)
    {
        std::cerr << "could not read frame delimiter + length" << std::endl;
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
    bytes_read = serial.read(frame_tail, length + 1);    // payload + checksum

    if (bytes_read != length + 1)
    {
        std::cerr << "could not read rest of frame (expected: " << length + 1 << ", read: " << bytes_read << " bytes)" << std::endl;
        return nullptr;
    }

    std::clog << "read  [" << util::get_frame_hex(frame_tail) << "] (" << bytes_read << " bytes)" << std::endl;
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
            return std::unique_ptr<uart_frame>(new uart_frame(frame_head[1], frame_head[2],
                std::make_shared<rx_packet_64_frame>(frame_tail), received_checksum));

        case frame_data::api_identifier::at_command_response:
            return std::unique_ptr<uart_frame>(new uart_frame(frame_head[1], frame_head[2],
                std::make_shared<at_command_response_frame>(frame_tail), received_checksum));

        case frame_data::api_identifier::tx_status:
            return std::unique_ptr<uart_frame>(new uart_frame(frame_head[1], frame_head[2],
                std::make_shared<tx_status_frame>(frame_tail), received_checksum));

        default:
            std::cerr << "invalid api identifier value" << std::endl;
            return nullptr;
    }
}
