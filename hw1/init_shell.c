#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "init_shell.h"

void init_shell() {
    /* Our shell is connected to standard input. */
    shell_terminal = STDIN_FILENO;

    /* Check if we are running interactively */
    shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive) {
        /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
         * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
         *  foreground, we'll receive a SIGCONT. */
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
            kill(-shell_pgid, SIGTTIN);

         /* Ignore interactive and job-contol signals by hawk*/
         signal (SIGINT, SIG_IGN);
         signal (SIGTSTP, SIG_IGN);
         signal (SIGTTIN, SIG_IGN);
         signal (SIGTTOU, SIG_IGN);
         signal (SIGCHLD, SIG_IGN);


         /* Saves the shell's process id */
         shell_pgid = getpid();
         //put Ourselves in out own process group
         if (setpgid(shell_pgid, shell_pgid) < 0) {
             perror("Couldn't put the shell in its own process group");
             exit(1);
         }

         /* Take control of the terminal */
         tcsetpgrp(shell_terminal, shell_pgid);

         /* Save the current termios to a variable, so it can be restored later. */
         tcgetattr(shell_terminal, &shell_tmodes);
    }
}


job *find_job (pid_t pgid) {
    job *j;
    for (j = first_job; j; j = j->next)
        if (j->pgid == pgid)
            return j;
    return NULL;
}

int job_is_stopped(job *j)
{
    process *p;
    for (p = j->first_process; p; p = p->next)
        if (!p->completed && !p->stopped)
            return 0;
    return 1;
}

int job_is_completed(job *j)
{
    process *p;
    for (p = j->first_process; p; p = p->next)
        if (!p->completed) return 0;
    return 1;
}

void job_destroy() {
    job *pre = first_job, *j; 
    process *p, *tmp;
    while (pre != NULL){
        j = pre->next;
        if (j != NULL && job_is_completed(j)){
            for (p = j->first_process; p;) {
		tmp = p;
		p = p->next;
		free(tmp);
	    }
            pre->next = j->next;
            free(j);
        } else 
            pre = pre->next;
    }
}
