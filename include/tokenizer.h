#pragma once
#include <stdint.h>
#include <stddef.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__!=202311L
#include <stdbool.h>
#ifndef nullptr
#define nullptr NULL
#endif
#endif

typedef enum {
    TOKEN_NONE,
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_APPEND_OUT,
    TOKEN_HEREDOC,
    TOKEN_SEMICOLON,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_AMPERSAND,
    TOKEN_EOL,
} TokenType;

typedef struct {
    TokenType type;
    char *value;
} Token;

uint8_t next_token(Token *token, const char *line, size_t *pos);
