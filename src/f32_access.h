#ifndef _F32_ACCESS_H__
#define _F32_ACCESS_H__

#ifdef DESKTOP
#include <stdint.h>
#else
#include <avr/io.h>
#endif

uint8_t io_init(void);
uint8_t io_read_block(uint32_t addr, uint8_t * buf);
uint8_t io_write_block(uint32_t addr, const uint8_t *buf);

#endif