#include <string.h>
#include "debug.h"
#include "kernel.h"
#include "mm/kmalloc.h"
#include "mm/virt.h"

static byte *loaded;

void init_mod()
{
	/* map the module list into the kernel pagedir */
	km_identity_map((paddr_t)multiboot_info.mods_addr, VM_COMMON_FLAGS,
	                multiboot_info.mods_count * sizeof(multiboot_mod_t));

	loaded = kmalloc(multiboot_info.mods_count);
	memset(loaded, 0, multiboot_info.mods_count);
}

void mod_load(int n)
{
	if (n >= multiboot_info.mods_count || n < 0) {
		panic("mod_load: module #%d does not exist.", n);
	}

	if (loaded[n])
		return;

	dbg_printf(DBG_MODULE, "Loading module %d.\n", n);

	multiboot_mod_t *mod = (multiboot_mod_t*)multiboot_info.mods_addr + n * sizeof(multiboot_mod_t);

	dbg_vprintf(DBG_MODULE, "  Mapping module: 0x%08x\n", mod->mod_start);
	vaddr_t mod_start = km_alloc_addr((paddr_t)mod->mod_start, VM_COMMON_FLAGS, mod->mod_end - mod->mod_start);
	dbg_vprintf(DBG_MODULE, "  Mapping cmdline: 0x%08x\n", mod->cmdline);
	vaddr_t cmdline = km_alloc_addr((paddr_t)mod->cmdline, VM_COMMON_FLAGS, 1024);

	dbg_vprintf(DBG_MODULE, "  Loading elf module\n");
	elf_load(mod_start, (char*)cmdline, (pid_t)1);

	dbg_vprintf(DBG_MODULE, "  Freeing module & cmdline...\n");
	km_free_addr(mod_start, mod->mod_end - mod->mod_start);
	km_free_addr(cmdline, 1024);

	dbg_printf(DBG_MODULE, "Module loaded.\n");

	loaded[n] = 1;
}

void mod_load_all()
{
	int i=0;
	for (; i < multiboot_info.mods_count; ++i) {
		mod_load(i);
	}
}
