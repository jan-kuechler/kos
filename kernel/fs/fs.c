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

dword sys_open(dword calln, dword fname, dword flags, dword arg2)
{
	return -ENOSYS;
}

dword sys_close(dword calln, dword fd, dword arg1, dword arg2)
{
	return -ENOSYS;
}

dword sys_read(dword calln, dword fd, dword buffer, dword size)
{
	return -ENOSYS;
}

dword sys_write(dword calln, dword fd, dword buffer, dword size)
{
	return -ENOSYS;
}

dword sys_mknod(dword calln, dword fname, dword flags, dword arg2)
{
	return -ENOSYS;
}

dword sys_readdir(dword calln, dword fname, dword index, dword arg2)
{
	return -ENOSYS;
}

dword sys_mount(dword calln, dword mountp, dword ftype, dword device)
{
	return -ENOSYS;
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

