#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "job_control.h"

/* simple andle IO redirection */
void io_redirection(char** args) {
    int redirect_in, redirect_out;
    while (*args != NULL) {
       if (strcmp(*args, "<") == 0) {
            *args = NULL;
            if ((redirect_in = open(*(args+1), O_RDONLY, 0)) == -1) {
                perror ("Counldn't open input file!");
                exit(EXIT_FAILURE);
            }
            dup2(redirect_in, STDIN_FILENO);
            close(redirect_in);
            break;
       } else if (strcmp(*args, ">") == 0) {
            *args = NULL;
            if ((redirect_out = open(*(args+1), O_WRONLY|O_APPEND|O_CREAT, 0644)) == -1) {
            perror("Couldn't open output file!");
            exit(EXIT_FAILURE);
            }
            dup2(redirect_out, STDOUT_FILENO);
            close(redirect_out);
            break;
       }
       args++;
    }
}

void launch_process (process *p, pid_t pgid,
            int infile, int outfile, int errfile, int foreground) {
    pid_t pid;

    /* Put the process into the process group and give the process group
     * the terminal, if appropriate.
     * This has to be done both by the shell and in the individual
     * child processes because of potential race conditions.*/
    if (shell_is_interactive) {
        pid = getpid();
        if (pgid == 0) pgid = pid;
        /* set pgid and foreground in subprocess to avoid race conditions */
        setpgid (pid, pgid);
        if (foreground)
            tcsetpgrp(shell_terminal, pgid);

        signal (SIGINT, SIG_DFL);
        signal (SIGQUIT, SIG_DFL);
        signal (SIGTSTP, SIG_DFL);
        signal (SIGTTIN, SIG_DFL);
        signal (SIGTTOU, SIG_DFL);
        signal (SIGCHLD, SIG_DFL);
    }
    io_redirection(p->argv);   
    /* Set the standard input/output channels of the new process.  */
    if (infile != STDIN_FILENO) {
        dup2(infile, STDIN_FILENO);
        close(infile);
    }
    if (outfile != STDOUT_FILENO) {
        dup2 (outfile, STDOUT_FILENO);
        close (outfile);
    }
    if (errfile != STDERR_FILENO) {
        dup2(errfile, STDERR_FILENO);
        close (errfile);
    }
    /* Exec the new process.  Make sure we exit.  */
    execvp (p->argv[0], p->argv);
    perror ("execvp");
    exit (1);
}

void put_job_in_foreground(job* j, int cont);
void put_job_in_background(job* j, int cont);
void wait_for_job(job *j);
void format_job_info (job *j, const char *status);

void launch_job (job *j, int foreground) {
    pid_t pid;
    process *p;
    int mypipe[2], infile, outfile;

    infile = STDIN_FILENO;
    for (p = j->first_process; p; p = p->next) {
        if (p->next) {
            /* need pipe to implement "|", create pipe for every process */
            if (pipe(mypipe) < 0){
                perror ("pipe");
                exit(1);
            }
            outfile = mypipe[1];
        } else 
            outfile = STDOUT_FILENO;

        /* Fork the child processes.  */
        pid = fork();
        if (pid == 0)
            launch_process(p, j->pgid, infile, outfile, j->stderr, foreground);
        else if (pid < 0) {
            perror ("fork");
            exit (1);
        } else {
            p->pid = pid;
            /* set pgid in parent to avoid race conditions */
            if (shell_is_interactive) {
                if (j->pgid == 0)
                    j->pgid = pid;
                setpgid (pid, j->pgid);
            }
        }

        /* Clean up after pipes. */
        if (infile != j->stdin)
            close (infile);
        if (outfile != j->stdout)
            close (outfile);
        infile = mypipe[0];
    }
    
    format_job_info (j, "launched");
    if (!shell_is_interactive)
        wait_for_job(j);
    /* set foreground in parent to avoid race conditions */
    else if (foreground)
        put_job_in_foreground(j, 0);
    else
        put_job_in_background(j, 0);
}

/* Put job j in the foreground.  If cont is nonzero,
 * restore the saved terminal modes and send the process group a
 * SIGCONT signal to wake it up before we block.  */
void put_job_in_foreground (job* j, int cont) {
    /* Put the job into the foreground.  */
    tcsetpgrp (shell_terminal, j->pgid);
    tcgetattr(shell_terminal, &shell_tmodes);
    tcsetattr(shell_terminal, TCSADRAIN, &j->tmodes);
    /* Send the job a continue signal, if necessary */
    if (cont) 
        /* boardcast to process in 'gpid' process group */
        if (kill(-j->pgid, SIGCONT) < 0)
            perror("kill (SIGCONT)");
    /* Wait for it to report.  */
    wait_for_job(j);
    /* Put the shell back in the foreground.  */
    tcsetpgrp(shell_terminal, shell_pgid);
    /* Restore the shell’s terminal modes.  */
    tcgetattr(shell_terminal, &j->tmodes);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
}

void put_job_in_background (job *j, int cont){
    if (cont)
        if (kill(-j->pgid, SIGCONT) < 0)
            perror("kill (SIGCONT)");
    //wait_for_job(j, 0);
}


/* Store the status of the process pid that was returned by waitpid.
 *    Return 0 if all went well, nonzero otherwise.  */
int mark_process_status (pid_t pid, int status) {
    job* j;
    process *p;
    if (pid > 0) {
        for (j = first_job->next; j; j=j->next)
            for (p = j->first_process; p; p = p->next)
                if (p->pid == pid) {
                    p->status = status;
                    if (WIFSTOPPED(status))
                        p->stopped = 1;
                    else {
                        p->completed = 1;
                        if (WIFSIGNALED(status))
                            fprintf(stderr, "%d: Terminated by signal %d.\n",
                                (int)pid, WTERMSIG(p->status));
                    }
                 return 0;
            }
        fprintf(stderr, "No child process %d.\n", pid);
    }
    else if (pid == 0 || errno == ECHILD)
        return -1;
    else{
        perror("waitpid");
        return -1;
    }
    return -1;
}

void format_job_info (job *j, const char *status) {
   fprintf (stderr, "%ld (%s): %s\n", (long)j->pgid, status, j->command);
}

void wait_for_job(job *j) {
    int status;
    pid_t pid;

    do
        //pid = waitpid(WAIT_ANY, &status, WUNTRACED);
        pid = waitpid(-j->pgid, &status, WUNTRACED);
    while (!mark_process_status(pid, status)
            && !job_is_stopped(j)
            && !job_is_completed(j));
}

void wait_background() {
    job *j;
    for (j = first_job->next; j; j = j->next)
        if (!job_is_completed(j))
            wait_for_job(j);
}

void update_status() {
    int status;
    pid_t pid;
    
    do
        pid = waitpid(WAIT_ANY, &status, WUNTRACED|WNOHANG);
    while (!mark_process_status(pid, status));
}

void mark_job_as_running(job* j) {
    process *p;
    for (p = j->first_process; p; p=p->next)
        p->stopped = 0;
}

void continue_job (job* j, int foreground) {
    mark_job_as_running(j);
    if (foreground)
        put_job_in_foreground(j, 1);
    else
        put_job_in_background(j, 1);
}

