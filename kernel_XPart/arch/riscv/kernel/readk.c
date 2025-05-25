#include <readk.h>

struct sbiret sbi_read(uint64_t num_bytes, void* buf) {
    return sbi_ecall(0x4442434e, 1, num_bytes, (uint64_t)buf, 0, 0, 0, 0);
}


char getchark() {
    char ret;
    while (1) {
        struct sbiret sbi_result = sbi_read(1, ((uint64_t)&ret - PA2VA_OFFSET));
        if (sbi_result.error == 0 && sbi_result.value == 1) {
            break;
        }
    }
    return ret;
}

uint64_t readk(char* buf, uint64_t count) {
    uint64_t ret = 0;
    char c;
    while (ret < count) {
        c = getchark();
        if (c == '\n') {
            break;
        }
        buf[ret++] = c;
    }
    buf[ret] = '\0';
    return ret;
}