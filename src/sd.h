#ifndef __SD_H__
#define __SD_H__

#include <stdint.h>
#include "f32.h"

uint8_t sd_initialize(void);
uint8_t read_sector(uint32_t addr, f32_sector * buf);
uint8_t write_sector(uint32_t addr, const f32_sector * buf);
uint8_t sd_close(void);

#endif