#ifndef AT_COMMAND_H
#define AT_COMMAND_H

#include <sstream>
#include <string>

class at_command
{
public:
    static const char *const RESPONSE_SUCCESS;
    static const char *const RESPONSE_ERROR;

    static const char *const AT_PREFIX;
    static const char *const API_ENABLE;
    static const char *const EXIT_COMMAND_MODE;
    static const char *const SOURCE_ADDRESS_16_BIT;

    static const char *const REGISTER_QUERY;
    static const char CR;

    at_command(const std::string &command, const std::string &parameter = REGISTER_QUERY);

    operator std::string() const;

private:
    std::string command;
    std::string parameter;
};

#endif
