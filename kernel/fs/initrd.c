#include <kos/initrd.h>
#include "kernel.h"
#include "module.h"
#include "fs/fs.h"
#include "fs/initrd.h"

static int id_mount(struct superblock *sb, char *dev, int flags);

static struct file_ops fops = {

};

static struct inode_ops iops = {

};

static struct superblock_ops sops = {

};

static struct superblock super = {
	.ops = &sops;
};

static struct fstype initrd_type = {
	.name  = "initrd",
	.flags = 0,
	.mount = id_mount,
};

static struct inode *root;


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

static inline struct inode *new_inode(struct id_entry *e, void *disk)
{
	struct inode *ino = kmalloc(sizeof(*ino));
	memset(ino, 0, sizeof(ino));

	ino->name = (char*)id_get(e, name, disk);
	ino->length = e->count;

	ino->sb = &super;
	ino->ops = &iops;
}

static struct inode *parse_file(struct id_entry *file, void *disk)
{

}

static struct inode *parse_dir(struct id_entry *dir, void *disk)
{
	if (dir->type != ID_TYPE_DIR) {
		panic("Tried to parse %s as a dir, but it is a file.\n", (char*)id_get(dir, name, disk));
	}

	struct indode *ino = new_inode(dir, disk);
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

void init_initrd(void)
{
	if (multiboot_info.mods_count < 1) {
		panic("No initrd.");
	}

	void *disk;
	char *params;

	int size = mod_load(0, &disk, &params);

	if (!check_img(disk)) {
		panic("Module 1 is not a kOS initrd (%s)\n", params);
	}

	root = parse_dir(disk + sizeof(struct id_header), disk);
	super.root = root;
}
