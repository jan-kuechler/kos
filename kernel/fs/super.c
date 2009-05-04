#include <bitop.h>
#include <errno.h>
#include "debug.h"
#include "fs/fs.h"
#include "mm/kmalloc.h"

int fs_mount(inode_t *ino, fstype_t *type, char *device, int flags)
{
	kassert(ino);
	kassert(type);

	if (bisset(type->flags, FST_NEED_DEV) && !device) {
		return -EINVAL;
	}

	if (bnotset(ino->flags, FS_DIR )) {
		return -EINVAL;
	}

	superblock_t *sb = kmalloc(sizeof(superblock_t));
	int err = type->get_sb(sb, device, flags);
	if (err != 0)
		return err;

	bset(ino->flags, FS_MOUNTP);
	ino->link = sb->root;

	return 0;
}

int fs_umount(superblock_t *sb)
{
	kassert(sb);
	if (!sb->ops->remount)
		return -EINVAL;

	int err = sb->ops->remount(sb, FSM_UMOUNT);

	if (err != 0)
		return err;

	kfree(sb);
	return 0;
}
