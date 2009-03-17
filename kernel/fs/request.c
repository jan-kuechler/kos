#include <kos/error.h>

#include "pm.h"
#include "fs/request.h"

/**
 *  fs_query_rq(rq, wait, proc)
 *
 * Querys a request.
 */
int fs_query_rq(fs_request_t *rq, int wait, proc_t *proc)
{
	if (!rq->fs->query) {
		return E_NOT_SUPPORTED;
	}

	rq->proc = proc;

	int status = rq->fs->query(rq->fs, rq);
	if (status != OK) return status;

	if (wait)
		return fs_wait_rq(rq);

	return OK;
}

/**
 *  fs_wait_rq(rq)
 *
 * Blocks the current proc when the request isn't finished.
 */
int fs_wait_rq(fs_request_t *rq)
{
	if (rq->finished) return OK;

	rq->wait_proc = cur_proc;
	pm_block(rq->wait_proc, BR_WAIT_FS);

	return OK;
}

/**
 *  fs_finish_rq(rq)
 *
 * Finishes the rq and unblocks a maybe blocked waiting proc.
 */
int fs_finish_rq(fs_request_t *rq)
{
	rq->finished = 1;
	if (rq->wait_proc) {
		rq->wait_proc->sc_regs->eax = rq->result;

		pm_unblock(rq->wait_proc);
	}

	return OK;
}
