#include "adc.h"

void init_adc_spi_pins()
{
    gpio_init(ADC_SPI_CS);
    gpio_set_dir(ADC_SPI_CS, GPIO_OUT);
    gpio_put(ADC_SPI_CS, 1);

    gpio_set_function(ADC_SPI_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(ADC_SPI_MISO, GPIO_FUNC_SPI);
    gpio_set_function(ADC_SPI_SCK, GPIO_FUNC_SPI);
}

short read_ADC(uint8_t channel)
{

    /* 
        Based on code from Sparkfun: 
        https://learn.sparkfun.com/tutorials/python-programming-tutorial-getting-started-with-the-raspberry-pi/experiment-3-spi-and-analog-input
    */

    /* 
        First bit is always 1
        Second bit is single mode or differental mode
        Third bit is channel select
        Fourth bit is least significant byte or most significant byte?
    */

    uint8_t msg[2] = {0x03,0x00}; /* construct message for MCP3002 */
    uint8_t buffer[2];
    short adc_result = 0;

    if (channel < 0)
        channel = 0;
    else if (channel > 1)
        channel = 1;
    
    msg[0] = ((msg[0] << 1) + channel) << 5;
    
    gpio_put(ADC_SPI_CS, 0);
    spi_write_blocking(ADC_SPI, msg, 2);
    spi_read_blocking(ADC_SPI, 0, buffer, 2);
    adc_result = (short)(buffer[0] << 8 | buffer[1]); // Cast a two byte array into a short int number
    gpio_put(ADC_SPI_CS, 1);

    return adc_result;
}