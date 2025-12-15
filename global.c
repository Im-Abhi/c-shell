#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "global.h"

void refresh_prompt(void) {
    snprintf(PROMPT, PROMPT_MAX, "\033[32m%s\033[0m$ ", CWD);
}

void refresh_cwd(void) {
    if (getcwd(CWD, PATH_MAX) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    refresh_prompt();
}
