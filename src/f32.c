#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "f32.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

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

FILE * in;
f32_sys * fs;
f32_sector * buf;

/** Module definitions */
static uint32_t f32_get_next_cluster(uint32_t current_cluster);
static uint32_t f32_cluster_to_sector(uint32_t cluster);
static inline uint32_t f32_sector_to_cluster(uint32_t sector);
static uint8_t f32_find_file(uint32_t dir_cluster, const char * fname, const char * ext, f32_file * fd);
static uint8_t f32_check_file(const char * fname, const char * ext, const DIR_Entry * en);
static void print_sector(const f32_sector * buf);
static uint32_t f32_allocate_free(void);
static void f32_print_dir_entry(DIR_Entry * dir);
static inline void print_fname(const char * fname);
static inline void print_dirname(const char * dirname);
static inline void print_date(uint16_t date);
static inline void print_timestamp(uint16_t timestamp);
static void f32_print_bpb(BootParameterBlock * sec);
static inline uint8_t f32_dir_entry_empty(const DIR_Entry * en);
static uint8_t f32_find_empty_entry(uint32_t dir_cluster, uint32_t * sector_offset, uint16_t * dir_offset);
static uint8_t f32_create_file(f32_file * fd, const char fname[], const char ext[], uint32_t dir_sector, uint16_t dir_offset);
static uint8_t f32_allocate_cluster(uint32_t current_cluster, uint32_t free_cluster);

uint8_t f32_mount(f32_sector * sec) {
    in = fopen("test_mmc.img", "rb+");

    if(in == NULL) {
        printf("Could not open file!\n");
        return 1;
    }

    uint32_t boot_sector = 0;
    buf = sec;
    fs = malloc(sizeof(f32_sys));

    /** Read boot parameter block */
    if(read_sector(boot_sector, buf)) {
        printf("Error reading sector\n");
        return 1;
    }

    if((buf->data[0x1FE] != 0x55) || (buf->data[0x1FF] != 0xAA)) {
        printf("Invalid BPB!\n");
        return 1;
    }

    BootParameterBlock * bs = (BootParameterBlock*)buf->data;
    fs->sec_per_cluster = bs->BPB_SecPerClus,
    fs->fat_start = bs->BPB_RsvdSecCnt,
    fs->data_start_sec = bs->BPB_RsvdSecCnt + bs->BPB_FATSz32*bs->BPB_NumFATs;
    fs->fat_size = bs->BPB_FATSz32;

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
    // printf("Reading sector: 0x%08X\n", f32_cluster_to_sector(fd->current_cluster) + fd->sector_count);
    while(fd->file_offset < fd->size) {
        if(fd->sector_count >= fs->sec_per_cluster) {
            fd->sector_count = 0;
            uint32_t next_cluster = f32_get_next_cluster(fd->current_cluster);
            if(F32_CLUSTER_IS_EOF(next_cluster)) {
                return F32_EOF;
            }
            fd->current_cluster = next_cluster;
        }

        if(read_sector(f32_cluster_to_sector(fd->current_cluster) + fd->sector_count, buf)) {
            return 0;
        }

        uint32_t bytes_read = 0;
        if((fd->size - fd->file_offset) < SEC_SIZE) {
            bytes_read = fd->size - fd->file_offset;
        } else {
            bytes_read = SEC_SIZE;
        }

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

f32_file * f32_open(const char * __restrict__ fname, const char * __restrict__ modes) {
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

    size_t n;
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
        if(modes[0] == 'w') {
            // create file
            uint32_t sector_offset;
            uint16_t dir_offset;
            if(f32_find_empty_entry(cluster, &sector_offset, &dir_offset) != 0) {
                free(fd);
                return NULL;
            }

            if(f32_create_file(fd, dir_name, &dir_name[8], sector_offset, dir_offset) != 0) {
                free(fd);
                return NULL;
            }

            return fd;

        } else {
            free(fd);
            return NULL;
        }
    }

    return fd;
}

static uint8_t f32_find_empty_entry(
        uint32_t dir_cluster,
        uint32_t * sector_offset,
        uint16_t * dir_offset)
{
    uint32_t sec_count = 0, dir_sec;

    while(!F32_CLUSTER_IS_EOF(dir_cluster)) {
        dir_sec = f32_cluster_to_sector(dir_cluster);

        // iterate through every sector in the cluster
        for(uint32_t sec = 0; sec < fs->sec_per_cluster; sec++) {
            read_sector(dir_sec + sec, buf);

            // iterate through the entries in current sector
            for(int i = 0; i < SEC_SIZE/sizeof(DIR_Entry); i++) {
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

static uint8_t f32_create_file(
        f32_file * fd,
        const char fname[],
        const char ext[],
        uint32_t dir_sector,
        uint16_t dir_offset)
{
    uint32_t free_cluster = f32_allocate_free();
    if(free_cluster == 0) { // no free clusters left! :(
        printf("No free sectors!\n");
        return 1;
    }

    DIR_Entry en;
    uint8_t i = 0;
    // copy filename
    for(i = 0; ((i < 8) && (fname[i] != 0)); i++) {
        en.DIR_Name[i] = fname[i] & ~(1 << 5); // make sure capital
    }
    // pad with spaces
    for(; i < 8; i++) {
        en.DIR_Name[i] = 0x20;
    }
    // copy extension
    for(i = 8; ((i < 11) && (ext[i-8] != 0)); i++) {
        en.DIR_Name[i] = ext[i-8] & ~(1 << 5); // make sure capital
    }
    // pad with spaces
    for(; i < 11; i++) {
        en.DIR_Name[i] = 0x20;
    }
    
    en.DIR_FstClusHI = ((uint16_t)free_cluster >> 16);
    en.DIR_FstClusLO = ((uint16_t)free_cluster & 0xFFFF);

    en.DIR_Attr = ATTR_ARCHIVE;
    en.DIR_NTRes = 0;
    en.DIR_FileSize = 0;

    #if F32_NO_RTC
    en.DIR_CrtTimeTenth = 0;
    en.DIR_CrtTime = 0;
    en.DIR_CrtDate = 0;
    en.DIR_LstAccDate = 0;
    #else
    #error "No RTC functions defined!"
    #endif

    en.DIR_WrtTime = en.DIR_CrtTime;
    en.DIR_WrtDate = en.DIR_CrtDate;

    if(read_sector(dir_sector, buf) != 0) {
        printf("Error reading sector!\n");
        return 1;
    }

    memcpy(&buf->data[dir_offset], &en, sizeof(en));
    if(write_sector(dir_sector, buf) != 0) {
        printf("Error writing sector!\n");
        return 1;
    }

    fd->start_cluster = (en.DIR_FstClusHI << 16) | (en.DIR_FstClusLO);
    fd->size = 0;
    fd->current_cluster = fd->start_cluster;
    fd->file_offset = 0;
    fd->sector_count = 0;
    fd->file_entry_sector = dir_sector;
    fd->file_entry_offset = dir_offset;

    return 0;
}

void f32_update_file(const f32_file * fd) {
    read_sector(fd->file_entry_sector, buf);
    DIR_Entry * en = (DIR_Entry*)&buf->data[fd->file_entry_offset];
    en->DIR_FileSize = fd->size;
    write_sector(fd->file_entry_sector, buf);
}

uint8_t f32_write_sec(f32_file * fd) {
    uint32_t curr_sector = f32_cluster_to_sector(fd->current_cluster) + fd->sector_count;

    // printf("Writing sector: 0x%08X\n", curr_sector);
    write_sector(curr_sector, buf);
    fd->size += SEC_SIZE;
    fd->file_offset += SEC_SIZE;

    // update file attributes
    f32_update_file(fd);

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

static uint32_t f32_allocate_free() {
    uint32_t cluster = 0;
    for(uint32_t i = 0; i < fs->fat_size; i++) {

        read_sector(fs->fat_start + i, buf);
        for(uint8_t j = 0; j < SEC_SIZE/4; j++) {
            if(*(uint32_t*)&buf->data[j*4] == F32_CLUSTER_FREE) {
                // printf("Allocating cluster 0x%08X at offset 0x%08X\n", cluster, j*4);
                *(uint32_t*)&buf->data[j*4] |= F32_CLUSTER_EOF;
                write_sector(fs->fat_start + i, buf);
                // fgetc(stdin);
                return cluster;
            }

            cluster++;
        }
    }

    return 0;
}

static uint32_t f32_get_next_cluster(uint32_t current_cluster) {
    uint16_t fat_sec = fs->fat_start + current_cluster/128;
    uint16_t fat_entry = (current_cluster*4) % 512;
    read_sector(fat_sec, buf);
    return *(uint32_t*)&buf->data[fat_entry] & 0x0FFFFFFF;
}

// FIXME
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
            read_sector(dir_sec + sec, buf);

            // iterate through the entries in current sector
            for(int i = 0; i < SEC_SIZE/sizeof(DIR_Entry); i++) {
                DIR_Entry * en = (DIR_Entry*)&buf->data[i*sizeof(DIR_Entry)];

                // skip any empty entries
                if(f32_dir_entry_empty(en)) {
                    continue;
                }

                // check if file matches and return if so
                if(f32_check_file(fname, ext, en)) {
                    fd->start_cluster = (en->DIR_FstClusHI << 16) | (en->DIR_FstClusLO);
                    fd->size = en->DIR_FileSize;
                    fd->current_cluster = fd->start_cluster;
                    fd->file_offset = 0;
                    fd->sector_count = 0;
                    fd->file_entry_sector = buf->address;
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
        if(fname[i] != en->DIR_Name[i]) return 0;
    }

    for(int i = 8; i < 11; i++) {
        if(ext[i - 8] != en->DIR_Name[i]) return 0;
    }

    return 1;
}

static uint8_t f32_allocate_cluster(uint32_t current_cluster, uint32_t free_cluster) {
    uint32_t sec = fs->fat_start + (4*current_cluster)/SEC_SIZE;
    if(read_sector(sec, buf) != 0) {
        return 1;
    }

    uint16_t offset = 4*(current_cluster % 128);
    *(uint32_t*)&buf->data[offset] = (free_cluster) & 0x0FFFFFFF;
    if(write_sector(sec, buf) != 0) {
        return 1;
    }

    // printf("Pointing cluster 0x%08X to 0x%08X at offset %d\n", current_cluster, free_cluster, offset);
    // fgetc(stdin);

    return 0;
}

#define PRINT_WIDTH     32
static void print_sector(const f32_sector * buf) {
    return;
    printf("Sector 0x%08X\n", buf->address);

    printf("    \t");
    for(int i = 0; i < PRINT_WIDTH; i++) {
        printf("%02X ", i);
    }
    for(int i = 0; i < 512; i++) {
        if((i % PRINT_WIDTH) == 0)
        {
            printf("\n%03X\t", i);
        }
        printf("%02X ", buf->data[i]);
    }
    printf("\n\n");
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

void f32_ls(uint32_t dir_cluster) {
    uint32_t sec_count = 0, dir_sec;

    while(!F32_CLUSTER_IS_EOF(dir_cluster)) {
        dir_sec = f32_cluster_to_sector(dir_cluster);

        // iterate through every sector in the cluster
        for(uint32_t sec = 0; sec < fs->sec_per_cluster; sec++) {
            read_sector(dir_sec + sec, buf);

            // iterate through the entries in current sector
            for(int i = 0; i < SEC_SIZE/sizeof(DIR_Entry); i++) {
                DIR_Entry * en = (DIR_Entry*)&buf->data[i*sizeof(DIR_Entry)];

                // skip any empty entries
                if(f32_dir_entry_empty(en))
                {
                    continue;
                }

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
    printf("\tFAT Size Sectors: %u\n", sec->BPB_FATSz32);
    printf("\tRoot Cluser: %u\n", sec->BPB_RootClus);
    printf("\tVolume label: ");
    for(int i = 0; i < 11; i++) printf("%c", sec->BS_VolLab[i]);
    printf("\n");
}