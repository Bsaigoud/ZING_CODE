#ifndef _MCP23017_H
#define _MCP23017_H
#include <mcp23x17.h>

static mcp23x17_t dev = { 0 };
static mcp23x17_t dev1 = { 0 };


void set_all_gpio(bool );
void mcp23x17_set_output_mode(void);
void mcp23x17_set_input_mode(void);
void io_init(void);





#endif