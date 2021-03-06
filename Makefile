CC = /usr/bin/cc
CFLAGS = -c -Wall -Wpedantic -Wno-overlength-strings -std=c11 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE -D_POSIX_SOURCE
LDFLAGS = -lm
RM = /bin/rm
SOURCES = packet.c net.c timer.c
SEND_SOURCES = $(SOURCES) sender.c
RECV_SOURCES = $(SOURCES) receiver.c
SEND_OBJECTS = $(SEND_SOURCES:.c=.o)
RECV_OBJECTS = $(RECV_SOURCES:.c=.o)
SEND_EXECUTABLE = sender
RECV_EXECUTABLE = receiver
EXECUTABLES = $(SEND_EXECUTABLE) $(RECV_EXECUTABLE)

all: $(EXECUTABLES)

debug: CFLAGS += -g -DDEBUG

debug: $(EXECUTABLES)

$(SEND_EXECUTABLE): $(SEND_OBJECTS) 
	$(CC) $(SEND_OBJECTS) -o $@ $(LDFLAGS)

$(RECV_EXECUTABLE): $(RECV_OBJECTS) 
	$(CC) $(RECV_OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c %.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(EXECUTABLES) $(SEND_OBJECTS) receiver.o
