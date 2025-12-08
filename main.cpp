#include <stdio.h>
#include <string>
#include <cstring>
#include <unistd.h> // Required for fork() and execv()
#include <sys/wait.h> // Required for waitpid()

#include "./lib/linenoise.h"

#define PROMPT "$ "
#define HISTORY_LENGTH 1024
#define MAX_ARGS 64
#define TOKEN_SEP " \t"

using namespace std;

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

int main(void) {
    if(!linenoiseHistorySetMaxLen(HISTORY_LENGTH)) {
        fprintf(stderr, "Could not set linenoise history!");
        exit(1);
    }

    char *line;
    char *args[MAX_ARGS];

    while((line = linenoise(PROMPT)) != NULL) {

        // read step
        int args_count = s_read(line, args, MAX_ARGS);

        fprintf(stdout, "Read %d args\n", args_count);

        for(int i = 0; i < args_count; i++) {
            fprintf(stdout, "arg[%d]: %s\n", i, args[i]);
        }

        // skip empty lines
        if (args_count == 0) {
            linenoiseFree(line);
            continue;
        }

        // TODO: eval + processing step
        char *cmd = args[0];
        char **cmd_args = args;

        s_execute(cmd, cmd_args);

        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }


    return 0;
}