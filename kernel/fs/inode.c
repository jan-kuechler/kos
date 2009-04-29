#include <bitop.h>
#include <errno.h>
#include "debug.h"
#include "fs/fs.h"

int fs_open(inode_t *inode, dword flags)
{
	kassert(inode);

	if (inode->ops && inode->ops->open)
		return inode->ops->open(inode, flags);
	return -EINVAL;
}

int fs_close(inode_t *inode)
{
	kassert(inode);

	if (inode->ops && inode->ops->close)
		return inode->ops->close(inode);
	return -EINVAL;
}

int fs_read(inode_t *inode, dword offset, void *buffer, dword size)
{
	kassert(inode);

	if (inode->ops && inode->ops->read)
		inode->ops->read(inode, offset, buffer, size);
	return -EINVAL;
}

int fs_write(inode_t *inode, dword offset, void *buffer, dword size)
{
	kassert(inode);

	if (inode->ops && inode->ops->write)
		inode->ops->write(inode, offset, buffer, size);
	return -EINVAL;
}

int fs_mknod(inode_t *inode, char *name, dword flags)
{
	kassert(inode);

	if (bisset(inode->flags, FS_DIR) && inode->ops && inode->ops->mknod)
		return inode->ops->mknod(inode, name, flags);
	return -EINVAL;
}

dirent_t *fs_readdir(inode_t *inode, dword index)
{
	kassert(inode);

	if (bisset(inode->flags, FS_DIR) && inode->ops && inode->ops->readdir)
		return inode->ops->readdir(inode, index);

	return NULL;
}

inode_t *fs_finddir(inode_t *inode, char *name)
{
	kassert(inode);

	if (bisset(inode->flags, FS_DIR) && inode->ops && inode->ops->finddir)
		return inode->ops->finddir(inode, name);

	return NULL;
}
