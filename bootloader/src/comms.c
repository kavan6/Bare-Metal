#include <string.h>

#include "comms.h"
#include "core/uart.h"
#include "core/crc8.h"

typedef enum comms_state_t
{
    comms_state_length,
    comms_state_data,
    comms_state_crc,
} comms_state_t;

typedef struct comms_state_info_t
{
    comms_state_t state;
    uint8_t num_data_bytes;
} comms_state_info_t;

static comms_state_info_t state_info = {
    .state = comms_state_length,
    .num_data_bytes = 0
};

static comms_packet_t temp_packet = {
    .length = 0,
    .data = {0},
    .crc = 0
};

static comms_packet_t retx_packet = {
    .length = 0,
    .data = {0},
    .crc = 0
};

static comms_packet_t ack_packet = {
    .length = 0,
    .data = {0},
    .crc = 0
};

static comms_packet_t last_packet = {
    .length = 0,
    .data = {0},
    .crc = 0
};

static comms_packet_t packet_buffer[PACKET_BUFFER_SIZE];
static uint32_t packet_read_index = 0;
static uint32_t packet_write_index = 0;
static uint32_t packet_buffer_mask = PACKET_BUFFER_SIZE - 1;

void comms_create_single_byte_packet(comms_packet_t* packet, uint8_t byte)
{
    memset(packet, 0xff, sizeof(comms_packet_t));
    packet->length = 1;
    packet->data[0] = byte;
    packet->crc = comms_compute_crc(packet);
}

bool comms_is_single_byte_packet(const comms_packet_t* packet, uint8_t byte)
{
    if (packet->length != 1) 
    {
        return false;
    }

    if (packet->data[0] != byte)
    {
        return false;
    }

    for (uint8_t i = 1; i < PACKET_DATA_BYTES; i++)
    {
        if (packet->data[i] != 0xff)
        {
            return false;
        }
    }

    return true;
}

void comms_setup(void)
{
    comms_create_single_byte_packet(&retx_packet, PACKET_RETX_DATA0);
    comms_create_single_byte_packet(&ack_packet, PACKET_ACK_DATA0);
}

void comms_update(void)
{
    while (uart_data_available())
    {
        switch(state_info.state)
        {
            case comms_state_length:
            {
                temp_packet.length = uart_read_byte();
                state_info.state = comms_state_data;
            } break;

            case comms_state_data:
            {
                temp_packet.data[state_info.num_data_bytes++] = uart_read_byte();
                if (state_info.num_data_bytes >= PACKET_DATA_BYTES)
                {
                    state_info.num_data_bytes = 0;
                    state_info.state = comms_state_crc;
                }
            } break;

            case comms_state_crc:
            {
                temp_packet.crc = uart_read_byte();
                uint8_t computed_crc = comms_compute_crc(&temp_packet);

                if (temp_packet.crc != computed_crc)
                {
                    comms_send(&retx_packet);
                    state_info.state = comms_state_length;
                    break;
                }

                if (comms_is_single_byte_packet(&temp_packet, PACKET_RETX_DATA0))
                {
                    comms_send(&last_packet);
                    state_info.state = comms_state_length;
                    break;
                }

                if (comms_is_single_byte_packet(&temp_packet, PACKET_ACK_DATA0))
                {
                    state_info.state = comms_state_length;
                    break;
                }

                uint32_t next_write_index = (packet_write_index + 1) & packet_buffer_mask;
                if (next_write_index == packet_read_index)
                {
                    __asm__("BKPT #0");
                }
                
                memcpy(&packet_buffer[packet_write_index], &temp_packet, sizeof(comms_packet_t));
                packet_write_index = next_write_index;
                
                comms_send(&ack_packet);

                state_info.state = comms_state_length;
            } break;

            default:
            {
                state_info.state = comms_state_length;
            } break;
        }
    }
}

bool comms_packets_available(void)
{
    return !(packet_read_index == packet_write_index);
}

void comms_send(comms_packet_t* packet)
{
    memcpy(&last_packet, packet, sizeof(comms_packet_t));
    uart_write((uint8_t*)packet, PACKET_LENGTH);
}

void comms_receive(comms_packet_t* packet)
{
    memcpy(packet, &packet_buffer[packet_read_index], sizeof(comms_packet_t));
    packet_read_index = (packet_read_index + 1) & packet_buffer_mask;
}

uint8_t comms_compute_crc(comms_packet_t* packet)
{
    return crc8((uint8_t*)packet, PACKET_LENGTH - PACKET_CRC_BYTES);
}