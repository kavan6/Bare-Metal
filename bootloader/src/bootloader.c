#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "core/uart.h"
#include "core/system.h"
#include "comms.h"

#define BOOTLOADER_SIZE         (0x8000U)
#define MAIN_APP_START_ADDRESS  (FLASH_BASE + BOOTLOADER_SIZE)

#define USART2_PORT         (GPIOA)
#define USART2_CTS_PIN      (GPIO0)
#define USART2_RTS_PIN      (GPIO1)
#define USART2_TX_PIN       (GPIO2)
#define USART2_RX_PIN       (GPIO3)
#define USART2_CK_PIN       (GPIO4)

static void gpio_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(USART2_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, (USART2_TX_PIN | USART2_RX_PIN));
    gpio_set_af(USART2_PORT, GPIO_AF7, (USART2_TX_PIN | USART2_RX_PIN));
}

static void jump_to_main(void)
{
    vector_table_t* main_vector_table = (vector_table_t*)(MAIN_APP_START_ADDRESS);
    main_vector_table->reset();
}

int main(void)
{
    system_setup();
    gpio_setup();
    uart_setup();
    comms_setup();

    comms_packet_t packet = {
        .length = 9,
        .data = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
        .crc = 0
    };
    packet.crc = comms_compute_crc(&packet);

    while (true)
    {
        comms_update();
        comms_send(&packet);
        system_delay(500);
    }

    // teardown

    jump_to_main();

    // Never return
    return 0;
}