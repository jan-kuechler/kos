#include <dirent.h>
#include "dir.h"

void rewinddir(DIR *dir)
{
	if (!dir) return;

	dir->index = 0;
}
