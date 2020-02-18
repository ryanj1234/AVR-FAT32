#include "f32_print.h"
#include <stdint.h>
#include <stdio.h>
#include "f32_struct.h"
#include "f32_disk_mgmt.h"

extern f32_sector * buf;
extern f32_sys * fs;

void f32_print_dir_entry(DIR_Entry * dir) {
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


void print_fname(const char * fname) {
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

void print_dirname(const char * dirname) {
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

void print_date(uint16_t date) {
    uint8_t month = (date >> 5) & 0x7;
    uint8_t day = date & 0xF;
    uint16_t year = (date >> 9) + 1980;
    printf("%u/%u/%u", month, day, year);
}

void print_timestamp(uint16_t timestamp) {
    uint8_t hours = (timestamp >> 11);
    uint8_t minutes = (timestamp >> 5) & 0x3F;
    uint8_t seconds = (timestamp & 0x1F) * 2;
    printf("%02u:%02u:%02u\n", hours, minutes, seconds);
}

void f32_print_bpb(BootParameterBlock * sec) {
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

#define PRINT_WIDTH     32
void print_sector(const f32_sector * buf) {
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