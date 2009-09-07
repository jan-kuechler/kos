#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include "dir.h"
#include "helper.h"

#define NNAMES 32

static inline void cleanup(char **ar, int n)
{
	int i=0;
	for (; i < n; ++i) {
		free(ar[i]);
	}
	free(ar);
}

DIR *opendir(const char *name)
{
	/* TODO: Check file type for better error reporting */
	if (!name) return NULL;

	DIR *dir = malloc(sizeof(*dir));
	if (!dir) return NULL;

	dir->index = 0;
	dir->count = 0;

	int i=0;
	int max = NNAMES;
	char **names = malloc(NNAMES * sizeof(char*));
	if (!names) {
		free(dir);
		return NULL;
	}
	int err = 0;

	STR_PARAM(path, name);

	struct strparam entry;
	entry.len = MAX_PATH;

	while (1) {
		char *buffer = malloc(MAX_PATH);
		if (!buffer) {
			cleanup(names, i);
			return NULL;
		}

		entry.ptr = buffer;
		int err = SYSCALL3(SC_READDIR, path, (uint32_t)&entry, i);
		if (err) /* no more entries */
			break;

		if (i >= max) {
			max *= 2;
			names = realloc(names, max * sizeof(char*));
			if (!names) {
				free(dir);
				return NULL;
			}
		}
		names[i++] = buffer;
	}
	if (!i && err == -EINVAL) {
		/* maybe not a directory... */
		free(names);
		free(dir);
		return NULL;
	}

	//names = realloc(names, i);

	dir->index = 0;
	dir->count = i;
	dir->names = names;

	return dir;
}
