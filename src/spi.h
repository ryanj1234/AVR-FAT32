#ifndef __SPI_H__
#define __SPI_H__

#include <avr/io.h>

// pin definitions
#if defined(__AVR_ATmega328P__)
#define DDR_SPI             DDRB
#define PORT_SPI            PORTB
#define CS                  PINB2
#define MOSI                PINB3
#define MISO                PINB4
#define SCK                 PINB5
#elif defined(__AVR_ATmega32U4__)
#define DDR_SPI             DDRB
#define PORT_SPI            PORTB
#define CS                  PINB0
#define MOSI                PINB2
#define MISO                PINB3
#define SCK                 PINB1
#else
#error "No definitions for defined micro"
#endif

// SPI functions
void spi_init(void);
uint8_t spi_transfer(uint8_t data);

#endif
