#include <errno.h>
#include <string.h>
#include <kos/syscalln.h>
#include "debug.h"
#include "syscall.h"
#include "fs/fs.h"
#include "util/list.h"

/* A list of all registered filesystems */
static list_t *fslist = 0;

static inode_t root = {
	.name = "/",
	.flags = FS_DIR,
};
inode_t *fs_root = &root;

int fs_register(fstype_t *type)
{
	kassert(type);

	if (!fslist)
		fslist = list_create();

	list_add_back(fslist, type);

	return 0;
}

int fs_unregister(fstype_t *type)
{
	kassert(type);

	list_entry_t *e;
	list_iterate(e, fslist) {
		if (e->data == type) {
			list_del_entry(fslist, e);
			return 0;
		}
	}
	return -EINVAL;
}

fstype_t *fs_find_type(char *name)
{
	kassert(name);

	list_entry_t *e;
	list_iterate(e, fslist) {
		fstype_t *type = e->data;
		if (strcmp(type->name, name) == 0)
			return type;
	}
	return 0;
}

// FIXME: please please fix me!
static inline char *safe_path(dword addr)
{
	char *name = vm_user_to_kernel(cur_proc->pagedir, (vaddr_t)addr, 1024);
	return name;
}

// FIXME: please please fix me!
static inline char *safe_name(dword addr)
{
	char *name = vm_user_to_kernel(cur_proc->pagedir, (vaddr_t)addr, 1024);
	return name;
}

static inline inode_t *inode_from_fd(dword fd)
{
	if (fd >= cur_proc->numfds)
		return NULL;

	return cur_proc->fds[fd];
}

dword sys_open(dword calln, dword fname, dword flags, dword arg2)
{
	char *name = safe_name(fname);

	inode_t *inode = fs_lookup(name, cur_proc->cwd);

	if (!inode)
		return -ENOENT;

	if (cur_proc->numfds >= PROC_NUM_FDS)
		return -EMFILE;

	int res = fs_open(inode, flags);
	if (res < 0)
		return res;

	cur_proc->fds[cur_proc->numfds] = inode;

	return cur_proc->numfds++;
}

dword sys_close(dword calln, dword fd, dword arg1, dword arg2)
{
	inode_t *inode = inode_from_fd(fd);

	if (!inode)
		return -ENOENT;

	return fs_close(inode);
}

dword sys_read(dword calln, dword fd, dword buffer, dword size)
{
	void *kbuf = vm_user_to_kernel(cur_proc->pagedir, (vaddr_t)buffer, size);

	inode_t *inode = inode_from_fd(fd);

	if (!inode)
		return -ENOENT;

	return fs_read(inode, 0, kbuf, size);
}

dword sys_write(dword calln, dword fd, dword buffer, dword size)
{
	void *kbuf = vm_user_to_kernel(cur_proc->pagedir, (vaddr_t)buffer, size);

	inode_t *inode = inode_from_fd(fd);

	if (!inode)
		return -ENOENT;

	return fs_write(inode, 0, kbuf, size);
}

dword sys_mknod(dword calln, dword fname, dword flags, dword arg2)
{
	return -ENOSYS;

	//char *name = safe_path(fname);

	//inode_t *inode = fs_lookup_dir(name, cur_proc->cwd);
	//char *file = last_part(name);

	//if (!inode)
	//	return -ENOENT;

	//return fs_mknod(inode, file, flags);
}

dword sys_readdir(dword calln, dword fname, dword index, dword arg2)
{
	char *name = safe_path(fname);

	inode_t *inode = fs_lookup(name, cur_proc->cwd);

	if (!inode)
		return -ENOENT;

	return fs_readdir(inode, index);
}

dword sys_mount(dword calln, dword mountp, dword ftype, dword device)
{
	if (!mountp || ! ftype)
		return -EINVAL;


	char *path = safe_path(mountp);
	char *type = safe_name(ftype);

	char *dev  = NULL;
	if (device != 0)
		dev = safe_path(device);

	fstype_t *driver = fs_find_type(type);

	inode_t *inode = fs_lookup(path);

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
	syscall_register(SC_MKNOD, sys_mknod);
	syscall_register(SC_READDIR, sys_readdir);
	syscall_register(SC_MOUNT, sys_mount);

	//init_devfs();
}

