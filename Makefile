CC = /usr/bin/cc
CFLAGS = -c -Wall -Wpedantic -std=c11 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE -D_POSIX_SOURCE -lrt
RM = /bin/rm
LDFLAGS = 
SEND_SOURCES = sender.c packet.c net.c sighandler.c timer.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = sender

all: $(EXECUTABLE)

debug: CFLAGS += -g -DDEBUG

debug: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.c %.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(EXECUTABLE) $(OBJECTS)
