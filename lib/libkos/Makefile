NAME = libkos.a

AR = i586-elf-ar

CC = i586-elf-gcc
CFLAGS = -Wall -O2 -static -g -Iinclude -I../../share/include 

SRCFILES = $(wildcard *.c)
OBJFILES = $(patsubst %.c,%.o,$(SRCFILES))

all: $(NAME)

$(NAME): $(OBJFILES)
	@rm -f $(NAME)
	@$(AR) rc $(NAME) $(OBJFILES)
	
install:
	@cp -f $(NAME) ..
	
install-shared:
	@cp -f $(NAME).a ../../share/lib
	
clean:
	@rm -rf *.o
	@rm -f $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) -o$@ -c $<
