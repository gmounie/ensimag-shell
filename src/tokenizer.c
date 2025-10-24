#include "tokenizer.h"
#include "memory_utils.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// -------------- TOKENIZER -------------
static bool is_shell_operator(const char c)
{
    return ('|' == c) || ('&' == c) || (';' == c) || ('>' == c) || ('<' == c);
}

static bool is_quote(const char c)
{
    return ('\"' == c) || ('\'' == c);
}

static void fill_token(Token *token, TokenType type, char *value)
{
    token->type = type;
    token->value = value;
}

static void read_operation_token(Token *token, const char *line, size_t *pos)
{
    if (strncmp(line + *pos, "&&", 2) == 0) {
        fill_token(token, TOKEN_AND, nullptr);
        *pos += 2;
        return;
    }
    if (strncmp(line + *pos, "||", 2) == 0) {
        fill_token(token, TOKEN_OR, nullptr);
        *pos += 2;
        return;
    }
    if (strncmp(line + *pos, ">>", 2) == 0) {
        fill_token(token, TOKEN_APPEND_OUT, nullptr);
        *pos += 2;
        return;
    }
    if (strncmp(line + *pos, "<<", 2) == 0) {
        fill_token(token, TOKEN_HEREDOC, nullptr);
        *pos += 2;
        return;
    }

    if (line[*pos] == '|') {
        fill_token(token, TOKEN_PIPE, nullptr);
        (*pos)++;
        return;
    }
    if (line[*pos] == '&') {
        fill_token(token, TOKEN_AMPERSAND, nullptr);
        (*pos)++;
        return;
    }
    if (line[*pos] == ';') {
        fill_token(token, TOKEN_SEMICOLON, nullptr);
        (*pos)++;
        return;
    }
    if (line[*pos] == '<') {
        fill_token(token, TOKEN_REDIR_IN, nullptr);
        (*pos)++;
        return;
    }
    if (line[*pos] == '>') {
        fill_token(token, TOKEN_REDIR_OUT, nullptr);
        (*pos)++;
        return;
    }
}

static void put_char_in_word_buffer(char **buffer, size_t *alloc_size, size_t *pos, char c)
{
    if (*pos >= *alloc_size) {
        *alloc_size <<= 1;
        *buffer = xrealloc(*buffer, *alloc_size);
    }
    (*buffer)[*pos] = c;
    (*pos)++;
}

static void quoted_string(const char *line, size_t *pos, char **word_buffer, size_t *alloc_size, size_t *buffer_pos,
                          const size_t line_len)
{
    char quote_char = line[*pos];
    size_t end_quote_pos = *pos + 1;
    bool escaped = false;
    while (line[end_quote_pos]) {
        if (!escaped && line[end_quote_pos] == '\\') {
            escaped = true;
            end_quote_pos++;
            continue;
        }

        if (line[end_quote_pos] == quote_char && !escaped) {
            break;
        }

        if (escaped && (!(line[end_quote_pos] == '\\' || line[end_quote_pos] == quote_char))) {
            put_char_in_word_buffer(word_buffer, alloc_size, buffer_pos, '\\');
        }

        if (escaped) {
            escaped = false;
        }

        put_char_in_word_buffer(word_buffer, alloc_size, buffer_pos, line[end_quote_pos]);
        end_quote_pos++;
    }
    (*word_buffer)[*buffer_pos] = '\0';

    *pos = (end_quote_pos < line_len) ? end_quote_pos + 1 : line_len;
}

static void regular_word(const char *line, size_t *pos, char **word_buffer, size_t *alloc_size, size_t *buffer_pos)
{
    size_t word_end = *pos;
    bool escaped = false;
    while (line[word_end] &&
           (escaped || (!isspace(line[word_end]) && !is_shell_operator(line[word_end]) && !is_quote(line[word_end])))) {
        if (!escaped && line[word_end] == '\\') {
            escaped = true;
            word_end++;
            continue;
        }

        put_char_in_word_buffer(word_buffer, alloc_size, buffer_pos, line[word_end]);
        word_end++;
        if (escaped) {
            escaped = false;
        }
    }

    *pos = word_end;
}

static void read_word_token(Token *token, const char *line, size_t *pos)
{
    const size_t line_len = strlen(line);
    size_t alloc_size = 32;
    char *word_buffer = xmalloc(sizeof(char) * alloc_size);
    memset(word_buffer, 0, sizeof(char) * alloc_size);
    size_t buffer_pos = 0;
    while (*pos < line_len && line[*pos] && !isspace(line[*pos]) && !is_shell_operator(line[*pos])) {
        if (is_quote(line[*pos])) {
            quoted_string(line, pos, &word_buffer, &alloc_size, &buffer_pos, line_len);
        } else {
            regular_word(line, pos, &word_buffer, &alloc_size, &buffer_pos);
        }
    }
    fill_token(token, TOKEN_WORD, word_buffer);
}

uint8_t next_token(Token *token, const char *line, size_t *pos)
{
    if (nullptr == line) {
        return 1;
    }
    // Skip leading spaces & al
    while (line[*pos] && isspace(line[*pos]))
        (*pos)++;

    if (!line[*pos]) {
        fill_token(token, TOKEN_EOL, nullptr);
        return 0;
    }

    if (is_shell_operator(line[*pos])) {
        read_operation_token(token, line, pos);
    } else {
        read_word_token(token, line, pos);
    }
    return 0;
}
