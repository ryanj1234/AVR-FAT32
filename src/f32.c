#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "f32.h"
#include "f32_disk_mgmt.h"
#include "f32_struct.h"
#include "sd.h"
#include "f32_print.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

f32_sys * fs;
f32_sector * buf;

/** Module definitions */
static uint8_t f32_find_file(uint32_t dir_cluster, const char * fname, const char * ext, f32_file * fd);
static uint8_t f32_check_file(const char * fname, const char * ext, const DIR_Entry * en);

uint8_t f32_mount(f32_sector * sec) {
    sd_initialize();

    uint32_t boot_sector = 0;
    buf = sec;
    fs = malloc(sizeof(f32_sys));

    /** Read boot parameter block */
    if(read_sector(boot_sector, buf)) {
        return 1;
    }

    if((buf->data[0x1FE] != 0x55) || (buf->data[0x1FF] != 0xAA)) {
        return 1;
    }

    BootParameterBlock * bs = (BootParameterBlock*)buf->data;

    if(bs->BPB_FATSz16 != 0) {
        free(fs);
        return 1;
    }

    fs->sec_per_cluster = bs->BPB_SecPerClus;
    fs->fat_start = bs->BPB_RsvdSecCnt;
    fs->data_start_sec = bs->BPB_RsvdSecCnt + bs->BPB_FATSz32*bs->BPB_NumFATs;
    fs->fat_size = bs->BPB_FATSz32;

    // read fsinfo sector
    if(read_sector(boot_sector+1, buf)) {
        return 1;
    }

    uint32_t cluster_free;
    FSInfoStruct * fsinfo = (FSInfoStruct*)buf->data;
    if((fsinfo->FSI_LeadSig == 0x41615252) && (fsinfo->FSI_StrucSig == 0x61417272) && (fsinfo->FSI_TrailSig == 0xAA550000)) {
        printf("Number of free clusters: 0x%08X\n", fsinfo->FSI_Free_Count);
        printf("Next free cluster: 0x%08X\n", fsinfo->FSI_Nxt_Free);
        uint32_t fsinfo_free = fsinfo->FSI_Nxt_Free;
        cluster_free = f32_find_free();
        if(cluster_free != fsinfo_free) {
            printf("FREE CLUSTERS DON'T MATCH: 0x%08X vs 0x%08X\n\n", fsinfo_free, cluster_free);
        }
    }

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
    return sd_close();
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
