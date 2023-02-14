CC=gcc
CFLAGS=-c -Wall
LDFLAGS=-lgpiod
SOURCES=src/main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=PIC16LF1847-firmware-reader

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)
