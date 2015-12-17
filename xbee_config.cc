#include "xbee_config.h"

const char *const xbee_config::CONFIG_FILE = "xbee_config";
const char *const xbee_config::PORT_KEY = "port";
const char *const xbee_config::BAUD_KEY = "baud";
const char xbee_config::SEPARATOR = '=';

xbee_config::xbee_config()
{
    std::ifstream config(CONFIG_FILE);
    std::string line;

    while (std::getline(config, line))
    {
        std::istringstream iss(line);
        std::string key, value;

        std::getline(iss, key, SEPARATOR);
        if (key.empty())
        {
            // TODO: create custom exception types, std::invalid_argument missing on odroid?
            // throw std::invalid_argument("missing config key");
            continue;
        }

        std::getline(iss, value, SEPARATOR);
        if (value.empty())
        {
            // throw std::invalid_argument("missing config value");
            continue;
        }

        if (key == PORT_KEY)
        {
            port = value;
        }
        else if (key == BAUD_KEY)
        {
            baud = std::stoul(value);
        }
    }

    std::clog << "using port: " << port << ", baud: " << baud << std::endl;
}
