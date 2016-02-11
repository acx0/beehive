#include <cstdlib>
#include <iostream>

#include "frame_processor.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " [read|write]" << std::endl;
        return EXIT_FAILURE;
    }

    std::string op(argv[1]);
    std::cout << "op = " << op << std::endl;

    try
    {
        frame_processor processor;

        if (!processor.try_initialize_hardware())
        {
            return EXIT_FAILURE;
        }

        if (op == "read")
        {
            processor.run();
        }

        if (op == "write")
        {
            std::cout << "press enter to start send loop";
            std::string tmp;
            std::getline(std::cin, tmp);

            while (true)
            {
                processor.test_write_messsage();
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "main: caught exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
