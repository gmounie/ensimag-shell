#include "parser.h"
#include "tokenizer.h"
#include "memory_utils.h"
#include <stdlib.h>
#include <stdio.h>

struct command_wrapper {
    Command *command;
    size_t argv_allocated_size;
};

// --------- COMMANDS ---------

static void free_command(Command *cmd)
{
    if (nullptr == cmd) {
        return;
    }
    if (cmd->argv != nullptr) {
        for (size_t i = 0; cmd->argv[i] != nullptr; i++) {
            free(cmd->argv[i]);
        }
        free(cmd->argv);
    }

    if (cmd->in.file != nullptr) {
        free(cmd->in.file);
    }

    if (cmd->out.file != nullptr) {
        free(cmd->out.file);
    }

    free(cmd);
}

void free_command_list(Command *commands)
{
    Command *tmp = commands;
    while (tmp != nullptr) {
        Command *next = tmp->next_link.next_command;
        free_command(tmp);
        tmp = next;
    }
}

static void allocate_command(struct command_wrapper *wrapper)
{
    Command *cmd = xmalloc(sizeof(Command));
    // Avoid allocating too much memory for no arg / few args commands
    size_t argv_allocated_size = 4;
    cmd->argv = xmalloc(sizeof(char *) * argv_allocated_size);
    for (size_t i = 0; i < argv_allocated_size; i++) {
        cmd->argv[i] = nullptr;
    }
    wrapper->command = cmd;
    wrapper->argv_allocated_size = argv_allocated_size;
    wrapper->command->argc = 0;
    wrapper->command->in.type = INHERITS_IN;
    wrapper->command->in.file = nullptr;
    wrapper->command->out.type = INHERITS_OUT;
    wrapper->command->out.file = nullptr;
    wrapper->command->background = false;
    wrapper->command->next_link.type = RELATION_NONE;
    wrapper->command->next_link.next_command = nullptr;
}

// --------- PARSER ---------

static void add_to_argv(struct command_wrapper *wrapper, char *arg)
{
    if (wrapper->command->argc + 1 >= wrapper->argv_allocated_size) {
        wrapper->argv_allocated_size <<= 1;
        wrapper->command->argv = xrealloc(wrapper->command->argv, sizeof(char *) * wrapper->argv_allocated_size);
    }
    wrapper->command->argv[wrapper->command->argc] = arg;
    wrapper->command->argv[wrapper->command->argc + 1] = nullptr;
    wrapper->command->argc++;
}

Command *parser_error(Command *commands, const Token *bad_tok, const char *msg)
{
    fprintf(stderr, "ensiSHell: %s\n", msg);
    free_command_list(commands);
    if (bad_tok != nullptr) {
        free(bad_tok->value);
    }
    return nullptr;
}

Command *parse_commands(const char *line)
{
    if (!line || !*line) {
        return nullptr;
    }
    struct command_wrapper wrapper = { nullptr, 0 };
    allocate_command(&wrapper);
    Command *head = wrapper.command;

    size_t pos = 0;
    Token token = { TOKEN_NONE, nullptr };
    CommandRelation previous_command_relation = RELATION_NONE;

    while (true) {
        free(token.value);
        token.value = nullptr;

        if (next_token(&token, line, &pos) != 0) {
            return parser_error(head, &token, "Failed to get token from command: command is null");
        }
        switch (token.type) {
        // Arg token
        case TOKEN_WORD:
            add_to_argv(&wrapper, token.value);
            token.value = nullptr;
            break;
        // In tokens
        case TOKEN_HEREDOC:
        case TOKEN_REDIR_IN:
            if (wrapper.command->in.type != INHERITS_IN) {
                return parser_error(head, &token, "Syntax error: Cannot have two input streams for a single command");
            }
            IOTypeIn in_type = (token.type == TOKEN_REDIR_IN) ? FILE_IN : HEREDOC_IN;
            Token in_file_token = { TOKEN_NONE, nullptr };
            if (next_token(&in_file_token, line, &pos) != 0 || in_file_token.type != TOKEN_WORD) {
                free(in_file_token.value);
                return parser_error(head, &token, "Syntax error: Expected filename or delimiter after redirection");
            }
            wrapper.command->in.type = in_type;
            wrapper.command->in.file = in_file_token.value;
            break;
        // Out tokens
        case TOKEN_REDIR_OUT:
        case TOKEN_APPEND_OUT:
            if (wrapper.command->out.type != INHERITS_OUT) {
                return parser_error(head, &token, "Syntax error: Cannot have two output streams for a single command");
            }
            IOTypeOut out_type = (token.type == TOKEN_REDIR_OUT) ? FILE_OUT : APPEND_OUT;
            Token out_file_token = { TOKEN_NONE, nullptr };
            if (next_token(&out_file_token, line, &pos) != 0 || out_file_token.type != TOKEN_WORD) {
                free(out_file_token.value);
                return parser_error(head, &token, "Syntax error: Expected filename after redirection");
            }
            wrapper.command->out.type = out_type;
            wrapper.command->out.file = out_file_token.value;
            break;
        // Inter commands tokens
        case TOKEN_PIPE:
        case TOKEN_AND:
        case TOKEN_OR:
        case TOKEN_SEMICOLON:
        case TOKEN_AMPERSAND: // Ampersand IS a separator (like ;), but it sends the command in the background
            if (0 == wrapper.command->argc) {
                return parser_error(head, &token, "Syntax error: Operator with no leading command");
            }
            if (token.type == TOKEN_PIPE) {
                if (wrapper.command->out.type != INHERITS_OUT) {
                    return parser_error(head, &token,
                                        "Syntax error: Cannot have two output streams for a single command");
                }
                wrapper.command->out.type = PIPE_OUT;
                wrapper.command->next_link.type = RELATION_PIPE;
            } else if (token.type == TOKEN_AND) {
                wrapper.command->next_link.type = RELATION_AND;
            } else if (token.type == TOKEN_OR) {
                wrapper.command->next_link.type = RELATION_OR;
            } else if (token.type == TOKEN_AMPERSAND) {
                wrapper.command->next_link.type = RELATION_NONE;
                wrapper.command->background = true;
            }
            // Link commands
            struct command_wrapper new_wrapper = { nullptr, 0 };
            allocate_command(&new_wrapper);
            wrapper.command->next_link.next_command = new_wrapper.command;

            if (token.type == TOKEN_PIPE) {
                new_wrapper.command->in.type = PIPE_IN;
            }
            previous_command_relation = wrapper.command->next_link.type;
            wrapper = new_wrapper;
            break;
        // EOL
        case TOKEN_EOL:
            // Empty command line
            if (0 == wrapper.command->argc && head == wrapper.command) {
                free_command_list(head);
                return nullptr;
            }

            if (0 == wrapper.command->argc && previous_command_relation != RELATION_NONE) {
                return parser_error(head, &token, "Syntax error: Missing command after relational operator");
            }

            if (0 == wrapper.command->argc) { // Empty command at the end of line (cmd ; ), we free it
                Command *tmp = head;
                while (tmp && tmp->next_link.next_command != wrapper.command) {
                    tmp = tmp->next_link.next_command;
                }
                if (tmp) {
                    tmp->next_link.next_command = nullptr;
                }
                free_command(wrapper.command);
            }

            goto parsing_loop_end;

        case TOKEN_NONE:
            break;
        }
    }
parsing_loop_end:
    free(token.value);
    return head;
}
