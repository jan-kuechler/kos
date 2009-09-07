NAME = mkid.exe

CC=gcc
CFLAGS=-g -Wall 

all: $(NAME)

$(NAME): main.c initrd.c initrd.h
	$(CC) $(CFLAGS) -o$(NAME) main.c initrd.c
	
install: $(NAME)
	cp $(NAME) ../../bin
