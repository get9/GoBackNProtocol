CC = /usr/bin/cc
CFLAGS = -c -Wall -Wpedantic -std=c11 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE -D_POSIX_SOURCE
LDFLAGS = -lrt
RM = /bin/rm
LDFLAGS = 
SEND_SOURCES = sender.c packet.c net.c sighandler.c timer.c
SEND_OBJECTS = $(SEND_SOURCES:.c=.o)
EXECUTABLE = sender

all: $(EXECUTABLE)

debug: CFLAGS += -g -DDEBUG

debug: $(EXECUTABLE)

$(EXECUTABLE): $(SEND_OBJECTS) 
	$(CC) $(LDFLAGS) $(SEND_OBJECTS) -o $@

%.o: %.c %.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(EXECUTABLE) $(SEND_OBJECTS)
