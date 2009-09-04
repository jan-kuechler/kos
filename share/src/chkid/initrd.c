#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "initrd.h"

static inline void space(FILE *f, int c) { while(c--) fprintf(f, " "); }

#define LOG(...) \
	do { space(log, idt); fprintf(log, __VA_ARGS__); space(stdout, idt); fprintf(stdout, __VA_ARGS__); } while (0)

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

static char *type_to_name[2] = {"File", "Dir"};

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


static int check_header(void *buf, FILE *log)
{
	int idt = 0;
	struct id_header *hdr = buf;

	LOG("* Checking Header at offset 0:\n");
	LOG("|- Magic: '%c' '%c' '%c'\n", hdr->magic[0], hdr->magic[1], hdr->magic[2]);
	LOG("|- Version: %d\n", hdr->version);
	LOG("`- New offset is %d\n", sizeof(*hdr));

	return sizeof(*hdr);
}

static int check_file(void *buf, int offset, FILE *log, int idt)
{
	struct id_entry *entry = (buf + offset);

	LOG("* Checking file at offset %d:\n", offset);
	LOG("|- Type:        %d (%s)\n", entry->type, type_to_name[entry->type]);
	LOG("|- NameOffs:    %d\n", entry->name);
	LOG("|- Count:       %d\n", entry->count);
	LOG("|- ContentOffs: %d\n", entry->content);
	LOG("|- NextOffs:    %d\n", entry->next);

	char *name = (char*)(buf + entry->name);
	LOG("`- Name: '%s' with length %d\n", name, strlen(name) + 1);

	return entry->next;
}

static int check_dir(void *buf, int offset, FILE *log, int idt)
{
	struct id_entry *entry = (buf + offset);

	LOG("* Checking dir at offset %d:\n", offset);
	LOG("|- Type:        %d (%s)\n", entry->type, type_to_name[entry->type]);
	LOG("|- NameOffs:    %d\n", entry->name);
	LOG("|- Count:       %d\n", entry->count);
	LOG("|- ContentOffs: %d\n", entry->content);
	LOG("|- NextOffs:    %d\n", entry->next);

	char *name = (char*)(buf + entry->name);
	LOG("`- Name: '%s' with length %d\n", name, strlen(name) + 1);

	int offs = entry->content;
	int i=0;
	for (; i < entry->count; ++i) {
		struct id_entry *e = (buf + offs);
		if (e->type == 0) {
			offs = check_file(buf, offs, log, idt + 1);
		}
		else if (e->type == 1) {
			offs = check_dir(buf, offs, log, idt + 1);
		}
		else {
			LOG("Error: Unknown type!\n");
			abort();
		}
	}

	return entry->next;
}

void id_check(const char *file, const char *logfile)
{
	FILE *log = fopen(logfile, "w");

	int fs = file_size(file);

	FILE *f = fopen(file, "rb");
	char *buffer = malloc(fs);
	fread(buffer, 1, fs, f);
	fclose(f);

	int offs = check_header(buffer, log);
	check_dir(buffer, offs, log, 0);
}
