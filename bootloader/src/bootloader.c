#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "core/uart.h"
#include "core/system.h"
#include "util/simple-timer.h"
#include "comms.h"
#include "bl-flash.h"

#define BOOTLOADER_SIZE         (0x8000U)
#define MAIN_APP_START_ADDRESS  (FLASH_BASE + BOOTLOADER_SIZE)
#define MAX_FW_LENGTH           ((1024U * 512U) - BOOTLOADER_SIZE)

#define USART2_PORT         (GPIOA)
#define USART2_CTS_PIN      (GPIO0)
#define USART2_RTS_PIN      (GPIO1)
#define USART2_TX_PIN       (GPIO2)
#define USART2_RX_PIN       (GPIO3)
#define USART2_CK_PIN       (GPIO4)

#define DEVICE_ID           (0x42)

#define SYNC_SEQ_0          (0xc4)
#define SYNC_SEQ_1          (0x55)
#define SYNC_SEQ_2          (0x7e)
#define SYNC_SEQ_3          (0x10)

#define DEFAULT_TIMEOUT     (5000)

typedef enum bl_state_t
{
    bl_state_sync,
    bl_state_wait_for_update_req,
    bl_state_device_id_req,
    bl_state_device_id_res,
    bl_state_fw_length_req,
    bl_state_fw_length_res,
    bl_state_erase_application,
    bl_state_receive_fw,
    bl_state_done
} bl_state_t;

typedef struct bl_state_info_t
{
    bl_state_t state;
    uint32_t fw_length;
    uint32_t bytes_written;
    uint8_t sync_seq[4];

} bl_state_info_t;

static bl_state_info_t state_info = {
    .state = bl_state_sync,
    .fw_length = 0,
    .bytes_written = 0,
    .sync_seq = {0},
};

static void gpio_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(USART2_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, (USART2_TX_PIN | USART2_RX_PIN));
    gpio_set_af(USART2_PORT, GPIO_AF7, (USART2_TX_PIN | USART2_RX_PIN));
}

static void gpio_teardown(void)
{
    gpio_mode_setup(USART2_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, (USART2_TX_PIN | USART2_RX_PIN));
    rcc_periph_clock_disable(RCC_GPIOA);
}

static void jump_to_main(void)
{
    vector_table_t* main_vector_table = (vector_table_t*)(MAIN_APP_START_ADDRESS);
    main_vector_table->reset();
}

static void bootloading_fail(comms_packet_t* packet)
{
    comms_create_single_byte_packet(packet, BL_PACKET_NACK_DATA0);
    comms_send(packet); 
    state_info.state = bl_state_done;   
}

static void check_for_timeout(simple_timer_t* timer, comms_packet_t* packet)
{
    if (simple_timer_has_elapsed(timer))
    {
        bootloading_fail(packet);
    }
}

static bool is_device_id_packet(const comms_packet_t* packet)
{
    if (packet->length != 2) 
    {
        return false;
    }

    if (packet->data[0] != BL_PACKET_DEVICE_ID_RES_DATA0)
    {
        return false;
    }

    for (uint8_t i = 2; i < PACKET_DATA_BYTES; i++)
    {
        if (packet->data[i] != 0xff)
        {
            return false;
        }
    }

    return true;  
}

static bool is_fw_length_packet(const comms_packet_t* packet)
{
    if (packet->length != 5) 
    {
        return false;
    }

    if (packet->data[0] != BL_PACKET_FW_LENGTH_RES_DATA0)
    {
        return false;
    }

    for (uint8_t i = 5; i < PACKET_DATA_BYTES; i++)
    {
        if (packet->data[i] != 0xff)
        {
            return false;
        }
    }

    return true;  
}


int main(void)
{
    system_setup();
    gpio_setup();
    uart_setup();
    comms_setup();

    simple_timer_t timer;
    comms_packet_t packet;

    simple_timer_setup(&timer, DEFAULT_TIMEOUT, false);

    while (state_info.state != bl_state_done)
    {

        if (state_info.state == bl_state_sync)
        {
            if (!uart_data_available())
            {
                check_for_timeout(&timer, &packet);
                continue;
            }

            state_info.sync_seq[0] = state_info.sync_seq[1];
            state_info.sync_seq[1] = state_info.sync_seq[2];
            state_info.sync_seq[2] = state_info.sync_seq[3];
            state_info.sync_seq[3] = uart_read_byte();

            bool is_match = state_info.sync_seq[0] == SYNC_SEQ_0 &&
                            state_info.sync_seq[1] == SYNC_SEQ_1 &&
                            state_info.sync_seq[2] == SYNC_SEQ_2 &&
                            state_info.sync_seq[3] == SYNC_SEQ_3;

            if (is_match)
            {
                comms_create_single_byte_packet(&packet, BL_PACKET_SYNC_OBSERVED_DATA0);
                comms_send(&packet);

                simple_timer_reset(&timer);

                state_info.state = bl_state_wait_for_update_req;
            } 
            else 
            {
                check_for_timeout(&timer, &packet);
            }

            continue;
        }

        comms_update();

        switch(state_info.state)
        {
            case bl_state_wait_for_update_req:
            {
                if (comms_packets_available())
                {
                    comms_receive(&packet);

                    if (comms_is_single_byte_packet(&packet, BL_PACKET_FW_UPDATE_REQ_DATA0))
                    {
                        comms_create_single_byte_packet(&packet, BL_PACKET_FW_UPDATE_RES_DATA0);
                        comms_send(&packet);

                        simple_timer_reset(&timer);

                        state_info.state = bl_state_device_id_req;
                    }
                    else
                    {
                        bootloading_fail(&packet);
                    }
                }
                else
                {
                    check_for_timeout(&timer, &packet);
                }
            } break;
            case bl_state_device_id_req:
            {
                comms_create_single_byte_packet(&packet, BL_PACKET_DEVICE_ID_REQ_DATA0);
                comms_send(&packet);

                simple_timer_reset(&timer);

                state_info.state = bl_state_device_id_res;                
            } break;
            case bl_state_device_id_res:
            {
                if (comms_packets_available())
                {
                    comms_receive(&packet);

                    if (is_device_id_packet(&packet) && (packet.data[1] == DEVICE_ID))
                    {
                        simple_timer_reset(&timer);

                        state_info.state = bl_state_fw_length_req;
                    }
                    else
                    {
                        bootloading_fail(&packet);
                    }
                }
                else
                {
                    check_for_timeout(&timer, &packet);
                }
            } break;
            case bl_state_fw_length_req:
            {
                comms_create_single_byte_packet(&packet, BL_PACKET_FW_LENGTH_REQ_DATA0);
                comms_send(&packet);

                simple_timer_reset(&timer);

                state_info.state = bl_state_fw_length_res;  
            } break;
            case bl_state_fw_length_res:
            {
                if (comms_packets_available())
                {
                    comms_receive(&packet);

                    state_info.fw_length = (
                        (packet.data[1])        |
                        (packet.data[2] << 8)   |                        
                        (packet.data[3] << 16)  |
                        (packet.data[4] << 24)
                    );

                    if (is_fw_length_packet(&packet) && (state_info.fw_length <= MAX_FW_LENGTH))
                    {
                        state_info.state = bl_state_erase_application;
                    }
                    else
                    {
                        bootloading_fail(&packet);
                    }
                }
                else
                {
                    check_for_timeout(&timer, &packet);
                }
            } break;
            case bl_state_erase_application:
            {
                bl_flash_erase_main_application();
                simple_timer_reset(&timer);

                state_info.state = bl_state_receive_fw;
            } break;
            case bl_state_receive_fw:
            {
                if (comms_packets_available())
                {
                    comms_receive(&packet);

                    const uint8_t packet_length = (packet.length & 0x0f) + 1; 

                    bl_flash_write(MAIN_APP_START_ADDRESS + state_info.bytes_written, packet.data, packet_length);

                    state_info.bytes_written += packet_length;

                    simple_timer_reset(&timer);

                    if (state_info.bytes_written >= state_info.fw_length)
                    {
                        comms_create_single_byte_packet(&packet, BL_PACKET_UPDATE_SUCCESSFUL_DATA0);
                        comms_send(&packet);

                        state_info.state = bl_state_done;
                    }
                }
                else
                {
                    check_for_timeout(&timer, &packet);
                }
            } break;
            default:
            {
                state_info.state = bl_state_sync;
            } break;
        }
    }

    system_delay(150);
    uart_teardown();
    gpio_teardown();
    system_teardown();

    jump_to_main();

    // Never return
    return 0;
}