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

LIBC_DIR=libc
LIBC_INC=$(LIBC_DIR)/include

LIBK_DIR=libk
LIBK_INC=$(LIBK_DIR)/include

CC=gcc
CC_INC= -I$(INC_DIR) -I$(ARCH_INC) -I$(KERNEL_INC) -I$(LIBC_INC) -I$(LIBK_INC)
CC_FLAGS=-O3 -c -g -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CC_INC)

LD=ld
LD_FLAGS=-T link.ld 

ASM=nasm
ASM_FLAGS=-felf

###########
# Targets #
###########

all: kernel

version:
	$(ASM) -v
	$(CC)  -v
	$(LD)  -v
	
## Makefile generation ##
prepare: targets objlist

targets: ktar lctar lktar

ktar:
	rm -f kernel.target
	for F in $(KERNEL_DIR)/*.c; do $(LUA) $(PRINT_LUA) bin/ >> kernel.target && $(CC) $(CC_INC) -MM $$F >> kernel.target && $(LUA) $(PRINT_LUA) !tab "$(CC) $(CC_FLAGS) -o \$$@ $$<" !nl >> kernel.target; done
	for F in $(FS_DIR)/*.c; do $(LUA) $(PRINT_LUA) bin/ >> kernel.target && $(CC) $(CC_INC) -MM $$F >> kernel.target && $(LUA) $(PRINT_LUA) !tab "$(CC) $(CC_FLAGS) -o \$$@ $$<" !nl >> kernel.target; done
	for F in $(MM_DIR)/*.c; do $(LUA) $(PRINT_LUA) bin/ >> kernel.target && $(CC) $(CC_INC) -MM $$F >> kernel.target && $(LUA) $(PRINT_LUA) !tab "$(CC) $(CC_FLAGS) -o \$$@ $$<" !nl >> kernel.target; done
	
lctar:
	rm -f libc.target
	for F in $(LIBC_DIR)/*.c; do $(LUA) $(PRINT_LUA) bin/ >> libc.target && $(CC) $(CC_INC) -MM $$F >> libc.target && $(LUA) $(PRINT_LUA) !tab "$(CC) $(CC_FLAGS) -o \$$@ $$<" !nl >> libc.target; done
	
lktar:
	rm -f libk.target
	for F in $(LIBK_DIR)/*.c; do $(LUA) $(PRINT_LUA) bin/ >> libk.target && $(CC) $(CC_INC) -MM $$F >> libk.target && $(LUA) $(PRINT_LUA) !tab "$(CC) $(CC_FLAGS) -o \$$@ $$<" !nl >> libk.target; done

.PHONY: objlist
objlist:
	rm -f .objlist
	$(LUA) $(TOOLS)/makeobjlist.lua .objlist bin/ kernel.target libc.target libk.target
	
# Provides rules to make the kernel
-include kernel.target

# Provides rules to make the lib files
-include libc.target
-include libk.target

# Provides $(OBJS), which includes all objects from the above files
-include .objlist

## Image creation ##

.PHONY: floppy
floppy: 
	rm -rf tmp
	mkdir tmp
	cp menu.lst.floppy tmp/menu.lst
	cp ../tools/grub/grldr tmp
	cp $(BIN_DIR)/kos.bin tmp
	bfi -t=144 -f=img/floppy.img tmp -b=../tools/grub/grldr.mbr
	cmd "/C makeboot.bat img\floppy.img "
	rm -rf tmp
	
.PHONY: run
run:
	qemu -m 16 -L ../tools/qemu -fda img/floppy.img

dbg:
	qemu -m 16 -S -s -L ../tools/qemu -fda img/floppy.img 
	
## Rest ##

# Note: kstart.o _must_ be the first object to be linked!
ASM_OBJS = $(BIN_DIR)/kstart.o $(BIN_DIR)/int.o 
ALL_OBJS = $(ASM_OBJS) $(OBJS)

kernel: link

link: $(ALL_OBJS)
	$(LD) $(LD_FLAGS) -o $(BIN_DIR)/kos.bin $(ALL_OBJS)
	
$(BIN_DIR)/kstart.o: $(KERNEL_DIR)/kstart.s
	$(ASM) $(ASM_FLAGS) -o $(BIN_DIR)/kstart.o $(KERNEL_DIR)/kstart.s
	
$(BIN_DIR)/int.o: $(KERNEL_DIR)/int.s
	$(ASM) $(ASM_FLAGS) -o $(BIN_DIR)/int.o $(KERNEL_DIR)/int.s

## Clean ##

clean:
	rm -f $(BIN_DIR)/*.o
	rm -f link.map
	rm -f *.target
	rm -f .objlist
	
cleanall: clean
	rm -f $(BIN_DIR)/kos.bin
	rm -f img/floppy.img

link_map:
	$(LD) $(LD_FLAGS) -o $(BIN_DIR)/kos.bin $(ALL_OBJS) -Map link.map
	
