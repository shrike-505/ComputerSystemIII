#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

char getchar() {
    char c;
    read(STDIN_FILENO, &c, 1);
    return c;
}

uint64_t getcharn(char *buf, uint64_t count) {
    uint64_t ret = 0;
    char c;
    while (ret < count) {
        c = getchar();       
        if (c == '\n') {
            break;
        }
        buf[ret++] = c;
    }
    buf[ret] = '\0';
    return ret;
}