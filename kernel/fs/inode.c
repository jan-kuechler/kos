#include <bitop.h>
#include <errno.h>
#include "debug.h"
#include "fs/fs.h"

#define ret_null_and_err(err) do { fs_error = err; return NULL; } while (0);

#define inode_has_op(ino, op) ((ino)->oos && (ino)->ops->op)
#define file_has_op(file, op) ((file)->fops && (file)->fops->op)

struct inode *vfs_create(struct inode *dir, char *name, dword flags)
{
	if (!inode || !name)
		ret_null_and_err(-EINVAL);

	if (bnotset(inode->flags, FS_DIR))
		ret_null_and_err(-ENODIR);

	if (inode->ops && inode->ops->create)
		return inode->ops->create(dir, name, flags);

	ret_null_and_err(-ENOSYS);
}

int vfs_unlink(struct inode *ino)
{
	if (!ino)
		return -EINVAL;

	if (ino->opencount)
		return -EAGAIN;

	if (ino->ops && ino->ops->unlink)
		return ino->ops->unlink(ino);

	return -ENOSYS;
}

struct file *vfs_open(struct inode *ino, dword flags)
{
	if (!ino)
		ret_null_and_err(-EINVAL);

	struct file *file = kmalloc(sizeof(*file));
	memset(file, 0, sizeof(*file));

	if (ino->ops && ino->ops->open) {
		int status = ino->ops->open(ino, file, flags);
		if (status != 0) {
			kfree(file);
			ret_null_and_err(status);
		}
		return file;
	}

	ret_null_and_err(-EINVAL);
}

int vfs_close(struct file *file)
{
	if (!file)
		return -EINVAL;

	int status = -ENOSYS;
	if (file_has_op(file, close)) {
		status = file->fops->close(file);
		if (status == 0)
			kfree(file);
	}
	return status;
}

int vfs_read(struct file *file, void *buffer, dword count, dword offset)
{
	if (!file)
		return -EINVAL;

	if (file_has_op(file, read))
		file->fops->read(file, buffer, count, offset);
	return -ENOSYS;
}

int vfs_write(struct file *file, void *buffer, dword count, dword offset)
{
	if (!file)
		return -EINVAL;

	if (file_has_op(file, write))
		file->fops->write(file, buffer, count, offset);
	return -ENOSYS;
}

int vfs_read_async(struct request *rq)
{
	if (!rq || ! rq->file)
		return -EINVAL;M

	if (file_has_op(rq->file, read_async))
		return rq->file->fops->read_async(rq);
	return -ENOSYS;
}

int vfs_write_async(struct request *rq)
{
	if (!rq || ! rq->file)
		return -EINVAL;M

	if (file_has_op(rq->file, write_async))
		return rq->file->fops->write_async(rq);
	return -ENOSYS;

}

struct dirent *vfs_readdir(struct inode *ino, dword index)
{
	if (!file)
		ret_null_and_err(-EINVAL);

	if (bnotset(ino->flags, FS_DIR))
		ret_null_and_err(-EINVAL);

	if (inode_has_op(ino, readdir))
		return ino->ops->readdir(ino, index);

	ret_null_and_err(-ENOSYS);
}

struct inode *vfs_finddir(struct inode *ino, char *name)
{
	if (!ino)
		ret_null_and_err(-EINVAL);

	if (bnotset(ino->flags, FS_DIR))
		ret_null_and_err(-EINVAL);

	if (inode_has_op(ino, finddir))
		return ino->ops->finddir(ino, name);

	ret_null_and_err(-ENOSYS);
}
