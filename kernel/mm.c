#include "kernel.h"
#include <multiboot.h>
#include "mm.h"´
#include <string.h>

static unsigned int *mmap;
static unsigned int mmap_length;

struct memblock
{
	dword start, end;
}

static void mark(struct memblock *blocks, int *nblocks, dword start, dword end)
{
	int i = 0;

	start = PAGE_ALIGN_ROUND_UP(start);
	end   = PAGE_ALIGN_ROUND_DOWN(end);

	for (; i < *nblocks; ++i) {
		if (blocks[i].start == start) {
			if (blocks[i].end == end) {  // it's a whole block
				int j=i;

				*nblocks--;
				for (; j < *nblocks; ++j) {
					blocks[j].start = blocks[j+1].start;
					blocks[j].end   = blocks[j+1].end;

				}
				break;
			}
			else if (blocks[i].end > end) {
				blocks[i].start = end;
				break;
			}
		}
		else if (blocks[i].start < start) {
			if (blocks[i].end == end) {
				blocks[i].end = start;
				break;
			}
			else if (blocks[i].end > end) {
				blocks[*nblocks].start = end;
				blocks[*nblocks].end   = blocks[i].end;

				blocks[i].end = start;

				*nblocks++;

				break;
			}
		}
	}
}

void init_mm(void)
{
	multiboot_mmap_t *mb_mmap = 0;
	unsigned int mb_mmap_length = multiboot_info.mmap_length;
	struct memblock blocks[32] = {0};
	int nblocks = 0;

	int i = 0;
	for (mb_mmap = (multiboot_mmap_t*) multiboot_info.mmap_addr;
			 mb_mmap < (multiboot_mmap_t*) multiboot_info.mmap_addr + mb_mmap_length;
			 mb_mmap = (multiboot_mmap_t*) ((char*)mb_mmap + mb_mmap->size - 4)) {


		block[nblocks].start = PAGE_ALIGN_ROUND_UP(mb_mmap->base_addr);
		block[nblocks].end   = PAGE_ALIGN_ROUND_DOWN(mb_mmap->base_addr + mb_mmap->length);
		++nblock;
	}

	mark(blocks, &nblocks, (dword)kernel_phys_start, (dword)kernel_phys_end);

	for (i=0; i < multiboot_info.mods_count; ++i) {
		multiboot_mod_t *mod = (multiboot_mod_t*)((char*)multiboot_info.mods_addr + (i * sizeof(multiboot_mod_t)));
		mark(blocks, &nblocks, mod->start, mod->end);
		mark(blocks, &nblocks, mod->string, mod->string + strlen(mod->string) + 1);
	}

	dword upper_end = 0;
	for (i=0; i < nblocks; ++i) {
		if (blocks[i].end > upper_end)
			upper_end = blocks[i].end;
	}

	mmap_length = upper_end / PAGE_SIZE / (sizeof(unsigned int) * 8);
	for (i=0; i < nblocks; ++i) {
		if (blocks[i].start && ((blocks[i].end - blocks[i].start) >= (mmap_length * 4))) {
			mmap = (unsigned int*)blocks[i].start;
			blocks[i].start = PAGE_ALIGN_ROUND_UP(blocks[i].start + mmap_length * 4);
			break;
		}
	}

	/* TODO: mark free / used in bitmap */
}
