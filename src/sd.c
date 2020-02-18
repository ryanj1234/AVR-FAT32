#include "sd.h"
#include "f32_struct.h"
#include "f32.h" // remove
#include <stdint.h>
#include <stdio.h>
#include "f32_print.h"

FILE * in;

uint8_t sd_initialize() {
    in = fopen("test_mmc.img", "rb+");

    if(in == NULL) {
        return 1;
    }

    return 0;
}

uint8_t read_sector(uint32_t addr, f32_sector * buf) {
    // printf("\n\nReading sector 0x%08X\n", addr);
    if(in == NULL) return 1;

    fseek(in, addr*SEC_SIZE, SEEK_SET);
    if(fread(buf->data, SEC_SIZE, 1, in) == 1) {
        buf->address = addr;
        print_sector(buf);
        return 0;
    }

    buf->address = 0;
    return 1;
}

uint8_t write_sector(uint32_t addr, const f32_sector * buf) {
    // printf("\n\nWriting sector 0x%08X\n", addr);
    if(in == NULL) return 1;

    fseek(in, addr*SEC_SIZE, SEEK_SET);
    size_t a;
    if((a = fwrite(buf->data, SEC_SIZE, 1, in)) == 1) {
        print_sector(buf);

        return 0;
    }

    return 1;
}

uint8_t sd_close() {
    return fclose(in);
}