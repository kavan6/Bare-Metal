#include "printer.h"

void print_str(const char* str)
{
    while(*str)
    {
        print_char(*str++);
    }
}

void print_char(const char c)
{
    uart_write_byte((uint8_t)c);
}