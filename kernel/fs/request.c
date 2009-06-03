#include <errno.h>
#include <string.h>
#include "syscall.h"
#include "fs/request.h"
#include "mm/kmalloc.h"

struct request *rq_create(struct file *file, void *buffer, dword buflen)
{
	struct request *rq = kmalloc(sizeof(*rq));
	memset(rq, 0, sizeof(*rq));

	rq->file   = file;
	rq->buffer = buffer;
	rq->buflen = buflen;

	rq->proc   = cur_proc;

	rq->blocked = 0;
	rq->result  = -EINVAL;

	return rq;
}

void rq_block(struct request *rq)
{
	pm_block(rq->proc, BR_WAIT_FS);
	rq->blocked = 1;
}

void rq_finish(struct request *rq)
{
	if (rq->proc) {
		if (rq->blocked)
			pm_unblock(rq->proc);

		sc_late_result(rq->proc, (dword)rq->result);
	}
	else if (rq->func) {
		rq->func(rq->file, rq->result, rq->buffer, rq->buflen);
	}
	kfree(rq);
}
