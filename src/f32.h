#ifndef __F32_H__
#define __F32_H__

#ifdef DESKTOP
#include <stdint.h>
#else
#include <avr/io.h>
#endif

#ifndef F32_NO_RTC
#define F32_NO_RTC      0
#endif

#define SEC_SIZE        512
#define F32_READ_ONLY   0

#define F32_EOF         0xFFFF

/**
 * Basic struct describing a FAT32 sector
 */
typedef struct f32_sector {
    uint8_t data[SEC_SIZE];
} f32_sector;

/**
 * FAT32 file info
 */
typedef struct {
    uint32_t start_cluster; /* first data cluster in file */
    uint32_t current_cluster; /* current data cluster */
    uint16_t sector_count;
    uint32_t size; /* size of the file in bytes */
    uint32_t file_offset;
    uint32_t file_entry_sector;
    uint16_t file_entry_offset;
} f32_file;

uint8_t f32_mount(f32_sector * tmp);
f32_file * f32_open(const char * __restrict__ fname, const char * __restrict__ modes);
uint8_t f32_close(f32_file * fd);
uint16_t f32_read(f32_file * fd);
uint8_t f32_umount(void);
uint8_t f32_seek(f32_file * fd, uint32_t offset);
uint8_t f32_write_sec(f32_file * fd);

void f32_ls(uint32_t dir_cluster);
uint8_t read_sector(uint32_t addr, f32_sector * buf);
uint8_t write_sector(uint32_t addr, const f32_sector * buf);
uint8_t f32_write(f32_file * fd, const uint8_t * data, uint16_t num_bytes);

#endif