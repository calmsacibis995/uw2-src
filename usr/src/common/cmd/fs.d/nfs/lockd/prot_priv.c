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

#ident	"@(#)nfs.cmds:lockd/prot_priv.c	1.4"
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
 * consists of all private protocols for comm with
 * status monitor to handle crash and recovery
 */

#include <stdio.h>
#include <netdb.h>
#include <memory.h>
#include <sys/param.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include "prot_lock.h"
#include "priv_prot.h"
#include "sm_inter.h"

extern int debug;
extern int pid;
extern char hostname[MAXNAMELEN];
extern int local_state;
extern struct msg_entry *retransmitted();
void proc_priv_crash(), proc_priv_recovery();
void reclaim_locks();
extern struct lm_vnode *find_me();
extern msg_entry *msg_q;	/* head of msg queue */
void reclaim_pending();

int cookie;

void
priv_prog(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	char *(*Local)();
	struct status stat;
	extern bool_t xdr_status();

	if (debug)
		printf("Enter PRIV_PROG ...............\n");

	switch (rqstp->rq_proc) {
	case PRIV_CRASH:
		Local = (char *(*)()) proc_priv_crash;
		break;
	case PRIV_RECOVERY:
		Local = (char *(*)()) proc_priv_recovery;
		break;
	default:
		svcerr_noproc(transp);
		return;
	}

	(void)memset(&stat, 0, sizeof (struct status));
	if (!svc_getargs(transp, xdr_status, (char *)&stat)) {
		svcerr_decode(transp);
		return;
	}
	(*Local)(&stat);
	if (!svc_sendreply(transp, xdr_void, NULL)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_status, (char *)&stat)) {
		syslog(LOG_ERR, gettxt(":123", "%s: cannot free arguments"),
		       "priv_prog");
		exit(1);
	}
}

void
proc_priv_crash(statp)
	struct status *statp;
{
	struct hostent *hp;
	char buf[128];
	struct flock ld;
	int cmd, fd, err;

	if (debug)
		printf("enter proc_priv_CRASH....\n");

	if ((hp = gethostbyname(statp->mon_name)) == NULL) {
                if (debug)
                        printf( "RPC_UNKNOWNHOST\n");
        }
	sprintf(buf, "%02x%02x%02x%02x",
		(u_char) hp->h_addr[0], (u_char) hp->h_addr[1],
                (u_char) hp->h_addr[2], (u_char) hp->h_addr[3]);
	sscanf(buf,"%x", &ld.l_sysid);

	fd = open("/tmp/foo", O_CREAT|O_RDWR);

	cmd = F_RSETLK;
#ifdef NOTYET
	ld.l_type = F_UNLKSYS;
#else
	ld.l_type = 4;
#endif
	ld.l_whence = 0;
       	ld.l_start = 0;
       	ld.l_len = 0;
       	ld.l_pid = getpid();
       	ld.l_rpid = getpid();
       	if (debug) {
               	printf("ld.l_start=%d ld.l_len=%d ld.l_rpid=%d ld.l_sysid=%x\n",
                       	ld.l_start, ld.l_len, ld.l_rpid, ld.l_sysid);
       	}
       	if ((err = fcntl(fd, cmd, &ld)) == -1) {
               	syslog(LOG_ERR, gettxt(":124", "%s: %s failed: %m"),
		       "proc_priv_crash", "fcntl");
		syslog(LOG_ERR, gettxt(":125", "%s: cannot clear a lock."),
		       "proc_priv_crash");
       	}
	close(fd);
	delete_hash(statp->mon_name);
	/*
	 * In case /tmp/foo never get removed.
	 */
	unlink("/tmp/foo");
	return;
}

void
proc_priv_recovery(statp)
	struct status *statp;
{
	struct lm_vnode *mp;
	struct priv_struct *privp;

	if (debug)
		printf("enter proc_priv_RECOVERY.....\n");
	privp = (struct priv_struct *) statp->priv;
	if (privp->pid != pid) {
		if (debug)
			printf("this is not for me(%d): %d\n", privp->pid, pid);
		return;
	}

	if (debug)
		printf("enter proc_lm_recovery due to %s state(%d)\n",
			statp->mon_name, statp->state);

	destroy_client_shares(statp->mon_name);

	delete_hash(statp->mon_name);
	if (!up(statp->state)) {
		if (debug)
			printf("%s is not up.\n", statp->mon_name);
		return;
	}
	if (strcmp(statp->mon_name, hostname) == 0) {
		if (debug)
			printf("I have been declared as failed!!!\n");
		/*
		 * update local status monitor number
		 */
		local_state = statp->state;
	}

	mp = find_me(statp->mon_name);
	reclaim_locks(mp->exclusive);
	reclaim_locks(mp->shared);
	reclaim_pending(mp->pending);
}

/*
 * reclaim_locks() -- will send out reclaim lock requests to the server.
 *		      listp is the list of established/granted lock requests.
 */
void
reclaim_locks(listp)
	struct reclock *listp;
{
	struct reclock *ff;

	for (ff = listp; ff; ff = ff->next) {
		/* set reclaim flag & send out the request */
		ff->reclaim = 1;
		if (nlm_call(NLM_LOCK_RECLAIM, ff, 0) == -1) {
			if (queue(ff, NLM_LOCK_RECLAIM) == NULL)
				syslog(LOG_ERR,
				       gettxt(":126", "%s: reclaim request (%x) cannot be sent and cannot be queued for resend later!"), 
				       "reclaim_locks", ff);
		}
		if (ff->next == listp)	return;
	}
}

/*
 * reclaim_pending() -- will setup the existing queued msgs of the pending
 *		      	lock requests to allow retransmission.
 *			note that reclaim requests for these pending locks
 *			r not sent out.
 */
void
reclaim_pending(listp)
	struct reclock *listp;
{
	msg_entry *msgp;
	struct reclock *ff;

	/* for each pending lock request for the recovered server */
	for (ff = listp; ff; ff = ff->next) {

		/* find the msg in the queue that holds this lock request */
		for (msgp = msg_q; msgp != NULL; msgp = msgp->nxt) {
			
			/* if msg is found, free & nullified the exisiting */
			/* response of this lock request.  this will allow */
			/* retransmission of the requests.		   */
			if (msgp->req == ff) {
				if (msgp->reply != NULL) {
					release_res(msgp->reply);
					msgp->reply = NULL;
				}
				break;
			}
		}
		if (ff->next == listp)	return;
	}
	return;
}

