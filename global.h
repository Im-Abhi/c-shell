#ifndef GLOBAL_H
#define GLOBAL_H

#define PATH_MAX 4096
#define PROMPT_MAX 8192

extern char CWD[PATH_MAX];
extern char PROMPT[PROMPT_MAX];

void refresh_cwd(void);
void refresh_prompt(void);

#endif
