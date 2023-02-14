CC=gcc
CFLAGS=-Wall -Werror
LDFLAGS=-lgpiod

PIC16LF1847-firmware-reader: src/main.c
    $(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

.PHONY: clean

clean:
	rm -f PIC16LF1847-firmware-reader
