#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "core/system.h"

#define LED_PORT        (GPIOA)
#define LED_PIN_D2      (GPIO6)
#define LED_PIN_D3      (GPIO7)

static void gpio_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN_D2 | LED_PIN_D3);

    gpio_set(LED_PORT, LED_PIN_D3);
    gpio_clear(LED_PORT, LED_PIN_D2);
}

int main(void)
{
    system_setup();
    gpio_setup();

    uint64_t start_time = system_get_ticks();

    bool state = false;

    while (1)
    {
        if (system_get_ticks() - start_time >= 100)
        {
            if (state)
            {
                gpio_set(LED_PORT, LED_PIN_D3);
                gpio_clear(LED_PORT, LED_PIN_D2);
            }
            else
            {
                gpio_set(LED_PORT, LED_PIN_D2);
                gpio_clear(LED_PORT, LED_PIN_D3);
            }

            state = !state;
            start_time = system_get_ticks();
        }
    }

    // Never return
    return 0;
}