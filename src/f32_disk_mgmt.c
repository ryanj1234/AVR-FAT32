#include "f32.h"
#include "f32_disk_mgmt.h"
#include "f32_struct.h"
#include <string.h>

extern f32_sector * buf;
extern f32_sys * fs;

void f32_update_file(const f32_file * fd) {
    read_sector(fd->file_entry_sector, buf);
    DIR_Entry * en = (DIR_Entry*)&buf->data[fd->file_entry_offset];
    en->DIR_FileSize = fd->size;
    write_sector(fd->file_entry_sector, buf);
}

uint32_t f32_allocate_free() {
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

uint32_t f32_find_free() {
    uint32_t cluster = 0;
    for(uint32_t i = 0; i < fs->fat_size; i++) {

        read_sector(fs->fat_start + i, buf);
        for(uint8_t j = 0; j < SEC_SIZE/4; j++) {
            if(*(uint32_t*)&buf->data[j*4] == F32_CLUSTER_FREE) {
                return cluster;
            }

            cluster++;
        }
    }

    return 0;
}

uint8_t f32_find_empty_entry(
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

uint8_t f32_create_file(
        f32_file * fd,
        const char fname[],
        const char ext[],
        uint32_t dir_sector,
        uint16_t dir_offset)
{
    uint32_t free_cluster = f32_allocate_free();
    if(free_cluster == 0) { // no free clusters left! :(
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
        return 1;
    }

    memcpy(&buf->data[dir_offset], &en, sizeof(en));
    if(write_sector(dir_sector, buf) != 0) {
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

uint32_t f32_get_next_cluster(uint32_t current_cluster) {
    uint16_t fat_sec = fs->fat_start + current_cluster/128;
    uint16_t fat_entry = (current_cluster*4) % 512;
    read_sector(fat_sec, buf);
    return *(uint32_t*)&buf->data[fat_entry] & 0x0FFFFFFF;
}

uint8_t f32_allocate_cluster(uint32_t current_cluster, uint32_t free_cluster) {
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

inline uint32_t f32_sector_to_cluster(uint32_t sector) {
    return ((sector - fs->data_start_sec)/fs->sec_per_cluster) + 2;
}

inline uint32_t f32_cluster_to_sector(uint32_t cluster) {
    return ((cluster - 2)*fs->sec_per_cluster) + fs->data_start_sec;
}

uint8_t f32_dir_entry_empty(const DIR_Entry * en) {
    return  (en->DIR_Name[0] == 0xE5) || (en->DIR_Name[0] == 0x00);
}