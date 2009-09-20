#include <bitop.h>
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "fs/fs.h"
#include "fs/request.h"
#include "mm/kmalloc.h"

#define ret_null_and_err(err) do { fs_error = err; return NULL; } while (0);

#define inode_has_op(ino, op) ((ino)->ops && (ino)->ops->op)
#define file_has_op(file, op) ((file)->fops && (file)->fops->op)

#define FILE_OP(file,op) ((file)->fops->op ? (file)->fops->op : def_##op)

static int def_dup(struct file *file, struct file *newf)
{
	newf->inode = file->inode;
	newf->pos = file->pos;
	newf->fops = file->fops;
	return 0;
}

struct inode *vfs_create(struct inode *dir, char *name, dword flags)
{
	if (!dir || !name)
		ret_null_and_err(-EINVAL);

	if (bnotset(dir->flags, FS_DIR))
		ret_null_and_err(-EINVAL);

	if (inode_has_op(dir, create))
		return dir->ops->create(dir, name, flags);

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
		ino->opencount++;
		file->inode = ino;
		return file;
	}

	ret_null_and_err(-EINVAL);
}

int vfs_close(struct file *file)
{
	if (!file)
		return -EINVAL;

	int err = -ENOSYS;
	if (file_has_op(file, close)) {
		err = file->fops->close(file);
		if (!err) {
			file->inode->opencount--;
			kfree(file);
		}
	}
	return err;
}

struct file *vfs_dup(struct file *file)
{
	if (!file)
		ret_null_and_err(-EINVAL);

	struct file *newf = kmalloc(sizeof(*file));
	memset(newf, 0, sizeof(*newf));

	int err = FILE_OP(file, dup)(file, newf);
	if (err) {
		kfree(file);
		ret_null_and_err(err);
	}
	return newf;
}

int vfs_read(struct file *file, void *buffer, dword count, dword offset)
{
	if (!file)
		return -EINVAL;

	if (file_has_op(file, read))
		return file->fops->read(file, buffer, count, offset);
	return -ENOSYS;
}

int vfs_write(struct file *file, void *buffer, dword count, dword offset)
{
	if (!file)
		return -EINVAL;

	if (file_has_op(file, write))
		return file->fops->write(file, buffer, count, offset);
	return -ENOSYS;
}

int vfs_read_async(struct request *rq)
{
	if (!rq || ! rq->file)
		return -EINVAL;

	if (file_has_op(rq->file, read_async))
		return rq->file->fops->read_async(rq);
	return -ENOSYS;
}

int vfs_write_async(struct request *rq)
{
	if (!rq || ! rq->file)
		return -EINVAL;

	if (file_has_op(rq->file, write_async))
		return rq->file->fops->write_async(rq);
	return -ENOSYS;
}

struct dirent *vfs_readdir(struct inode *ino, dword index)
{
	if (!ino)
		ret_null_and_err(-EINVAL);

	if (bnotset(ino->flags, FS_DIR))
		ret_null_and_err(-EINVAL);

	if (inode_has_op(ino, readdir)) {
		return ino->ops->readdir(ino, index);
	}

	ret_null_and_err(-ENOSYS);
}

struct inode *vfs_finddir(struct inode *ino, char *name)
{
	dbg_vprintf(DBG_FS, "vfs_finddir(inode: '%s', '%s')\n", ino->name, name);

	if (!ino) {
		dbg_vprintf(DBG_FS, " no inode\n");
		ret_null_and_err(-EINVAL);
	}

	if (bnotset(ino->flags, FS_DIR)) {
		dbg_vprintf(DBG_FS, "  no directory\n");
		ret_null_and_err(-EINVAL);
	}

	if (inode_has_op(ino, finddir)) {
		dbg_vprintf(DBG_FS, " calling inodes finddir...\n");
		return ino->ops->finddir(ino, name);
	}

	ret_null_and_err(-ENOSYS);
}
