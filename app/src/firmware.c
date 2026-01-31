#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/scb.h>

#include "core/uart.h"
#include "core/system.h"
#include "command/command.h"
#include "timer.h"

#define BOOTLOADER_SIZE     (0x8000U)

#define LED_PORT            (GPIOA)
#define LED_PIN_D2          (GPIO6)
#define LED_PIN_D3          (GPIO7)

#define USART2_PORT         (GPIOA)
#define USART2_CTS_PIN      (GPIO0)
#define USART2_RTS_PIN      (GPIO1)
#define USART2_TX_PIN       (GPIO2)
#define USART2_RX_PIN       (GPIO3)
#define USART2_CK_PIN       (GPIO4)

static void vector_setup(void)
{
    SCB_VTOR = BOOTLOADER_SIZE;
}

static void gpio_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(LED_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, LED_PIN_D2 | LED_PIN_D3);

    gpio_set_af(LED_PORT, GPIO_AF2, (LED_PIN_D2 | LED_PIN_D3));

    gpio_mode_setup(USART2_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, (USART2_TX_PIN | USART2_RX_PIN));

    gpio_set_af(USART2_PORT, GPIO_AF7, (USART2_TX_PIN | USART2_RX_PIN));
}

int main(void)
{
    vector_setup();
    system_setup();
    gpio_setup();
    timer_setup();
    uart_setup();

    uint64_t start_time = system_get_ticks();
    float duty_cycle = 0.0f;

    timer_pwm_set_duty_cycle(duty_cycle);

    while (1)
    {
        if (system_get_ticks() - start_time >= 5)
        {
            duty_cycle += 1.0f;
            if (duty_cycle > 100.0f)
            {
                duty_cycle = 0.0f;
            }
            timer_pwm_set_duty_cycle(duty_cycle);

            start_time = system_get_ticks();
        }

        if (uart_data_available())
        {
            uint8_t data = uart_read_byte();
            process_char((const char)data);
        }

        system_delay(1000);
    }

    // Never return
    return 0;
}