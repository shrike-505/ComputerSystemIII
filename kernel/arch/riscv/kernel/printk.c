#include <stdio.h>
#include <printk.h>
#include <sbi.h>    

static size_t printk_sbi_write(FILE *restrict fp, const void *restrict buf, size_t len) {
  (void)fp;

  // 调用 SBI 接口输出 buf 中长度为 len 的内容
  // 返回实际输出的字节数
  // Hint：阅读 SBI v2.0 规范！
    char* temp = (char*) buf;
    size_t res = len;
    for(size_t i =0;i<len;i++){
        if(temp[i]==0){
            res = i;
            break;
        }
        sbi_ecall(0x4442434e, 2, temp[i], 0, 0, 0, 0, 0);
    }
    return res;
}

void printk(const char *fmt, ...) {
  FILE printk_out = {
      .write = printk_sbi_write,
  };

  va_list ap;
  va_start(ap, fmt);
  vfprintf(&printk_out, fmt, ap);
  va_end(ap);
}
