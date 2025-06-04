#include <readline.h>

history cmd_history;

const char* commands[] = {
    "echo",
    "exit",
    "run",
    "cat",
    "edit",
    "help",
    NULL
};

const char* files[] = {
    "email",
    "hello.elf",
    "output.txt",
    NULL
};

char *strcpy(char *restrict dst, const char *restrict src)
{
    char *d = dst;
    while ((*d++ = *src++))
    {
        // Copy until null terminator
    }
    return dst;
}

char *strchr(const char *s, int c)
{
    while (*s)
    {
        if (*s == (char)c)
        {
            return (char *)s; // Return pointer to the first occurrence of c
        }
        s++;
    }
    return NULL; // Return NULL if c is not found
}


void auto_complete(char *line, int length)
{
    for (int i = 0; commands[i] != NULL; i++)
    {
        if (strncmp(line, commands[i], length) == 0)
        {
            strcpy(line, commands[i]); // Copy the matching command back to line
            //printf("[DEBUG] Auto-complete: command = '%s'", commands[i]);
            break;
        }
    }

    if (strchr(line, ' ') != NULL) // No space means it's a command
    {
        if (strncmp(line, "cat ", 4) == 0 || strncmp(line, "edit ", 5 ) || strncmp(line, "run ", 4 ) == 0)
        {
            char *file_part = strchr(line, ' ') + 1; // Get the part after the command
            for (int i = 0; files[i] != NULL; i++)
            {
                if (strncmp(file_part, files[i], length - 4) == 0)
                {
                    strcpy(file_part, files[i]); // Copy the matching file back to line
                    //printf("[DEBUG] Auto-complete: file = '%s'", files[i]);
                    break;
                }
            }
        }
    }
}

void readline(char *line, int *length)
{
    char read_buf[2];
    int char_in_line = 0;
    *length = 0;

    while (1)
    {
        getcharn(read_buf, 1);
        if (read_buf[0] == '\r' || read_buf[0] == '\n')
        {
            if (char_in_line > 0)
            {
                line[char_in_line] = '\0';
                *length = char_in_line;
                printf("\n");
            }
            else
            {
                printf("\n");
                return;
            }
            break;
        }
        else if (read_buf[0] == 0x7f)
        { // Backspace
            if (char_in_line > 0)
            {
                printf("\b \b");
                char_in_line--;
            }
            continue;
        }
        else if (read_buf[0] == '\t')
        { // Tab for auto-completion
            if (char_in_line > 0)
            {
                auto_complete(line, char_in_line);
                printf("\r\x1b[K" BLUE "RIKESHell > " CLEAR "%s", line);
                char_in_line = strlen(line);
            }
            continue;
        }
        else if (read_buf[0] == 0x1b)
        {                          // Escape sequence
            getcharn(read_buf, 2); // Read the next two characters
            if (read_buf[0] == '[')
            {
                if (read_buf[1] == 'A')
                { // Up arrow
                    if (cmd_history.count > 0)
                    {
                        if (cmd_history.current > 0)
                        {
                            cmd_history.current--;
                        }
                        printf("\r\x1b[K" BLUE "RIKESHell > " CLEAR"%s", cmd_history.lines[cmd_history.current]);
                        char_in_line = strlen(cmd_history.lines[cmd_history.current]);
                        strcpy(line, cmd_history.lines[cmd_history.current]);
                    }
                }
                else if (read_buf[1] == 'B')
                { // Down arrow
                    if (cmd_history.count > 0)
                    {
                        if (cmd_history.current < cmd_history.count - 1)
                        {
                            cmd_history.current++;
                        }
                        printf("\r\x1b[K" BLUE "RIKESHell > " CLEAR"%s", cmd_history.lines[cmd_history.current]);
                        char_in_line = strlen(cmd_history.lines[cmd_history.current]);
                        strcpy(line, cmd_history.lines[cmd_history.current]);
                    }
                }
            }
            continue;
        }

        printf("%c", read_buf[0]);
        line[char_in_line++] = read_buf[0];
    }

    cmd_history.lines[cmd_history.count] = malloc(128 * sizeof(char));
    if (cmd_history.lines[cmd_history.count] == NULL)
    {
        printf("Memory allocation failed\n");
        return;
    }
    if (cmd_history.count < MAX_HISTORY)
    {
        cmd_history.count++;
        strcpy(cmd_history.lines[cmd_history.count - 1], line);
    }
    else
    {
        free(cmd_history.lines[0]); // Free the oldest command
        for (int i = 1; i < MAX_HISTORY; i++)
        {
            cmd_history.lines[i - 1] = cmd_history.lines[i];
        }
        cmd_history.lines[MAX_HISTORY - 1] = malloc(128 * sizeof(char));
        if (cmd_history.lines[MAX_HISTORY - 1] == NULL)
        {
            printf("Memory allocation failed\n");
            return;
        }
    }
    cmd_history.current = cmd_history.count; // Reset current index on new input
    return;

}