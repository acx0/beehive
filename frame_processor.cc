#include "frame_processor.h"

// TODO: robust recovery
bool frame_processor::try_initialize_hardware()
{
    if (!xbee.enable_api_mode())
    {
        return false;
    }

    if (!xbee.enable_64_bit_addressing())
    {
        return false;
    }

    if (!xbee.read_ieee_source_address())
    {
        return false;
    }

    return true;
}

void frame_processor::run()
{
    while (true)
    {
        auto frame = xbee.read_frame();
        if (frame == nullptr)
        {
            continue;
        }

        if (frame->get_api_identifier() == frame_data::api_identifier::rx_packet_64)
        {
            auto rx_packet = std::static_pointer_cast<rx_packet_64_frame>(frame->get_data());
            message_segment segment(rx_packet->get_rf_data());  // TODO: heap alloc?
            uint64_t destination_address = rx_packet->is_broadcast_frame()
                ? xbee_s1::BROADCAST_ADDRESS
                : xbee.get_address();

            connection_tuple key(rx_packet->get_source_address(), segment.get_source_port(), destination_address, segment.get_destination_port());
            log_segment(key, segment);

            if (segment.get_message_type() == message_segment::type::stream_segment)
            {
                std::shared_ptr<reliable_channel> channel;
                if (channel_map.find(key) == channel_map.end())
                {
                    channel_map[key] = std::make_shared<reliable_channel>();
                }

                channel = channel_map[key];
                channel->write(segment);
            }
        }
    }
}

void frame_processor::test_write_messsage()
{
    auto message = std::vector<uint8_t> { 0xaa, 0xbb, 0xcc, 0xdd, 0xee };
    message_segment segment(69, 80, 0, 0, message_segment::type::stream_segment, message_segment::flag::syn, message);

    xbee.write_frame(uart_frame(std::make_shared<tx_request_64_frame>(xbee_s1::BROADCAST_ADDRESS, segment)));
    xbee.read_frame();
}

void frame_processor::log_segment(const connection_tuple &key, const message_segment &segment)
{
    std::string flags(4, ' ');

    flags[0] = segment.is_ack() ? 'A' : ' ';
    flags[1] = segment.is_rst() ? 'R' : ' ';
    flags[2] = segment.is_syn() ? 'S' : ' ';
    flags[3] = segment.is_fin() ? 'F' : ' ';

    std::clog << "recv  " << key.to_string() << "[" << flags << "] type(" << +segment.get_message_type() << "), msg ["
        << util::get_frame_hex(segment.get_message()) << "] (" << segment.get_message().size() << " bytes)" << std::endl;
}
