#include "f32.h"
#include "f32_file.h"
#include "f32_access.h"
#include <string.h>
#include <stdio.h>
#include "rtc.h"

extern f32_sector * buf;
#if !F32_NO_RTC
extern RTC rtc;
#endif

uint8_t f32_create_file(
        f32_file * fd,
        const char fname[],
        uint32_t dir_sector,
        uint16_t dir_offset)
{
    uint32_t free_cluster = f32_allocate_free();
    if(free_cluster == 0) { // no free clusters left! :(
        return 1;
    }

    DIR_Entry en;

    memset(&en, 0, sizeof(en));
    memcpy(en.DIR_Name, fname, 11);

    en.DIR_FstClusHI = ((uint16_t)(free_cluster >> 16));
    en.DIR_FstClusLO = (uint16_t)free_cluster;

    en.DIR_Attr = ATTR_ARCHIVE;
    en.DIR_NTRes = 0;
    en.DIR_FileSize = 0;

    #if F32_NO_RTC
    en.DIR_CrtTimeTenth = 0;
    en.DIR_CrtTime = 0;
    en.DIR_CrtDate = 0;
    en.DIR_LstAccDate = 0;
    #else
    if(rtc.sec & 0x01) {
        en.DIR_CrtTimeTenth = 10;
    }
    en.DIR_CrtTime = (rtc.sec >> 1) & 0x1F;
    en.DIR_CrtTime |= (uint16_t)(rtc.min & 0x3F) << 5;
    en.DIR_CrtTime |= (uint16_t)(rtc.hour & 0x3F) << 11;
    en.DIR_CrtDate = rtc.mday;
    en.DIR_CrtDate |= (uint16_t)(rtc.month & 0x0F) << 5;
    en.DIR_CrtDate |= (uint16_t)((rtc.year - 1980) & 0x7F) << 9;
    #endif

    en.DIR_LstAccDate = en.DIR_CrtDate;
    en.DIR_WrtTime = en.DIR_CrtTime;
    en.DIR_WrtDate = en.DIR_CrtDate;

    if(io_read_block(dir_sector, buf->data)) {
        return 1;
    }

    memcpy(&buf->data[dir_offset], &en, sizeof(en));
    if(io_write_block(dir_sector, buf->data)) {
        return 1;
    }

    fd->start_cluster = ((uint32_t)en.DIR_FstClusHI << 16) | (en.DIR_FstClusLO);
    fd->size = 0;
    fd->current_cluster = fd->start_cluster;
    fd->file_offset = 0;
    fd->sector_count = 0;
    fd->file_entry_sector = dir_sector;
    fd->file_entry_offset = dir_offset;

    return 0;
}

uint8_t f32_update_file(const f32_file * fd) {
    if(io_read_block(fd->file_entry_sector, buf->data)) { return 1; }
    DIR_Entry * en = (DIR_Entry*)&buf->data[fd->file_entry_offset];
    en->DIR_FileSize = fd->size;
    #if !F32_NO_RTC
    en->DIR_WrtTime = (rtc.sec >> 1) & 0x1F;
    en->DIR_WrtTime |= (uint16_t)(rtc.min & 0x3F) << 5;
    en->DIR_WrtTime |= (uint16_t)(rtc.hour & 0x3F) << 11;
    en->DIR_WrtDate = rtc.mday;
    en->DIR_WrtDate |= (uint16_t)(rtc.month & 0x0F) << 5;
    en->DIR_WrtDate |= (uint16_t)((rtc.year - 1980) & 0x7F) << 9;
    #endif
    if(io_write_block(fd->file_entry_sector, buf->data)) { return 1; }
    return 0;
}