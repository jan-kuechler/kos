#include <errno.h>
#include <string.h>
#include "debug.h"
#include "syscall.h"
#include "fs/fs.h"
#include "util/list.h"

/* A list of all registered filesystems */
static list_t *fslist = 0;

static inode_t root = {
	"/",
	FS_DIR,
	0,
	0,
	0,
	0,
	0,
	NULL,
	NULL,
	NULL,
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
			list_del_entry(e);
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
	return 0;
}

dword sys_close(dword calln, dword fd, dword arg1, dword arg2)
{
	return 0;
}

dword sys_read(dword calln, dword fd, dword buffer, dword size)
{
	return 0;
}

dword sys_write(dword calln, dword fd, dword buffer, dword size)
{
	return 0;
}

dword sys_mount(dword calln, dword mountp, dword ftype, dword device)
{
	return 0;
}

void init_fs(void)
{
	syscall_register(SC_OPEN,  sys_open);
	syscall_register(SC_CLOSE, sys_close);
	syscall_register(SC_READ,  sys_read);
	syscall_register(SC_WRITE, sys_write);

	init_devfs();
}

