NAME = template

CC = i586-elf-gcc 
CFLAGS = -Wall

TESTCC = mingw32-gcc
TESTCFLAGS = -Wall -DTEST

SRCFILES = $(wildcard *.c)

all: $(NAME)

$(NAME):
	@$(CC) $(CLFAGS) -o$(NAME) $(SRCFILES) -lkos

install:
	@cp $(NAME) ../../initrd/bin

clean:
	@rm -f $(NAME)
