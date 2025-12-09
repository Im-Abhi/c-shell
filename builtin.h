#pragma once

#include <stddef.h>

void refresh_cwd(void);

typedef enum Builtin {
    BUILTIN_CD,
    BUILTIN_PWD,
    BUILTIN_EXIT,
    BUILTIN_INVALID
} Builtin;

Builtin builtin_code(char *cmd);
int is_builtin(char *cmd);
void s_execute_builtin(char *cmd, char **args, size_t n_args);

void Builtin_impl_cd(char **args, size_t n_args);
void Builtin_impl_pwd(char **args, size_t n_args);
void Builtin_impl_exit(char **args, size_t n_args);
