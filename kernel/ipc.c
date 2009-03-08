#include <bitop.h>
#include <kos/error.h>
#include "ipc.h"
#include "pm.h"
#include <string.h>

static byte send(pid_t p, msg_t *msg)
{
	proc_t *proc = pm_get_proc(p);

	if (proc->status == PS_SLOT_FREE)
		return E_INVALID_ARG;

	if (!msg)
		return E_INVALID_ARG;

	if (proc->msg_count == PROC_MSG_BUFFER_SIZE) {
		return E_TRY_AGAIN;
	}

	memcpy(proc->msg_head, msg, sizeof(msg_t));
	proc->msg_head++;
	proc->msg_count++;

	if (proc->msg_head == &proc->msg_buffer[PROC_MSG_BUFFER_SIZE])
		proc->msg_head = proc->msg_buffer;

	return OK;
}

static byte receive(pid_t p, msg_t *msg)
{
	proc_t *proc = pm_get_proc(p);

	if (proc->status == PS_SLOT_FREE)
		return E_INVALID_ARG;

	if (!msg)
		return E_INVALID_ARG;

	if (proc->msg_count == 0)
		return E_TRY_AGAIN;

	memcpy(msg, proc->msg_tail, sizeof(msg_t));
	proc->msg_tail++;
	proc->msg_count--;

	if (proc->msg_tail == &proc->msg_buffer[PROC_MSG_BUFFER_SIZE])
		proc->msg_tail = proc->msg_buffer;

	return OK;
}

byte ipc_send(pid_t from, pid_t to, msg_t *msg, byte block)
{
	msg->sender = from;
	byte status = send(to, msg);

	if (status == E_TRY_AGAIN && block) {
		proc_t *proc = pm_get_proc(from);
		bset(proc->flags, PF_SENDING);
		proc->status = PS_BLOCKED;
		pm_deactivate(proc);
		return OK;
	}

	return status;
}

byte ipc_receive(pid_t p, msg_t *msg, byte block)
{
	byte status = receive(p, msg);

	if (status == E_TRY_AGAIN && block) {
		proc_t *proc = pm_get_proc(p);
		bset(proc->flags, PF_RECEIVING);
		proc->status = PS_BLOCKED;
		pm_deactivate(proc);
		return OK;
	}

	return status;
}
