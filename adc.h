#ifndef ADC_H
#define ADC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "pico/stdlib.h"
#include "hardware/spi.h"

/* Define GPIO Pins to be part of the ADC SPI pins */
#define ADC_SPI_CS 1
#define ADC_SPI_MOSI 3
#define ADC_SPI_MISO 4
#define ADC_SPI_SCK 6

#define ADC_SPI spi0

extern void init_adc_spi_pins();

extern short read_ADC(uint8_t channel);


#ifdef __cplusplus
}
#endif

#endif