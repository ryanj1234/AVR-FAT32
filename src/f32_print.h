#ifndef _F32_PRINT_H_
#define _F32_PRINT_H_

#include "f32.h"
#include "f32_file.h"

void f32_print_dir_entry(DIR_Entry * dir);
void f32_print_fname(const char * fname);
void f32_print_dirname(const char * dirname);
void f32_print_date(uint16_t date);
void f32_print_timestamp(uint16_t timestamp);

#endif