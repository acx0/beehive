#ifndef FRAME_DATA_H
#define FRAME_DATA_H

#include <cstdint>
#include <mutex>
#include <vector>

// TODO: rename this class to indicate that it's an interface?
// TODO: derived classes should throw exception if payload > 100bytes
// TODO: create typedefs for common fields? i.e. api_identifier_t, frame_id_t, etc ?
//      - would allow refactoring MIN_FRAME_DATA_LENGTH calculation to not rely on private members/storage implementation
class frame_data
{
public:
    // informs xbee not to reply with a status response frame
    static const uint8_t FRAME_ID_DISABLE_RESPONSE_FRAME;
    static const size_t API_IDENTIFIER_OFFSET;

    const uint8_t api_identifier_value;

    enum api_identifier : uint8_t
    {
        tx_request_64 = 0x00,
        at_command = 0x08,
        rx_packet_64 = 0x80,
        at_command_response = 0x88,
        tx_status = 0x89,
    };

    static uint8_t get_next_frame_id();

    virtual operator std::vector<uint8_t>() const = 0;

protected:
    frame_data(uint8_t api_identifier_value);

private:
    static uint8_t next_frame_id;
    static std::mutex frame_id_lock;
};

#endif
