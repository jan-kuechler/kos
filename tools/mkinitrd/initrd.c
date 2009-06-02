#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "initrd.h"

struct entry
{
	const char *name;
	const char *path;
	unsigned int count;
	struct entry *files, *pdir;
	struct entry *next;
	struct entry *last_entry;
};

struct id_entry
{
	int type;
	int name;
	int count;
	int content;
	int next;
};

static int dir_level;
static struct entry *cur_dir, *cur_entry;
static struct entry *root;

static int entry_count;

void id_init(void)
{
	dir_level = 0;
	cur_dir   = NULL;
	cur_entry = NULL;
	root      = NULL;
	entry_count = 0;
}

static inline void add_entry(struct entry *e)
{
	if (!cur_dir->files)
		cur_dir->files = e;
	e->next = NULL;

	cur_dir->count++;

	if (cur_entry)
		cur_entry->next = e;
	cur_dir->last_entry = e;

	entry_count++;
}

void id_start_dir(const char *name)
{
	struct entry *dir = malloc(sizeof(*dir));
	dir->name  = name;
	dir->path  = NULL;
	dir->files = NULL;
	dir->pdir  = cur_dir;
	dir->count = 0;
	dir->next  = NULL;

	if (cur_dir)
		add_entry(dir);
	else
		root = dir;
	cur_dir = dir;
	cur_entry  = NULL;
}

void id_add_file(const char *name, const char *path)
{
	struct entry *file = malloc(sizeof(*file));
	file->name  = name;
	file->path  = path;
	file->files = NULL;
	file->pdir  = NULL;
	file->last_entry = NULL;

	add_entry(file);
	cur_entry = file;
}

void id_end_dir(void)
{
	cur_dir = cur_dir->pdir;
	if (cur_dir)
		cur_entry = cur_dir->last_entry;
}

static void spaces(int c)
{
	while (c--)
		printf(" ");
}

static void dummy(struct entry *dir)
{
	static int ident = 0;

	struct entry *cur = dir->files;

	spaces(ident * 2);
	printf("[D] %s (%d) %s\n", dir->name, dir->count, dir->next ? "has_next" : "");
	ident++;
	while (cur) {
		if (cur->path) {
			spaces(ident * 2);
			printf("[F] %s - %s\n", cur->name, cur->path);
		}
		else {
			dummy(cur);
		}
		struct entry *old = cur;
		cur = cur->next;
		free(old);
	}
	ident--;
}


#ifdef LINUX
static int file_size(const char *file)
{
	struct stat info;
	int err = stat(file, &info);

	if (!err)
		return info.st_size;

#define ERR(code) case code: printf(#code "\n"); break
	switch (errno) {
	ERR(EACCES);
	ERR(EBADF);
	ERR(EFAULT);
	ERR(ENAMETOOLONG);
	ERR(ENOENT);
	ERR(ENOMEM);
	ERR(ENOTDIR);
	default:
		printf("%d - huh?\n", err);
	};
	return 0;
}
#else
#include <windows.h>

static int file_size(const char *file)
{
	HANDLE f = CreateFile(file, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (f == INVALID_HANDLE_VALUE) {
		printf("Argh: %d\n", GetLastError());
		return 0;
	}

	int high = 42;
	int size = GetFileSize(f, &high);
	CloseHandle(f);

	if (size == INVALID_FILE_SIZE) {
		printf("Error: %d\n", GetLastError());
		return 0;
	}

	return size;
}
#endif

static int write_file(struct entry *file, FILE *out, int offset)
{
	int count = file_size(file->path);
	int namelen = strlen(file->name) + 1; /* including \0 */
	int namepos = offset + 20;
	int datapos = namepos + namelen;
	int newoffs = datapos + count;

	struct id_entry entry;
	entry.type = 0;
	entry.name = namepos;
	entry.count = count;
	entry.content = datapos;
	entry.next = file->next ? newoffs : 0;


	fwrite(&entry, sizeof(entry), 1, out);

	/*fwrite(&type,    4, 1, out);
	fwrite(&namepos, 4, 1, out);
	fwrite(&count  , 4, 1, out);
	fwrite(&datapos, 4, 1, out);
	fwrite(&nextpos, 4, 1, out);*/

	/* next is name and content */
	fwrite(file->name, 1, namelen, out);

	char *buffer = malloc(count);
	FILE *f = fopen(file->path, "r");
	fread(buffer, 1, count, f);
	fclose(f);

	fwrite(buffer, 1, count, out);
	free(buffer);


	return newoffs;
}

static int write_dir(struct entry *dir, FILE *out, int offset)
{
#ifdef DUMMY
	dummy(dir);
	return;
#endif

	int type = 1;
	int count = dir->count;
	int namelen = strlen(dir->name);
	int namepos = offset + 20;
	int datapos = namepos + namelen;
	int nextpos = 0; /* still not known */

	fwrite(&type,    4, 1, out);
	fwrite(&namepos, 4, 1, out);
	fwrite(&count  , 4, 1, out);
	fwrite(&datapos, 4, 1, out);

	long next_posinfile = ftell(out);
	fwrite(&nextpos, 4, 1, out);  /* dummy, change this later! */

	fwrite(dir->name, 1, namelen, out);

	struct entry *cur = dir->files;

	int offs = datapos;
	while (cur) {
		if (cur->path) {
			offs = write_file(cur, out, offs);
		}
		else {
			offs = write_dir(cur, out, offs);
		}
		cur = cur->next;
	}

	if (dir->next) {
		long cur_pos = ftell(out);
		fseek(out, next_posinfile, SEEK_SET);
		fwrite(&offs, 4, 1, out);
		fseek(out, cur_pos, SEEK_SET);
	}
	return offs;
}

void id_write(const char *file)
{
	FILE *f = fopen(file, "w");
	if (!f)
		fprintf(stderr, "Could not open file '%s': %s\n", file, strerror(errno));
	char header[4] = {'k', 'I', 'D', 0};
	fwrite(header, 1, 4, f);
	write_dir(root, f, 4);
	fclose(f);
}
