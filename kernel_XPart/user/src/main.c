#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <syscalls.h>
#include <printk.h>
#include <fs.h>
#include <readline.h>

// define some tests
#define PFH1 1001
#define PFH2 1002
#define FORK1 1101
#define FORK2 1102
#define FORK3 1103
#define FORK4 1104
#define XPART 1111
#define SHELL 1112

#if defined(USER_MAIN) && !(USER_MAIN > 1000 && USER_MAIN < 1200)
#warning "Invalid definition of USER_MAIN"
#undef USER_MAIN
#endif

#ifndef USER_MAIN
// 你可以修改这一行来提供代码高亮
#define USER_MAIN SHELL
#endif

#define DELAY_TIME 1247

static uint64_t user_clock(void) {
  uint64_t ret;
  asm volatile("rdtime %0" : "=r"(ret));
  return ret / 10;
}

static void delay(unsigned long ms) {
  uint64_t prev_clock = user_clock();
  while (user_clock() - prev_clock < ms * 1000)
    ;
}

#if USER_MAIN == PFH1

int main(void) {
  register const void *const sp asm("sp");

  while (1) {
    printf("\x1b[44m[U]\x1b[0m [PID = %d, sp = %p]\n", getpid(), sp);
    delay(DELAY_TIME);
  }
}

#elif USER_MAIN == PFH2

const char *const xdigits = "0123456789abcdef";
char space[0x2000] __attribute__((aligned(0x1000)));
size_t i;

int main(void) {
  while (1) {
    i = 0;
    printf("\x1b[44m[U]\x1b[0m [PID = %d] ", getpid());
    while (i < sizeof(space)) {
      space[i] = xdigits[i % 16];
      printf("\x1b[4%cm%c\x1b[0m", xdigits[rand() % 8], space[i]);
      i++;
      delay(1);
    }
    printf("\n");
  }
}

#elif USER_MAIN == FORK1

int var = 0;

int main(void) {
  pid_t pid = fork();
  const char *ident = pid ? "PARN" : "CHLD";
  if (pid != 0){
    waitpid(pid, NULL, 0); // wait for the child to finish
  }
  printf("\x1b[44m[U-%s]\x1b[0m [PID = %d] fork returns %d\n", ident, getpid(), pid);
  if (pid == 0){
    execve("exit.elf", NULL, NULL);
  }
  //int* malloc_var = (int*)malloc(sizeof(int));

  while (1) {
    printf("\x1b[44m[U-%s]\x1b[0m [PID = %d] var = %d\n", ident, getpid(), var++);
    //printf("\x1b[44m[U-%s]\x1b[0m [PID = %d] malloc_var = %d @ 0x%lx\n", ident, getpid(), *malloc_var, (unsigned long)malloc_var);
    //*malloc_var = var; // update malloc_var to point to the current var value
    delay(DELAY_TIME / 2 + rand() % DELAY_TIME);
  }
}

#elif USER_MAIN == FORK2

int var = 0;
char space[0x2000] __attribute__((aligned(0x1000)));

int main(void) {
  char* malloc_str1 = (char*)malloc(0x100);
  memcpy(malloc_str1, "ZJU Sys3 Malloc", 16);
  for (int i = 0; i < 3; i++) {
    printf("\x1b[44m[U]\x1b[0m [PID = %d] var = %d\n", getpid(), var++);
    printf("\x1b[44m[U]\x1b[0m [PID = %d] malloc_str1 = %s @ 0x%lx\n", getpid(), malloc_str1, (unsigned long)malloc_str1);
    delay(DELAY_TIME);
  }

  free(malloc_str1); // free the malloc_str1 memory
  memcpy(&space[0x1000], "ZJU Sys3 Lab5", 14);

  pid_t pid = fork();
  const char *ident = pid ? "PARN" : "CHLD";
  char* malloc_str2 = (char*)malloc(0x100);
  memcpy(malloc_str2, "ZJU Sys3 Fork2", 15);

  printf("\x1b[44m[U-%s]\x1b[0m [PID = %d] Message: %s @ 0x%lx\n", ident, getpid(), &space[0x1000], (unsigned long)&space[0x1000]);
  while (1) {
    printf("\x1b[44m[U-%s]\x1b[0m [PID = %d] var = %d\n", ident, getpid(), var++);
    printf("\x1b[44m[U-%s]\x1b[0m [PID = %d] malloc_str2 = %s @ 0x%lx\n", ident, getpid(), malloc_str2, (unsigned long)malloc_str2);
    delay(DELAY_TIME / 2 + rand() % DELAY_TIME);
  }
}

#elif USER_MAIN == FORK3

int var = 0;

int main(void) {
  printf("\x1b[44m[U]\x1b[0m [PID = %d] var = %d\n", getpid(), var++);
  fork();
  fork(); // multiple references to one page

  printf("\x1b[44m[U]\x1b[0m [PID = %d] var = %d\n", getpid(), var++);
  fork();

  while (1) {
    printf("\x1b[44m[U]\x1b[0m [PID = %d] var = %d\n", getpid(), var++);
    delay(DELAY_TIME / 2 + rand() % DELAY_TIME);
  }
}

#elif USER_MAIN == FORK4

#define LARGE 1000

int var = 0;
long bigarr[LARGE] __attribute__((aligned(0x1000))) = {};

int fib(int times) {
  if (times <= 2) {
    return 1;
  } else {
    return fib(times - 1) + fib(times - 2);
  }
}

const char *suffix(int num) {
  num %= 100;
  int i = num % 10;
  if (i == 1 && num != 11) {
    return "st";
  } else if (i == 2 && num != 12) {
    return "nd";
  } else if (i == 3 && num != 13) {
    return "rd";
  } else {
    return "th";
  }
}

int main(void) {
  for (int i = 0; i < LARGE; i++) {
    bigarr[i] = 3 * i + 1;
  }

  pid_t pid = fork();
  const char *ident = pid ? "PARN" : "CHLD";
  printf("\x1b[44m[U]\x1b[0m fork returns %d\n", pid);

  while (1) {
    var = 0;
    while (var < LARGE) {
      printf("\x1b[44m[U-%s]\x1b[0m [PID = %d] the %d%s fibonacci number is %d and "
             "the %d%s number in the big array is %ld\n",
             ident, getpid(), var, suffix(var), fib(var), LARGE - 1 - var, suffix(LARGE - 1 - var),
             bigarr[LARGE - 1 - var]);
      var++;
      delay(100);
    }
  }
}

#elif USER_MAIN == XPART

int main(void) {
  register const void *const sp asm("sp");

  printf("\x1b[44m[U]\x1b[0m [PID = %d, sp = %p]\n", getpid(), sp);

  int fd = fopen("email", F_READ); // open a file to test file system
  char buf[1536];
  fread(fd, buf, sizeof(buf), 0); // read the file
  printf("\x1b[44m[U]\x1b[0m [PID = %d] read %ld bytes from file 'email':\n", getpid(), sizeof(buf));
  printf("%s\n", buf); // print the content of the file
  fclose(fd); // close the file
  printf("\x1b[44m[U]\x1b[0m [PID = %d] file opened and closed successfully\n", getpid());

  while (1) {
    char c[8];
    getcharn(c, 8);
    printf("\x1b[44m[U]\x1b[0m [PID = %d] getcharn() = %s\n", getpid(), c);
    delay(DELAY_TIME);
  }
  //return 0;
}

#elif USER_MAIN == SHELL

// credits to https://github.com/ZJU-SEC/os24fall-stu

int in_shell;
char string_buf[32][64];
char string_buf2[2048];
int ind;
char filename[2048];

int atoi(char* str) {
    int ret = 0;
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        ret = ret * 10 + str[i] - '0';
    }
    return ret;
}

char *get_param(char *cmd) {
    for (int i = 0; i < 64; i++) {
        string_buf[ind][i] = '\0'; // clear the buffer
    }
    while (*cmd == ' ') {
        cmd++;
    }
    //printf("[DEBUG] parse cmd: %s\n", cmd);
    int pos = 0;
    while (*cmd != '\0' && *cmd != ' ') {
        string_buf[ind][pos++] = *(cmd++);
    }
    string_buf[ind][pos] = '\0';
    //printf("[DEBUG] get_param: %s\n", string_buf);
    return string_buf[ind++];
}

char *get_string(char *cmd) {
    while (*cmd == ' ') {
        cmd++;
    }

    if (*cmd == '"') { // quote wrapped
        cmd++;
        int pos = 0;
        while (*cmd != '"') {
            string_buf2[pos++] = *(cmd++);
        }
        string_buf2[pos] = '\0';
        return string_buf2;
    } else {
        return get_param(cmd);
    }
}

void parse_cmd(char *cmd, int len) {
    // printf("\x1b[44m[U]\x1b[0m [PID = %d] cmd = '%s'\n", getpid(), cmd);
    if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o') {
        cmd += 4;
        char *echo_content = get_string(cmd);
        len = strlen(echo_content);
        cmd += len;
        write(1, echo_content, len);
        write(1, "\n", 1);
    } 
    else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't') {
        cmd += 3;
        char *filename = get_param(cmd);
        //printf("[DEBUG] cat filename: %s\n", filename);
        char buf[1536];
        int fd = fopen(filename, F_READ);
        if (fd == -1) {
            printf("can't open file: %s\n", filename);
            return;
        }
        fread(fd, buf, sizeof(buf), 0);
        printf("%s\n", buf);
        fclose(fd);

    } else if (cmd[0] == 'e' && cmd[1] == 'd' && cmd[2] == 'i' && cmd[3] == 't' ) {
        cmd += 4;
        while (*cmd == ' ' && *cmd != '\0') {
            cmd++;
        }

        // Get filename(path)
        char* temp = get_param(cmd);
        int len = strlen(temp); 
        char filename[len + 1];
        for (int i = 0; i < len; i++) {
            filename[i] = temp[i];
        }
        filename[len] = '\0';
        cmd += len;

        while (*cmd == ' ' && *cmd != '\0') {
            cmd++;
        }

        // Get offset
        temp = get_param(cmd);
        len = strlen(temp);
        char offset[len + 1];
        for (int i = 0; i < len; i++) {
            if (temp[i] < '0' || temp[i] > '9') {
                printf("invalid offset: %s\n", temp);
                return;
            }
            offset[i] = temp[i];
        }
        offset[len] = '\0';
        cmd += len;

        while (*cmd == ' ' && *cmd != '\0') {
            cmd++;
        }

        // Get write content
        temp = get_string(cmd);
        len = strlen(temp);
        char content[len + 1];
        for (int i = 0; i < len; i++) {
            content[i] = temp[i];
        }
        content[len] = '\0';
        cmd += len;

        int offset_int = atoi(offset);

        int fd = fopen(filename, F_WRITE);
        fwrite(fd, content, len, offset_int);
        fclose(fd);
    // } else if (cmd[0] == 'l' && cmd[1] == 's') 
    // {
    //     char **filenames = get_filenames();
    //     if (filenames == NULL) {
    //         printf("no files found\n");
    //         return;
    //     }
    //     for (int i = 0; filenames[i] != NULL; i++) {
    //         printf("%s\n", filenames[i]);
    //     }    
    }
    else if(cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p') 
    {
        printf("RIKESHell commands:\n");
        printf("    echo <content> - Print content to stdout\n");
        printf("    cat <filename> - Print file content to stdout\n");
        printf("    edit <filename> <offset> <content> - Edit file at offset with content\n");
        printf("    run <exec filename> - Run a program by its filename\n");
        printf("    help - Show this help message\n");
        printf("    exit - Exit the shell\n");
    }
    else if (cmd[0] == 'e' && cmd[1] == 'x' && cmd[2] == 'i' && cmd[3] == 't')
    {
        in_shell = 0;
        printf("Exiting RIKESHell...\n");
        return;
    } 
    else if (cmd[0] == 'r' && cmd[1] == 'u' && cmd[2] == 'n') 
    {   
        cmd += 3;
        while (*cmd == ' ') {
            cmd++;
        }
        if (*cmd == '\0') {
            printf("filename not provided\n");
            return;
        }
        
        char *filename = get_param(cmd);
        //printf("[DEBUG] run filename: %s\n", filename);
        //printf("[DEBUG] run cmd: %s\n", cmd);
        cmd += strlen(filename);
        char *out = get_param(cmd);
        //printf("[DEBUG] run out: %s\n", out);
        //printf("[DEBUG] run cmd: %s\n", cmd);
        pid_t pid = fork();
        if (pid != 0){
          // if (out[0] != '\0') {
          //   int fd = fopen(out, F_WRITE);
          //   if (fd == -1) {
          //       printf("can't open file: %s\n", out);
          //       return;
          //   }
          //   dup3(pid, 1, fd); // redirect stdout to the file
          //   dup3(pid, 2, fd); // redirect stderr to the file
          // }
            waitpid(pid, NULL, 0); // wait for the child to finish
        }
        if(pid == 0){
            execve(filename, NULL, NULL);
            return;
        }
        // execve(filename, NULL, NULL);
        return;
    }
    else
    {
        printf("command not found: %s\n", cmd);
        return;
    }
}

int main() {
    printf("hello, stdout!\n");
    char read_buf[2];
    char line_buf[128];
    int char_in_line = 0;
    in_shell = 1;
    printf(BLUE "RIKESHell > " CLEAR);
    while (1) {
        readline(line_buf, &char_in_line);
        //printf("[DEBUG] line_buf = '%s', char_in_line = %d\n", line_buf, char_in_line);
        parse_cmd(line_buf, char_in_line);
        // clear the line buffer
        for (int i = 0; i < 128; i++) {
            line_buf[i] = '\0';
        }
        char_in_line = 0;
        printf(BLUE "RIKESHell > " CLEAR);

        if (in_shell == 0)
        {
            break;
        }
        
    }
    register const void *const sp asm("sp");
    while (1) {
        printf("\x1b[44m[U]\x1b[0m [PID = %d, sp = %p]\n", getpid(), sp);
        delay(DELAY_TIME);
    }
    return 0;
}

#endif
