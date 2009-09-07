NAME = crt0.o

AR = i586-elf-ar

AS = i586-elf-as
ASFLAGS = 

all: $(NAME)

$(NAME): crt0.s
	@$(AS) $(ASFLAGS) -o$(NAME) crt0.s
	
install:
	@cp -f $(NAME) ..
	
install-shared:
	@cp -f $(NAME) ../../share/i586-elf/lib
	
clean:
	@rm -f $(NAME)
