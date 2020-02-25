#include <avr/io.h>
#include "ds3231_access.h"
#include "i2cmaster.h"

#define DS3231_I2C_ADDR     0xD0

/** ds3231 i2c access functions */
inline void ds3231_periph_init(void)
{
    i2c_init();
}

uint8_t ds3231_read_register(uint8_t reg, uint8_t *data)
{
    if(i2c_start(DS3231_I2C_ADDR+I2C_WRITE)) {
        return DS3231_READ_FAIL;
    }
    
    if(i2c_write(reg)) {
        return DS3231_WRITE_FAIL;
    }

    if(i2c_start(DS3231_I2C_ADDR+I2C_READ)) {
        return DS3231_READ_FAIL;
    }

    *data = i2c_readNak();
    
    i2c_stop();

    return DS3231_SUCCESS;
}

uint8_t ds3231_write_register(uint8_t reg, uint8_t data)
{
    if(i2c_start(DS3231_I2C_ADDR+I2C_WRITE)) {
        return DS3231_READ_FAIL;
    }

    if(i2c_write(reg)) {
        return DS3231_WRITE_FAIL;
    }

    if(i2c_write(data)) {
        return DS3231_WRITE_FAIL;
    }

    i2c_stop();

    return DS3231_SUCCESS;
}

uint8_t ds3231_write_bytes(uint8_t reg, uint8_t *data, uint8_t n_bytes)
{
    if(i2c_start(DS3231_I2C_ADDR+I2C_WRITE)) {
        return DS3231_READ_FAIL;
    }

    if(i2c_write(reg)) {
        return DS3231_WRITE_FAIL;
    }

    for(uint8_t i = 0; i < n_bytes; i++)
    {
        if(i2c_write(data[i])) {
            i2c_stop();
            return DS3231_WRITE_FAIL;
        }
    }

    i2c_stop();

    return DS3231_SUCCESS;
}

uint8_t ds3231_read_bytes(uint8_t reg, uint8_t* buf, uint8_t cnt)
{
    if(i2c_start(DS3231_I2C_ADDR+I2C_WRITE)) {
        return DS3231_READ_FAIL;
    }

    if(i2c_write(reg)) {
        return DS3231_WRITE_FAIL;
    }

    if(i2c_start(DS3231_I2C_ADDR+I2C_READ)) {
        return DS3231_READ_FAIL;
    }
    
    uint8_t i = 0;
    for(; i < (cnt-1); i++) { buf[i] = i2c_readAck(); }
    buf[i] = i2c_readNak();

    i2c_stop();

    return DS3231_SUCCESS;
}