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
	if (!name) {
		fs_error = -EINVAL;
		return NULL;
	}

	list_entry_t *e;
	list_iterate(e, fslist) {
		struct fstype *type = e->data;
		if (strcmp(type->name, name) == 0)
			return type;
	}
	fs_error = -ENOENT;
	return NULL;
}

/* FS Syscalls */

static inline struct file *fd2file(dword fd)
{
	if (fd >= cur_proc->numfds)
		return NULL;

	return cur_proc->fds[fd];
}

dword sys_open(dword calln, dword fname, dword flags, dword arg2)
{
	dbg_vprintf(DBG_FS, "sys_open(%p, %d) \n", fname, flags);

	size_t namelen = 0;
	char *name = vm_map_string(cur_proc->as->pdir, (vaddr_t)fname, &namelen);
	int result = -1;

	dbg_vprintf(DBG_FS, "name is '%s' (len: %d)\n", name, namelen);

	struct inode *inode = vfs_lookup(name, cur_proc->cwd);

	if (!inode) {
		result = vfs_geterror();
		goto end;
	}

	if (cur_proc->numfds >= PROC_NUM_FDS) {
		result = -EMFILE;
		goto end;
	}

	struct file *file = vfs_open(inode, flags);
	if (!file) {
		result = vfs_geterror();
		goto end;
	}

	cur_proc->fds[cur_proc->numfds] = file;
	result = cur_proc->numfds++;
end:
	km_free_addr(name, namelen);
	return (dword)result;
}

dword sys_close(dword calln, dword fd, dword arg1, dword arg2)
{
	struct file *file = fd2file(fd);

	if (!file)
		return (dword)-ENOENT;

	int err = vfs_close(file);
	if (!err) {
		cur_proc->fds[fd] = NULL;
	}

	return (dword)err;
}

dword sys_readwrite(dword calln, dword fd, dword buffer, dword count)
{
	void *kbuf = vm_user_to_kernel(cur_proc->pagedir, (vaddr_t)buffer, count);
	struct file *file = fd2file(fd);
	int result = -ENOSYS;

	if (!file) {
		result = -ENOENT;
		goto end;
	}

	if (calln == SC_READ)
		result = vfs_read(file, kbuf, count, file->pos);
	else
		result = vfs_write(file, kbuf, count, file->pos);

end:
	if (result != -EAGAIN)
		km_free_addr(kbuf, count);
	return (dword)result;
}

dword sys_readdir(dword calln, dword fname, dword index, dword buffer)
{
	size_t namelen = 0;
	char *name = vm_map_string(cur_proc->pagedir, (vaddr_t)fname, &namelen);
	size_t buflen = 0;
	char *buf = vm_map_string(cur_proc->pagedir, (vaddr_t)buffer, &buflen);
	struct inode *inode = vfs_lookup(name, cur_proc->cwd);
	int status = -ENOSYS;

	if (!inode) {
		status = -ENOENT;
		goto end;
	}

	struct dirent *entry = vfs_readdir(inode, index);

	if (!entry) {
		status = -ENOENT;
		goto end;
	}

	size_t len = buflen < FS_MAX_NAME ? buflen : FS_MAX_NAME;
	strncpy(buf, entry->name, len);
	buf[len] = '\0';
	status = 0;
end:
	km_free_addr(name, namelen);
	km_free_addr(buf, buflen);
	return (dword)status;
}

dword sys_mount(dword calln, dword mountp, dword ftype, dword device)
{
	if (!mountp || !ftype)
		return (dword)-EINVAL;

	size_t pathlen = 0;
	char *path = vm_map_string(cur_proc->pagedir, (vaddr_t)mountp, &pathlen);
	size_t typelen = 0;
	char *type = vm_map_string(cur_proc->pagedir, (vaddr_t)ftype, &typelen);

	size_t devlen = 0;
	char *dev  = NULL;
	if (device != 0)
		dev = vm_map_string(cur_proc->pagedir, (vaddr_t)device, &devlen);

	struct fstype *driver = vfs_gettype(type);
	struct inode *inode = vfs_lookup(path, cur_proc->cwd);

	int err = 0;

	if (!driver || !inode) {
		err = -EINVAL;
		goto end;
	}

	err = vfs_mount(driver, inode, dev, 0);

end:
	km_free_addr(path, pathlen);
	km_free_addr(type, typelen);
	if (dev)
		km_free_addr(dev, devlen);
	return (dword)err;
}

void init_fs(void)
{
	dbg_printf(DBG_FS, "\nRegistering fs syscalls... ");

	syscall_register(SC_OPEN,  sys_open);
	syscall_register(SC_CLOSE, sys_close);
	syscall_register(SC_READ,  sys_readwrite);
	syscall_register(SC_WRITE, sys_readwrite);
	//syscall_register(SC_READDIR, sys_readdir);
	//syscall_register(SC_MOUNT, sys_mount);

	dbg_printf(DBG_FS, "done\n");

	//init_devfs();
}

