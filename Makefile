####################
# Makefile for kOS #
####################

######### osdev environment ######### 
OSDEV=/c/osdev
TOOLS=$(OSDEV)/tools

LUA=lua.exe
PRINT_LUA=$(TOOLS)/print.lua

######### constants #########
ARCH=i386

BIN_DIR=bin
INC_DIR=include
LIB_DIR=lib

ARCH_INC=$(INC_DIR)/arch/$(ARCH)

KERNEL_DIR=kernel
KERNEL_INC=$(KERNEL_DIR)/include

FS_DIR=$(KERNEL_DIR)/fs
MM_DIR=$(KERNEL_DIR)/mm

ASM=nasm
ASM_FLAGS=-felf

CC=gcc
CC_INC= -I$(INC_DIR) -I$(ARCH_INC) -I$(KERNEL_INC)
CC_FLAGS=-O3 -static -c -g -ffreestanding -nostdlib -nostartfiles -nodefaultlibs $(CC_INC) -Wall

LD=ld
LD_FLAGS=-L$(LIB_DIR) -static -Tlink.ld

LIBS=-lminc -lk 

######### targets ##########

all: kernel link_map

test: all floppy bochs

version:
	$(ASM) -v
	$(CC)  -v
	$(LD)  -v
	
## Makefile generation ##
prepare: targets objlist

targets:
	rm -f kernel.target
	for F in $(KERNEL_DIR)/*.c; do $(LUA) $(PRINT_LUA) bin/ >> kernel.target && $(CC) $(CC_INC) -MM $$F >> kernel.target && $(LUA) $(PRINT_LUA) !tab "$(CC) $(CC_FLAGS) -o \$$@ $$<" !nl >> kernel.target; done
	for F in $(FS_DIR)/*.c; do $(LUA) $(PRINT_LUA) bin/ >> kernel.target && $(CC) $(CC_INC) -MM $$F >> kernel.target && $(LUA) $(PRINT_LUA) !tab "$(CC) $(CC_FLAGS) -o \$$@ $$<" !nl >> kernel.target; done
	for F in $(MM_DIR)/*.c; do $(LUA) $(PRINT_LUA) bin/ >> kernel.target && $(CC) $(CC_INC) -MM $$F >> kernel.target && $(LUA) $(PRINT_LUA) !tab "$(CC) $(CC_FLAGS) -o \$$@ $$<" !nl >> kernel.target; done
	
.PHONY: objlist
objlist:
	rm -f .objlist
	$(LUA) $(TOOLS)/makeobjlist.lua .objlist bin/ kernel.target
	
-include kernel.target
-include .objlist
	
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
	
## Image creation ##

floppy: 
	./cpyfiles.sh floppy
	bfi -t=144 -f=img/kos.img tmp -b=../tools/grub/grldr.mbr
	cmd "/C makeboot.bat img\kos.img "
	rm -rf tmp
	
iso:
	./cpyfiles.sh iso
	mkisofs -R -b grldr -no-emul-boot -boot-load-size 4 -boot-info-table -o img/kos.iso tmp
	rm -rf tmp
	
run:
	qemu -m 16 -L ../tools/qemu -fda img/kos.img
	
run-iso:
	qemu -m 16 -L ../tools/qemu -cdrom img/kos.iso

dbg:
	qemu -m 16 -S -s -L ../tools/qemu -fda img/kos.img
	
bochs:
	bochs

## Clean ##

clean:
	rm -f $(BIN_DIR)/*.o
	rm -f link.map
	
cleanall: clean
	rm -f $(BIN_DIR)/kos.bin
	rm -f $(BIN_DIR)/*.mod
	rm -f $(BIN_DIR)/*.a
	rm -f img/kos.img
	rm -f img/kos.iso
	rm -f *.target
	rm -f .objlist
