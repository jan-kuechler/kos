#ifndef REQUEST_H
#define REQUEST_H

#include "pm.h"

typedef struct request {
	proc_t *proc;
	int     blocked;

	dword   result;

	void 	 *buffer;
	dword   buflen;
} request_t;

/**
 *  rq_create(buffer, buflen, proc)
 *
 * Creates a new request and fills in some information.
 * If proc is NULL cur_proc is used.
 * The created request must be destroyed using rq_finish.
 */
request_t *rq_create(void *buffer, dword buflen, proc_t *proc);

/**
 *  rq_block(rq)
 *
 * Blocks the requesting process.
 */
void rq_block(request_t *rq);

/**
 *  rq_finish(rq)
 *
 * Unblocks the previously blocked process and returns
 * the result from the request.
 * This function destroys the request, do not use it after
 * a call to rq_finish.
 */
void rq_finish(request_t *rq);

#endif /*REQUEST_H*/
