#include <cstdlib>
#include <iostream>

#include "at_command_frame.h"
#include "at_command.h"
#include "uart_frame.h"
#include "xbee_s1.h"

int main(int argc, char *argv[])
{
    try
    {
        xbee_s1 xbee;

        if (!xbee.enable_api_mode())
        {
            return EXIT_FAILURE;
        }

        xbee.write_frame(uart_frame(std::make_shared<at_command_frame>(at_command::API_ENABLE)));
        auto response = xbee.read_frame();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
