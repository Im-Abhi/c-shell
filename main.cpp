#include <stdio.h>
#include <string>
#include <cstring>

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

int main(void) {
    if(!linenoiseHistorySetMaxLen(HISTORY_LENGTH)) {
        fprintf(stderr, "Could not set linenoise history!");
        exit(1);
    }

    char *line;
    char *args[MAX_ARGS];

    while((line = linenoise(PROMPT)) != NULL) {

        // read step
        int args_count =  s_read(line, args, MAX_ARGS);

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


        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }


    return 0;
}