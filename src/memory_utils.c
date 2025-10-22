#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static void memory_error(void)
{
    errno = ENOMEM;
    perror(nullptr);
    exit(1);
}

void *xmalloc(size_t size)
{
    void *p = malloc(size);
    if (!p)
        memory_error();
    return p;
}

void *xrealloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size);
    if (!p)
        memory_error();
    return p;
}
