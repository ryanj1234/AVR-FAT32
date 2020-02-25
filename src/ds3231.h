#ifndef __DS3231_h__
#define __DS3231_h__

#include <stdint.h>
#include "rtc.h"

#define DS3231_STAT_OK      0
#define DS3231_STAT_FAIL    1

typedef struct {
    uint8_t mode;           /* 12/24 hour mode */
    uint8_t ctrl;           /* Control register 0x0E */
    uint8_t ctrl_stat;      /* Control/Status register 0x0F */
    uint8_t dev_status;     /* Device status */
} ds3231_dev;

enum alarm_mask {
    match_seconds = 0x01,
    match_minutes = 0x02,
    match_hours = 0x04,
    match_day = 0x08
};

/**
 * Initialize DS3231 with default settings
 */
extern ds3231_dev get_ds3213(void);

/**
 * Read current time from DS3231
 * 
 * @return  1 if success, else 0
 */
extern uint8_t ds3231_gettime(ds3231_dev dev, RTC* rtc);

/**
 * Set time in DS3231
 * 
 * @return 1 if success, else 0
 */
extern uint8_t ds3231_settime(ds3231_dev dev, const RTC* time);

/**
 * Set the time value for alarm 1 in the DS3231
 * 
 * Mask determines which values in the alarm are compared for a match. If all
 * are zero, match will occur every second. Otherwise, set to 1 to force a match
 * compare for that particular value.
 */
extern void ds3231_set_alarm1(ds3231_dev * dev, RTC * time, uint8_t mask);

/**
 * Set the time value for alarm 2 in the DS3231
 * 
 * Alarm 2 differs from alarm 1 in that it does not have a seconds value.
 * 
 * Mask determines which values in the alarm are compared for a match. If all
 * are zero, match will occur every minute. Otherwise, set to 1 to force a match
 * compare for that particular value.
 */
extern void ds3231_set_alarm2(ds3231_dev * dev, RTC * time, uint8_t mask);

/**
 * Disable the 32Khz oscillator output
 */
extern void ds3231_disable_32k(ds3231_dev*);

/**
 * Enable the 32kHz oscillator output
 */
extern void ds3231_enable_32k(ds3231_dev*);

/**
 * Disable alarm1
 */
extern void ds3231_disable_alarm1(ds3231_dev*);

/**
 * Enable alarm1
 */
extern void ds3231_enable_alarm1(ds3231_dev*);

/**
 * Disable alarm2
 */
extern void ds3231_disable_alarm2(ds3231_dev*);

/**
 * Enable alarm2
 */
extern void ds3231_enable_alarm2(ds3231_dev*);

/**
 * Disable interrupts
 */
extern void ds3231_disable_interrupts(ds3231_dev*);

/**
 * Enable interrupts
 */
extern void ds3231_enable_interrupts(ds3231_dev*);

/**
 * Clear alarm flag. 
 * 
 * This must be done to restore INT pin to its non triggered state
 */
extern void ds3231_clear_alarm1_flag(ds3231_dev *);

/**
 * Clear alarm flag. 
 * 
 * This must be done to restore INT pin to its non triggered state
 */
extern void ds3231_clear_alarm2_flag(ds3231_dev *);

#endif