#ifndef _F32_FILE__
#define _F32_FILE__

#ifdef DESKTOP
#include <stdint.h>
#else
#include <avr/io.h>
#endif

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

uint8_t f32_create_file(f32_file * fd, const char fname[], uint32_t dir_sector, uint16_t dir_offset);
uint8_t f32_update_file(const f32_file * fd);
uint32_t f32_allocate_free(void);

#endif