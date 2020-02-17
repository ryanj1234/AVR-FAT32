#ifndef __F32_H__
#define __F32_H__

#include <stdint.h>

#define F32_NO_RTC      1

#define SEC_SIZE        512
#define F32_READ_ONLY   0

#define F32_EOF         0xFFFF

/**
 * Basic struct describing a FAT32 sector
 */
typedef struct {
    uint32_t address;
    uint8_t data[SEC_SIZE];
} __attribute__((packed)) f32_sector;

/**
 * Partition table. Appears as the very first entry in a formatted sd card
 */
typedef struct {
    uint8_t first_byte;
    uint8_t start_chs[3];
    uint8_t partition_type;
    uint8_t end_chs[3];
    uint32_t start_sector;
    uint32_t length_sectors;
} __attribute__((packed)) PartitionTable;

/**
 * Boot Parameter Block
 */
typedef struct {
    uint8_t BS_jmpBoot[3]; /* jump instructions to boot coode */
    uint8_t BS_OEMName[8]; /* oem name */
    uint16_t BPB_BytsPerSec; /* bytes per sector */
    uint8_t BPB_SecPerClus; /* number of sectors per allocation unit */
    uint16_t BPB_RsvdSecCnt; /* number of reserved sectors in the reserved region */
    uint8_t BPB_NumFATs; /* count of FATs on the volume */
    uint16_t BPB_RootEntCnt; /* For FAT32, this must be set to 0 */
    uint16_t BPB_TotSec16; // if zero, later field is used
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;

    /* FAT32 */
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t BPB_Reserved[12];

    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    char BS_VolLab[11];
    char BS_FilSysType[8];
    char BS_none[420];
    uint16_t Signature_word;
} __attribute__((packed)) BootParameterBlock;


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

#endif