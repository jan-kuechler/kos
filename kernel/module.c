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

	multiboot_mod_t *mod = multiboot_info.mods_addr + n * sizeof(multiboot_mod_t);

	km_identity_map(mod->mod_start, VM_COMMON_FLAGS, mod->mod_end - mod->mod_start);
	km_identity_map(mod->cmdline, VM_COMMON_FLAGS, 4096);

	elf_load(mod->mod_start, mod->cmdline);

	loaded[n] = 1;
}

void mod_load_all()
{
	int i=0;
	for (; i < multiboot_info.mods_count; ++i) {
		mod_load(i);
	}
}
