################
# kOS Makefile #
################

BUILDNAME = Yggdrasil

ARCH = i386
COMPILER = gcc

SRCFILES = $(shell find kernel -mindepth 1 -maxdepth 4 -name "*.c")
HDRFILES = $(shell find kernel -mindepth 1 -maxdepth 4 -name "*.h")
ASMFILES = $(shell find kernel -mindepth 1 -maxdepth 4 -name "*.s")

OBJFILES = $(patsubst kernel/%.c,bin/%.o,$(SRCFILES))
ASMOBJS  = $(patsubst kernel/%.s,bin/%.s.o,$(ASMFILES))

## Include directories ##
INCLUDE_DIRS = -Ikernel/include -Ikernel/include/arch/$(ARCH) \
               -Ishare/include -Ishare/include/compiler/$(COMPILER)  \
               -Ilib/libc/includes -Ilib/libc/internals \
               -Ilib/libutil/include \
               -Ilib/libkos/include

ASM = nasm
ASMFLAGS = -felf

CC = i586-elf-gcc
CFLAGS = -Wall -O2 -std=gnu99 -static -c -g -DKERNEL -ffreestanding -nostdlib  \
         -nostartfiles -nodefaultlibs -fno-builtin $(INCLUDE_DIRS)
         
# using std=gnu99 to get the gnu asm statement

LD = i586-elf-ld
LDFLAGS = -Llib -static -Tlink.ld

LIBS = -lutil -lc -lutil -lkos -lgcc

QFLAGS = -m 32 -serial file:kos.log -L ../tools/qemu -no-kqemu

.PHONY: all clean version verupdate initrd floppy iso run run-iso

all: verupdate kernel linkmap.txt

clean:
	-@rm $(wildcard $(OBJFILES) $(ASMOBJS)) linkmap.txt 
	
version:
	@$(ASM) -v
	@$(CC) -v
	@$(LD) -v
	
verupdate:
	@KOS_BUILDNAME=${BUILDNAME} ./version.sh
	
stats:
	@echo  == Lines == 
	@wc -l $(SRCFILES) $(ASMFILES) $(HDRFILES) | sort
	@echo -e "\n== Characters =="
	@wc -c $(SRCFILES) $(ASMFILES) $(HDRFILES) | sort

kernel: bin/kos.bin

tf: kernel floppy run

ti: kernel iso run-iso

floppy: 
	@./cpyfiles.sh floppy
	@bfi -t=144 -f=img/kos.img tmp 
	@cmd "/C makeboot.bat img\kos.img "
	@rm -rf tmp
	
iso:
	@./cpyfiles.sh iso
	@mkisofs -R -b grldr -no-emul-boot -boot-load-size 4 -boot-info-table -o img/kos.iso tmp
	@rm -rf tmp
	
run:
	@rm -f kos*.log
	@qemu $(QFLAGS) -fda img/kos.img 
	
run-iso:
	@rm -f kos*.log
	@qemu $(QFLAGS) -cdrom img/kos.iso


bin/kos.bin: $(OBJFILES) $(ASMOBJS) 
	@$(LD) $(LDFLAGS) $(ASMOBJS) $(OBJFILES) $(LIBS) -obin/kos.bin -Map linkmap.txt
	
##
# The following target creates automatic dependency and rule scripts for all objects
##

.rules: $(SRCFILES) Makefile
	@rm -f .rules
	@$(foreach file,$(SRCFILES),\
	echo -n $(subst kernel,bin,$(dir $(file))) >> .rules;\
	$(CC) $(CFLAGS) -MM $(file) >> .rules;\
	echo -e "\t@$(CC) $(CFLAGS) -o$(patsubst kernel/%.c,bin/%.o,$(file)) $(file)" >> .rules;\
	)
	@$(foreach file,$(ASMFILES),\
	echo "$(patsubst kernel/%.s,bin/%.s.o,$(file)): $(file)" >> .rules;\
	echo -e "\t@$(ASM) $(ASMFLAGS) -o$(patsubst kernel/%.s,bin/%.s.o,$(file)) $(file)" >> .rules;\
	)

-include .rules


