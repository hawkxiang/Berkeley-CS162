#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "tokenizer.h"
#include "launch.h"

void cmd_launch(struct tokens *tokens, const char* line) {
    if (tokens_get_token(tokens, 0) == NULL) return;
    /* put the new job in the job list tail. */
    job *j = first_job;
    while (j->next != NULL) j = j->next;
    j = j->next = (job*)malloc(sizeof(job));
    j->next = NULL; j->pgid = 0; j->stdin = 0; j->stdout = 1; j->stderr = 2;
    j->command = line; tcgetattr(shell_terminal, &j->tmodes);

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
    update_status();
}

void job_wait() {
    wait_background();
}
void switch_fg(pid_t pgid) {
    job *j, *i;
    if (pgid > 0)
        j = find_job(pgid);
    else {
        for (i = first_job; i->next; i = i->next);
        j = i;
    }
    if (j != NULL)
        continue_job(j, 1);
}

void switch_bg(pid_t pgid) {
    job *j, *i;
    if (pgid > 0)
        j = find_job(pgid);
    else {
        for (i = first_job->next; i; i = i->next)
            if (job_is_stopped(i)) j = i;
    }
    if (j != NULL)
        continue_job(j, 0);

}
