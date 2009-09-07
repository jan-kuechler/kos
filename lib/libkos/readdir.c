#include <dirent.h>
#include <string.h>
#include "dir.h"

struct dirent *readdir(DIR *dir)
{
	if (!dir || (dir->index >= dir->count)) {
		return NULL;
	}

	strncpy(dir->dirent.d_name, dir->names[dir->index], MAX_PATH);
	dir->dirent.d_name[MAX_PATH-1] = '\0';

	dir->index++;

	return &dir->dirent;
}
