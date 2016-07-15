#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "tokenizer.h"
#include "job_control.h"

void cmd_launch(struct tokens *tokens, const char* line) {
    if (tokens_get_token(tokens, 0) == NULL) return;
    /* put the new job in the job list tail. */
    job *j = first_job;
    while (j->next != NULL) j = j->next;
    j = j->next = (job*)malloc(sizeof(job));
    j->next = NULL; j->pgid = 0; j->stdin = 0; j->stdout = 1; j->stderr = 2;
    j->command = line;

    /* build process list */
    process head, *p = &head;
    int len = tokens_get_length(tokens), i, k;
    char *args[len+1]; args[len] = NULL;
    for (i = 0, k = 0; i < len; i++) {
        args[i] = tokens_get_token(tokens, i);
        if (strcmp(args[i], "|") == 0) {
            args[i] = NULL;
            p = p->next = (process*)calloc(1, sizeof(process));
            p->argv = (args+k);
            k = i+1;
        }
    }
    p = p->next = (process*)calloc(1, sizeof(process));
    p->argv = (args+k);
    int foreground = 1;
    if (strcmp(args[len-1], "&") == 0) {
        foreground = 0;
        args[len-1] = NULL;
    }
    j->first_process = head.next;
    launch_job(j, foreground);
}
