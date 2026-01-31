#include "command.h"

#define CHAR_RETURN             '\r'
#define CHAR_NEWLINE            '\n'
#define CHAR_EMPTY              '\0'

#define BUFFER_SIZE             (64)

#define COMMAND_HELLO_WORLD     "hello world"

#define ARRAY_SIZE(x)           (sizeof(x) / sizeof((x)[0]))

static char buffer[BUFFER_SIZE];
static uint8_t buffer_ptr = 0;

void process_char(const char c)
{
    if (is_enter_char(c))
    {
        print_char('\r');
        print_char('\n');
        if (buffer_ptr > 0)
        {
            process_buffer((const char*)buffer);
        }
        clear_buffer();
    }
    else if (buffer_ptr < BUFFER_SIZE - 1)
    {
        buffer[buffer_ptr++] = c;
        buffer[buffer_ptr] = CHAR_EMPTY;
    }
}

void process_buffer(const char* buf)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(command_table); i++)
    {
        if (strcmp(buf, command_table[i].name) == 0)
        {
            command_table[i].fn();
            return;
        }
    }
}

bool is_enter_char(const char c)
{
    return (c == CHAR_RETURN) || (c == CHAR_NEWLINE);
}

void clear_buffer(void)
{
    buffer_ptr = 0;
    buffer[0] = CHAR_EMPTY;
}

void init_buffer(void)
{
    buffer_ptr = 0;
}