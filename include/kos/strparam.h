#ifndef KOS_STRPARAM_H
#define KOS_STRPARAM_H

struct strparam
{
	unsigned int len;
	char *ptr;
};

struct strlist
{
	unsigned int num;
	struct strparam *strings;
};

#endif /*KOS_STRPARAM_H*/
