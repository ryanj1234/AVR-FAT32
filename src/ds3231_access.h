#ifndef __DS3231_ACCESS_h__
#define __DS3231_ACCESS_h__

#include <avr/io.h>

#define DS3231_SUCCESS      0
#define DS3231_READ_FAIL    1
#define DS3231_WRITE_FAIL   2

extern void ds3231_periph_init(void);
extern uint8_t ds3231_read_register(uint8_t reg, uint8_t *data);
extern uint8_t ds3231_write_register(uint8_t reg, uint8_t data);
extern uint8_t ds3231_write_bytes(uint8_t reg, uint8_t *data, uint8_t n_bytes);
extern uint8_t ds3231_read_bytes(uint8_t reg, uint8_t* buf, uint8_t cnt);

#endif