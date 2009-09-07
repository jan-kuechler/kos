#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "initrd.h"

static inline void space(FILE *f, int c) { while(c--) fprintf(f, " "); }

#define LOG(...) do { space(log, idt); fprintf(log, __VA_ARGS__); } while (0)

#define LOFFS() do { space(log, idt); fprintf(log, "~ File offset is %d\n", ftell(out)); } while (0)

struct entry
{
	const char *name;
	const char *path;
	unsigned int count;
	struct entry *files, *pdir;
	struct entry *next;
	struct entry *last_entry;
};

struct id_header
{
	char magic[3];
	char version;
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

static int write_file(struct entry *file, FILE *out, int offset, FILE *log, int idt)
{
	LOG("~ Offset is  %d\n", offset);
	LOG("~ Filepos is %d\n", ftell(out));

	int count = file_size(file->path);
	int namelen = strlen(file->name) + 1; /* including '\0' */
	int namepos = offset + sizeof(struct id_entry);
	int datapos = namepos + namelen;
	int newoffs = datapos + count;

	space(stdout, idt * 2);
	printf("%-20s - %4d kb\n", file->name, count / 1024);

	struct id_entry entry;
	entry.type    = 0;
	entry.name    = namepos;
	entry.count   = count;
	entry.content = datapos;
	entry.next    = file->next ? newoffs : 0;

	LOG("* Writing file at offset %d:\n", offset);
	LOG("|- Type:        %d\n", entry.type);
	LOG("|- NameOffs:    %d\n", entry.name);
	LOG("|- Count:       %d\n", entry.count);
	LOG("|- ContentOffs: %d\n", entry.content);
	LOG("|- NextOffs:    %d\n", entry.next);

	fwrite(&entry, sizeof(entry), 1, out);

	/* next is name and content */
	LOG("|- Writing name: '%s' with length %d\n", file->name, namelen);
	fwrite(file->name, 1, namelen, out);
	LOFFS();

	char *buffer = malloc(count);
	FILE *f = fopen(file->path, "rb");
	fread(buffer, count, 1, f);
	fclose(f);

	LOG("|- Writing content with length %d\n", count);
	LOG("|- New offset should be %d\n", ftell(out) + count);
	int written = fwrite(buffer, count, 1, out);

	free(buffer);
	LOFFS();

	LOG("`- New offset is %d\n", newoffs);

	return newoffs;
}

static int write_dir(struct entry *dir, FILE *out, int offset, FILE *log, int idt)
{
	space(stdout, idt * 2);
	printf("/%s\n", dir->name);

	LOG("~ Offset is  %d\n", offset);
	LOG("~ Filepos is %d\n", ftell(out));

	int namelen = strlen(dir->name) + 1; /* including '\0' */
	int namepos = offset + sizeof(struct id_entry);
	int datapos = namepos + namelen;

	struct id_entry entry;
	entry.type    = 1;
	entry.name    = namepos;
	entry.count   = dir->count;
	entry.content = datapos;
	entry.next    = 0; /* still not known */

	LOG("* Writing directory at offset %d:\n", offset);
	LOG("|- Type:        %d\n", entry.type);
	LOG("|- NameOffs:    %d\n", entry.name);
	LOG("|- Count:       %d\n", entry.count);
	LOG("|- ContentOffs: %d\n", entry.content);
	LOG("|- NextOffs:    <still unknown>\n");

	fwrite(&entry, sizeof(entry), 1, out);

	long next_posinfile = ftell(out) - 4;
	LOG("|- Pos of next is %d\n", next_posinfile);

	LOG("|- Writing name: '%s' with length %d\n", dir->name, namelen);
	fwrite(dir->name, 1, namelen, out);

	struct entry *cur = dir->files;

	int offs = datapos;
	while (cur) {
		if (cur->path) {
			offs = write_file(cur, out, offs, log, idt + 1);
		}
		else {
			offs = write_dir(cur, out, offs, log, idt + 1);
		}
		cur = cur->next;
	}

	if (dir->next) {
		long cur_pos = ftell(out);
		fseek(out, next_posinfile, SEEK_SET);
		fwrite(&offs, 4, 1, out);
		fseek(out, cur_pos, SEEK_SET);
		LOG("|- Next offset changed to %d\n", offs);
		LOG("|- File pos is %d, was %d\n", ftell(out), cur_pos);
	}
	else {
		LOG("|- No next offset needed.\n");
	}

	LOG("`- New offset is %d\n", offs);
	return offs;
}

static int write_header(FILE *f, FILE *log)
{
	int idt = 0;
	LOG("* Writing Header at offset 0\n");
	struct id_header header = { {'k', 'I', 'D'}, 0 };
	fwrite(&header, sizeof(header), 1, f);
	return sizeof(header);
}

void id_write(const char *file, const char *logfile)
{
	FILE *f = fopen(file, "wb");
	FILE *log = fopen(logfile, "w");
	if (!f || !log) {
		fprintf(stderr, "Could not open file '%s' or '%s': %s\n", file, logfile, strerror(errno));
		return;
	}

	int offs = write_header(f, log);
	int size = write_dir(root, f, offs, log, 0);
	printf("Total size: %d kb\n", size / 1024);
	fclose(f);
	fclose(log);
}
