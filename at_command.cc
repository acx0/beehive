#include "at_command.h"

at_command::at_command(const char *command, int parameter)
    : command(command), parameter(parameter)
{
}

at_command::operator std::string() const
{
    std::stringstream ss;
    ss << AT_PREFIX << command;

    if (parameter != QUERY_COMMAND)
    {
        ss << std::hex << parameter;
    }

    ss << CR;
    return ss.str();
}
