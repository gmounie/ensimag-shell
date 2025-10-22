/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "variante.h"
#include "readcmd.h"
#include "parser.h"

#if defined(__STDC_VERSION__) && __STDC_VERSION__!=202311L
#include <stdbool.h>
#ifndef nullptr
#define nullptr NULL
#endif
#endif

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

#if USE_GNU_READLINE == 1
#define CLEAR_HISTORY clear_history()
#define ADD_TO_HISTORY(line) add_history(line)
#else
#define CLEAR_HISTORY
#define ADD_TO_HISTORY(line)
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#if USE_GUILE == 1
#include <libguile.h>

int question6_executer(char *line)
{
    /* Question 6: Insert your code to execute the command line
	 * identically to the standard execution scheme:
	 * parsecmd, then fork+execvp, for a single command.
	 * pipe and i/o redirection are not required.
	 */
    printf("Not implemented yet: can not execute %s\n", line);

    /* Remove this line when using parsecmd as it will free it */
    free(line);

    return 0;
}

SCM executer_wrapper(SCM x)
{
    return scm_from_int(question6_executer(scm_to_locale_stringn(x, nullptr)));
}
#endif

/*
void terminate(char *line)
{
#if USE_GNU_READLINE == 1
    clear_history();
#endif
    if (line)
        free(line);
    printf("exit\n");
    exit(0);
}
*/

int main()
{
    printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#if USE_GUILE == 1
    scm_init_guile();
    /* register "executer" function in scheme */
    scm_c_define_gsubr("executer", 1, 0, 0, &executer_wrapper);
#endif
    const char *prompt = "ensishell> ";

    while (1) {
        /* Readline use some internal memory structure that
		   can not be cleaned at the end of the program. Thus
		   one memory leak per command seems unavoidable yet */
        char *line = readline(prompt);
        if (line == nullptr || !strcmp(line, "exit")) {
            CLEAR_HISTORY;
            free(line);
            puts("exit");
            return 0;
        }

#if USE_GNU_READLINE == 1
        add_history(line);
#endif

#if USE_GUILE == 1
        /* The line is a scheme command */
        if (line[0] == '(') {
            char catchligne[strlen(line) + 256];
            sprintf(
                catchligne,
                "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))",
                line);
            scm_eval_string(scm_from_locale_string(catchligne));
            free(line);
            continue;
        }
#endif

        /* parsecmd free line and set it up to 0 */
        Command *commands = parse_commands(line);

        if (!commands) {
            /* Syntax error, read another command */
            continue;
        }

        if (commands->in.file)
            printf("in: %s\n", commands->in.file);
        if (commands->out.file)
            printf("out: %s\n", commands->out.file);
        if (commands->background)
            printf("background (&)\n");

        /* Display each command of the pipe */
        Command *cmd = commands;
        size_t command_nb = 0;
        while (cmd) {
            printf("Command[%lu]: ", command_nb);
            for (size_t i = 0; i < cmd->argc; i++) {
                printf("'%s' ", cmd->argv[i]);
            }
            printf("\n");
            command_nb++;
            cmd = cmd->next_link.next_command;
        }
        free_command_list(commands);
    }
}
