#include <avr/io.h>
#include "spi.h"

void spi_init() {
    // set MOSI and SCK to output
    DDR_SPI |= (1 << MOSI) | (1 << SCK);

    // enable pull up resistor in MISO
    DDR_SPI |= (1 << MISO);

    // set SPI params
    SPCR = (1 << SPE) | (1 << MSTR);
    SPSR = 0;
}

uint8_t spi_transfer(uint8_t data) {
    // load data into register
    SPDR = data;

    // Wait for transmission complete
    while(!(SPSR & (1 << SPIF)));

    // return SPDR
    return SPDR;
}
