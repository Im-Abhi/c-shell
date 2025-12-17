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

// tokenization method from glibc
int s_read(char *input, char **args, int max_args) {
    int i = 0;
    char *token = strtok(input, TOKEN_SEP);
    while (token != NULL && i < (max_args - 1)) {
        args[i++] = token;
        token = strtok(NULL, TOKEN_SEP);
    }

    args[i] = NULL;
    return i;
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

        int args_count = s_read(line, args, MAX_ARGS);

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

        if (is_builtin(cmd)) {
            s_execute_builtin(cmd, cmd_args + 1, args_count - 1);
            refresh_cwd();
        } else {
            s_execute(cmd, cmd_args);
        }

        free(line);
    }

    return 0;
}