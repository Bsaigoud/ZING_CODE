#include "MCP23017.h"



void set_all_gpio(bool state)
{
    for (uint8_t i = 0; i < 16; i++)
    {
        mcp23x17_set_level(&dev, i, state);
        mcp23x17_set_level(&dev1, i, state);
    }
}

void mcp23x17_set_output_mode(void)
{
    for(uint8_t i = 0; i < 16; i++)
    {
        mcp23x17_set_mode(&dev, i, MCP23X17_GPIO_OUTPUT);
        mcp23x17_set_mode(&dev1, i, MCP23X17_GPIO_OUTPUT);
    }
return;
}

void mcp23x17_set_input_mode(void)
{
    for(uint8_t i = 0; i < 16; i++)
    {
        mcp23x17_set_mode(&dev, i, MCP23X17_GPIO_INPUT);
        mcp23x17_set_mode(&dev1, i, MCP23X17_GPIO_INPUT);
    }
return;
}
void io_init(void)
{
    ESP_ERROR_CHECK(i2cdev_init());
    ESP_ERROR_CHECK(mcp23x17_init_desc(&dev, 0x20, 0, 21, 22));
    ESP_ERROR_CHECK(mcp23x17_init_desc(&dev1, 0x24, 0, 21, 22));

    // Setup PORTA PORTB as output
    mcp23x17_set_output_mode();
    // // setup PORTA PORTB as input
    // mcp23x17_set_input_mode();
    
    //set PORTA, PORTB pins high-1 or low-0  -- by default all pins are high
    set_all_gpio(0);      // true - HIGH , false - LOW

    // // set PORTA0 GPIO0 LOW
    // mcp23x17_set_level(&dev, 0, false);
    //set PORTA0 GPIO0 HIGH
    // mcp23x17_set_level(&dev, 0, true);
}
