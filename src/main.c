#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "f32.h"

int main(void)
{
    f32_sector sec;
    f32_mount(&sec);

    f32_file * fd = f32_open("/MYDIR~1/SECRETS/ASDF.TXT", "w");
    // f32_ls(0x14e);
    
    if(fd == NULL) {
        printf("File not found!\n");
    } else {
        FILE * fi = fopen("hamlet.txt", "r");

        if(fi == NULL) {
            printf("HAMLET not found!\n");
            return 1;
        }

        uint8_t buf[512];
        while(fread(buf, 512, 1, fi) == 1) {
            memcpy(sec.data, buf, 512);
            if(f32_write_sec(fd) != 0) {
                printf("Error writing sector!\n");
                break;
            }
        }

        fclose(fi);
        uint16_t rd;
        f32_seek(fd, 0);
        while((rd = f32_read(fd)) != F32_EOF) {
            for(uint16_t i = 0; i < rd; i++) {
                printf("%c", sec.data[i]);
            }
            printf("\n");
        }
        // printf("Bytes read %zu\n", rd);
        // f32_ls(2);
        // f32_ls(0x14e);
        // f32_close(fd);
    }
    
    return 0;
}


// f32_ls(0x02);
//         const char new_text[] = "Hello\n";
//         memcpy(sec.data, new_text, sizeof(new_text));
//         f32_write(fd, sizeof(new_text)-1);
//         fd->current_cluster = fd->start_cluster;
//         fd->file_offset = 0;
//         fd->sector_count = 0;
//         uint16_t rd;
//         while((rd = f32_read(fd)) != F32_EOF) {
//             for(uint16_t i = 0; i < rd; i++) {
//                 printf("%c", sec.data[i]);
//             }
//             printf("\n");
//         }
//         // printf("Bytes read %zu\n", rd);
//         f32_close(fd);
//         f32_umount();