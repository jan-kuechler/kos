#ifndef REQUEST_H
#define REQUEST_H

#include "pm.h"
#include "fs/types.h"

struct request {
	struct file *file;
	proc_t *proc;
	int     blocked;

	dword   result;

	void 	 *buffer;
	dword   buflen;

	fscallback_t func;
};

/**
 *  rq_create(inode, buffer, buflen)
 *
 * Creates a new request and fills in some information.
 * The created request must be destroyed using rq_finish.
 */
struct request *rq_create(struct file *file, void *buffer, dword buflen);

/**
 *  rq_block(rq)
 *
 * Blocks the requesting process.
 */
void rq_block(struct request *rq);

/**
 *  rq_finish(rq)
 *
 * Unblocks the previously blocked process and returns
 * the result from the request.
 * This function destroys the request, do not use it after
 * a call to rq_finish.
 */
void rq_finish(struct request *rq);

#endif /*REQUEST_H*/
