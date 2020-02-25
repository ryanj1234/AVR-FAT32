#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uart.h"
#include "f32.h"
#include "ds3231.h"
#include "rtc.h"

RTC rtc;

#define ALARM_PERIOD        1 // seconds

#define LED_PIN             PINB0
#define LED_PORT            PORTB
#define LED_DDR             DDRB

void configure_int0(void)
{
    DDRD &= ~(1 << PIND2);
    PORTD |= (1 << PIND2);
    EICRA |= (1 << ISC01); // trigger on rising edge
    EIMSK |= (1 << INT0);  // enable int0 interrupts
}

volatile uint8_t update_time = 0;
ISR(INT0_vect)
{
    update_time = 1;
}

int main(void) {
    RTC atime; // RTC alarm time
    uart_init(57600);
    printf("uart initialized\n");

    ds3231_dev ds3231 = get_ds3213();
    printf("RTC initialized\n");

    LED_DDR |= (1 << LED_PIN);

    f32_sector sec;
    if(f32_mount(&sec)) {
        LED_PORT |= (1 << LED_PIN);
        printf("Error mounting filesystem\n");
        while(1) {}
    }

    ds3231_gettime(ds3231, &rtc);
    f32_file * fd = f32_open("HELLO.TXT", "a");

    if(fd == NULL) {
        LED_PORT |= (1 << LED_PIN);
        printf("Error opening file!\n");
        while(1) {}
    }

    /** alarm time */
    atime.sec = 0;
    ds3231_enable_alarm1(&ds3231);

    configure_int0();
    sei();

    char buf[32];

    while(1) {
        LED_PORT |= (1 << LED_PIN);

        ds3231_gettime(ds3231, &rtc);
        int n = sprintf(buf,
            "[%4u/%02u/%02u %02u:%02u:%02u] Hello\n",
            rtc.year,
            rtc.month,
            rtc.mday,
            rtc.hour,
            rtc.min,
            rtc.sec);
        uart_puts(buf);

        ds3231_clear_alarm1_flag(&ds3231);
        atime.sec = rtc.sec + ALARM_PERIOD;
        if(atime.sec > 59) { atime.sec -= 60; }
        ds3231_set_alarm1(&ds3231, &atime, match_seconds);

        if(f32_write(fd, (uint8_t*)buf, n)) {
            printf("Error writing to file!\n");
        }

        LED_PORT &= ~(1 << LED_PIN);

        while(!update_time) {}
        update_time = 0;
    }
}
