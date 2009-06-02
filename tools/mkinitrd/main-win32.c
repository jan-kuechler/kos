#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "initrd.h"

void dummy()
{
	/*
	  /
		|- foo
		|- bar
		`- directory
		   |- abc
		   `- xyz
	*/
	id_start_dir("root");
		id_add_file("foo", "foo-path");
		id_add_file("bar", "bar-path");
		id_start_dir("directory");
			id_add_file("abc", "path");
			id_add_file("xyz", "path");
		id_end_dir();
	id_end_dir();
}

void scan_dir(const char *path, const char *name)
{
	WIN32_FIND_DATA info;
	HANDLE find;

	char workingdir[512];

	GetCurrentDirectory(512, workingdir);

	SetCurrentDirectory(path);

	find = FindFirstFile("*", &info);

	if (find == INVALID_HANDLE_VALUE) {
		printf("INVALID_HANDLE_VALUE");
		return;
	}

	id_start_dir(name);

	do {
		if ((strcmp(info.cFileName, ".") == 0) ||
		    (strcmp(info.cFileName, "..") == 0))
		{
			continue;
		}

		if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			char *n = strdup(info.cFileName);
			scan_dir(n, n);
		}
		else {
			char *filename;
			char *buffer = malloc(4096);

			GetFullPathName(info.cFileName, 4096, buffer, &filename);
			id_add_file(filename, buffer);
		}
	} while (FindNextFile(find, &info));

	id_end_dir();

	FindClose(find);

	SetCurrentDirectory(workingdir);
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "Usage: mkid <dir> <outfile>\n");
		return 1;
	}

	id_init();

	#ifdef DUMMY
		dummy();
		id_write("dummy");
	#else
		printf("Scanning...");
		scan_dir(argv[1], "/");
		printf("\t\tdone\n");

		printf("Writing InitRD...");
		id_write(argv[2]);
		printf("\tdone\n");
	#endif

	return 0;
}
