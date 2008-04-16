#include "Deque.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    Deque *deque;
    char line[4096];
    size_t len;
    void *data;
    size_t size;

    if (argc == 1)
    {
        deque = Memory_Deque_create();
    }
    else
    if (argc == 2 && argv[1][0] != '-')
    {
        deque = File_Deque_create(argv[1]);
    }
    else
    {
        printf("Usage:\n"
               "  test-deque            -- use the in-memory deque\n"
               "  test-deque <path>     -- use the file-based deque\n"
               "\n"
               "Commands:\n"
               "  destroy       -- destroy the deque and exit\n"
               "  empty         -- report if the deque is empty\n"
               "  size          -- print number of elements in the deque\n"
               "  push_back     -- add element at the back\n"
               "  push_front    -- add element at the front\n"
               "  get_back      -- print element at the back\n"
               "  get_front     -- print element at the front\n"
               "  pop_back      -- remove element at the back\n"
               "  pop_front     -- remove element at the front\n");
        return 1;
    }

    if (deque == NULL)
    {
        fprintf(stderr, "Unable to create deque!\n");
        exit(1);
    }

    while (fgets(line, sizeof(line), stdin) != NULL)
    {
        /* Determine length and strip trailing newline character */
        len = strlen(line);
        if (len == 0)
            break;
        if (line[len - 1] == '\n')
        {
            line[len - 1] = '\0';
            len -= 1;
        }

        if (strcmp(line, "destroy") == 0)
        {
            break;
        }
        else
        if (strcmp(line, "empty") == 0)
        {
            printf("empty=%s\n", deque->empty(deque) ? "true" : "false");
        }
        else
        if (strcmp(line, "size") == 0)
        {
            printf("size=%ld\n", (long)deque->size(deque));
        }
        else
        if (strncmp(line, "push_back ", 10) == 0)
        {
            if (!deque->push_back(deque, line + 10, len - 10))
                printf("push_back failed!\n");
        }
        else
        if (strncmp(line, "push_front ", 11) == 0)
        {
            if (!deque->push_front(deque, line + 11, len - 11))
                printf("push_front failed!\n");
        }
        else
        if (strcmp(line, "get_back") == 0)
        {
            if (!deque->get_back(deque, &data, &size))
                printf("get_back failed!\n");
            else
            {
                fwrite(data, size, 1, stdout);
                fputc('\n', stdout);
            }
        }
        else
        if (strcmp(line, "get_front") == 0)
        {
            if (!deque->get_front(deque, &data, &size))
                printf("get_front failed!\n");
            else
            {
                fwrite(data, size, 1, stdout);
                fputc('\n', stdout);
            }
        }
        else
        if (strcmp(line, "pop_back") == 0)
        {
            if (!deque->pop_back(deque))
                printf("pop_back failed!\n");
        }
        else
        if (strcmp(line, "pop_front") == 0)
        {
            if (!deque->pop_front(deque))
                printf("pop_front failed!\n");
        }
        else
        {
            printf("Unrecognized input line: %s!\n", line);
        }
    }

    deque->destroy(deque);

    return 0;
}
