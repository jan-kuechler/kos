#ifndef SYS_UTSNAME_H
#define SYS_UTSNAME_H

/* sys/utsname.h header for the implementation in libkos. */
/* This file is specific to a part of the kOS operating system. */

#define L_uname 64

struct utsname
{
	const char sysname[L_uname];
	const char nodename[L_uname];
	const char release[L_uname];
	const char version[L_uname];
	const char machine[L_uname];
};

int uname(struct utsname *name);

#endif /*SYS_UTSNAME_H*/
