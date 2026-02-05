#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>

#include "core/uart.h"
#include "core/ring-buffer.h"

#define DATA_BITS        (8)
#define PARITY_BITS      (0)
#define STOP_BITS        (1)
#define BAUD_RATE        (115200)
#define RING_BUFFER_SIZE (128)

static ring_buffer_t rb =   {0U};
static uint8_t data_buffer[RING_BUFFER_SIZE] = {0U};

void usart2_isr(void)
{
    uint32_t usart_flags = USART_SR(USART2);

    if (usart_flags & (USART_FLAG_ORE | USART_FLAG_RXNE))
    {
        if (!ring_buffer_write(&rb, (uint8_t)usart_recv(USART2)))
        {
            // Handle failure?
        }
    }
}

void uart_setup(void)
{
    ring_buffer_setup(&rb, data_buffer, RING_BUFFER_SIZE);

    rcc_periph_clock_enable(RCC_USART2);

    usart_set_mode(USART2,  USART_MODE_TX_RX);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_set_databits(USART2, DATA_BITS);
    usart_set_baudrate(USART2, BAUD_RATE);
    usart_set_parity(USART2, PARITY_BITS);
    usart_set_stopbits(USART2, STOP_BITS);

    usart_enable_rx_interrupt(USART2);
    nvic_enable_irq(NVIC_USART2_IRQ);

    usart_enable(USART2);
}

void uart_teardown(void)
{
    usart_disable_rx_interrupt(USART2);
    usart_disable(USART2);
    nvic_disable_irq(NVIC_USART2_IRQ);
    rcc_periph_clock_disable(RCC_USART2);
}


void uart_write(uint8_t* data, const uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        uart_write_byte(data[i]);
    }
}

void uart_write_byte(uint8_t data)
{
    usart_send_blocking(USART2, (uint16_t)data);
}

uint32_t uart_read(uint8_t* data, const uint32_t length)
{
    if (length == 0)
    {
        return 0;
    }

    for (uint32_t bytes_read = 0; bytes_read < length; bytes_read++)
    {
        if (!ring_buffer_read(&rb, &data[bytes_read]))
        {
            return bytes_read;
        }
    }

    return length;
}

uint8_t uart_read_byte(void)
{
    uint8_t byte = 0;
    (void)uart_read(&byte, 1U);
    return byte;
}

bool uart_data_available(void)
{
    return !ring_buffer_empty(&rb);
}