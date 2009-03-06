/* multiboot.h - the header for Multiboot */
/* Copyright (C) 1999, 2001  Free Software Foundation, Inc.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <types.h>

/* The magic number for the Multiboot header. */
#define MULTIBOOT_HEADER_MAGIC          0x1BADB002

/* Types. */

/* The Multiboot header. */
typedef struct multiboot_header
{
	dword magic;
	dword flags;
	dword checksum;
	dword header_addr;
	dword load_addr;
	dword load_end_addr;
	dword bss_end_addr;
	dword entry_addr;
} multiboot_header_t;

/* The symbol table for a.out. */
typedef struct aout_symbol_table
{
	dword tabsize;
	dword strsize;
	dword addr;
	dword reserved;
} aout_symbol_table_t;

/* The section header table for ELF. */
typedef struct elf_section_header_table
{
	dword num;
	dword size;
	dword addr;
	dword shndx;
} elf_section_header_table_t;

/* The Multiboot information. */
typedef struct multiboot_info
{
	dword flags;
	dword mem_lower;
	dword mem_upper;
	dword boot_device;
	dword cmdline;
	dword mods_count;
	dword mods_addr;
	union
	{
		aout_symbol_table_t aout_sym;
		elf_section_header_table_t elf_sec;
	} u;
	dword mmap_length;
	dword mmap_addr;
} multiboot_info_t;

/* The module structure. */
typedef struct multiboot_mod
{
	dword mod_start;
	dword mod_end;
	dword string;
	dword reserved;
} multiboot_mod_t;

/* The memory map. Be careful that the offset 0 is base_addr_low
	but no size. */
typedef struct multiboot_mmap
{
	dword size;
	qword base_addr;
	qword length;
	dword type;
} multiboot_mmap_t;

#endif /*MULTIBOOT_H*/
