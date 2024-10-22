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

#ident	"@(#)nfs.cmds:lockd/prot_alloc.c	1.1.1.4"
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
 * prot_alloc.c
 * consists of routines used to allocate and free space
 */

#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include "prot_lock.h"

char *xmalloc(), *malloc();
extern int res_len, lock_len, debug;

struct process_locks *proc_locks = NULL;
struct process_locks *free_proc_locks = NULL;

/* # of entries used */
int used_reclock = 0;
int used_res = 0;
int used_me = 0;

void
xfree(a)
	char **a;
{
	if (*a != NULL) {
		free(*a);
		*a = NULL;
	}
}

release_res(resp)
	remote_result *resp;
{
	used_res--;
	xfree(&resp->cookie_bytes);
	xfree(&resp->stat.nlm_testrply_u.holder.oh_bytes);
	free((char *)resp);
}

free_reclock(a)
	reclock *a;
{
	if (debug)
		printf("enter free_reclock..release %x\n", a);

	used_reclock--;
	/*
	 * Make sure that this lock is not queued on msg_q.
	 * This addresses bugs 1012630 and 1011992, though
	 * the cause remains unknown.
	 */
	dequeue_lock(a);
	/* free up all space allocated through malloc */
	xfree(&a->lck.fh_bytes);
	xfree(&a->lck.oh_bytes);
	xfree(&a->cookie_bytes);
	if (a->lck.svr == a->lck.caller_name) {
		if (a->lck.svr == a->lck.clnt_name) {
			xfree(&a->lck.svr);
			a->lck.clnt_name=
				a->lck.caller_name = NULL;
		} else {
			xfree(&a->lck.svr);
			a->lck.caller_name = NULL;
			xfree(&a->lck.clnt_name);
		}
	} else if (a->lck.clnt_name == a->lck.caller_name) {
		xfree(&a->lck.clnt_name);
		a->lck.caller_name = NULL;
		xfree(&a->lck.svr);
	} else if (a->lck.svr == a->lck.clnt_name) {
		xfree(&a->lck.svr);
		a->lck.clnt_name = NULL;
		xfree(&a->lck.caller_name);
	} else {
		xfree(&a->lck.clnt_name);
		xfree(&a->lck.caller_name);
		xfree(&a->lck.svr);
	}
	(void) memset((char *) a, 0, sizeof (*a));
	free((char *) a);
}

release_reclock(a)
	reclock *a;
{
	if (a->rel) {
		free_reclock(a);
	}
}

release_nlm_lockargs(a)
	nlm_lockargs *a;
{
	/* free up all space allocated through malloc */
	xfree(&a->cookie_bytes);
	xfree(&a->lck.caller_name);
	xfree(&a->lck.fh_bytes);
	xfree(&a->lck.oh_bytes);
	(void) memset((char *) a, 0, sizeof (*a));
	free((char *) a);
}

release_nlm_unlockargs(a)
	nlm_unlockargs *a;
{
	/* free up all space allocated through malloc */
	xfree(&a->cookie_bytes);
	xfree(&a->lck.caller_name);
	xfree(&a->lck.fh_bytes);
	xfree(&a->lck.oh_bytes);
	(void) memset((char *) a, 0, sizeof (*a));
	free((char *) a);
}

release_nlm_testargs(a)
	nlm_testargs *a;
{
	/* free up all space allocated through malloc */
	xfree(&a->cookie_bytes);
	xfree(&a->lck.caller_name);
	xfree(&a->lck.fh_bytes);
	xfree(&a->lck.oh_bytes);
	(void) memset((char *) a, 0, sizeof (*a));
	free((char *) a);
}

release_nlm_cancargs(a)
	nlm_cancargs *a;
{
	/* free up all space allocated through malloc */
	xfree(&a->cookie_bytes);
	xfree(&a->lck.caller_name);
	xfree(&a->lck.fh_bytes);
	xfree(&a->lck.oh_bytes);
	(void) memset((char *) a, 0, sizeof (*a));
	free((char *) a);
}

release_nlm_testres(a)
	nlm_testres *a;
{
	/* free up all space allocated through malloc */
	xfree(&a->cookie_bytes);
	xfree(&a->stat.nlm_testrply_u.holder.oh_bytes);
	(void) memset((char *) a, 0, sizeof (*a));
	free((char *) a);
}

release_nlm_res(a)
	nlm_res *a;
{
	/* free up all space allocated through malloc */
	xfree(&a->cookie_bytes);
	(void) memset((char *) a, 0, sizeof (*a));
	free((char *) a);
}

release_klm_lockargs(a)
	klm_lockargs *a;
{
	/* free up all space allocated through malloc */
	xfree(&a->lck.svr);
	xfree(&a->lck.fh_bytes);
	(void) memset((char *) a, 0, sizeof (*a));
	free((char *) a);
}

release_klm_unlockargs(a)
	klm_unlockargs *a;
{
	/* free up all space allocated through malloc */
	xfree(&a->lck.svr);
	xfree(&a->lck.fh_bytes);
	(void) memset((char *) a, 0, sizeof (*a));
	free((char *) a);
}

release_klm_testargs(a)
	klm_testargs *a;
{
	/* free up all space allocated through malloc */
	xfree(&a->lck.svr);
	xfree(&a->lck.fh_bytes);
	(void) memset((char *) a, 0, sizeof (*a));
	free((char *) a);
}


/*
 * allocate space and zero it;
 * in case of malloc error, print console msg and return NULL;
 */
char *
xmalloc(len)
	unsigned len;
{
	char *new;

	if ((new = malloc(len)) == 0) {
		syslog(LOG_ERR, gettxt(":114", "%s: %s failed"), 
		       "xmalloc", "malloc");
		return (NULL);
	} else {
		(void) memset(new, 0, len);
		return (new);
	}
}


/*
 * these routines are here in case we try to optimize calling to malloc
 */
struct lm_vnode *
get_me()
{
	used_me++;
	return ((struct lm_vnode *) xmalloc((u_int) sizeof (struct lm_vnode)));
}

remote_result *
get_res()
{
	used_res++;
	return ((remote_result *) xmalloc((u_int) res_len));
}

reclock *
get_reclock()
{
	reclock	*tmp;

	used_reclock++;
	if ((tmp = (reclock *) xmalloc(lock_len)) != NULL)
		tmp->lck.lox.req = tmp; /* set the data_lock back ptr to req */
	return (tmp);
}

klm_lockargs *
get_klm_lockargs()
{
	return ((struct klm_lockargs *) xmalloc((u_int) sizeof (struct klm_lockargs)));
}

klm_unlockargs *
get_klm_unlockargs()
{
	return ((struct klm_unlockargs *)
		xmalloc((u_int) sizeof (struct klm_unlockargs)));
}

klm_testargs *
get_klm_testargs()
{
	return ((struct klm_testargs *) xmalloc((u_int) sizeof (struct klm_testargs)));
}

nlm_lockargs *
get_nlm_lockargs()
{
	return ((struct nlm_lockargs *) xmalloc((u_int) sizeof (struct nlm_lockargs)));
}

nlm_unlockargs *
get_nlm_unlockargs()
{
	return ((struct nlm_unlockargs *)
		xmalloc((u_int) sizeof (struct nlm_unlockargs)));
}

nlm_cancargs *
get_nlm_cancargs()
{
	return ((struct nlm_cancargs *) xmalloc((u_int) sizeof (struct nlm_cancargs)));
}

nlm_testargs *
get_nlm_testargs()
{
	return ((struct nlm_testargs *) xmalloc((u_int) sizeof (struct nlm_testargs)));
}

nlm_testres *
get_nlm_testres()
{
	return ((struct nlm_testres *) xmalloc((u_int) sizeof (struct nlm_testres)));
}

nlm_res *
get_nlm_res()
{
	return ((struct nlm_res *) xmalloc((u_int) sizeof (struct nlm_res)));
}

char *
copy_str(str)
	char *str;
{
	char *tmpstr;

	if (str != NULL) {
		if ((tmpstr = xmalloc((u_int) (strlen(str)+1))) != NULL) {
			(void) strcpy(tmpstr, str);
			return (tmpstr);
		}
	}
	return (NULL);
}

void
release_klm_req(proc, klm_req)
	int proc;
	char *klm_req;
{
	switch (proc) {
	case KLM_TEST:
		release_klm_testargs(klm_req);
		break;
	case KLM_LOCK:
	case KLM_CANCEL:
		release_klm_lockargs(klm_req);
		break;
	case KLM_UNLOCK:
		release_klm_unlockargs(klm_req);
		break;
	}
	return;
}

void
release_nlm_req(proc, nlm_req)
	int proc;
	char *nlm_req;
{
	switch (proc) {
	case NLM_TEST:
	case NLM_TEST_MSG:
	case NLM_GRANTED_MSG:
		release_nlm_testargs(nlm_req);
		break;
	case NLM_LOCK:
	case NLM_LOCK_MSG:
	case NLM_LOCK_RECLAIM:
	case NLM_NM_LOCK:
		release_nlm_lockargs(nlm_req);
		break;
	case NLM_CANCEL:
	case NLM_CANCEL_MSG:
		release_nlm_cancargs(nlm_req);
		break;
	case NLM_UNLOCK:
	case NLM_UNLOCK_MSG:
		release_nlm_unlockargs(nlm_req);
		break;
	}
	return;
}


void
release_nlm_rep(proc, nlm_rep)
	int proc;
	char *nlm_rep;
{
	switch (proc) {
	case NLM_TEST_RES:
		release_nlm_testres(nlm_rep);
		break;
	case NLM_LOCK_RES:
	case NLM_CANCEL_RES:
	case NLM_UNLOCK_RES:
	case NLM_GRANTED_RES:
		release_nlm_res(nlm_rep);
		break;
	}
	return;
}

/*
 * Function get_proc_list(p)
 *
 * This function will use one of the process lock list slots and assign
 * it to this process.
 * XXX - enhance performance with free_procs list pointer ?
 */
struct process_locks *
get_proc_list(p)
	long	p;	/* Process ID to use */
{
	struct process_locks *proclist;

	if (debug)
		printf("enter get_proc_list()...");

	/*
	 * Reuse process lock list slots.
	 */
	proclist = free_proc_locks;

	if (free_proc_locks == NULL)
		proclist = (struct process_locks *) xmalloc((u_int) sizeof (*proclist));
	else
		free_proc_locks = free_proc_locks->next;

	(void) memset((caddr_t) proclist, 0, sizeof (*proclist));
	proclist->pid = p;
	proclist->next = proc_locks;
	proc_locks = proclist;
	proclist->lox = NULL;
	return (proclist);
}
