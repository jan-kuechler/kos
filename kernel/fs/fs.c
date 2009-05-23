#include <errno.h>
#include <string.h>
#include <kos/syscalln.h>
#include "debug.h"
#include "syscall.h"
#include "fs/fs.h"
#include "mm/util.h"
#include "util/list.h"

/* A list of all registered filesystems */
static list_t *fslist = NULL;

static struct inode root = {
	.name = "/",
	.flags = FS_DIR,
};
struct inode *fs_root = &root;
int fs_error = 0;

int vfs_geterror()
{
	return fs_error;
}

int vfs_register(struct fstype *type)
{
	if (!type)
		return -EINVAL;

	if (!fslist)
		fslist = list_create();

	list_add_back(fslist, type);

	return 0;
}

int vfs_unregister(struct fstype *type)
{
	if (!type)
		return -EINVAL;

	list_entry_t *e;
	list_iterate(e, fslist) {
		if (e->data == type) {
			list_del_entry(fslist, e);
			return 0;
		}
	}
	return -EINVAL;
}

struct fstype *vfs_gettype(char *name)
{
	if (!name)
		return -EINVAL;

	list_entry_t *e;
	list_iterate(e, fslist) {
		struct fstype *type = e->data;
		if (strcmp(type->name, name) == 0)
			return type;
	}
	return 0;
}

/* FS Syscalls */

static inline struct file *fd2file(dword fd)
{
	if (fd >= cur_proc->numfds)
		return NULL;

	return cur_proc->fds[fd];
}

int sys_open(dword calln, dword fname, dword flags, dword arg2)
{
	size_t namelen = 0;
	char *name = vm_map_string(cur_proc->pagedir, fname, &namelen);
	int result = -1;

	struct inode *inode = vfs_lookup(name, cur_proc->cwd);

	if (!inode) {
		result = vfs_geterror();
		goto end;
	}

	if (cur_proc->numfds >= PROC_NUM_FDS) {
		result = -EMFILE;
		goto end;
	}

	struct file *fíle = vfs_open(inode, flags);
	if (!file) {
		result = vfs_geterror();
		goto end;
	}

	cur_proc->fds[cur_proc->numfds] = file;
	result = cur_proc->numfds++;
end:
	km_free_addr(name, namelen);
	return result;
}

int sys_close(dword calln, dword fd, dword arg1, dword arg2)
{
	struct file *file = fd2file(fd);

	if (!file)
		return -ENOENTM

	int err = vfs_close(file);
	if (!err) {
		cur_proc->fds[fd] = NULL;
	}

	return err;
}

int sys_readwrite(dword calln, dword fd, dword buffer, dword count)
{
	void *kbuf = vm_user_to_kernel(cur_proc->pagedir, (vaddr_t)buffer, count);
	struct file *file = fd2file(fd);
	int result = -ENOSYS;

	if (!file) {
		result = -ENOENT;
		goto end;
	}

	result = (calln == SC_READ ? vfs_read : vfs_write)(file, kbuf, count, file->pos);
end:
	km_free_addr(kbuf, count);
	return result;
}

int sys_readdir(dword calln, dword fname, dword index, dword buffer)
{
	size_t namelen = 0;
	char *name = vm_map_string(cur_proc->pagedir, fname, &namelen);
	size_t buflen = 0;
	char *buf = vm_map_string(cur_proc->pagedir, buffer, &buflen);
	struct inode *inode = vfs_lookup(name, cur_proc->cwd);
	int status = -ENOSYS;

	if (!inode) {
		status = -ENOENT;
		goto end;
	}

	struct inode *entry = vfs_readdir(inode, index);

	if (!entry) {
		status = -ENOENT;
		goto end;
	}

	strncpy(buf, entry->name, buflen);
	buf[buflen] = '\0';
	status = 0;
end:
	km_free_addr(name, namelen);
	km_free_addr(buf, buflen);
	return status;
}

int sys_mount(dword calln, dword mountp, dword ftype, dword device)
{
	if (!mountp || ! ftype)
		return -EINVAL;


	char *path = safe_path(mountp);
	char *type = safe_name(ftype);

	char *dev  = NULL;
	if (device != 0)
		dev = safe_path(device);

	struct fstype *driver = fs_find_type(type);

	struct inode *inode = fs_lookup(path);

	if (!driver || !inode)
		return -EINVAL;

	return fs_mount(inode, type, device, 0);
}

void init_fs(void)
{
	syscall_register(SC_OPEN,  sys_open);
	syscall_register(SC_CLOSE, sys_close);
	syscall_register(SC_READ,  sys_read);
	syscall_register(SC_WRITE, sys_write);
	syscall_register(SC_READDIR, sys_readdir);
	syscall_register(SC_MOUNT, sys_mount);

	//init_devfs();
}

