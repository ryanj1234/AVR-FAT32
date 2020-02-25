#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "sdcard.h"
#include "spi.h"
#include <stdio.h>

#define SD_BLOCK_LEN        512

// command definitions
#define CMD0                0
#define CMD0_ARG            0x00000000
#define CMD0_CRC            0x94
#define CMD8                8
#define CMD8_ARG            0x0000001AA
#define CMD8_CRC            0x86
#define CMD9                9
#define CMD9_ARG            0x00000000
#define CMD9_CRC            0x00
#define CMD10               9
#define CMD10_ARG           0x00000000
#define CMD10_CRC           0x00
#define CMD13               13
#define CMD13_ARG           0x00000000
#define CMD13_CRC           0x00
#define CMD17               17
#define CMD17_CRC           0x00
#define CMD24               24
#define CMD24_CRC           0x00
#define CMD55               55
#define CMD55_ARG           0x00000000
#define CMD55_CRC           0x00
#define CMD58               58
#define CMD58_ARG           0x00000000
#define CMD58_CRC           0x00
#define ACMD41              41
#define ACMD41_ARG          0x40000000
#define ACMD41_CRC          0x00

#define SD_IN_IDLE_STATE    0x01
#define SD_READY            0x00
#define SD_R1_NO_ERROR(X)   X < 0x02

#define R3_BYTES            4
#define R7_BYTES            4

#define CMD0_MAX_ATTEMPTS       255
#define CMD55_MAX_ATTEMPTS      255
#define SD_READ_START_TOKEN     0xFE
#define SD_INIT_CYCLES          80

#define SD_START_TOKEN          0xFE
#define SD_ERROR_TOKEN          0x00

#define SD_DATA_ACCEPTED        0x05
#define SD_DATA_REJECTED_CRC    0x0B
#define SD_DATA_REJECTED_WRITE  0x0D

#define CS_ENABLE()             SD_CS_PORT &= ~(1 << SD_CS_PIN)
#define CS_DISABLE()            SD_CS_PORT |= (1 << SD_CS_PIN)

/** Module specific functions */
static void sd_command(uint8_t cmd, uint32_t arg, uint8_t crc);
static uint8_t sd_read_res1(void);
static void sd_read_res7(uint8_t *res);
static inline void sd_read_bytes(uint8_t *res, uint8_t n);
static uint8_t sd_go_idle(void);
static void sd_send_if_cond(uint8_t *res);
static uint8_t sd_send_app(void);
static uint8_t sd_send_op_cond(void);

uint8_t sd_init() {
    uint8_t res[5], cmdAttempts = 0;

    SD_CS_DDR |= (1 << SD_CS_PIN);
    CS_DISABLE();
    spi_init();

    _delay_ms(10);

    while((res[0] = sd_go_idle()) != SD_IN_IDLE_STATE) {
        if(cmdAttempts++ == CMD0_MAX_ATTEMPTS) {
            return 1;
        }
    }

    _delay_ms(1);

    sd_send_if_cond(res);
    if(res[0] != SD_IN_IDLE_STATE) {
        return 1;
    }

    if(res[4] != 0xAA) {
        return 1;
    }

    cmdAttempts = 0;
    while(cmdAttempts++ <= CMD55_MAX_ATTEMPTS) {
        res[0] = sd_send_app();
        if(SD_R1_NO_ERROR(res[0])) {
            res[0] = sd_send_op_cond();
        }

        if(res[0] == SD_READY) {
            return 0;
        }

        _delay_ms(1);
    }
    
    return 1;
}

#define SD_MAX_READ_ATTEMPTS    20000
// #define SD_MAX_READ_ATTEMPTS    1563

uint8_t sd_read_block(uint32_t addr, uint8_t *buf) {
    uint8_t res1;
    uint16_t readAttempts;

    // set token to none
    uint8_t token = 0xFF;

    // assert chip select
    spi_transfer(0xFF);
    CS_ENABLE();
    spi_transfer(0xFF);

    // send CMD17
    sd_command(CMD17, addr, CMD17_CRC);

    // read R1
    res1 = sd_read_res1();

    // if response received from card
    if(res1 == 0x00) {
        // wait for a response token (timeout = 100ms)
        readAttempts = 0;
        while(++readAttempts != SD_MAX_READ_ATTEMPTS) {
            if((token = spi_transfer(0xFF)) != 0xFF) break;
        }

        // if response token is 0xFE
        if(token == SD_START_TOKEN) {
            // read 512 byte block
            for(uint16_t i = 0; i < SD_BLOCK_LEN; i++) *buf++ = spi_transfer(0xFF);

            // read 16-bit CRC
            spi_transfer(0xFF);
            spi_transfer(0xFF);
        }
    }

    // deassert chip select
    spi_transfer(0xFF);
    CS_DISABLE();
    spi_transfer(0xFF);

    if((res1 == 0) && (token == SD_START_TOKEN)) {
        return 0;
    }

    return 1;
}

#define SD_MAX_WRITE_ATTEMPTS   60000
// #define SD_MAX_WRITE_ATTEMPTS   3907

uint8_t sd_write_block(uint32_t addr, const uint8_t *buf) {
    uint16_t readAttempts;
    uint8_t res1;

    // set token to none
    uint8_t token = 0xFF;

    // assert chip select
    spi_transfer(0xFF);
    CS_ENABLE();
    spi_transfer(0xFF);

    // send CMD24
    sd_command(CMD24, addr, CMD24_CRC);

    // read response
    res1 = sd_read_res1();

    // if no error
    if(res1 == SD_READY) {
        // send start token
        spi_transfer(SD_START_TOKEN);

        // write buffer to card
        for(uint16_t i = 0; i < SD_BLOCK_LEN; i++) token = spi_transfer(buf[i]);

        // wait for a response (timeout = 250ms)
        readAttempts = 0;
        while(token == 0xFF) {
            if(readAttempts++ == SD_MAX_WRITE_ATTEMPTS) {
                break;
            }
            token = spi_transfer(0xFF);
        }

        // if data accepted
        if((token & 0x1F) == 0x05) {
            // wait for write to finish (timeout = 250ms)
            readAttempts = 0;
            while(spi_transfer(0xFF) == 0x00) {
                if(readAttempts++ == SD_MAX_WRITE_ATTEMPTS) { 
                    token = 0x00;
                    printf("Card timed out!\n");
                    break;
                }
            }
        } else {
            printf("Data not accepted!: 0x%02X\n", token);
        }
    } else {
        printf("Card not ready!: 0x%02X\n", res1);
    }

    // deassert chip select
    spi_transfer(0xFF);
    CS_DISABLE();
    spi_transfer(0xFF);

    if((res1 == 0x00) && ((token & 0x1F) == 0x05)) {
        return 0;
    }

    printf("Res1: 0x%02X, token: 0x%02X\n", res1, token);

    return token;
}

void sd_command(uint8_t cmd, uint32_t arg, uint8_t crc) {
    // transmit command to sd card
    spi_transfer(cmd | 0x40);

    // transmit argument
    spi_transfer((uint8_t)(arg >> 24));
    spi_transfer((uint8_t)(arg >> 16));
    spi_transfer((uint8_t)(arg >> 8));
    spi_transfer((uint8_t)(arg));

    // transmit crc
    spi_transfer(crc|0x01);
}

uint8_t sd_read_res1() {
    uint8_t i = 0, res1;

    // keep polling until actual data received
    while((res1 = spi_transfer(0xFF)) & 0x80) {
        // if no data received for 8 bytes, break
        if(i++ > 8) break;
    }

    return res1;
}

void sd_read_res7(uint8_t *res) {
    // read response 1 in R7
    res[0] = sd_read_res1();

    // if error reading R1, return
    if(res[0] > 1) return;

    // read remaining bytes
    sd_read_bytes(res + 1, R7_BYTES);
}

inline void sd_read_bytes(uint8_t *res, uint8_t n) {
    while(n--) *res++ = spi_transfer(0xFF);
}

uint8_t sd_go_idle() {
    // assert chip select
    spi_transfer(0xFF);
    CS_ENABLE();
    spi_transfer(0xFF);

    // send CMD0
    sd_command(CMD0, CMD0_ARG, CMD0_CRC);

    // read response
    uint8_t res1 = sd_read_res1();

    // deassert chip select
    spi_transfer(0xFF);
    CS_DISABLE();
    spi_transfer(0xFF);

    return res1;
}

void sd_send_if_cond(uint8_t *res) {
    // assert chip select
    spi_transfer(0xFF);
    CS_ENABLE();
    spi_transfer(0xFF);

    // send CMD8
    sd_command(CMD8, CMD8_ARG, CMD8_CRC);

    // read response
    sd_read_res7(res);

    // deassert chip select
    spi_transfer(0xFF);
    CS_DISABLE();
    spi_transfer(0xFF);
}

uint8_t sd_send_app() {
    // assert chip select
    spi_transfer(0xFF);
    CS_ENABLE();
    spi_transfer(0xFF);

    // send CMD0
    sd_command(CMD55, CMD55_ARG, CMD55_CRC);

    // read response
    uint8_t res1 = sd_read_res1();

    // deassert chip select
    spi_transfer(0xFF);
    CS_DISABLE();
    spi_transfer(0xFF);

    return res1;
}

uint8_t sd_send_op_cond() {
    // assert chip select
    spi_transfer(0xFF);
    CS_ENABLE();
    spi_transfer(0xFF);

    // send CMD0
    sd_command(ACMD41, ACMD41_ARG, ACMD41_CRC);

    // read response
    uint8_t res1 = sd_read_res1();

    // deassert chip select
    spi_transfer(0xFF);
    CS_DISABLE();
    spi_transfer(0xFF);

    return res1;
}
