#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "tokenizer.h"
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
  
  pid = fork();
  if (pid == 0) {
    int len = tokens_get_length(tokens), i;
    char *args[len+1]; args[len] = NULL;
    for (i = 0; i < len; i++)
       args[i] = tokens_get_token(tokens, i);
    char *path = args[0];
    // struct obsolute path.
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
