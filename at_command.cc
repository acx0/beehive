#include "at_command.h"

const char *const at_command::RESPONSE_SUCCESS = "OK";
const char *const at_command::RESPONSE_ERROR = "ERROR";

const char *const at_command::AT_PREFIX = "AT";
const char *const at_command::API_ENABLE = "AP";
const char *const at_command::EXIT_COMMAND_MODE = "CN";
const char *const at_command::SOURCE_ADDRESS_16_BIT = "MY";
const char *const at_command::SERIAL_NUMBER_HIGH = "SH";
const char *const at_command::SERIAL_NUMBER_LOW = "SL";
const char *const at_command::SOFTWARE_RESET = "FR";
const char *const at_command::MAC_MODE = "MM";
const char *const at_command::INTERFACE_DATA_RATE = "BD";
const char *const at_command::RESTORE_DEFAULTS = "RE";
const char *const at_command::WRITE = "WR";

const char *const at_command::REGISTER_QUERY = "";
const char at_command::CR = '\r';

at_command::at_command(const std::string &command, const std::string &parameter)
    : command(command), parameter(parameter)
{
}

at_command::operator std::string() const
{
    std::ostringstream oss;
    oss << AT_PREFIX << command << parameter << CR;

    return oss.str();
}
