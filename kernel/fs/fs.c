#include <errno.h>
#include <string.h>
#include <kos/syscalln.h>
#include "debug.h"
#include "syscall.h"
#include "tty.h"
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
int fs_error = -ENOSYS;

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
	if (fd >= syscall_proc->numfds)
		return NULL;

	return syscall_proc->fds[fd];
}

int32_t sys_open(int32_t fname, int32_t flags)
{
	dbg_vprintf(DBG_FS, "sys_open(%p, %d) \n", fname, flags);

	size_t namelen = 0;
	char *name = vm_map_string(syscall_proc->as->pdir, (vaddr_t)fname, &namelen);
	int result = -1;

	dbg_printf(DBG_SC, "sys_open(\"%s\", %d)\n", name, flags);

	struct inode *inode = vfs_lookup(name, syscall_proc->cwd);

	if (!inode) {
		result = vfs_geterror();
		goto end;
	}

	if (syscall_proc->numfds >= PROC_NUM_FDS) {
		result = -EMFILE;
		goto end;
	}

	struct file *file = vfs_open(inode, flags);
	if (!file) {
		result = vfs_geterror();
		goto end;
	}

	syscall_proc->fds[syscall_proc->numfds] = file;
	result = syscall_proc->numfds++;
end:
	km_free_addr(name, namelen);

	dbg_printf(DBG_SC, "sys_open: result is %d\n", result);

	return result;
}

int32_t sys_close(int32_t fd)
{
	struct file *file = fd2file(fd);

	if (!file)
		return (dword)-ENOENT;

	int err = vfs_close(file);
	if (!err) {
		syscall_proc->fds[fd] = NULL;
	}

	return err;
}

int32_t sys_readwrite(int32_t fd, int32_t buffer, int32_t count)
{
	void *kbuf = vm_user_to_kernel(syscall_proc->as->pdir, (vaddr_t)buffer, count);
	struct file *file = fd2file(fd);
	int result = -ENOSYS;

	if (!file) {
		result = -ENOENT;
		goto end;
	}

	if (syscall_proc->sc_regs->eax == SC_READ)
		result = vfs_read(file, kbuf, count, file->pos);
	else
		result = vfs_write(file, kbuf, count, file->pos);

end:
	if (result != -EAGAIN)
		km_free_addr(kbuf, count);
	return (int32_t)result;
}

int32_t sys_getcwd(void *bufaddr, uint32_t count)
{
	char *buffer = vm_user_to_kernel(syscall_proc->as->pdir, bufaddr, count);

	strncpy(buffer, syscall_proc->cwd->name, count);
	buffer[count-1] = '\0';

	km_free_addr(buffer, count);

	return (int32_t)bufaddr;
}

int32_t sys_stat(int32_t path, int32_t sbuf)
{
	return -ENOSYS;
}

int32_t sys_isatty(int32_t fd)
{
	struct file *file = fd2file(fd);
	if (!file)
		return 0;

	return tty_isatty(file);
}

int32_t sys_readdir(uint32_t path, uint32_t userbuf, int index)
{
	int err = 0;

	size_t namelen = 0;
	char *name = vm_map_string(syscall_proc->as->pdir, (vaddr_t)path, &namelen);
	size_t size = 0;
	char *buffer =  vm_map_string(syscall_proc->as->pdir, userbuf, &size);

	if (!name || !buffer) {
		err = -ENOMEM;
		goto end;
	}

	dbg_printf(DBG_FS, "readdir %s %d\n", name, index);


	struct inode *dir = vfs_lookup(name, syscall_proc->cwd);
	if (!dir) {
		err = -ENOENT;
		goto end;
	}

	struct dirent *dirent = vfs_readdir(dir, index);
	if (!dirent) {
		err = vfs_geterror();
		goto end;
	}

	strncpy(buffer, dirent->name, size > FS_MAX_NAME ? FS_MAX_NAME : size);

end:
	if (name)
		km_free_addr(name, namelen);
	if (buffer)
		km_free_addr(buffer, size);

	return err;
}

int32_t sys_mount(int32_t mountp, int32_t ftype, int32_t device)
{
	if (!mountp || !ftype)
		return (dword)-EINVAL;

	size_t pathlen = 0;
	char *path = vm_map_string(syscall_proc->as->pdir, (vaddr_t)mountp, &pathlen);
	size_t typelen = 0;
	char *type = vm_map_string(syscall_proc->as->pdir, (vaddr_t)ftype, &typelen);

	size_t devlen = 0;
	char *dev  = NULL;
	if (device != 0)
		dev = vm_map_string(syscall_proc->as->pdir, (vaddr_t)device, &devlen);

	struct fstype *driver = vfs_gettype(type);
	struct inode *inode = vfs_lookup(path, syscall_proc->cwd);

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

int32_t sys_open_std()
{
	dbg_printf(DBG_SC, "sys_open_std()\n");

	const char *tty = syscall_proc->tty;

	if (!tty) {
		// TODO: open /dev/null instead
		return 0;
	}

	struct inode *inode = vfs_lookup(tty, syscall_proc->cwd);
	int n=0;

	if (!syscall_proc->fds[0]) {
		syscall_proc->fds[0] = vfs_open(inode, FSO_READ);
		n++;
	}

	if (!syscall_proc->fds[1]) {
		syscall_proc->fds[1] = vfs_open(inode, FSO_WRITE);
		n++;
	}

	if (!syscall_proc->fds[2]) {
		syscall_proc->fds[2] = vfs_open(inode, FSO_WRITE);
		n++;
	}

	if (syscall_proc->numfds < 3)
		syscall_proc->numfds = 3;

	return n;
}

void init_fs(void)
{
	dbg_printf(DBG_FS, "\nRegistering fs syscalls... ");

	syscall_register(SC_OPEN,  sys_open, 2);
	syscall_register(SC_CLOSE, sys_close, 1);
	syscall_register(SC_READ,  sys_readwrite, 3);
	syscall_register(SC_WRITE, sys_readwrite, 3);
	syscall_register(SC_ISATTY, sys_isatty, 1);

	syscall_register(SC_STAT, sys_stat, 2);

	syscall_register(SC_GETCWD, sys_getcwd, 2);
	syscall_register(SC_READDIR, sys_readdir, 3);

	syscall_register(SC_OPEN_STD, sys_open_std, 0);

	dbg_printf(DBG_FS, "done\n");

	//init_devfs();
}

