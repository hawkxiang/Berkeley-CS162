SRCS=init_shell.c shell.c tokenizer.c launch.c job_control.c
EXECUTABLES=shell

CC=gcc
CFLAGS=-g -Wall -std=gnu99
LDFLAGS=

OBJS=$(SRCS:.c=.o)

all: $(EXECUTABLES)

$(EXECUTABLES): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(EXECUTABLES) $(OBJS)
