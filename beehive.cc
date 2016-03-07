#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "frame_processor.h"
#include "xbee_s1.h"

int main(int argc, char *argv[])
{
    bool configure = false;
    bool reset = false;

    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            if (std::string(argv[i]) == "--configure")
            {
                configure = true;
            }
            else if (std::string(argv[i]) == "--reset")
            {
                reset = true;
            }
            else
            {
                std::cerr << "invalid argument: " << argv[i] << std::endl;
                return EXIT_FAILURE;
            }
        }
    }

    try
    {
        if (reset)
        {
            auto baud_attempts = std::vector<uint32_t>{ 9600, 115200 };

            for (auto baud : baud_attempts)
            {
                xbee_s1 xbee(baud);
                if (xbee.reset_firmware_settings())
                {
                    return EXIT_SUCCESS;
                }

                std::cerr << "connection attempt at " << baud << " baud failed" << std::endl;
            }

            return EXIT_FAILURE;
        }

        if (configure)
        {
            xbee_s1 xbee(9600);
            return xbee.configure_firmware_settings() ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        frame_processor processor;
        processor.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "main: caught exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
