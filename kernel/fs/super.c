#include <bitop.h>
#include <errno.h>
#include "debug.h"
#include "fs/fs.h"
#include "mm/kmalloc.h"

int fs_mount(struct inode *ino, struct fstype *type, char *device, int flags)
{
	if (!ino) return -EINVAL;
	if (!type) return -EINVAL;

	if (bisset(type->flags, FST_NEED_DEV) && !device) {
		return -EINVAL;
	}

	if (bnotset(ino->flags, FS_DIR )) {
		return -EINVAL;
	}

	struct superblock *sb = kmalloc(sizeof(struct superblock));
	int err = type->get_sb(sb, device, flags);
	if (err != 0)
		return err;

	bset(ino->flags, FS_MOUNTP);
	ino->link = sb->root;

	return 0;
}

int fs_umount(struct superblock *sb)
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
