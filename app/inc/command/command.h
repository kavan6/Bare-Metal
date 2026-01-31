#ifndef COMMAND_H
#define COMMAND_H

#include "command/command_handlers.h"
#include "common-defines.h"

typedef void(*command_fn)(void);

typedef struct
{
    const char* name;
    command_fn fn;
} command_entry_t;

static const command_entry_t command_table[] = 
{
    {
        "hello",
        print_hello_world
    }
};

void process_char(const char c);

void process_buffer(const char* buffer);

bool is_enter_char(const char c);

void clear_buffer(void);

void init_buffer(void);

#endif