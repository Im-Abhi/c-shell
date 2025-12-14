#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> // Required for fork() and execv()
#include <sys/wait.h> // Required for waitpid()

#include "builtin.h"

char PROMPT[8192];

#define BUFFER_SIZE 1024
#define HISTORY_LENGTH 1024
#define MAX_ARGS 64
#define TOKEN_SEP " \t"
#define PATH_MAX 4096

char CWD[PATH_MAX];

void refresh_prompt(void) {
    snprintf(PROMPT, sizeof(PROMPT),
             "\033[32m%s\033[0m$ ", CWD);
}

void refresh_cwd(void) {
    if (getcwd(CWD, sizeof(CWD)) == NULL) {
        fprintf(stderr,"Error: could not read working directory");
        exit(1);
    }

    refresh_prompt();
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

int s_execute(char *cmd, char ** cmd_args) {
    fprintf(stdout, "Executing '%s'!\n", cmd);

    int status;
    pid_t pid;

    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Could not execute!\n");
        return -1;
    }

    if (pid == 0) {
        // child
        execvp(cmd, cmd_args);
    } else {
        // parent wait for child
        if (waitpid (pid, &status, 0) != pid) {
            fprintf(stderr, "Could not wait for child!\n");
            return -1;
        }
    }
    return status;
}

char *read_input() {
    char *input = malloc(BUFFER_SIZE * sizeof(char));
    if (!input) {
        fprintf(stderr, "allocation failed\n");
        exit(EXIT_FAILURE);
    }

    fgets(input, BUFFER_SIZE, stdin);
    input[strcspn(input, "\n")] = '\0';
    return input;
}

int main(void) {
    refresh_cwd();

    char *line;
    char *args[MAX_ARGS];

    while(1) {
        fprintf(stdout, "\033[1;32;40m %s $\033[0m: ", CWD);
        line = read_input();
        // read step
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
        } else {
            s_execute(cmd, cmd_args);
        }
    }

    return 0;
}