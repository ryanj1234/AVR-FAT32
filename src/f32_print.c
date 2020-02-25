#include "f32_print.h"
#ifdef DESKTOP
  #include <stdint.h>
#else
  #include <avr/io.h>
#endif
#include <stdio.h>

void f32_print_dir_entry(DIR_Entry * dir) {
    if(dir->DIR_Attr == 0x0f) {
        return;
    }
    else if(dir->DIR_Attr & ATTR_VOLUME_ID) {
        printf("Volume: %s\n", dir->DIR_Name);
        return;
    }
    else if(dir->DIR_Attr & ATTR_DIRECTORY) {
        f32_print_dirname(dir->DIR_Name);
    }
    else {
        f32_print_fname(dir->DIR_Name);
    }
    printf("\tAttr: 0x%02X\n", dir->DIR_Attr);
    printf("\tCluster: 0x%02X%02X\n", dir->DIR_FstClusHI, dir->DIR_FstClusLO);
    printf("\tSize: %lu (bytes)\n", dir->DIR_FileSize);
    printf("\tCreated: ");
    f32_print_date(dir->DIR_CrtDate);
    printf(" - ");
    f32_print_timestamp(dir->DIR_CrtTime);
    printf("\tLast Access: ");
    f32_print_date(dir->DIR_LstAccDate);
    printf("\n\tLast Modified: ");
    f32_print_date(dir->DIR_WrtDate);
    printf(" - ");
    f32_print_timestamp(dir->DIR_WrtTime);
}


void f32_print_fname(const char * fname) {
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

void f32_print_dirname(const char * dirname) {
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

void f32_print_date(uint16_t date) {
    uint8_t month = (date >> 5) & 0x7;
    uint8_t day = date & 0xF;
    uint16_t year = (date >> 9) + 1980;
    printf("%u/%u/%u", month, day, year);
}

void f32_print_timestamp(uint16_t timestamp) {
    uint8_t hours = (timestamp >> 11);
    uint8_t minutes = (timestamp >> 5) & 0x3F;
    uint8_t seconds = (timestamp & 0x1F) * 2;
    printf("%02u:%02u:%02u\n", hours, minutes, seconds);
}
