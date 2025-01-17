/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)nfs.cmds:lockd/prot_pklm.c	1.4"
#ident	"$Header: $"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

/*
 * prot_pklm.c
 * consists of all procedures called by klm_prog
 */

#include <stdio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include "prot_lock.h"

int tmp_ck;				/* flag to trigger tmp check */
extern int debug;
extern msg_entry *klm_msg, *search_msg();	/* record last klm msg */
extern msg_entry *msg_q;
extern SVCXPRT *klm_transp;

extern msg_entry *retransmitted(), *queue();
extern bool_t remote_data();
extern char *copy_str();
extern remote_result res_working;

remote_result 	*remote_test();
remote_result 	*remote_lock();
remote_result 	*remote_unlock();
remote_result 	*remote_cancel();
remote_result   *remote_grant();

int 		klm_lockargstoreclock();
int 		klm_unlockargstoreclock();
int 		klm_testargstoreclock();

proc_klm_test(a)
	reclock *a;
{
	klm_msg_routine(a, KLM_TEST, remote_test);
}

proc_klm_lock(a)
	reclock *a;
{
	klm_msg_routine(a, KLM_LOCK, remote_lock);
}

proc_klm_cancel(a)
	reclock *a;
{
	tmp_ck = 1;
	klm_msg_routine(a, KLM_CANCEL, remote_cancel);
}

proc_klm_unlock(a)
	reclock *a;
{
	klm_msg_routine(a, KLM_UNLOCK, remote_unlock);
}

proc_klm_granted(a)
	reclock *a;
{
	klm_msg_routine(a, KLM_GRANTED, remote_grant);
}

/*
 * common routine to handle msg passing form of communication;
 * klm_msg_routine is shared among all klm procedures:
 * proc_klm_test, proc_klm_lock, proc_klm_cancel, proc_klm_unlock;
 * proc specifies the name of the routine to branch to for reply purpose;
 * local and remote specify the name of routine that handles the call
 *
 * when a msg arrives, it is first checked to see
 *   if retransmitted;
 *	 if a reply is ready,
 *	   a reply is sent back and msg is erased from the queue
 *	 or msg is ignored!
 *   else if this is a new msg;
 *	 if data is remote
 *		a rpc request is send and msg is put into msg_queue,
 *	 else (request lock or similar lock)
 *		reply is sent back immediately.
 */
klm_msg_routine(a, proc, remote)
	reclock *a;
	int proc;
	remote_result *(*remote)();
{
	struct msg_entry *msgp;
	remote_result *result;
	remote_result resp;

	if (debug) {
		printf("\nenter klm_msg_routine(proc =%d): op=%d, (%d, %d) by ",
			proc, a->lck.op, a->lck.lox.base, a->lck.lox.length);
		pr_oh(&a->lck.oh);
		printf("\n");
		pr_lock(a);
		(void) fflush(stdout);
	}

	if ((msgp = retransmitted(a, proc)) != NULL) { /* retransmitted msg */
		if (debug)
			printf("retransmitted msg!\n");
		a->rel = 1;
		if (msgp->reply == NULL) {
			klm_reply(proc, &res_working);
			return;
		} else {
			/*
			 * reply to kernel and dequeue only if we have
			 * our blocking request result
			 */
			klm_reply(proc, msgp->reply);

			if (msgp->reply->lstat != blocking) {
				if ((msgp->proc != NLM_LOCK) &&
				    (msgp->proc != NLM_LOCK_MSG) &&
				    (msgp->proc != NLM_LOCK_RECLAIM)) {
					/* set free reclock */
					if (msgp->req != NULL)
						msgp->req->rel = 1;
				}
				dequeue(msgp);
			}
			return;
		}
	}

	result = remote(a, MSG); /* specify msg passing type of comm */

	/* if we receive KLM_GRANTED from the kernel, reply that we got it */
	if ((proc == KLM_GRANTED) || (proc == KLM_CANCEL)) {
		resp.lstat = klm_granted;
		klm_reply(proc, &resp);
	} else if (result != NULL && result->lstat != blocking) {
		msgp = queue(a, proc);
		msgp->reply = result;
		klm_msg = msgp;
		klm_reply(proc, msgp->reply);
		/*
		 * set rel= 0, and reset it back to 1 after dequeue
		 * otherwise, dequeue() will try to free the structure
		 */
		a->rel = 0;
		dequeue(msgp);
		a->rel = 1;
	}
}

/*
 * klm_reply send back reply from klm to requestor(kernel):
 * proc specify the name of the procedure return the call;
 * corresponding xdr routines are then used;
 */
klm_reply(proc, reply)
	int proc;
	remote_result *reply;
{
	bool_t (*xdr_reply)();
	klm_testrply args;
	register klm_testrply *argsp = &args;
	klm_stat stat_args;
	register klm_stat *stat_argsp = &stat_args;

	switch (proc) {
	case KLM_TEST:
	case NLM_TEST_MSG:	/* record in msgp->proc */
		xdr_reply = xdr_klm_testrply;
		if (reply->lstat == nlm_granted) {
			argsp->stat = klm_granted;
		} else {
			if (reply->lstat == nlm_denied)
				argsp->stat = klm_denied;
			else if (reply->lstat == nlm_denied_nolocks)
				argsp->stat = klm_denied_nolocks;
			argsp->klm_testrply_u.holder.pid =
				reply->stat.nlm_testrply_u.holder.svid;
			argsp->klm_testrply_u.holder.base =
				reply->stat.nlm_testrply_u.holder.l_offset;
			argsp->klm_testrply_u.holder.length =
				reply->stat.nlm_testrply_u.holder.l_len;
			if (reply->stat.nlm_testrply_u.holder.exclusive)
				argsp->klm_testrply_u.holder.exclusive = TRUE;
			else
				argsp->klm_testrply_u.holder.exclusive = FALSE;
		}
		if (debug) {
			printf("KLM_REPLY : svid=%d l_offset=%d l_len=%d\n",
				argsp->klm_testrply_u.holder.pid,
				argsp->klm_testrply_u.holder.base,
				argsp->klm_testrply_u.holder.length);
		}
		if (!svc_sendreply(klm_transp, xdr_reply, (caddr_t)argsp))
			svcerr_systemerr(klm_transp);
		break;
	case KLM_LOCK:
	case NLM_LOCK_MSG:
	case NLM_LOCK_RECLAIM:
	case KLM_CANCEL:
	case NLM_CANCEL_MSG:
	case KLM_UNLOCK:
	case NLM_UNLOCK_MSG:
	case NLM_GRANTED: /* same as KLM_GRANTED */
	case NLM_GRANTED_MSG:
		xdr_reply = xdr_klm_stat;
		if (reply->lstat == nlm_denied)
			stat_argsp->stat = klm_denied;
		else if (reply->lstat == nlm_granted)
			stat_argsp->stat = klm_granted;
		else if (reply->lstat == nlm_denied_nolocks)
			stat_argsp->stat = klm_denied_nolocks;
		else if (reply->lstat == nlm_deadlck)
			stat_argsp->stat = klm_deadlck;
		else if (reply->lstat == nlm_blocked)
			stat_argsp->stat = klm_working;
		if (debug) {
			printf("KLM_REPLY : stat=%d\n",
				stat_argsp->stat);
			if (klm_msg && klm_msg->req)
				printf("KLM_REPLY : klm_msg->req=%x\n",
					klm_msg->req);
		}
		if (!svc_sendreply(klm_transp, xdr_reply, (caddr_t)stat_argsp))
			svcerr_systemerr(klm_transp);
		break;
	default:
		xdr_reply = xdr_void;
		syslog(LOG_ERR, gettxt(":139",
				       "%s: %s value (%d) is not supported"),
		       "klm_reply", "proc", proc);
		if (!svc_sendreply(klm_transp, xdr_reply,
					(caddr_t)&reply->stat))
			svcerr_systemerr(klm_transp);
	}
	if (debug)
		printf("klm_reply: stat=%d \n", reply->lstat);
	return;
}

int
klm_lockargstoreclock(from, to)
	struct klm_lockargs *from;
	struct reclock *to;
{
	if ((to->lck.server_name = copy_str(from->alock.server_name)) == NULL)
		return (-1);
	if (from->alock.fh.n_len) {
		if (obj_copy(&to->lck.fh, &from->alock.fh) == -1)
			return (-1);
	} else
		to->lck.fh.n_len = 0;
	to->lck.lox.base = from->alock.base;
	to->lck.lox.length = from->alock.length;
	if (from->exclusive) {
		to->lck.lox.type = F_WRLCK;
		to->exclusive = TRUE;
	} else {
		to->lck.lox.type = F_RDLCK;
		to->exclusive = FALSE;
	}
	if (from->block) {
		to->block = TRUE;
	} else {
		to->block = FALSE;
	}
	to->lck.lox.granted = 0;
	to->lck.lox.color = 0;
	to->lck.lox.LockID = 0;
	to->lck.lox.pid = from->alock.pid;
	to->lck.lox.class = LOCKMGR;
	to->cookie.n_len = from->alock.pid;
	to->lck.lox.rsys = from->alock.rsys;
	to->lck.lox.rpid = from->alock.pid;

	return (0);
}

int
klm_unlockargstoreclock(from, to)
	struct klm_unlockargs *from;
	struct reclock *to;
{
	if ((to->lck.server_name = copy_str(from->alock.server_name)) == NULL)
		return (-1);
	if (from->alock.fh.n_len) {
		if (obj_copy(&to->lck.fh, &from->alock.fh) == -1)
			return (-1);
	} else
		to->lck.fh.n_len = 0;
	to->lck.lox.base = from->alock.base;
	to->lck.lox.length = from->alock.length;
	to->lck.lox.granted = 0;
	to->lck.lox.color = 0;
	to->lck.lox.LockID = 0;
	to->lck.lox.pid = from->alock.pid;
	to->lck.lox.class = LOCKMGR;
	to->lck.lox.rsys = from->alock.rsys;
	to->lck.lox.rpid = from->alock.pid;
	to->cookie.n_len = from->alock.pid;

	return (0);
}

int
klm_testargstoreclock(from, to)
	struct klm_testargs *from;
	struct reclock *to;
{
	if ((to->lck.server_name = copy_str(from->alock.server_name)) == NULL)
		return (-1);
	if (from->alock.fh.n_len) {
		if (obj_copy(&to->lck.fh, &from->alock.fh) == -1)
			return (-1);
	} else
		to->lck.fh.n_len = 0;
	to->lck.lox.base = from->alock.base;
	to->lck.lox.length = from->alock.length;
	if (from->exclusive) {
		to->lck.lox.type = F_WRLCK;
		to->exclusive = TRUE;
	} else {
		to->lck.lox.type = F_RDLCK;
		to->exclusive = FALSE;
	}
	to->block = TRUE;
	to->lck.lox.granted = 0;
	to->lck.lox.color = 0;
	to->lck.lox.LockID = 0;
	to->lck.lox.pid = from->alock.pid;
	to->lck.lox.class = LOCKMGR;
	to->lck.lox.rsys = from->alock.rsys;
	to->lck.lox.rpid = from->alock.pid;
	to->cookie.n_len = from->alock.pid;

	return (0);
}
