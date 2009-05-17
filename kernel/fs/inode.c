#include <bitop.h>
#include <errno.h>
#include "debug.h"
#include "fs/fs.h"

int fs_open(struct inode *inode, dword flags)
{
	if (!inode) return -EINVAL;

	if (inode->ops && inode->ops->open)
		return inode->ops->open(inode, flags);
	return -EINVAL;
}

int fs_close(struct inode *inode)
{
	if (!inode) return -EINVAL;

	if (inode->ops && inode->ops->close)
		return inode->ops->close(inode);
	return -EINVAL;
}

int fs_read(struct inode *inode, void *buffer, dword size, dword offset)
{
	if (!inode) return -EINVAL;

	if (inode->ops && inode->ops->read)
		inode->ops->read(inode, offset, buffer, size);
	return -EINVAL;
}

int fs_write(struct inode *inode, void *buffer, dword size, dword offset)
{
	if (!inode) return -EINVAL;

	if (inode->ops && inode->ops->write)
		inode->ops->write(inode, offset, buffer, size);
	return -EINVAL;
}

int fs_mknod(struct inode *inode, char *name, dword flags)
{
	if (!inode) return -EINVAL;

	if (bisset(inode->flags, FS_DIR) && inode->ops && inode->ops->mknod)
		return inode->ops->mknod(inode, name, flags);
	return -EINVAL;
}

dirent_t *fs_readdir(struct inode *inode, dword index)
{
	if (!inode) return -EINVAL;

	if (bisset(inode->flags, FS_DIR) && inode->ops && inode->ops->readdir)
		return inode->ops->readdir(inode, index);

	return NULL;
}

struct inode *fs_finddir(struct inode *inode, char *name)
{
	if (!inode) return -EINVAL;

	if (bisset(inode->flags, FS_DIR) && inode->ops && inode->ops->finddir)
		return inode->ops->finddir(inode, name);

	return NULL;
}
