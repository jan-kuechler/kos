#include <dirent.h>
#include <stdlib.h>
#include "dir.h"

static inline void cleanup(char **ar, int n)
{
	int i=0;
	for (; i < n; ++i) {
		free(ar[i]);
	}
	free(ar);
}

int closedir(DIR *dir)
{
	if (!dir) return -1;

	cleanup(dir->names, dir->count);
	free(dir);

	return 0;
}
