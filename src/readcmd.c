/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 *           Matthieu Moy 2008                       *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#include "readcmd.h"

#if defined(__STDC_VERSION__) && __STDC_VERSION__!=202311L
#include <stdbool.h>
#ifndef nullptr
#define nullptr NULL
#endif
#endif

static void memory_error(void)
{
    errno = ENOMEM;
    perror(nullptr);
    exit(1);
}

static void *xmalloc(size_t size)
{
    void *p = malloc(size);
    if (!p)
        memory_error();
    return p;
}

static void *xrealloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size);
    if (!p)
        memory_error();
    return p;
}

#if USE_GNU_READLINE == 0
/* Read a line from standard input and put it in a char[] */
char *readline(const char *prompt)
{
    size_t buf_len = 16;
    char *buf = xmalloc(buf_len * sizeof(char));

    printf(prompt);
    if (fgets(buf, buf_len, stdin) == NULL) {
        free(buf);
        return NULL;
    }

    do {
        size_t l = strlen(buf);
        if ((l > 0) && (buf[l - 1] == '\n')) {
            l--;
            buf[l] = 0;
            return buf;
        }
        if (buf_len >= (INT_MAX / 2))
            memory_error();
        buf_len *= 2;
        buf = xrealloc(buf, buf_len * sizeof(char));
        if (fgets(buf + l, buf_len - l, stdin) == NULL)
            return buf;
    } while (1);
}
#endif

#define READ_CHAR *(*cur_buf)++ = *(*cur)++
#define SKIP_CHAR (*cur)++

static void read_single_quote(char **cur, char **cur_buf)
{
    SKIP_CHAR;
    while (1) {
        char c = **cur;
        switch (c) {
        case '\'':
            SKIP_CHAR;
            return;
        case '\0':
            fprintf(stderr, "Missing closing '\n");
            return;
        default:
            READ_CHAR;
            break;
        }
    }
}

static void read_double_quote(char **cur, char **cur_buf)
{
    SKIP_CHAR;
    while (1) {
        char c = **cur;
        switch (c) {
        case '"':
            SKIP_CHAR;
            return;
        case '\\':
            SKIP_CHAR;
            READ_CHAR;
            break;
        case '\0':
            fprintf(stderr, "Missing closing \"\n");
            return;
        default:
            READ_CHAR;
            break;
        }
    }
}

static void read_word(char **cur, char **cur_buf)
{
    while (1) {
        char c = **cur;
        switch (c) {
        case '\0':
        case ' ':
        case '\t':
        case '<':
        case '>':
        case '|':
        case '&':
            **cur_buf = '\0';
            return;
        case '\'':
            read_single_quote(cur, cur_buf);
            break;
        case '"':
            read_double_quote(cur, cur_buf);
            break;
        case '\\':
            SKIP_CHAR;
            READ_CHAR;
            break;
        default:
            READ_CHAR;
            break;
        }
    }
}

/* Split the string in words, according to the simple shell grammar. */
static char **split_in_words(char *line)
{
    char *cur = line;
    char *buf = xmalloc(strlen(line) + 1);
    char *cur_buf;
    char **tab = nullptr;
    size_t l = 0;
    char c;

    while ((c = *cur) != 0) {
        char *w = nullptr;
        switch (c) {
        case ' ':
        case '\t':
            /* Ignore any whitespace */
            cur++;
            break;
        case '&':
            w = "&";
            cur++;
            break;
        case '<':
            w = "<";
            cur++;
            break;
        case '>':
            w = ">";
            cur++;
            break;
        case '|':
            w = "|";
            cur++;
            break;
        default:
            /* Another word */
            cur_buf = buf;
            read_word(&cur, &cur_buf);
            w = strdup(buf);
        }
        if (w) {
            tab = xrealloc(tab, (l + 1) * sizeof(char *));
            tab[l++] = w;
        }
    }
    tab = xrealloc(tab, (l + 1) * sizeof(char *));
    tab[l++] = nullptr;
    free(buf);
    return tab;
}

static void freeseq(char ***seq)
{
    for (size_t i = 0; seq[i] != nullptr; i++) {
        char **cmd = seq[i];

        for (size_t j = 0; cmd[j] != nullptr; j++)
            free(cmd[j]);
        free(cmd);
    }
    free(seq);
}

/* Free the fields of the structure but not the structure itself */
static void freecmd(struct cmdline *s)
{
    if (s->in)
        free(s->in);
    if (s->out)
        free(s->out);
    if (s->seq)
        freeseq(s->seq);
}

struct cmdline *parsecmd(char **pline)
{
    char *line = *pline;
    static struct cmdline *static_cmdline = nullptr;
    struct cmdline *s = static_cmdline;
    int i;
    char *w;

    if (line == NULL) {
        if (s) {
            freecmd(s);
            free(s);
        }
        return static_cmdline = nullptr;
    }

    char** cmd = xmalloc(sizeof(char*));
    cmd[0] = nullptr;
    size_t cmd_len = 0;
    char*** seq = xmalloc(sizeof(char**));
    seq[0] = nullptr;
    size_t seq_len = 0;

    char** words = split_in_words(line);
    free(line);
    *pline = nullptr;

    if (!s)
        static_cmdline = s = xmalloc(sizeof(struct cmdline));
    else
        freecmd(s);
    s->err = nullptr;
    s->in = nullptr;
    s->out = nullptr;
    s->seq = nullptr;
    s->bg = 0;

    i = 0;
    while ((w = words[i++]) != nullptr) {
        switch (w[0]) {
        case '<':
            /* Tricky : the word can only be "<" */
            if (s->in) {
                s->err = "only one input file supported";
                goto error;
            }
            if (words[i] == nullptr) {
                s->err = "filename missing for input redirection";
                goto error;
            }
            switch (words[i][0]) {
            case '<':
            case '>':
            case '&':
            case '|':
                s->err = "incorrect filename for input redirection";
                goto error;
                break;
            }
            s->in = words[i++];
            break;
        case '>':
            /* Tricky : the word can only be ">" */
            if (s->out) {
                s->err = "only one output file supported";
                goto error;
            }
            if (words[i] == nullptr) {
                s->err = "filename missing for output redirection";
                goto error;
            }
            switch (words[i][0]) {
            case '<':
            case '>':
            case '&':
            case '|':
                s->err = "incorrect filename for output redirection";
                goto error;
                break;
            }
            s->out = words[i++];
            break;
        case '&':
            /* Tricky : the word can only be "&" */
            if (cmd_len == 0 || words[i] != nullptr) {
                s->err = "misplaced ampersand";
                goto error;
            }
            if (s->bg == 1) {
                s->err = "only one ampersand supported";
                goto error;
            }
            s->bg = 1;
            break;
        case '|':
            /* Tricky : the word can only be "|" */
            if (cmd_len == 0) {
                s->err = "misplaced pipe";
                goto error;
            }
            if (words[i] == nullptr) {
                s->err = "second command missing for pipe redirection";
                goto error;
            }
            switch (words[i][0]) {
            case '<':
            case '>':
            case '&':
            case '|':
                s->err = "incorrect pipe usage";
                goto error;
                break;
            }
            seq = xrealloc(seq, (seq_len + 2) * sizeof(char **));
            seq[seq_len++] = cmd;
            seq[seq_len] = nullptr;

            cmd = xmalloc(sizeof(char *));
            cmd[0] = nullptr;
            cmd_len = 0;
            break;
        default:
            cmd = xrealloc(cmd, (cmd_len + 2) * sizeof(char *));
            cmd[cmd_len++] = w;
            cmd[cmd_len] = nullptr;
        }
    }

    if (cmd_len != 0) {
        seq = xrealloc(seq, (seq_len + 2) * sizeof(char **));
        seq[seq_len++] = cmd;
        seq[seq_len] = nullptr;
    } else if (seq_len != 0) {
        s->err = "misplaced pipe";
        i--;
        goto error;
    } else
        free(cmd);
    free(words);
    s->seq = seq;
    return s;
error:
    while ((w = words[i++]) != nullptr) {
        switch (w[0]) {
        case '&':
        case '<':
        case '>':
        case '|':
            break;
        default:
            free(w);
        }
    }
    free(words);
    freeseq(seq);
    for (i = 0; cmd[i] != nullptr; i++)
        free(cmd[i]);
    free(cmd);
    if (s->in) {
        free(s->in);
        s->in = nullptr;
    }
    if (s->out) {
        free(s->out);
        s->out = nullptr;
    }
    return s;
}
