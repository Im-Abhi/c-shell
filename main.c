#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> // Required for fork() and execv()
#include <sys/wait.h> // Required for waitpid()

#include "./lib/linenoise.h"

char PROMPT[8192];

#define HISTORY_LENGTH 1024
#define MAX_ARGS 64
#define TOKEN_SEP " \t"
#define PATH_MAX 4096

char CWD[PATH_MAX];

typedef enum Builtin {
    CD,
    PWD,
    INVALID
} Builtin;

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

void Builtin_impl_cd(char **args, size_t n_args) {
    char *new_dir = *args;
    if(chdir(new_dir) != 0) {
        fprintf(stderr, "Could not change directory to '%s'\n", new_dir);
        exit(1);
    }

    refresh_cwd();
}

void Builtin_impl_pwd(char **args, size_t n_args) {
    fprintf(stdout, "%s\n", CWD);
}

void (*BUILTIN_TABLE[]) (char **args, size_t n_args) = {
    [CD] = Builtin_impl_cd,
    [PWD] = Builtin_impl_pwd
};

Builtin builtin_code(char *cmd) {
    if (strcmp(cmd, "cd") == 0) {
        return CD;
    } else if (strcmp(cmd, "pwd") == 0) {
        return PWD;
    } else {
        return INVALID;
    }
}

int is_builtin(char *cmd) {
    return builtin_code(cmd) != INVALID;
}

// from command to builtin code command
// BUILTIN_TABLE[builtin_code(cmd)] -> function pointer
// use the function pointer with arguments args, n_args
void s_execute_builtin(char *cmd, char **args, size_t n_args) {
    BUILTIN_TABLE[builtin_code(cmd)](args, n_args);
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

int main(void) {
    refresh_cwd();

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

        // eval + processing step
        char *cmd = args[0];
        char **cmd_args = args;

        if (is_builtin(cmd)) {
            s_execute_builtin(cmd, cmd_args + 1, args_count - 1);
        } else {
            s_execute(cmd, cmd_args);
        }

        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }

    return 0;
}