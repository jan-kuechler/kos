CC = kgcc
CFLAGS = -Wall 

all: echo echo2 echo3

echo: main.c
	@$(CC) $(CFLAGS) -oecho main.c
	
echo2: main.c
	@$(CC) $(CFLAGS) -oecho2 -DV2 main.c

echo3: main.c
	@$(CC) $(CFLAGS) -oecho3 -DV3 main.c

install:
	@cp echo ../../initrd/bin
	@cp echo2 ../../initrd/bin
	@cp echo3 ../../initrd/bin

clean:
	rm -f echo echo2
