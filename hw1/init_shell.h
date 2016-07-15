#pragma once

#include <termios.h>
#include <unistd.h>
#include <stdbool.h>

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

/* Intialization procedures for this shell */
void init_shell();

/* A process is a single process.  */
typedef struct process {
  struct process *next;
  char **argv;
  pid_t pid;
  char completed;
  char stopped;
  int status;
} process;

/* A job is a pipeline of processes.  */
typedef struct job {
  struct job *next;
  const char *command;
  process *first_process;
  pid_t pgid;
  //char notified;           //true if user told about stopped job
  struct termios tmodes;   //saved terminal modes
  int stdin, stdout, stderr;  //standard i/o channels
} job;
/* The active jobs are linked into a list. This is head. */
job *first_job;

/* Find the active job with the indicated pgid.  */
job *find_job (pid_t pgid);

/* Return true if all processes in the job have stopped or completed.  */
int job_is_stopped (job* j);

/* Return true if all processes in the job have completed.  */
int job_is_completed (job *j);

/* Clean memory */
void job_destroy();
