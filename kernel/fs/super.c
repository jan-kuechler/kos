#include <errnno.h>
#include "fs/fs.h"
#include "utils/list.h"

static list_t *superblocks = 0;


int fs_mount(fstype_t *type, char *device, int flags)
{
	if (bisset(type->flags, FST_NEED_DEV) && !device) {
		return -EINVAL;
	}

	if (!superblocks) {
		superblocks = list_create();
	}

	superblock_t *sb = kmalloc(sizeof(superblock_t));
	int result = type->get_sb(sb, device, flags);
	if (result != 0)
		return result;

	list_add_before(superblocks, sb);
}
