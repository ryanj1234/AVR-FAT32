#include "f32_access.h"
#include "f32.h"
#include "sdcard.h"
#include <stdio.h>

#define PRINT_WIDTH     32
static void print_sector(uint32_t addr, const uint8_t * buf) {
    return;
    printf("Sector %lu\n", addr);
    // return;

    printf("    \t");
    for(uint16_t i = 0; i < 512; i++) {
        if((i % PRINT_WIDTH) == 0)
        {
            printf("\n");
        }
        if(buf[i] < 0x10)
            printf("0%X ", buf[i]);
        else
            printf("%X ", buf[i]);
    }
    printf("\n\n");
}

uint8_t io_init() {
    return sd_init();
}

inline uint8_t io_read_block(uint32_t addr, uint8_t * buf) {
    if(sd_read_block(addr, buf)) {
        return 1;
    }

    print_sector(addr, buf);
    return 0;
}

inline uint8_t io_write_block(uint32_t addr, const uint8_t *buf) {
    if(sd_write_block(addr, buf)) {
        return 1;
    }

    print_sector(addr, buf);
    return 0;
}

#ifdef DESKTOP
#include <stdint.h>
#include <stdio.h>

extern FILE * in;

uint8_t sd_init() {
    in = fopen("test_mmc.img", "rb+");

    if(in == NULL) {
        return 1;
    }

    return 0;
}


uint8_t sd_read_block(uint32_t addr, uint8_t * buf) {
    // printf("\n\nReading sector 0x%08X\n", addr);
    if(in == NULL) return 1;

    fseek(in, addr*SEC_SIZE, SEEK_SET);
    if(fread(buf, SEC_SIZE, 1, in) == 1) {
        print_sector(addr, buf);
        return 0;
    }

    return 1;
}

uint8_t sd_write_block(uint32_t addr, const uint8_t *buf) {
    // printf("\n\nWriting sector 0x%08X\n", addr);
    if(in == NULL) return 1;

    fseek(in, addr*SEC_SIZE, SEEK_SET);
    size_t a;
    if((a = fwrite(buf, SEC_SIZE, 1, in)) == 1) {
        print_sector(addr, buf);

        return 0;
    }

    return 1;
}
#endif