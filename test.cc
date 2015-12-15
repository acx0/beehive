#include <cstdlib>
#include <iostream>

#include <unistd.h>

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
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
