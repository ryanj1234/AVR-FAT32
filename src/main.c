#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include "uart.h"
#include <string.h>
#include "f32.h"

int main(void)
{
    uart_init(9600);

    printf("uart initialized\n");

    f32_sector sec;
    printf("mounting...\n");
    if(f32_mount(&sec)) {
        printf("Error mounting filesystem\n");
        while(1) {}
    }

    printf("Filesystem mounted\n");

    f32_file * fd = f32_open("HARG.TXT", "a");
    f32_ls(0x2);

    if(fd == NULL) {
        printf("Error opening file!\n");
    // } else {
    //     uint16_t rd;
    //     while((rd = f32_read(fd)) != F32_EOF) {
    //         if(rd == 0) {
    //             printf("No bytes read!\n");
    //             break;
    //         }
    //         for(uint16_t i = 0; i < rd; i++) {
    //             printf("%c", sec.data[i]);
    //         }
    //         printf("\n");
    //     }

    //     f32_close(fd);
    // }
    } else {
        // char tmp[] = "Hello, world!\n";
        const uint8_t tmp1[] = {'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!', '\n'};
        const uint8_t tmp2[] = {'E', 'a', 't', ' ', 'm', 'y', ' ', 'b', 'u', 't', 't', '!', '\n'};

        for(uint16_t i = 0; i < 2000; i++) {
            printf("%u\n", i);
            if(f32_write(fd, tmp1, sizeof(tmp1))) {
                printf("Error writing to file!\n");
                while(1) {}
            }

            if(f32_write(fd, tmp2, sizeof(tmp2))) {
                printf("Error writing to file!\n");
                while(1) {}
            }
        }

        // uint16_t rd;
        // f32_seek(fd, 0);
        // while((rd = f32_read(fd)) != F32_EOF) {
        //     for(uint16_t i = 0; i < rd; i++) {
        //         printf("%c", sec.data[i]);
        //     }
        //     printf("\n");
        // }

        f32_ls(0x2);

        f32_close(fd);
    }

    while(1) {}
}
