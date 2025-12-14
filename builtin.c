#include "builtin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char CWD[4096];

void Builtin_impl_cd(char **args, size_t n_args) {
    char *new_dir = *args;
    if (args[1] == NULL) {
        fprintf(stderr, "expected argument to \"cd\"\n");
        return;
    }

    if (chdir(new_dir) != 0) {
        fprintf(stderr, "Could not change directory to '%s'\n", new_dir);
        return; // Do NOT exit the whole shell!
    }
}

void Builtin_impl_pwd(char **args, size_t n_args) {
    if (getcwd(CWD, sizeof(CWD)) != NULL) {
        printf("%s\n", CWD);
    } else {
        perror("pwd");
    }
}

void Builtin_impl_exit(char **args, size_t n_args) {
    exit(0);
}

void (*BUILTIN_TABLE[])(char **, size_t) = {
    [BUILTIN_CD]   = Builtin_impl_cd,
    [BUILTIN_PWD]  = Builtin_impl_pwd,
    [BUILTIN_EXIT] = Builtin_impl_exit
};

Builtin builtin_code(char *cmd) {
    if (strcmp(cmd, "cd") == 0)   return BUILTIN_CD;
    if (strcmp(cmd, "pwd") == 0)  return BUILTIN_PWD;
    if (strcmp(cmd, "exit") == 0) return BUILTIN_EXIT;
    return BUILTIN_INVALID;
}

int is_builtin(char *cmd) {
    return builtin_code(cmd) != BUILTIN_INVALID;
}

void s_execute_builtin(char *cmd, char **args, size_t n_args) {
    BUILTIN_TABLE[builtin_code(cmd)](args, n_args);
}
