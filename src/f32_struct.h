#ifndef __F32_STRUCT_H__
#define __F32_STRUCT_H__

#include <stdint.h>

/**
 * Cluster definitions
 */
#define F32_CLUSTER_FREE        0x0000000
#define F32_CLUSTER_DEFECTIVE   0xFFFFFF7
#define F32_CLUSTER_EOF         0xFFFFFFF
#define F32_CLUSTER_IS_EOF(X)   (X > 0xFFFFFF7)

/** 
 * File attribute definitions 
 */
#define ATTR_READ_ONLY      0x01
#define ATTR_HIDDEN         0x02
#define ATTR_SYSTEM         0x04
#define ATTR_VOLUME_ID      0x08
#define ATTR_DIRECTORY      0x10
#define ATTR_ARCHIVE        0x20
#define ATTR_LONG_NAME      ((ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID))

/**
 * FAT32 file system info
 */
typedef struct {
    uint16_t sec_per_cluster;
    uint16_t fat_start;
    uint16_t data_start_sec;
    uint32_t fat_size; /* size of FAT in sectors */
} __attribute__((packed)) f32_sys;

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

typedef struct {
    uint32_t FSI_LeadSig;
    uint8_t FSI_Reserved1[480];
    uint32_t FSI_StrucSig;
    uint32_t FSI_Free_Count;
    uint32_t FSI_Nxt_Free;
    uint8_t FSI_Reserved2[12];
    uint32_t FSI_TrailSig;
} __attribute__((packed)) FSInfoStruct;

/**
 * File/Directory entry
 */
typedef struct {
    uint8_t DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
    
} __attribute__((packed)) DIR_Entry;

#endif