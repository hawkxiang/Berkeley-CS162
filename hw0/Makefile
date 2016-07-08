CC = gcc                                                                  
CFLAGS = -g -c
LDFLAGS =
SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = main

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -g -o $@

clean:
	-rm main *.o

.PHONY: clean
 
include $(SOURCES:.c=.d)
 
%.d: %.c
	set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
 
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

