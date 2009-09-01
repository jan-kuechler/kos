#include <bitop.h>
#include <string.h>
#include <errno.h>
#include "ipc.h"
#include "syscall.h"
#include "mm/util.h"
#include "mm/virt.h"

static int send(struct proc *proc, msg_t *msg)
{
	/* check args */
	if (proc->status == PS_SLOT_FREE)
		return EINVAL;

	if (!msg)
		return EINVAL;

	if (!rbuf_freesize(proc->msgbuffer)) {
		return EAGAIN;
	}

	rbuf_write(proc->msgbuffer, msg, 1);

	return 0;
}

static int receive(struct proc *proc, msg_t *msg)
{
	/* check args */
	if (proc->status == PS_SLOT_FREE)
		return EINVAL;

	if (!msg)
		return EINVAL;

	if (rbuf_empty(proc->msgbuffer))
		return EAGAIN;

	rbuf_read(proc->msgbuffer, msg, 1);

	return 0;
}

/**
 *  ipc_send(from, to, msg)
 *
 * Sends a message from a process to another.
 * This function may unblock the 'to'-proc, when it
 * was blocked for receiving a not yet exisiting message.
 */
int ipc_send(struct proc *from, struct proc *to, msg_t *msg)
{
	msg->sender = from->pid;
	int err = send(to, msg);

	// if the target was blocked by waiting for a message wake it up
	if (pm_is_blocked_for(to, BR_RECEIVING)) {
		receive(to, to->msg_waitbuf);
		km_free_addr(to->msg_waitbuf, sizeof(msg_t));
		to->msg_waitbuf = NULL;
		pm_unblock(to);
	}

	return err;
}

/**
 *  ipc_receive(proc, msg, block)
 *
 * Tries to receive a message for a process.
 * If there is no message yet the process is blocked (when 'block' is true)
 * or EAGAIN is returnd.
 */
int ipc_receive(struct proc *proc, msg_t *msg, byte block)
{
	int err = receive(proc, msg);

	// if there's no message to receive and we
	// should block, disable the process
	if (err == EAGAIN && block) {
		proc->msg_waitbuf = msg;
		pm_block(proc, BR_RECEIVING);
		return 0;
	}
	else if (!err) {
		km_free_addr(msg, sizeof(msg_t));
	}

	return err;
}

int32_t sys_send(int32_t target, int32_t msgptr)
{
	struct proc *proc = pm_get_proc(target);
	msg_t *msg = vm_user_to_kernel(syscall_proc->as->pdir, (vaddr_t)msgptr,
	                               sizeof(msg_t));

	int result = ipc_send(syscall_proc, proc, msg);

	km_free_addr(msg, sizeof(msg_t));

	return result;
}

int32_t sys_receive(int32_t msgptr, int32_t block)
{
	msg_t *msg = vm_user_to_kernel(syscall_proc->as->pdir, (vaddr_t)msgptr,
	                               sizeof(msg_t));

	// receive has to free the address
	return ipc_receive(syscall_proc, msg, block);
}

void init_ipc(void)
{
	syscall_register(SC_SEND, sys_send, 2);
	syscall_register(SC_RECV, sys_receive, 2);
}
