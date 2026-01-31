#ifndef COMMS_H
#define COMMS_H

#include "common-defines.h"

#define PACKET_DATA_BYTES   (16)
#define PACKET_LENGTH_BYTES (1)
#define PACKET_CRC_BYTES    (1)
#define PACKET_LENGTH       (PACKET_LENGTH_BYTES + PACKET_DATA_BYTES + PACKET_CRC_BYTES)

#define PACKET_RETX_DATA0   (0x19)
#define PACKET_ACK_DATA0    (0x15)

#define PACKET_BUFFER_SIZE  (8)

typedef struct comms_packet_t
{
    uint8_t length;
    uint8_t data[PACKET_DATA_BYTES];
    uint8_t crc;
} comms_packet_t;

void comms_setup(void);
void comms_update(void);

bool comms_packets_available(void);
void comms_send(comms_packet_t* packet);
void comms_receive(comms_packet_t* packet);

uint8_t comms_compute_crc(comms_packet_t* packet);

#endif