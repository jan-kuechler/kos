#ifndef DIRENT_H
#define DIRENT_H

#include <sys/types.h>

/* dirent.h header for the implementation in libkos. */
/* This file is specific to a part of the kOS operating system. */

#define MAX_PATH 256

struct dirent
{
	ino_t d_ino;
	char  d_name[MAX_PATH];
};

struct libkos_DIR;
typedef struct libkos_DIR DIR;

DIR           *opendir(const char *name);
int            closedir(DIR *dir);
struct dirent *readdir(DIR *dir);
void           rewinddir(DIR *dir);


#endif /*DIRENT_H*/
