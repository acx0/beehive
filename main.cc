#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>

#include "beehive.h"
#include "beehive_config.h"
#include "logger.h"
#include "simulated_communication_endpoint.h"
#include "util.h"
#include "xbee_communication_endpoint.h"
#include "xbee_s1.h"

// TODO: add signal handler for ctrl-c?
int main(int argc, char *argv[])
{
    bool configure = false;
    bool reset = false;
    bool simulate_xbee = false;
    bool simulate_wireless = false;
    bool test_xbee = false;
    bool read_xbee_config = false;
    bool custom_socket_path = false;

    uint32_t packet_loss_percent = 0;
    uint32_t baud = xbee_s1::DEFAULT_BAUD;
    std::string device = xbee_s1::DEFAULT_DEVICE;
    beehive_config config;

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
        else if (std::string(argv[i]) == "--simulate-xbee")
        {
            simulate_xbee = true;
        }
        else if (std::string(argv[i]) == "--simulate-wireless")
        {
            simulate_wireless = true;
        }
        else if (std::string(argv[i]) == "--test-xbee")
        {
            test_xbee = true;
        }
        else if (std::string(argv[i]) == "--read-xbee-config")
        {
            read_xbee_config = true;
        }
        else if (std::string(argv[i]) == "--baud")
        {
            if (i + 1 == argc)
            {
                LOG_ERROR("baud not supplied");
                return EXIT_FAILURE;
            }

            if (!util::try_parse_uint32_t(argv[i + 1], baud))
            {
                LOG_ERROR("failed to parse baud: ", argv[i + 1]);
                return EXIT_FAILURE;
            }

            ++i;
        }
        else if (std::string(argv[i]) == "--device")
        {
            if (i + 1 == argc)
            {
                LOG_ERROR("device not supplied");
                return EXIT_FAILURE;
            }

            device = argv[i + 1];
            std::ifstream serial_device(device.c_str());
            if (!serial_device)
            {
                LOG("serial device not readable");
                return EXIT_FAILURE;
            }

            ++i;
        }
        else if (std::string(argv[i]) == "--socket")
        {
            if (i + 1 == argc)
            {
                LOG_ERROR("socket not supplied");
                return EXIT_FAILURE;
            }

            custom_socket_path = true;
            config = beehive_config(argv[i + 1]);
            ++i;
        }
        else if (std::string(argv[i]) == "--packet-loss")
        {
            if (i + 1 == argc)
            {
                LOG_ERROR("percentage not supplied");
                return EXIT_FAILURE;
            }

            if (!util::try_parse_uint32_t(argv[i + 1], packet_loss_percent))
            {
                LOG_ERROR("failed to parse percentage: ", argv[i + 1]);
                return EXIT_FAILURE;
            }

            if (packet_loss_percent > 100)
            {
                LOG_ERROR("invalid percentage: ", packet_loss_percent);
                return EXIT_FAILURE;
            }

            ++i;
        }
        else
        {
            LOG_ERROR("invalid argument: ", argv[i]);
            return EXIT_FAILURE;
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
     *          - note: RF data rate is not affected by BD parameter, if interface rate is set
     *              higher than the RF data rate, flow control is required
     *              - xbee pro RF data rate: 250000 b/s ~= 30 kB/s
     *
     *  - potentially useful settings:
     *      - ATCA
     *          - CCA threshold
     *      - ATPL
     *          - power level, might be able to incorporate this to save power
     *      - ATAC
     *          - used to explicitly apply changes to module parameters
     *          - module is re-initialized based on changes to values
     *          - in contrast to WR which saves values to non-volatile memory but module still
     *              operates according to previous values until module is rebooted or CN is issued
     */

    try
    {
        if (reset)
        {
            return xbee_s1(device).reset_firmware_settings() ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        else if (configure)
        {
            return xbee_s1(device, baud).configure_firmware_settings()
                ? EXIT_SUCCESS
                : EXIT_FAILURE;
        }
        else if (simulate_wireless)
        {
            return simulated_broadcast_medium(packet_loss_percent).start()
                ? EXIT_SUCCESS
                : EXIT_FAILURE;
        }
        else if (read_xbee_config)
        {
            return xbee_s1(device, baud).read_configuration_registers()
                ? EXIT_SUCCESS
                : EXIT_FAILURE;
        }
        else if (test_xbee)
        {
            // TODO: get rid of this flag? just use --read-xbee-config
            return xbee_s1(device, baud).read_and_set_address() ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        else
        {
            std::shared_ptr<communication_endpoint> endpoint;
            if (simulate_xbee)
            {
                endpoint = std::make_shared<simulated_communication_endpoint>();
            }
            else
            {
                endpoint = std::make_shared<xbee_communication_endpoint>(device, baud);
            }

            // ensure uniqueness of socket paths when testing multiple devices on a single machine
            if (!custom_socket_path)
            {
                config = beehive_config(beehive_config::BEEHIVE_SOCKET_PATH_PREFIX
                    + util::to_hex_string(endpoint->get_address()));
            }

            LOG("address:   ", util::to_hex_string(endpoint->get_address()));
            LOG("server:    ./server_stream.py beehive", util::to_hex_string(endpoint->get_address()));
            LOG("client:    ./client_stream.py beehive", util::to_hex_string(endpoint->get_address()));

            beehive(config, endpoint).run();
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("main: caught exception: ", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
