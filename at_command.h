#ifndef AT_COMMAND_H
#define AT_COMMAND_H

#include <sstream>
#include <string>

class at_command
{
public:
    static constexpr const char *RESPONSE_SUCCESS = "OK";
    static constexpr const char *RESPONSE_ERROR = "ERROR";

    static constexpr const char *API_ENABLE = "AP";
    static constexpr const char *EXIT_COMMAND_MODE = "CN";

    static constexpr char CR = '\r';
    static constexpr int QUERY_COMMAND = -1;

    at_command(const char *command, int parameter = QUERY_COMMAND);

    operator std::string() const;

private:
    static constexpr const char *AT_PREFIX = "AT";

    const char *command;
    int parameter;
};

#endif
