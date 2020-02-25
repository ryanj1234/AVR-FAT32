#include <avr/io.h>
#include "ds3231.h"
#include "ds3231_access.h"
#include "i2cmaster.h"
#include <stdio.h> // remove

/** Register defintions */
#define DS3231_SECONDS_REG              0x00
#define DS3231_MINUTES_REG              0x01
#define DS3231_HOUR_REG                 0x02
#define DS3231_DAY_REG                  0x03
#define DS3231_MONTH_CENT_REG           0x04
#define DS3231_DATE_REG                 0x05
#define DS3231_YEAR_REG                 0x06

/** Alarm 1 */
#define DS3231_ALARM1_SECONDS_REG       0x07
#define DS3231_ALARM1_MINUTES_REG       0x08
#define DS3231_ALARM1_HOUR_REG          0x09
#define DS3231_ALARM1_DAY_DATE_REG      0x0A

/** Alarm 2 */
#define DS3231_ALARM2_MINUTES_REG       0x0B
#define DS3231_ALARM2_HOUR_REG          0x0C
#define DS3231_ALARM2_DAY_DATE_REG      0x0D

#define DS3231_CONTROL_REG              0x0E
#define DS3231_CONTROL_STAT_REG         0x0F
#define DS3231_AGING_OFFSET_REG         0x10
#define DS3231_TEMP_MSB_REG             0x11
#define DS3231_TEMP_LSB_REG             0x12

#define TO_BCD(X)                       ((X/10) << 4) | ((X%10) & 0x0F)
#define NUM2ASCII(X)                    ('0' + X)

/** Time register masks */
#define DS3231_HOUR_10_MASK             0x10
#define DS3231_HOUR_20_MASK             0x20
#define DS3231_HOUR_AM_PM_MASK          0x20
#define DS3231_HOUR_12_24_MASK          0x40

/** Control bit definitions */
#define _A1IE       0
#define _A2IE       1
#define _INTCN      2
#define _RS1        3
#define _RS2        4
#define _CONV       5
#define _BBSQW      6
#define _EOSC       7

/** Control/Status bit definitions */
#define _A1F        0
#define _A2F        1
#define _BSY        2
#define _EN32K      3
#define _OSF        7

#define _12HR_MODE   1
#define _24HR_MODE   0

/** Local module functions */
static uint8_t hour_decode(uint8_t raw);
static void decode_rtc(const uint8_t * buf, RTC * time);
static void encode_rtc(uint8_t * buf, const RTC * time);

// void dump(void)
// {
//     uint8_t buf[16];
//     ds3231_read_bytes(0, buf, 16);
//     for(uint8_t i = 0; i < 16; i++)
//     {
//         printf("%u: 0x%02X\n", i, buf[i]);
//     }
// }

ds3231_dev get_ds3213(void)
{
    ds3231_dev tmp;
    ds3231_periph_init();
    tmp.mode = _24HR_MODE;

    if(ds3231_read_register(DS3231_CONTROL_REG, &tmp.ctrl) || 
        ds3231_read_register(DS3231_CONTROL_STAT_REG, &tmp.ctrl_stat)) 
    { 
        tmp.dev_status = DS3231_STAT_FAIL;
    }
    else
    {
        tmp.dev_status = DS3231_STAT_OK;
    }

    ds3231_enable_interrupts(&tmp);
    return tmp;
}

uint8_t ds3231_gettime(ds3231_dev dev, RTC* rtc)
{
    uint8_t buf[7];
    if(ds3231_read_bytes(0x00, buf, 7) != DS3231_SUCCESS) return 0;

    decode_rtc(buf, rtc);
    
    return 1;
}

uint8_t ds3231_settime(ds3231_dev dev, const RTC* time)
{
    uint8_t buf[7];
    
    encode_rtc(buf, time);
    
    if(ds3231_write_bytes(0, buf, 7) == DS3231_SUCCESS)
        return 1;
    else
        return 0;
}

void ds3231_disable_32k(ds3231_dev* dev)
{
    dev->ctrl_stat &= ~_BV(_EN32K);
    ds3231_write_register(DS3231_CONTROL_STAT_REG, dev->ctrl_stat);
}

void ds3231_enable_32k(ds3231_dev* dev)
{
    dev->ctrl_stat |= _BV(_EN32K);
    ds3231_write_register(DS3231_CONTROL_STAT_REG, dev->ctrl_stat);
}

void ds3231_disable_alarm1(ds3231_dev* dev)
{
    dev->ctrl &= ~_BV(_A1IE);
    ds3231_write_register(DS3231_CONTROL_REG, dev->ctrl);
}

void ds3231_enable_alarm1(ds3231_dev* dev)
{
    dev->ctrl |= _BV(_A1IE);
    dev->ctrl |= _BV(_BBSQW);
    // dev->ctrl &= ~_BV(_BBSQW);
    ds3231_write_register(DS3231_CONTROL_REG, dev->ctrl);
}

void ds3231_clear_alarm1_flag(ds3231_dev *dev)
{
    dev->ctrl_stat &= ~_BV(_A1F);
    ds3231_write_register(DS3231_CONTROL_STAT_REG, dev->ctrl_stat);
}

void ds3231_clear_alarm2_flag(ds3231_dev *dev)
{
    dev->ctrl_stat &= ~_BV(_A2F);
    ds3231_write_register(DS3231_CONTROL_STAT_REG, dev->ctrl_stat);
}

void ds3231_disable_alarm2(ds3231_dev* dev)
{
    dev->ctrl &= ~_BV(_A1IE);
    ds3231_write_register(DS3231_CONTROL_REG, dev->ctrl);
}

void ds3231_enable_alarm2(ds3231_dev* dev)
{
    dev->ctrl |= _BV(_A1IE);
    ds3231_write_register(DS3231_CONTROL_REG, dev->ctrl);
}

void ds3231_disable_interrupts(ds3231_dev* dev)
{
    dev->ctrl &= ~_BV(_INTCN);
    ds3231_write_register(DS3231_CONTROL_REG, dev->ctrl);
}

void ds3231_enable_interrupts(ds3231_dev* dev)
{
    dev->ctrl |= _BV(_INTCN);
    ds3231_write_register(DS3231_CONTROL_REG, dev->ctrl);
}

void ds3231_set_alarm1(ds3231_dev * dev, RTC * time, uint8_t mask)
{
    uint8_t buf[4];
    buf[0] = ((time->sec/10) << 4) | (time->sec % 10);
    buf[1] = ((time->min/10) << 4) | (time->min % 10);
    buf[2] = (time->hour % 10);
    if(time->hour > 19) { buf[2] |= 0x20; }
    else if(time->hour > 9) { buf[2] |= 0x10; }
    buf[3] = time->wday;

    if(!(mask & match_seconds)) { buf[0] |= 0x80; }
    if(!(mask & match_minutes)) { buf[1] |= 0x80; }
    if(!(mask & match_hours)) { buf[2] |= 0x80; }
    if(!(mask & match_day)) { buf[3] |= 0x80; }

    ds3231_write_bytes(DS3231_ALARM1_SECONDS_REG, buf, 4);
}

void ds3231_set_alarm2(ds3231_dev * dev, RTC * time, uint8_t mask)
{
    uint8_t buf[3];
    buf[0] = ((time->min/10) << 4) | (time->min % 10);
    buf[1] = (time->hour % 10);
    if(time->hour > 19) { buf[2] |= 0x20; }
    else if(time->hour > 9) { buf[2] |= 0x10; }
    buf[2] = time->wday;

    if(!(mask & match_minutes)) { buf[0] |= 0x80; }
    if(!(mask & match_hours)) { buf[1] |= 0x80; }
    if(!(mask & match_day)) { buf[2] |= 0x80; }

    ds3231_write_bytes(DS3231_ALARM1_SECONDS_REG, buf, 3);
}

static inline uint8_t hour_decode(uint8_t raw)
{
    uint8_t hour = raw & 0x0F;

    if(raw & 0x10) { hour += 10; }

    if(raw & DS3231_HOUR_12_24_MASK)
    {
        if(raw & 0x20) { hour += 12; }
    }
    else
    {
        if(raw & 0x20) { hour += 20; }
    }

    return hour;
}

static void decode_rtc(const uint8_t * buf, RTC * time)
{
    time->sec = (buf[0] & 0x0F) + 10*(buf[0] >> 4);
    time->min = (buf[1] & 0x0F) + 10*(buf[1] >> 4);
    time->hour = hour_decode(buf[2]);
    time->wday = buf[3];
    time->mday = (buf[4] & 0x0F) + 10*(buf[4] >> 4);
    time->month = (buf[5] & 0x0F) + 10*((buf[5] >> 4) & 0x01);
    time->year = 1900 + (buf[6] & 0x0F) + 10*(buf[6] >> 4);
    if(buf[5] & 0x80) { time->year += 100; }
}

static void encode_rtc(uint8_t * buf, const RTC * time)
{
    buf[0] = ((time->sec/10) << 4) | (time->sec % 10);
    buf[1] = ((time->min/10) << 4) | (time->min % 10);
    buf[2] = (time->hour % 10);
    if(time->hour > 19) { buf[2] |= 0x20; }
    else if(time->hour > 9) { buf[2] |= 0x10; }
    buf[3] = time->wday;
    buf[4] = ((time->mday/10) << 4) | (time->mday % 10);
    buf[5] = ((time->month/10) << 4) | (time->month % 10);
    uint8_t tmp_year;
    if(time->year > 1999)
    {
        buf[5] |= 0x80;
        tmp_year = time->year - 2000;
    }
    else
    {
        tmp_year = time->year - 1900;
    }
    buf[6] = ((tmp_year/10) << 4) | (tmp_year % 10);
}