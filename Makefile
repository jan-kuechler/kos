####################
# Makefile for kOS #
####################

###############
# Environment #
###############

OSDEV=/c/osdev
TOOLS=$(OSDEV)/tools

LUA=lua.exe
PRINT_LUA=$(TOOLS)/print.lua

#############
# Constants #
#############
ARCH=i386

BIN_DIR=bin
INC_DIR=include

ARCH_INC=$(INC_DIR)/arch/$(ARCH)

KERNEL_DIR=kernel
KERNEL_INC=$(KERNEL_DIR)/include

FS_DIR=$(KERNEL_DIR)/fs
MM_DIR=$(KERNEL_DIR)/mm

LIB_DIR=lib

CC=gcc
CC_INC= -I$(INC_DIR) -I$(ARCH_INC) -I$(KERNEL_INC)
CC_FLAGS=-O3 -static -c -g -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CC_INC) -Wall

LD=ld
LD_FLAGS=-L$(LIB_DIR) -static -Tlink.ld

LIBS=-lminc -lk 

ASM=nasm
ASM_FLAGS=-felf

###########
# Targets #
###########

all: kernel link_map

version:
	$(ASM) -v
	$(CC)  -v
	$(LD)  -v
	
## Makefile generation ##
prepare: targets objlist

targets: ktar

ktar:
	rm -f kernel.target
	for F in $(KERNEL_DIR)/*.c; do $(LUA) $(PRINT_LUA) bin/ >> kernel.target && $(CC) $(CC_INC) -MM $$F >> kernel.target && $(LUA) $(PRINT_LUA) !tab "$(CC) $(CC_FLAGS) -o \$$@ $$<" !nl >> kernel.target; done
	for F in $(FS_DIR)/*.c; do $(LUA) $(PRINT_LUA) bin/ >> kernel.target && $(CC) $(CC_INC) -MM $$F >> kernel.target && $(LUA) $(PRINT_LUA) !tab "$(CC) $(CC_FLAGS) -o \$$@ $$<" !nl >> kernel.target; done
	for F in $(MM_DIR)/*.c; do $(LUA) $(PRINT_LUA) bin/ >> kernel.target && $(CC) $(CC_INC) -MM $$F >> kernel.target && $(LUA) $(PRINT_LUA) !tab "$(CC) $(CC_FLAGS) -o \$$@ $$<" !nl >> kernel.target; done
	
.PHONY: objlist
objlist:
	rm -f .objlist
	$(LUA) $(TOOLS)/makeobjlist.lua .objlist bin/ kernel.target
	
# Provides rules to make the kernel
-include kernel.target

# Provides $(OBJS), which includes all objects from the above files
-include .objlist

## Image creation ##

.PHONY: floppy
floppy: 
	rm -rf tmp
	mkdir tmp
	cp menu-floppy.lst tmp/menu.lst
	cp ../tools/grub/grldr tmp
	cp $(BIN_DIR)/kos.bin tmp
	cp $(BIN_DIR)/test.mod tmp
	bfi -t=144 -f=img/floppy.img tmp -b=../tools/grub/grldr.mbr
	cmd "/C makeboot.bat img\floppy.img "
	rm -rf tmp
	
.PHONY: iso
iso:
	rm -rf tmp
	mkdir tmp
	cp menu-iso.lst tmp/menu.lst
	cp $(BIN_DIR)/kos.bin tmp
	cp ../tools/grub/grldr tmp
	mkisofs -R -b grldr -no-emul-boot -boot-load-size 4 -boot-info-table -o img/kos.iso tmp
	rm -rf tmp
	
.PHONY: run
run:
	qemu -m 16 -L ../tools/qemu -fda img/floppy.img
	
run-iso:
	qemu -m 16 -L ../tools/qemu -cdrom img/kos.iso

dbg:
	qemu -m 16 -S -s -L ../tools/qemu -fda img/floppy.img 
	
## Rest ##

# Note: kstart.o _must_ be the first object to be linked!
ASM_OBJS = $(BIN_DIR)/kstart.o $(BIN_DIR)/int.o 
ALL_OBJS = $(ASM_OBJS) $(OBJS)

kernel: link

link: $(ALL_OBJS)
	$(LD) $(LD_FLAGS) -o$(BIN_DIR)/kos.bin $(ALL_OBJS) $(LIBS)
	
link_map:
	$(LD) $(LD_FLAGS) -o$(BIN_DIR)/kos.bin $(ALL_OBJS) $(LIBS) -Map link.map
		
$(BIN_DIR)/kstart.o: $(KERNEL_DIR)/kstart.s
	$(ASM) $(ASM_FLAGS) -o $(BIN_DIR)/kstart.o $(KERNEL_DIR)/kstart.s
	
$(BIN_DIR)/int.o: $(KERNEL_DIR)/int.s
	$(ASM) $(ASM_FLAGS) -o $(BIN_DIR)/int.o $(KERNEL_DIR)/int.s

## Clean ##

clean:
	rm -f $(BIN_DIR)/*.o
	rm -f link.map
	
cleanall: clean
	rm -f $(BIN_DIR)/kos.bin
	rm -f img/floppy.img
	rm -f *.target
	rm -f .objlist
