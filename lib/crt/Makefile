CC=gcc
INC=-I../../include -I../../include/arch/i386 -I../libk/include
CCFLAGS=-Wall -c $(INC)

ASM=nasm
ASMFLAGS=-felf

crt0:
	$(CC) $(CCFLAGS) -o crt0.o crt0.c

crtbegin:
	$(ASM) $(ASMFLAGS) -o crtbegin.o crtbegin.s
	
crtend:
	$(ASM) $(ASMFLAGS) -o crtend.o crtend.s	
	