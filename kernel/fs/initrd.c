#include <bitop.h>
#include <string.h>
#include <kos/initrd.h>
#include "debug.h"
#include "kernel.h"
#include "module.h"
#include "fs/fs.h"
#include "fs/initrd.h"
#include "fs/request.h"
#include "mm/kmalloc.h"
#include "util/list.h"

#ifndef FS_TYPES_H
#error "no types??"
#endif

static int id_close(struct file *file);
static int id_read(struct file *file, void *buffer, dword size, dword offset);
static int id_read_async(struct request *rq);

static int id_open(struct inode *ino, struct file *file, dword flags);
static struct dirent *id_readdir(struct inode *ino, dword index);
static struct inode *id_finddir(struct inode *ino, char *name);

static int id_remount(struct superblock *sb, dword flags);
static int id_mount(struct superblock *sb, char *dev, int flags);

static struct file_ops fops = {
	.close = id_close,
	.read  = id_read,
	.read_async = id_read_async,
};

static struct inode_ops iops = {
	.open = id_open,
	.readdir = id_readdir,
	.finddir = id_finddir,
};

static struct sb_ops sbops = {
	.remount = id_remount,
};

static struct superblock super = {
	.ops = &sbops,
};

static struct fstype initrd = {
	.name  = "initrd",
	.flags = 0,
	.mount = id_mount,
};

static struct inode *root;

/* VFS interface */

static int id_close(struct file *file)
{
	return 0;
}

static int do_read(struct file *file, dword offs, void *buffer, dword count)
{
	struct inode *ino = file->inode;
	if (offs > ino->length)
		return -1;

	if ((offs + count) > ino->length)
		count = ino->length - offs;

	void *data = (void*)(ino->impl + offs);
	memcpy(buffer, data, count);

	file->pos += count;

	return count;
}

static int id_read(struct file *file, void *buffer, dword count, dword offset)
{
	return do_read(file, offset, buffer, count);
}

static int id_read_async(struct request *rq)
{
	int result = do_read(rq->file, rq->offset, rq->buffer, rq->buflen);
	rq->result = result;
	rq_finish(rq);
	return result;
}

static int id_open(struct inode *ino, struct file *file, dword flags)
{
	file->pos = 0;
	file->fops = &fops;

	return 0;
}

static struct dirent *id_readdir(struct inode *ino, dword index)
{
	list_t *entries = (list_t*)ino->impl;

	int i=0;
	list_entry_t *pos;
	struct dirent *dirent = kmalloc(sizeof(*dirent));

	list_iterate(pos, entries) {
		if (i == index) {
			struct inode *e = pos->data;
			strcpy(dirent->name, e->name);
			dirent->inode = e;
			return dirent;
		}
		i++;
	}

	kfree(dirent);
	return NULL;
}

static struct inode *id_finddir(struct inode *ino, char *name)
{
	dbg_vprintf(DBG_FS, "id_finddir(inode: '%s', '%s')\n", ino->name, name);
	list_t *entries = (list_t*)ino->impl;

	list_entry_t *pos;
	list_iterate(pos, entries) {
		struct inode *e = pos->data;
		dbg_vprintf(DBG_FS, " comparing '%s' with '%s'\n", e->name, name);
		if (strcmp(e->name, name) == 0) {
			return e;
		}

	}

	dbg_vprintf(DBG_FS, " nothing found )-:\n");
	return NULL;
}

static int id_remount(struct superblock *sb, dword flags)
{
	return 0;
}

static int id_mount(struct superblock *sb, char *dev, int flags)
{
	sb->root = root;
	sb->ops  = &sbops;

	bclr(flags, FSM_WRITE | FSM_EXEC);
	sb->flags = flags;

	return 0;
}

/* initrd parsing */

static int check_img(void *img)
{
	struct id_header *header = img;

	if (header->magic[0] != ID_MAGIC0)
		return 0;
	if (header->magic[1] != ID_MAGIC1)
		return 0;
	if (header->magic[2] != ID_MAGIC2)
		return 0;

	return (header->version == ID_VERSION);
}

static struct inode *new_inode(struct id_entry *e, void *disk)
{
	struct inode *ino = kmalloc(sizeof(*ino));
	memset(ino, 0, sizeof(*ino));

	ino->name = (char*)id_get(e, name, disk);
	ino->length = e->count;

	ino->sb = &super;
	ino->ops = &iops;

	dbg_vprintf(DBG_FS, " created new inode: %s\n", ino->name);

	return ino;
}

static struct inode *parse_file(struct id_entry *file, void *disk)
{
	struct inode *ino = new_inode(file, disk);
	ino->flags = FS_FILE;
	ino->impl  = (dword)id_get(file, content, disk);
	return ino;
}

static struct inode *parse_dir(struct id_entry *dir, void *disk)
{
	if (dir->type != ID_TYPE_DIR) {
		panic("Tried to parse %s as a dir, but it's a file.\n", (char*)id_get(dir, name, disk));
	}

	struct inode *ino = new_inode(dir, disk);
	ino->flags = FS_DIR;
	list_t *entries = list_create();
	ino->impl = (dword)entries;

	struct id_entry *entry = id_get(dir, content, disk);

	while (entry) {
		struct inode *e = NULL;
		if (entry->type == ID_TYPE_FILE) {
			e = parse_file(entry, disk);
		}
		else if (entry->type == ID_TYPE_DIR) {
			e = parse_dir(entry, disk);
		}
		else {
			panic("Invalid type: %d\n", entry->type);
		}

		list_add_back(entries, e);

		entry = id_get(entry, next, disk);
	}

	return ino;
}

/* initializing */

void init_initrd(void)
{
	if (multiboot_info.mods_count < 1) {
		panic("No initrd.");
	}

	void *disk;
	char *params;

	int size = mod_load(0, &disk, &params);

	dbg_printf(DBG_FS, "Loading initrd '%s' at %p with %d bytes length.\n", params, disk, size);

	if (!check_img(disk)) {
		panic("Module 1 is not a kOS initrd (%s)\n", params);
	}

	root = parse_dir(disk + sizeof(struct id_header), disk);

	vfs_register(&initrd);
}
