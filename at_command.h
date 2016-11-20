#ifndef AT_COMMAND_H
#define AT_COMMAND_H

#include <sstream>
#include <string>

class at_command
{
public:
    // TODO: convert to std::string?
    static const char *const RESPONSE_SUCCESS;
    static const char *const RESPONSE_ERROR;

    static const char *const AT_PREFIX;
    static const char *const API_ENABLE;
    static const char *const EXIT_COMMAND_MODE;
    static const char *const SOURCE_ADDRESS_16_BIT;
    static const char *const SERIAL_NUMBER_HIGH;
    static const char *const SERIAL_NUMBER_LOW;
    static const char *const SOFTWARE_RESET;
    static const char *const MAC_MODE;
    static const char *const INTERFACE_DATA_RATE;
    static const char *const PERSONAL_AREA_NETWORK_ID;
    static const char *const CHANNEL;
    static const char *const XBEE_RETRIES;
    static const char *const POWER_LEVEL;
    static const char *const CCA_THRESHOLD;
    static const char *const RESTORE_DEFAULTS;
    static const char *const WRITE;

    static const char *const REGISTER_QUERY;
    static const char CR;

    at_command(const std::string &command, const std::string &parameter = REGISTER_QUERY);

    operator std::string() const;

private:
    std::string command;
    std::string parameter;
};

#endif
