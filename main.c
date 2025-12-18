#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>     // Required for fork() and execv()
#include <sys/wait.h>   // Required for waitpid()
#include <fcntl.h>      // Required for open()
#include <signal.h>     // Required for signals & SIGINT

#include "builtin.h"
#include "global.h"

#define BUFFER_SIZE 1024
#define HISTORY_LENGTH 1024
#define PATH_MAX 4096
#define MAX_ARGS 64

#define TOKEN_SEP " \t"
#define PROMPT_STR "\033[1;32m%s@\033[1;34m%s\033[0m$ "

char PROMPT[8192];
char CWD[PATH_MAX];

void handle_redirection(char **args);
int check_background(char **args);

// function to handle SIGINT singla (ctrl + c)
void handle_signal(int sig) {
    if (sig == SIGINT) {
        fprintf(stdout, "\n\033[1;32m%s@\033[1;34m%s\033[0m$ ", getenv("USER"), CWD);
        fflush(stdout);
    }
}

// input command tokenisation it modifies the input command and breaks into agrugments
int s_read(char *input, char **args, int max_args, int *has_pipe)
{
    int argc = 0;
    char *p = input;

    while (*p && argc < max_args - 1) {

        // 1. Skip leading whitespace
        while (*p == ' ' || *p == '\t') p++;

        if (*p == '\0')
            break;

        char *start;
        int in_quote = 0;
        char quote_char = 0;

        // 2. Check for quoted argument
        if (*p == '"' || *p == '\'') {
            in_quote = 1;
            quote_char = *p;
            start = ++p;   // skip opening quote
        } else {
            start = p;
        }

        // 3. Scan until end of argument
        while (*p) {
            if (in_quote) {
                if (*p == quote_char) {
                    *p = '\0';
                    p++;
                    break;
                }
            } else {
                if (*p == ' ' || *p == '\t') {
                    *p = '\0';
                    p++;
                    break;
                }
            }
            p++;
        }

        args[argc++] = start;
        if (strcmp(start, "|") == 0) {
            *has_pipe = argc - 1;
        }
    }

    args[argc] = NULL;
    return argc;
}

// main function which executes the command forks a new process and uses exec
int s_execute(char *cmd, char **cmd_args) {
    fprintf(stdout, "Executing '%s'!\n", cmd);

    int status;
    pid_t pid;

    int is_background_process = check_background(cmd_args);
    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Could not execute!\n");
        return -1;
    }

    if (pid == 0) {
        // child
        handle_redirection(cmd_args);
        execvp(cmd, cmd_args);
    } else {
        // parent wait for child

        if (!is_background_process) {
            if (waitpid (pid, &status, 0) != pid) {
                fprintf(stderr, "Could not wait for child!\n");
                return -1;
            }
        }

    }
    return status;
}

// read the command from the shell
char *read_input() {
    char *input = malloc(BUFFER_SIZE * sizeof(char));
    if (!input) {
        fprintf(stderr, "allocation failed\n");
        exit(EXIT_FAILURE);
    }

    if (!fgets(input, BUFFER_SIZE, stdin)) {
        free(input);        // cleanup on failure
        return NULL;
    }

    // reject the newline character and return the length and at last put null char
    input[strcspn(input, "\n")] = '\0';
    return input;
}

// function to i/o redirection (>, <)
void handle_redirection(char **args) {
    for(int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                fprintf(stderr, "Unable to open the required file\n");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
        } else if (strcmp(args[i], "<") == 0) {
            // handling for input redirection
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "Unable to open the required file\n");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[i] = NULL;
        }
    }
}

// function to check '&' presense in the command 
int check_background(char **args) {
    int i = 0;
    while(args[i] != NULL) {
        i++;
    }

    if (i > 0 && strcmp(args[i - 1], "&") == 0) {
        args[i - 1] = NULL;
        return 1;
    }

    return 0;
}

// function to split the pipeline command into two
void split_pipe(char **args, int pipe_index, char ***left, char ***right)
{
    if (pipe_index == 0 || args[pipe_index + 1] == NULL) {
        fprintf(stderr, "Invalid pipeline\n");
        return;
    }

    args[pipe_index] = NULL;

    *left  = args;
    *right = &args[pipe_index + 1];
}

// separate function to execute the pipeline command
int execute_pipeline(char **cmd1, char **cmd2) {
    int status1, status2;
    pid_t pid1, pid2;

    int fd[2];

    if (pipe(fd) < 0) {
        fprintf(stderr, "Internal Error Please try again!");
        return -1;
    } 

    pid1 = fork();
    
    if (pid1 < 0) {
        fprintf(stderr, "Could not execute!\n");
        return -1;
    }

    if (pid1 == 0) {
        // first child process
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execvp(cmd1[0], cmd1);
        fprintf(stderr, "Failure");
        return -1;
    }

    pid2 = fork();

    if (pid2 < 0) {
        fprintf(stderr, "Could not execute!\n");
        return -1;
    }

    if (pid2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[1]);
        close(fd[0]);
        execvp(cmd2[0], cmd2);
        fprintf(stderr, "Failure");
        return -1;
    }

    close(fd[0]);
    close(fd[1]);

    status1 = waitpid(pid1, NULL, 0);
    status2 = waitpid(pid2, NULL, 0);

    return 0;
}

int main(void) {
    refresh_cwd();

    signal(SIGINT, handle_signal);

    char *line;
    char *args[MAX_ARGS];

    
    while(1) {
        fprintf(stdout, PROMPT_STR, getenv("USER"), CWD);

        // read step
        line = read_input();
        
        if (line == NULL) {
            fprintf(stderr, "Invalid/ mallformed command, Please try again\n");
            continue;
        }
        
        int pipe_index = -1;

        int args_count = s_read(line, args, MAX_ARGS, &pipe_index);
        // fprintf(stdout, "%s\n", line);
        
        fprintf(stdout, "Read %d args\n", args_count);
        
        for(int i = 0; i < args_count; i++) {
            fprintf(stdout, "arg[%d]: %s\n", i, args[i]);
        }
        
        // skip empty lines
        if (args_count == 0) {
            continue;
        }
        
        // eval + processing step
        char *cmd = args[0];
        char **cmd_args = args;

        if (pipe_index != -1) {
            // pipeline flow need to fork two childs and all commands now run as external commands
            char **cmd1, **cmd2;
            split_pipe(args, pipe_index, &cmd1, &cmd2);
            // cmd1 and cmd2 are string arrays with command and arguments both
            int status = execute_pipeline(cmd1, cmd2);
            if (status == -1) {
                fprintf(stderr, "System failure exiting");
                exit(1);
            }
        } else {
            // normal execution flow execution remains same
            if (is_builtin(cmd)) {
                s_execute_builtin(cmd, cmd_args + 1, args_count - 1);
                refresh_cwd();
            } else {
                s_execute(cmd, cmd_args);
            }   
        }

        free(line);
    }

    return 0;
}