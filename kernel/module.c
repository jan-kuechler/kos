#include <string.h>
#include "debug.h"
#include "kernel.h"
#include "mm/kmalloc.h"
#include "mm/virt.h"

static size_t *size;
static void  **addr;
static char  **cmdline;

void init_mod(void)
{
	if (!multiboot_info.mods_count)
		return;

	dword count = multiboot_info.mods_count;

	km_identity_map((paddr_t)multiboot_info.mods_addr, VM_COMMON_FLAGS,
	                count * sizeof(multiboot_mod_t));

	size    = kmalloc(count * sizeof(size_t));
	addr    = kmalloc(count * sizeof(void *));
	cmdline = kmalloc(count * sizeof(char *));

	memset(size,    0, count);
	memset(addr,    0, count);
	memset(cmdline, 0, count);
}

size_t mod_load(int n, void **module, char **params)
{
	if (n >= multiboot_info.mods_count || n < 0) {
		panic("mod_load: module #%d does not exist.", n);
	}

	if (!size[n]) {
		multiboot_mod_t *mod = (multiboot_mod_t*)multiboot_info.mods_addr +
		                                         n * sizeof(multiboot_mod_t);

		size[n] = mod->mod_end - mod->mod_start;
		addr[n] = km_alloc_addr((paddr_t)mod->mod_start, VM_COMMON_FLAGS,
		                        mod->mod_end - mod->mod_start);
		cmdline[n] = (char*)km_alloc_addr((paddr_t)mod->cmdline,
		                                  VM_COMMON_FLAGS, 1024);
	}

	if (module)
		*module = addr[n];

	if (params)
		*params = cmdline[n];

	return size[n];
}

size_t mod_size(int n)
{
	if (n >= multiboot_info.mods_count || n < 0) {
		panic("mod_size: module #%d does not exist.", n);
	}

	return size[n];
}
