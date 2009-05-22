#include <bitop.h>
#include <errno.h>
#include "debug.h"
#include "fs/fs.h"
#include "mm/kmalloc.h"

int vfs_mount(struct fstype *type, struct inode *point, char *device, dword flags)
{
	if (!type || !point || bnotset(point->flags, FS_DIR))
		return -EINVAL;

	if (bisset(type->flags, FST_NEED_DEV) && !device)
		return -EINVAL;

	struct superblock *sb = kmalloc(sizeof(*sb));
	int err = type->mount(sb, device, flags);
	if (err != 0) {
		kfree(sb);
		return err;
	}

	bset(point->flags, FS_MOUNTP);
	ino->link = sb->root;

	return 0;
}

int vfs_umount(struct inode *ino)
{
	if (!ino || bnotset(ino->flags, FS_MOUNTP) || !ino->link)
		return -EINVAL;

	struct superblock *sb = ino->link->sb;
	if (!sb->ops || !sb->ops->remount)
		return -ENOSYS;

	int err = sb->ops->remount(sb, FSM_UMOUNT);

	if (err != 0)
		return err;

	kfree(sb);

	bclr(ino->flags, FS_MOUNTP);
	ino->link = NULL;

	return 0;
}
