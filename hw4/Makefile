SRC = .
MAIN_SRC = main
CFLAGS = -std=gnu99 -ggdb3 -Wall -I$(SRC)
MKDIR_P = mkdir -p

LINKFLAGS = -lpthread
BIN = bin

SRCS = $(wildcard *.c)
MAIN_SRCS = $(wildcard $(MAIN_SRC)/*.c)

OBJS = $(SRCS:.c=.o) index.o
MAIN_OBJS = $(MAIN_SRCS:.c=.o)

all: $(BIN)/tpcfollower $(BIN)/tpcleader
	ln -sf ../$(MAIN_SRC)/tpcsystem bin/tpcsystem

$(BIN)/%: $(MAIN_SRC)/%.o $(OBJS)
	$(MKDIR_P) $(BIN)
	$(CC) $(OBJS) $< $(LINKFLAGS) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.o: %.S
	$(CC) -c $(CFLAGS) $< -o $@

index.o: index.h index.S index.html

clean:
	rm -f *.o $(MAIN_SRC)/*.o
	rm -rf $(BIN)

.PHONY: all clean
