#include <cstdlib>
#include <string>
#include <vector>

#include "beehive.h"
#include "xbee_s1.h"

// TODO: add signal handler for ctrl-c?
int main(int argc, char *argv[])
{
    bool configure = false;
    bool reset = false;
    bool run_tests = false;

    if (argc > 1)
    {
        // TODO: flag for mode that does startup test, reads values in API mode and exits
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
            else if (std::string(argv[i]) == "--test")
            {
                run_tests = true;
            }
            else
            {
                LOG_ERROR("invalid argument: ", argv[i]);
                return EXIT_FAILURE;
            }
        }
    }

    /*
     * TODO:
     *  notes:
     *      - seems like PHY/MAC layer does at least 3 retries (can't be disabled?)
     *
     *  - settings to write:
     *      + ATAP : "1"
     *          - enable API mode without escape characters
     *      + ATMY : 0xffff
     *          - enable 64 bit addressing mode
     *      - ATID : 0xf00d (mm.. f00d?)
     *      - ATCH
     *      + ATMM : "1"
     *          - no digi header (needed?) + no acks
     *          - "when MM=1 or 3, MAC and CCA failure retries are not supported"
     *      - ATRR : "0" (default)
     *          - xbee retries
     *      + ATBD : "7" or 0x3d090
     *          - "7" = 115200 baud
     *          - 0x3d090 = 250000 baud
     *              - doesn't work? try 0x38400 = 230400 instead ?
     *              - so far only "7" seems to work
     *          - note: RF data rate is not affected by BD parameter, if interface rate is set higher
     *              than the RF data rate, flow control is required
     *              - xbee pro RF data rate: 250000 b/s ~= 30 kB/s
     *
     *  - potentially useful settings:
     *      - ATCA
     *          - CCA threshold
     *      - ATPL
     *          - power level, might be able to incorporate this to save power
     *      - ATAC
     *          - used to explicitly apply changes to module parameters - module is re-initialized based on changes to values
     *          - in contrast to WR which saves values to non-volatile memory but module still operates according to previous
     *              values until module is rebooted or CN is issued
     */

    try
    {
        if (reset)
        {
            auto baud_attempts = std::vector<uint32_t>{ 9600, 115200 };

            for (auto baud : baud_attempts)
            {
                xbee_s1 xbee(baud);     // TODO: can expose set_baud so we don't have to create new object
                if (xbee.reset_firmware_settings())
                {
                    return EXIT_SUCCESS;
                }

                LOG_ERROR("connection attempt at ", baud, " baud failed");
            }

            return EXIT_FAILURE;
        }

        // TODO: as of now configure only supported after a reset is performed
        if (configure)
        {
            xbee_s1 xbee(9600);
            return xbee.configure_firmware_settings() ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        beehive().run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("main: caught exception: ", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
