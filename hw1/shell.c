#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "init_shell.h"
#include "tokenizer.h"
#include "launch.h"


int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);
int cmd_wait(struct tokens *tokens);
int cmd_fg(struct tokens *tokens);
int cmd_bg(struct tokens *tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_pwd, "pwd", "get the current working directory"},
  {cmd_cd, "cd", "change the current working directory to that directory"},
  {cmd_wait, "wait", "wait the background jobs complement"},
  {cmd_fg, "fg", "put job in foreground"},
  {cmd_bg, "bg", "continue background job"},
};

/* Prints a helpful description for the given command */
int cmd_help(struct tokens *tokens) {
  for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(struct tokens *tokens) {
  exit(0);
}

int cmd_wait(struct tokens *tokens) {
    job_wait();
    return 1;
}

int cmd_pwd(struct tokens *tokens) {
  const int FILEPATH_MAX=80;
  char file_path[FILEPATH_MAX];
  getcwd(file_path, FILEPATH_MAX);
  printf("%s\n", file_path);
  return 1;
}
#define HOME "/home/vagrant"
int cmd_cd(struct tokens *tokens) {
  char *dir;
  if ((dir = tokens_get_token(tokens, 1)) == NULL || strcmp(dir, "~") == 0)
    /*fprintf(stderr, "sh:  expected argument to \"cd\"\n");*/
    dir = HOME;
  if (chdir(dir) != 0)
      perror("sh");
  return 1;
}

int cmd_fg(struct tokens *tokens) {
    char *num;
    if ((num = tokens_get_token(tokens, 1)) == NULL)
        switch_fg(-1);
    else 
        switch_fg(atoi(num));
    return 1;
}
int cmd_bg(struct tokens *tokens) {
    char *num;
    if ((num = tokens_get_token(tokens, 1)) == NULL)
        switch_bg(-1);
    else 
        switch_bg(atoi(num));
    return 1;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
/* init_shell implement move to init_shell.h and init_shell.c */
int main(int argc, char *argv[]) {
  init_shell();
  static char line[4096];
  int line_num = 0;
  first_job = (job *)calloc(1, sizeof(job));

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "SHELL %d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE this to run commands as programs. */
      //fprintf(stdout, "This shell doesn't know how to run programs.\n");
      cmd_launch(tokens, line);
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "SHELL %d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
    job_destroy();
  }

  return 0;
}
