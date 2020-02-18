#ifndef __F32_DISK_MGMT__
#define __F32_DISK_MGMT__

#include "f32.h"
#include "f32_struct.h"

void f32_update_file(const f32_file * fd);
uint8_t f32_create_file(f32_file * fd, const char fname[], const char ext[], uint32_t dir_sector, uint16_t dir_offset);
uint8_t f32_find_empty_entry(uint32_t dir_cluster, uint32_t * sector_offset, uint16_t * dir_offset);
uint32_t f32_get_next_cluster(uint32_t current_cluster);
uint8_t f32_allocate_cluster(uint32_t current_cluster, uint32_t free_cluster);
uint32_t f32_sector_to_cluster(uint32_t sector);
uint32_t f32_cluster_to_sector(uint32_t cluster);
uint8_t f32_dir_entry_empty(const DIR_Entry * en);
uint32_t f32_allocate_free(void);
uint32_t f32_find_free();

#endif