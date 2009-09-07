NAME = template

CC = i586-elf-gcc 
CFLAGS = -Wall

LD = i586-elf-ld
LDFLAGS = -L../../share/lib -L../../share/i586-elf/lib -Tkos.ld

SRCFILES = $(wildcard *.c)
OBJFILES = $(patsubst %.c,%.o,$(SRCFILES))

all: $(NAME)
	
$(NAME): $(OBJFILES)
	@$(LD) $(LDFLAGS) -o$(NAME) $(OBJFILES) ../../share/lib/crt0.o -lkos -lc -lkos

install:
	@cp $(NAME) ../../initrd/bin

clean:
	@rm -f $(NAME)

%.o: %.c
	@$(CC) $(CFLAGS) -o$@ -c $<
