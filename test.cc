#include <cstdlib>
#include <iostream>

#include "frame_processor.h"

int main(int argc, char *argv[])
{
    try
    {
        frame_processor processor;

        if (!processor.try_initialize_hardware())
        {
            return EXIT_FAILURE;
        }

        processor.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "main: caught exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
