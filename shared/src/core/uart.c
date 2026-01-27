#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#include "core/uart.h"

#define USART2_PORT     (GPIOA)
#define USART2_CTS      (GPIO0)
#define USART2_RTS      (GPIO1)
#define USART2_TX       (GPIO2)
#define USART2_RX       (GPIO3)
#define USART2_CK       (GPIO4)

#define DATA_BITS       (8U)
#define PARITY_BITS     (0U)
#define STOP_BITS       (1U)
#define BAUD_RATE       (115200U)


void uart_setup(void)
{
    rcc_periph_clock_enable(RCC_USART2);

    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_set_databits(USART2, DATA_BITS);
    usart_set_baudrate(USART2, BAUD_RATE);
    usart_set_parity(USART2, PARITY_BITS);
    usart_set_stopbits(USART2, STOP_BITS);

    usart_set_mode(USART2,  USART_MODE_TX_RX);
}

void uart_write(uint8_t* data, const uint32_t length)
{

}

void uart_write_byte(uint8_t data)
{

}

uint32_t uart_read(uint8_t* data, const uint32_t length)
{

}

uint8_t uart_read_byte(void)
{

}

bool uart_data_available(void)
{

}