#ifndef __F32_PRINT_H__
#define __F32_PRINT_H__

#include <stdint.h>
#include "f32_struct.h"
#include "f32.h"

void f32_print_dir_entry(DIR_Entry * dir);
void print_fname(const char * fname);
void print_dirname(const char * dirname);
void print_date(uint16_t date);
void print_timestamp(uint16_t timestamp);
void f32_print_bpb(BootParameterBlock * sec);
void print_sector(const f32_sector * buf);
void f32_ls(uint32_t dir_cluster);

#endif