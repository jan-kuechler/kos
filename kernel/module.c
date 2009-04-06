#include "debug.h"
#include "kernel.h"
#include "mm/virt.h"

static byte *loaded;

void init_mod()
{
	/* map the module list into the kernel pagedir */
	km_identity_map(multiboot_info.mods_addr, VM_COMMON_FLAGS,
	                multiboot_info.mods_count * sizeof(multiboot_mod_t));

	loaded = kmalloc(multiboot_info.mods_count);
}

void mod_load(int n)
{
	if (n >= multiboot_info.mods_count || n < 0) {
		panic("mod_load: module #%d does not exist.", n);
	}

	if (loaded[n])
		return;

	dbg_printf(DBG_MODULE, "Loading module %d.\n", n);

	return;

	multiboot_mod_t *mod = multiboot_info.mods_addr + n * sizeof(multiboot_mod_t);

	dbg_vprintf(DBG_MODULE, "Mapping: 0x%x and 0x%x\n", mod->mod_start, mod->cmdline);
	km_identity_map(mod->mod_start, VM_COMMON_FLAGS, mod->mod_end - mod->mod_start);
	km_identity_map(mod->cmdline, VM_COMMON_FLAGS, 4096);

	elf_load(mod->mod_start, mod->cmdline);

	km_free_addr(mod->mod_start, mod->mod_end - mod->mod_start);
	km_free_addr(mod->cmdline, 4096);

	loaded[n] = 1;
}

void mod_load_all()
{
	int i=0;
	for (; i < multiboot_info.mods_count; ++i) {
		mod_load(i);
	}
}
