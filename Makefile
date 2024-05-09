C = gcc

CFLAGS = -W -Wall -Wextra -Werror -std=c11 -pedantic -lX11 -lX11-xcb -lxkbcommon -lncurses

.PHONY: clean

all: main

main: LayoutController.c Makefile
	$(C) $(CFLAGS) LayoutController.c -o main

clean:
	rm -f main
