#include "xbee_s1.h"

xbee_s1::xbee_s1()
    : serial(config.port, config.baud, serial::Timeout::simpleTimeout(DEFAULT_TIMEOUT_MS))
{
}

bool xbee_s1::enable_api_mode()
{
    // put device in API mode 2 (enabled with escape characters)
    std::string response = execute_command(at_command(at_command::API_ENABLE, 2));
    if (response != at_command::RESPONSE_SUCCESS)
    {
        std::cerr << "could not enter api mode" << std::endl;
        return false;
    }

    std::clog << "api mode successfully enabled" << std::endl;
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
    std::clog << "write [" << util::get_escaped_string(str) << "] (" << bytes_written << " bytes)" << std::endl;
}

std::string xbee_s1::read_line()
{
    std::string result;
    size_t bytes_read = serial.readline(result);
    std::clog << "read  [" << util::get_escaped_string(result) << "] (" << bytes_read << " bytes)" << std::endl;

    return util::strip_newline(result);
}
