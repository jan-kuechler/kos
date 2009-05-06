#include <errno.h>
#include "syscall.h"
#include "fs/request.h"
#include "mm/kmalloc.h"

request_t *rq_create(void *buffer, dword buflen, proc_t *proc)
{
	request_t *rq = kmalloc(sizeof(request_t));

	rq->buffer = buffer;
	rq->buflen = buflen;

	rq->proc    = proc ? proc : cur_proc;
	rq->blocked = 0;
	rq->result  = -EINVAL;

	return rq;
}

void rq_block(request_t *rq)
{
	pm_block(rq->proc, BR_WAIT_FS);
	rq->blocked = 1;
}

void rq_finish(request_t *rq)
{
	if (rq->blocked)
		pm_unblock(rq->proc);

	sc_late_result(rq->proc, rq->result);

	kfree(rq);
}
