#pragma once

#include <stddef.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__!=202311L
#include <stdbool.h>
#ifndef nullptr
#define nullptr NULL
#endif
#endif

typedef enum {
    INHERITS_OUT,
    PIPE_OUT,
    FILE_OUT,
    APPEND_OUT,
} IOTypeOut;

typedef enum {
    INHERITS_IN,
    PIPE_IN,
    FILE_IN,
    HEREDOC_IN,
} IOTypeIn;

typedef enum {
    RELATION_NONE,
    RELATION_PIPE,
    RELATION_AND,
    RELATION_OR
} CommandRelation;

typedef struct command Command;

typedef struct {
    CommandRelation type;
    Command *next_command;
} CommandLink;

typedef struct command {
    size_t argc;
    char **argv;
    struct {
        IOTypeIn type;
        char *file;
        // char *doc; // UNUSED
    } in;
    struct {
        IOTypeOut type;
        char *file;
    } out;
    bool background;
    CommandLink next_link;
} Command;

Command *parse_commands(const char *line);
void free_command_list(Command *commands);
