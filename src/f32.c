#include <stdio.h>
#ifdef DESKTOP
#include <stdint.h>
#else
#include <avr/io.h>
#include <util/delay.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "f32.h"
#include "f32_file.h"
#include "f32_access.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

/**
 * Cluster definitions
 */
#define F32_CLUSTER_FREE        0x0000000
#define F32_CLUSTER_DEFECTIVE   0xFFFFFF7
#define F32_CLUSTER_EOF         0xFFFFFFF
#define F32_CLUSTER_IS_EOF(X)   (X > 0xFFFFFF7)

#define FAT32_ENTRY_SIZE    4 /* size of a FAT32 entry in bytes */

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
 * FAT32 file system info
 */
typedef struct {
    uint16_t sec_per_cluster;
    uint16_t fat_start;
    uint16_t data_start_sec;
    uint32_t fat_size; /* size of FAT in sectors */
} __attribute__((packed)) f32_sys;

typedef struct {
    uint32_t FSI_LeadSig;
    uint8_t FSI_Reserved1[480];
    uint32_t FSI_StrucSig;
    uint32_t FSI_Free_Count;
    uint32_t FSI_Nxt_Free;
    uint8_t FSI_Reserved2[12];
    uint32_t FSI_TrailSig;
} __attribute__((packed)) FSInfoStruct;

FILE * in;
f32_sys * fs;
f32_sector * buf;

/** Module definitions */
static uint32_t f32_get_next_cluster(uint32_t current_cluster);
static uint32_t f32_cluster_to_sector(uint32_t cluster);
static uint32_t f32_sector_to_cluster(uint32_t sector);
static uint8_t f32_find_file(uint32_t dir_cluster, const char * fname, const char * ext, f32_file * fd);
static uint8_t f32_check_file(const char * fname, const char * ext, const DIR_Entry * en);

static uint32_t f32_find_free(void);
static void f32_print_dir_entry(DIR_Entry * dir);
static void print_fname(const char * fname);
static void print_dirname(const char * dirname);
static void print_date(uint16_t date);
static void print_timestamp(uint16_t timestamp);
static void f32_print_bpb(BootParameterBlock * sec);
static uint8_t f32_dir_entry_empty(const DIR_Entry * en);
static uint8_t f32_find_empty_entry(uint32_t dir_cluster, uint32_t * sector_offset, uint16_t * dir_offset);
static uint8_t f32_allocate_cluster(uint32_t current_cluster, uint32_t free_cluster);
static uint32_t f32_count_free(void);

uint8_t f32_mount(f32_sector * sec) {
    if(io_init()) {
        return 1;
    }

    uint32_t boot_sector = 0;
    buf = sec;
    fs = malloc(sizeof(f32_sys));

    /** Read boot parameter block */
    if(io_read_block(boot_sector, buf->data)) {
        return 1;
    }

    if((buf->data[0x1FE] != 0x55) || (buf->data[0x1FF] != 0xAA)) {
        return 1;
    }

    if((buf->data[0] != 0xEB && buf->data[2] != 0x90) && (buf->data[0] != 0xE9)) {
        uint8_t i = 0;
        for(; i < 4; i++) {
            PartitionTable * pt = (PartitionTable*)&buf->data[446 + i*sizeof(PartitionTable)];

            // 0x0C is FAT32
            if(pt->partition_type == 0x0C) {
                boot_sector = pt->start_sector;
                break;
            }
        }

        if(i == 4) return 1;

        if(io_read_block(boot_sector, buf->data)) {
            return 1;
        }

        if((buf->data[0x1FE] != 0x55) || (buf->data[0x1FF] != 0xAA)) {
            return 1;
        }
    }

    BootParameterBlock * bs = (BootParameterBlock*)buf->data;
    fs->sec_per_cluster = bs->BPB_SecPerClus,
    fs->fat_start = boot_sector + bs->BPB_RsvdSecCnt,
    fs->data_start_sec = boot_sector + bs->BPB_RsvdSecCnt + bs->BPB_FATSz32*bs->BPB_NumFATs;
    fs->fat_size = bs->BPB_FATSz32;

    f32_print_bpb(bs);

    if(io_read_block(fs->fat_start, buf->data)) {
        return 1;
    }

    // read fsinfo sector
    // if(io_read_block(boot_sector+1, buf->data)) {
    //     return 1;
    // }

    // uint32_t cluster_free, cluster_count;
    // FSInfoStruct * fsinfo = (FSInfoStruct*)buf->data;
    // if((fsinfo->FSI_LeadSig == 0x41615252) && (fsinfo->FSI_StrucSig == 0x61417272) && (fsinfo->FSI_TrailSig == 0xAA550000)) {
    //     uint32_t fsinfo_free = fsinfo->FSI_Nxt_Free;
    //     uint32_t fsinfo_count = fsinfo->FSI_Free_Count;
    //     cluster_free = f32_find_free();
    //     if(cluster_free != fsinfo_free) {
    //         printf("FREE CLUSTERS DON'T MATCH: 0x%08X vs 0x%08X\n\n", fsinfo_free, cluster_free);
    //     }
    //     cluster_count = f32_count_free();
    //     if(cluster_count != fsinfo_count) {
    //         printf("FREE CLUSTER COUNT DON'T MATCH: %lu vs %lu\n\n", fsinfo_count, cluster_count);
    //     }
    // }

    return 0;
}

uint8_t f32_close(f32_file * fd) {
    if(fd != NULL) {
        free(fd);
        fd = NULL;
    }

    return 0;
}

uint8_t f32_umount() {
    free(fs);
    fclose(in);
    return 0;
}

uint16_t f32_read(f32_file * fd) {
    while(fd->file_offset < fd->size) {
        if(fd->sector_count >= fs->sec_per_cluster) {
            fd->sector_count = 0;
            uint32_t next_cluster = f32_get_next_cluster(fd->current_cluster);
            if(F32_CLUSTER_IS_EOF(next_cluster)) {
                // should not get to this point if file size is correct
                return F32_EOF;
            }
            fd->current_cluster = next_cluster;
        }

        if(io_read_block(f32_cluster_to_sector(fd->current_cluster) + fd->sector_count, buf->data)) {
            return 0;
        }

        uint32_t bytes_read = MIN(fd->size - fd->file_offset, SEC_SIZE);

        fd->file_offset += bytes_read;

        fd->sector_count++;

        return bytes_read;
    }

    return F32_EOF;
}

static void f32_extract_folder(const char * start, const char * end, char * folder) {
    size_t n = (size_t)end - (size_t)start;
    memcpy(folder, start, MIN(11, n));
    if(n < 11) {
        memset(&folder[n], 0x20, 11-n);
    }
}

static void f32_extract_file(const char * start, const char * end, char * dir_name) {
    size_t n = (size_t)end - (size_t)start;
    memcpy(dir_name, start, MIN(n, 8));
    if(n < 8) {
        memset(&dir_name[n], 0x20, 8 - n);
    }
}

static void f32_extract_ext(const char * start, const char * end, char * ext) {
    memcpy(&ext[8], end+1, 3);
}

f32_file * f32_open(
        const char * __restrict__ fname, 
        const char * __restrict__ modes) 
{
    char dir_name[11] = {0};
    const char * pStart;
    const char * pEnd;

    // discard leading slash if provided
    if(fname[0] == '/') {
        pStart = &fname[1];
    } else {
        pStart = &fname[0];
    }

    pEnd = strchr(pStart, '/');

    // allocate space for the potential file
    f32_file * fd = malloc(sizeof(f32_file));

    uint32_t cluster = f32_sector_to_cluster(fs->data_start_sec);
    while(pEnd != NULL) {
        f32_extract_folder(pStart, pEnd, dir_name);

        if(!f32_find_file(cluster, dir_name, &dir_name[8], fd)) {
            free(fd);
            return NULL;
        }

        cluster = fd->start_cluster;
        pStart = pEnd+1;
        pEnd = strchr(pStart, '/');
    }

    // check if filename/extension exists in entry
    pEnd = strchr(pStart, '.');
    if(pEnd == NULL) {
        free(fd);
        return NULL;
    }

    f32_extract_file(pStart, pEnd, dir_name);
    f32_extract_ext(pStart, pEnd, dir_name);

    if(!f32_find_file(cluster, dir_name, &dir_name[8], fd)) {
        if((modes[0] == 'w') || (modes[0] == 'a')) {
            // create file
            uint32_t sector_offset;
            uint16_t dir_offset;
            if(f32_find_empty_entry(cluster, &sector_offset, &dir_offset)) {
                free(fd);
                return NULL;
            }

            if(f32_create_file(fd, dir_name, sector_offset, dir_offset)) {
                free(fd);
                return NULL;
            }

            return fd;

        } else {
            free(fd);
            return NULL;
        }
    }

    if(modes[0] == 'w') {
        fd->size = 0;
    } else if (modes[0] == 'a') {
        if(f32_seek(fd, fd->size)) {
        }
    }

    return fd;
}

static uint8_t f32_find_empty_entry(
        uint32_t dir_cluster,
        uint32_t * sector_offset,
        uint16_t * dir_offset)
{
    uint32_t dir_sec;

    while(!F32_CLUSTER_IS_EOF(dir_cluster)) {
        dir_sec = f32_cluster_to_sector(dir_cluster);

        // iterate through every sector in the cluster
        for(uint32_t sec = 0; sec < fs->sec_per_cluster; sec++) {
            io_read_block(dir_sec + sec, buf->data);

            // iterate through the entries in current sector
            for(uint16_t i = 0; i < SEC_SIZE/sizeof(DIR_Entry); i++) {
                DIR_Entry * en = (DIR_Entry*)&buf->data[i*sizeof(DIR_Entry)];

                // skip any empty entries
                if(f32_dir_entry_empty(en)) {
                    *dir_offset = i*sizeof(DIR_Entry);
                    *sector_offset = dir_sec + sec;
                    return 0;
                }
            }
        }

        dir_cluster = f32_get_next_cluster(dir_cluster);
    }

    return 1;
}

uint8_t f32_write_sec(f32_file * fd) {
    uint32_t curr_sector = f32_cluster_to_sector(fd->current_cluster) + fd->sector_count;

    uint16_t byte_offset = fd->file_offset % SEC_SIZE;
    fd->file_offset -= byte_offset; // reset to beginning of sector

    io_write_block(curr_sector, buf->data);
    fd->file_offset += SEC_SIZE;
    if(fd->file_offset > fd->size) {
        fd->size += SEC_SIZE - byte_offset;

        // update file attributes
        f32_update_file(fd);
    }

    fd->sector_count++;
    if(fd->sector_count >= fs->sec_per_cluster) {
        uint32_t next_cluster = f32_get_next_cluster(fd->current_cluster);
        // printf("Next cluster: 0x%08X\n", next_cluster);
        // fgetc(stdin);
        if(F32_CLUSTER_IS_EOF(next_cluster)) {
            // allocate new cluster
            uint32_t free_cluster = f32_allocate_free();
            if(free_cluster == 0) {
                // no clusters left!
                return 1;
            }

            f32_allocate_cluster(fd->current_cluster, free_cluster);
            next_cluster = free_cluster;
            // f32_allocate_cluster(free_cluster, F32_EOF);
        }

        fd->current_cluster = next_cluster;
        fd->sector_count = 0;
    }

    return 0;
}

uint8_t f32_write(f32_file * fd, const uint8_t * data, uint16_t num_bytes) {
    uint16_t copied_bytes = 0;
    while(copied_bytes < num_bytes) {
        uint16_t byte_offset = fd->file_offset % SEC_SIZE;
        uint16_t chunk = SEC_SIZE - byte_offset; // remaining space in current sector
        uint16_t remains = num_bytes - copied_bytes;

        uint32_t curr_sector = f32_cluster_to_sector(fd->current_cluster) + fd->sector_count;

        if(remains >= chunk) {
            fd->sector_count++; // move on to next sector
        } else {
            chunk = remains;
        }

        if(io_read_block(curr_sector, buf->data)) {
            return 1;
        }

        memcpy(&buf->data[byte_offset], &data[copied_bytes], chunk);

        if(io_write_block(curr_sector, buf->data)) {
            return 1;
        }

        copied_bytes += chunk;
        fd->file_offset += chunk;
        if(fd->file_offset > fd->size) {
            fd->size += chunk;
        }

        if(fd->sector_count >= fs->sec_per_cluster) {
            uint32_t next_cluster = f32_get_next_cluster(fd->current_cluster);
            if(F32_CLUSTER_IS_EOF(next_cluster)) {
                // allocate new cluster
                uint32_t free_cluster = f32_allocate_free();
                if(free_cluster == 0) {
                    // no clusters left!
                    return 1;
                }

                f32_allocate_cluster(fd->current_cluster, free_cluster);
                next_cluster = free_cluster;
            }

            fd->current_cluster = next_cluster;
            fd->sector_count = 0;
        }
    }

    if(f32_update_file(fd)) {
        return 1;
    }

    return 0;
}

uint8_t f32_seek(f32_file * fd, uint32_t offset) {
    if(offset > fd->size) {
        return 1;
    }

    fd->current_cluster = fd->start_cluster;
    fd->sector_count = 0;
    fd->file_offset = 0;
    while(fd->file_offset != offset) {
        // if we are in the correct cluster
        if(offset - fd->file_offset < fs->sec_per_cluster*SEC_SIZE) {
            fd->sector_count = (offset - fd->file_offset)/SEC_SIZE;
            fd->file_offset = offset;
        } else {
            uint32_t next_cluster = f32_get_next_cluster(fd->current_cluster);
            if(F32_CLUSTER_IS_EOF(next_cluster)) {
                fd->current_cluster = fd->start_cluster;
                fd->sector_count = 0;
                fd->file_offset = 0;
                return 1;
            }

            fd->current_cluster = next_cluster;
            fd->file_offset += fs->sec_per_cluster*SEC_SIZE;
        }
    }
    return 0;
}

static uint32_t _f32_find_free(uint8_t allocate) {
    uint32_t cluster = 0;
    for(uint32_t i = 0; i < fs->fat_size; i++) {

        io_read_block(fs->fat_start + i, buf->data);
        for(uint8_t j = 0; j < SEC_SIZE/FAT32_ENTRY_SIZE; j++) {
            if(*(uint32_t*)&buf->data[j*FAT32_ENTRY_SIZE] == F32_CLUSTER_FREE) {
                // printf("Allocating cluster 0x%08X at offset 0x%08X\n", cluster, j*4);
                if(allocate) {
                    *(uint32_t*)&buf->data[j*FAT32_ENTRY_SIZE] |= F32_CLUSTER_EOF;
                    io_write_block(fs->fat_start + i, buf->data);
                }
                // fgetc(stdin);
                return cluster;
            }

            cluster++;
        }
    }

    return 0;
}

inline uint32_t f32_allocate_free() {
    return _f32_find_free(1);
}

static inline uint32_t f32_find_free() {
    return _f32_find_free(0);
}

static uint32_t f32_count_free() {
    uint32_t cluster = 0;
    for(uint32_t i = 0; i < fs->fat_size; i++) {
        io_read_block(fs->fat_start + i, buf->data);
        for(uint8_t j = 0; j < SEC_SIZE/FAT32_ENTRY_SIZE; j++) {
            if(*(uint32_t*)&buf->data[j*FAT32_ENTRY_SIZE] == F32_CLUSTER_FREE) {
                cluster++;
            }
        }
    }

    return cluster;
}

static uint32_t f32_get_next_cluster(uint32_t current_cluster) {
    uint16_t fat_sec = fs->fat_start + current_cluster/128;
    uint16_t fat_entry = (current_cluster*FAT32_ENTRY_SIZE) % 512;
    io_read_block(fat_sec, buf->data);
    return *(uint32_t*)&buf->data[fat_entry] & 0x0FFFFFFF;
}

static inline uint32_t f32_sector_to_cluster(uint32_t sector) {
    return ((sector - fs->data_start_sec)/fs->sec_per_cluster) + 2;
}

static inline uint32_t f32_cluster_to_sector(uint32_t cluster) {
    return ((cluster - 2)*fs->sec_per_cluster) + fs->data_start_sec;
}

static inline uint8_t f32_dir_entry_empty(const DIR_Entry * en) {
    return  (en->DIR_Name[0] == 0xE5) || (en->DIR_Name[0] == 0x00);
}

static uint8_t f32_find_file(uint32_t dir_cluster, const char * fname, const char * ext, f32_file * fd) {
    uint32_t sec_count = 0, dir_sec;

    while(!F32_CLUSTER_IS_EOF(dir_cluster)) {
        dir_sec = f32_cluster_to_sector(dir_cluster);

        // iterate through every sector in the cluster
        for(uint32_t sec = 0; sec < fs->sec_per_cluster; sec++) {
            io_read_block(dir_sec + sec, buf->data);

            // iterate through the entries in current sector
            for(uint16_t i = 0; i < SEC_SIZE/sizeof(DIR_Entry); i++) {
                DIR_Entry * en = (DIR_Entry*)&buf->data[i*sizeof(DIR_Entry)];

                // skip any empty entries
                if(en->DIR_Name[0] == 0xE5) {
                    continue;
                }

                // all following entries are empty
                if(en->DIR_Name[0] == 0x00) {
                    return 0;
                }

                // check if file matches and return if so
                if(f32_check_file(fname, ext, en)) {
                    fd->start_cluster = ((uint32_t)en->DIR_FstClusHI << 16) | (en->DIR_FstClusLO);
                    fd->size = en->DIR_FileSize;
                    fd->current_cluster = fd->start_cluster;
                    fd->file_offset = 0;
                    fd->sector_count = 0;
                    fd->file_entry_sector = dir_sec;
                    fd->file_entry_offset = i*sizeof(DIR_Entry);
                    return 1;
                }
            }
        }

        dir_cluster = f32_get_next_cluster(dir_cluster);
    }

    return 0;
}

static uint8_t f32_check_file(const char * fname, const char * ext, const DIR_Entry * en) {
    for(int i = 0; i < 8; i++) {
        // printf("0x%02X : 0x%02X\n", fname[i], en->DIR_Name[i]);
        if(fname[i] != en->DIR_Name[i]) return 0;
    }

    for(int i = 8; i < 11; i++) {
        // printf("0x%02X : 0x%02X\n", ext[i-8], en->DIR_Name[i]);
        if(ext[i - 8] != en->DIR_Name[i]) return 0;
    }

    return 1;
}

static uint8_t f32_allocate_cluster(uint32_t current_cluster, uint32_t free_cluster) {
    uint32_t sec = fs->fat_start + (FAT32_ENTRY_SIZE*current_cluster)/SEC_SIZE;
    if(io_read_block(sec, buf->data)) {
        return 1;
    }

    uint16_t offset = FAT32_ENTRY_SIZE*(current_cluster % 128);
    *(uint32_t*)&buf->data[offset] = (free_cluster) & 0x0FFFFFFF;
    if(io_write_block(sec, buf->data)) {
        return 1;
    }

    // printf("Pointing cluster 0x%08X to 0x%08X at offset %d\n", current_cluster, free_cluster, offset);
    // fgetc(stdin);

    return 0;
}

void f32_ls(uint32_t dir_cluster) {
    uint32_t sec_count = 0, dir_sec;

    while(!F32_CLUSTER_IS_EOF(dir_cluster)) {
        dir_sec = f32_cluster_to_sector(dir_cluster);

        // iterate through every sector in the cluster
        for(uint32_t sec = 0; sec < fs->sec_per_cluster; sec++) {
            io_read_block(dir_sec + sec, buf->data);

            // iterate through the entries in current sector
            for(uint16_t i = 0; i < SEC_SIZE/sizeof(DIR_Entry); i++) {
                DIR_Entry * en = (DIR_Entry*)&buf->data[i*sizeof(DIR_Entry)];

                // all remaining entries are empty
                if(en->DIR_Name[0] == 0x00) return;

                // skip empty
                if(en->DIR_Name[0] == 0xE5) continue;

                f32_print_dir_entry(en);
            }
        }

        dir_cluster = f32_get_next_cluster(dir_cluster);
    }
}

static void f32_print_dir_entry(DIR_Entry * dir) {
    if(dir->DIR_Attr == 0x0f) {
        return;
    }
    else if(dir->DIR_Attr & ATTR_VOLUME_ID) {
        printf("Volume: %s\n", dir->DIR_Name);
        return;
    }
    else if(dir->DIR_Attr & ATTR_DIRECTORY) {
        print_dirname(dir->DIR_Name);
    }
    else {
        print_fname(dir->DIR_Name);
    }
    printf("\tAttr: 0x%02X\n", dir->DIR_Attr);
    printf("\tCluster: 0x%02X%02X\n", dir->DIR_FstClusHI, dir->DIR_FstClusLO);
    printf("\tSize: %lu (bytes)\n", dir->DIR_FileSize);
    printf("\tCreated: ");
    print_date(dir->DIR_CrtDate);
    printf(" - ");
    print_timestamp(dir->DIR_CrtTime);
    printf("\tLast Access: ");
    print_date(dir->DIR_LstAccDate);
    printf("\n\tLast Modified: ");
    print_date(dir->DIR_WrtDate);
    printf(" - ");
    print_timestamp(dir->DIR_WrtTime);
}


static inline void print_fname(const char * fname) {
    const char * pbegin = fname;
    const char * pend = &fname[7];
    while(*pend == 0x20) {
        pend--;
        if(pend == pbegin)
            return;
    }
    while(pbegin != pend) printf("%c", *pbegin++);
    printf("%c.%c%c%c\n", *pend, fname[8], fname[9], fname[10]);
}

static inline void print_dirname(const char * dirname) {
    const char * pbegin = dirname;
    const char * pend = &dirname[10];
    while(*pend == 0x20)  {
        pend--;
        if(pend == pbegin)
            return;
    }
    while(pbegin != pend) printf("%c", *pbegin++);
    printf("%c/\n", *pend);
}

static inline void print_date(uint16_t date) {
    uint8_t month = (date >> 5) & 0x7;
    uint8_t day = date & 0xF;
    uint16_t year = (date >> 9) + 1980;
    printf("%u/%u/%u", month, day, year);
}

static inline void print_timestamp(uint16_t timestamp) {
    uint8_t hours = (timestamp >> 11);
    uint8_t minutes = (timestamp >> 5) & 0x3F;
    uint8_t seconds = (timestamp & 0x1F) * 2;
    printf("%02u:%02u:%02u\n", hours, minutes, seconds);
}

static void f32_print_bpb(BootParameterBlock * sec) {
    printf("\tSector Size: %u\n", sec->BPB_BytsPerSec);
    printf("\tSectors Per Cluster: %u\n", sec->BPB_SecPerClus);
    printf("\tReserved Sectors: %u\n", sec->BPB_RsvdSecCnt);
    printf("\tNum FAT: %u\n", sec->BPB_NumFATs);
    printf("\tMedia Descriptor: 0x%02X\n", sec->BPB_Media);
    printf("\tFAT Size Sectors: %lu\n", sec->BPB_FATSz32);
    printf("\tRoot Cluser: %lu\n", sec->BPB_RootClus);
    printf("\tVolume label: ");
    for(int i = 0; i < 11; i++) printf("%c", sec->BS_VolLab[i]);
    printf("\n");
}