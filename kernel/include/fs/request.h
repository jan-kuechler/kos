#ifndef REQUEST_H
#define REQUEST_H

#include "pm.h"
#include "fs/fs.h"

typedef enum {
	RQ_READ,
	RQ_WRITE,
	RQ_SEEK,
	RQ_IOCTL,
	RQ_OPEN,
	RQ_CLOSE,
	RQ_MKNOD,
} fs_rq_type_t;

typedef struct fs_request
{
	fs_rq_type_t     type;

	fs_filesystem_t *fs;
	fs_file_t       *file;

	dword            flags;

	dword            buflen;
	void            *buf;

	dword            offs;
	dword            orig;

	dword            status;
	dword            result;
	byte             finished;

	proc_t          *proc;
	proc_t          *wait_proc;
} fs_request_t;

int fs_query_rq(fs_request_t *rq, int wait, proc_t *proc);
int fs_wait_rq(fs_request_t *rq);
int fs_finish_rq(fs_request_t *rq);

#endif /*REQUEST_H*/
