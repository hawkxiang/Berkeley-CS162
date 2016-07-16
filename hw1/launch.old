#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "tokenizer.h"
// struct obsolute path.
char *absolute_path(char *filename) {
    char *path = getenv("PATH"), *dir = strtok(path, ":");
    while (dir != NULL && openat(open(dir, O_RDONLY), filename, O_RDONLY) == -1)
        dir = strtok(NULL, ":");
    if (dir != NULL) {
        path = (char*)malloc(strlen(filename)+strlen(dir)+2);
        strcpy(path, dir); strcat(path, "/"); strcat(path, filename);
        return path;
    } else 
	return NULL;
}

int cmd_launch(struct tokens *tokens) {
  pid_t pid;
  int status;
  
  //flush cache
  fflush(0);
  pid = fork();
  if (pid == 0) {
    //struct process args & handle redirection
    int len = tokens_get_length(tokens), i;
    char *args[len+1]; args[len] = NULL;
    char *input = NULL, *output = NULL;
    for (i = 0; i < len; i++) {
        args[i] = tokens_get_token(tokens, i);
        if (strcmp(args[i], "<") == 0) {
           args[i] = NULL;
           input = tokens_get_token(tokens, i+1);;
        } else if (strcmp(args[i], ">") == 0) {
           args[i] = NULL;
           output = tokens_get_token(tokens, i+1);
        }
    }

    if (input != NULL) {
        int fd_in;
        if ((fd_in = open(input, O_RDONLY, 0)) == -1) {
           perror("Couldn't open input file!");
           exit(EXIT_FAILURE);
        }
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }
    if (output != NULL) {
        int fd_out;
        if ((fd_out = open(output, O_WRONLY|O_APPEND|O_CREAT, 0644)) == -1) {
           perror("Couldn't open output file!");
           exit(EXIT_FAILURE);
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }
    
    //handle obsolute path.
    char *path = args[0];
    if (strstr(path, "/") == NULL)
        path = absolute_path(args[0]);

    //not use PATH value
    if (execv(path, args) == -1)
        perror("sh");
    free(path);
    exit(EXIT_FAILURE);
  } else if (pid < 0)
	perror("sh");
  else {
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }
  return 1;
}
