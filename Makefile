CC=cc
CFLAGS=-Wall -Werror

acpic: acpic.c
	$(CC) $(CFLAGS) -o acpic acpic.c

install: acpic
	install -m 755 acpic /usr/bin

