#ifndef UART_FRAME_H
#define UART_FRAME_H

#include <cstdint>
#include <iterator>
#include <memory>
#include <numeric>
#include <vector>

#include "at_command_response_frame.h"
#include "frame_data.h"
#include "logger.h"
#include "rx_packet_64_frame.h"
#include "tx_request_64_frame.h"
#include "tx_status_frame.h"
#include "util.h"

class uart_frame
{
public:
    static const uint8_t FRAME_DELIMITER;
    static const uint8_t ESCAPE;
    static const uint8_t XON;
    static const uint8_t XOFF;
    static const uint8_t XOR_CONST;
    static const uint8_t CHECKSUM_TARGET;
    static const size_t FRAME_DELIMITER_OFFSET;
    static const size_t LENGTH_MSB_OFFSET;
    static const size_t LENGTH_LSB_OFFSET;
    static const size_t API_IDENTIFIER_OFFSET;
    static const size_t IDENTIFIER_DATA_OFFSET;
    static const size_t HEADER_LENGTH;
    static const size_t MIN_FRAME_SIZE;
    static const size_t MAX_FRAME_SIZE;

    static std::shared_ptr<uart_frame> parse_frame(
        std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end);

    uart_frame(std::shared_ptr<frame_data> data);
    uart_frame(
        uint8_t length_msb, uint8_t length_lsb, std::shared_ptr<frame_data> data, uint8_t checksum);

    uint8_t get_api_identifier() const;
    std::shared_ptr<frame_data> get_data();

    static uint8_t compute_checksum(
        std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end);

    operator std::vector<uint8_t>() const;

private:
    // note: length and checksum fields only populated when reading response frames
    uint8_t length_msb;
    uint8_t length_lsb;
    std::shared_ptr<frame_data> data;
    uint8_t checksum;
};

#endif
