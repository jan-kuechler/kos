#include <errno.h>
#include <string.h>
#include "fs/fs.h"
#include "util/list.h"

/* A list of all registered filesystems */
static list_t *fslist = 0;

/**
 *  fs_register(type)
 *
 * Registers a new filesystem type.
 */
int fs_register(fstype_t *type)
{
	if (!fslist)
		fslist = list_create();

	list_add_back(fslist, type);

	return 0;
}

/**
 *  fs_unregister(type)
 *
 * Unregisters a previously registered filesystem type.
 */
int fs_unregister(fstype_t *type)
{
	list_entry_t *e;
	list_iterate(e, fslist) {
		if (e->data == type) {
			list_del_entry(e);
			return 0;
		}
	}
	return -EINVAL;
}

/**
 *  fs_find_type(name)
 *
 * Returns the filesystem type with the given name or 0.
 */
fstype_t *fs_find_type(char *name)
{
	list_entry_t *e;
	list_iterate(e, fslist) {
		fstype_t *type = e->data;
		if (strcmp(type->name, name) == 0)
			return type;
	}
	return 0;
}
