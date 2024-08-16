/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:cmd/lpsched/lpsched/getstatus.c	1.14.3.7"
#ident  "$Header: $"

#include "stdlib.h"
#include "unistd.h"

#include "lpsched.h"

int			Redispatch	= 0;

RSTATUS *		Status_List	= 0;

static SUSPENDED	*Suspend_List	= 0;

#ifdef	__STDC__
RSTATUS *	mkreq ( SSTATUS *, PSTATUS * );
#else
RSTATUS *	mkreq ();
#endif

#define SHOULD_NOTIFY(PRS) 	((PRS)->request->actions & \
				 (ACT_MAIL|ACT_WRITE|ACT_NOTIFY) || \
				 (PRS)->request->alert)

/**
 ** mesgdup()
 **/

static char *
#ifdef	__STDC__
mesgdup (
	char *			m
)
#else
mesgdup (m)
	char *			m;
#endif
{
	DEFINE_FNNAME (mesgdup)

	char *			p;

	unsigned long		size	= msize(m);

	p = Malloc(size);
	(void) memcpy (p, m, size);
	return (p);
}

/**
 ** mesgadd()
 **/

void
#ifdef	__STDC__
mesgadd (
	SSTATUS *		pss,
	char *			mesg
)
#else
mesgadd (pss, mesg)
	SSTATUS *		pss;
	char *			mesg;
#endif
{
	DEFINE_FNNAME (mesgadd)

	size_t			len;


	if (pss->tmp_pmlist) {
		len = lenlist(pss->tmp_pmlist);
		pss->tmp_pmlist = (char **)Realloc(
			pss->tmp_pmlist,
			(len + 2) * sizeof(char *)
		);
		pss->tmp_pmlist[len] = mesgdup(mesg);
		pss->tmp_pmlist[len + 1] = 0;
	} else {
		pss->tmp_pmlist = (char **)Calloc(2, sizeof(char *));
		pss->tmp_pmlist[0] = mesgdup(mesg);
		pss->tmp_pmlist[1] = 0;
	}
}

/**
 ** askforstatus()
 **/

void
#ifdef	__STDC__
askforstatus (
	SSTATUS *		pss,
	MESG *			md
)
#else
askforstatus (pss, md)
	SSTATUS *		pss;
	MESG *			md;
#endif
{
	DEFINE_FNNAME (askforstatus)

	WAITING *		w;

	time_t			now;

	
#if	defined(USE_TIMER)
	/*
	 * If wait is -1, the user has been through all of this once
	 * already and should not be kept waiting. This remedies the
	 * situation where the "md" is waiting for 2 or more systems
	 * and the response from one system comes more than
	 * USER_STATUS_EXPIRED seconds after another has reported back
	 * (i.e., while waiting for one system, the other expired again).
	 * Without this check, the <md> could deadlock always waiting
	 * for the status from one more system.
	 */
	if (md->wait == -1) {
		schedlog ("Already waited for status once, don't wait again.\n");
		return;
	}

	now = time((time_t *)0);
	if ((now - pss->laststat) > USER_STATUS_EXPIRED) {
#endif
		w = (WAITING *)Malloc(sizeof(WAITING));
		w->md = md;
		w->next = pss->waiting;
		pss->waiting = w;
		md->wait++;
		schedlog ("Scheduling status call to %s\n", pss->system->name);
		(void) mkreq (pss, (PSTATUS *)0);
		schedule (EV_SYSTEM, pss);
#if	defined(USE_TIMER)
	} else
		schedlog ("Timer has not expired yet.\n");
#endif

	return;
}

/**
 ** waitforstatus()
 **/

int
#ifdef	__STDC__
waitforstatus (
	char *			m,
	MESG *			md
)
#else
waitforstatus (m, md)
	char *			m;
	MESG *			md;
#endif
{
	DEFINE_FNNAME (waitforstatus)

	SUSPENDED *		s;
	

	if (md->wait <= 0) {
		md->wait = 0;
		schedlog ("No requests to wait for.\n");
		return (-1);
	}

	s = (SUSPENDED *)Malloc(sizeof(SUSPENDED));
	s->message = mesgdup(m);
	s->md = md;

	s->next = Suspend_List;
	Suspend_List = s;

	schedlog ("Suspend %lu for status\n", md);

	return (0);
}

/*
 * Procedure:     load_bsd_stat
 *
 * Restrictions:
 *               open_lpfile: None
 *               Unlink: None
 *               fgets: None
 *               close_lpfile: None
*/

void
#ifdef	__STDC__
load_bsd_stat (
	SSTATUS *		pss,
	PSTATUS *		pps
)
#else
load_bsd_stat (pss, pps)
	SSTATUS *		pss;
	PSTATUS *		pps;
#endif
{
	DEFINE_FNNAME (load_bsd_stat)

	FILE *			fp;

	char			buf[BUFSIZ];
	char			mbuf[MSGMAX];

	char *			file;
	char *			rmesg	= 0;
	char *			dmesg	= 0;
	char *			req	= "";
	char *			cp;
	char *			job_id;

	RSTATUS *		prs;

	time_t			now;

	short			status	= 0;

	short			rank;


	file = pps->alert->msgfile;
	if ((fp = open_lpfile(file, "r", MODE_NOREAD)) == NULL)
		return;
	(void) Unlink (file);
	
	while (fgets(buf, BUFSIZ, fp)) {
		buf[strlen(buf) - 1] = '\0';
	
		schedlog (">>>%s\n", buf);

		switch(*buf) {
		case '%':
			/*
			 * MORE: add code to fetch old status and restore
			 * it
			 */
			break;
		    
		case '-':
			if (strstr(buf + 2, "queue")) {
				schedlog ("Added to reject reason\n");
				status |= PS_REJECTED;
				(void) addstring (&rmesg, buf + 2);
				(void) addstring (&rmesg, "\n");
			} else {
				schedlog ("Added to disable reason\n");
				status |= PS_DISABLED;
				(void) addstring (&dmesg, buf + 2);
				(void) addstring (&dmesg, "\n");
			}
			break;
		    
		default:
			/*
			 * Message format:
			 *
			 *	user:rank:jobid:host
			 */
			(void) strtok (buf, ":");
			if (!(cp = strtok((char *)0, ":")))
				break;
			rank = atoi(cp);
			if (!(job_id = strtok((char *)0, ":")))
				break;
			prs = request_by_jobid (pps->printer->name, job_id,
				pss->system->protocol);
			if (!prs)
			{
				schedlog (
				"Could not find request for jobid (%s)\n",
				job_id);
				break;
			}
			schedlog ("Saving a rank of %d\n", rank);
			prs->status |= (RSS_MARK|RSS_RANK);
			if ((prs->rank = rank) == 0) {
				status |= PS_BUSY;
				req = prs->secure->req_id;
			}
		}
	}

	schedlog ("Cleaning up old requests\n");
	BEGIN_WALK_BY_PRINTER_LOOP (prs, pps)
		if (!(prs->request->outcome & RS_SENT))
			continue;
		if (prs->status & RSS_MARK) {
			prs->status &= ~RSS_MARK;
			continue;
		}
		schedlog ("Completed \"%s\"\n", prs->secure->req_id);
		prs->request->outcome &= ~RS_ACTIVE;
		prs->request->outcome |= RS_PRINTED;
		if (SHOULD_NOTIFY(prs))
			prs->request->outcome |= RS_NOTIFY;
		notify (prs, (char *)0, 0, 0, 0);
		prs->printer->request = 0;
		check_request(prs);
	END_WALK_LOOP

	now = time((time_t *)0);
	schedlog ("Saving printer status\n");
	(void) putmessage(mbuf, R_INQUIRE_REMOTE_PRINTER,
		MOKMORE,
		pps->printer->name,
		"",
		"",
		dmesg,
		rmesg,
		status,
		req,
		(long)now,
		(long)now
	);

	mesgadd (pss, mbuf);

	if (dmesg)
		Free(dmesg);
	if (rmesg)
		Free(rmesg);

	(void) close_lpfile(fp);
	return;
}

/**
 ** update_req()
 **/

void
#ifdef	__STDC__
update_req (
	char *			req_id,
	long			rank
)
#else
update_req (req_id, rank)
	char *			req_id;
	long			rank;
#endif
{
	DEFINE_FNNAME (update_req)

	RSTATUS		*prs;


	if (!(prs = request_by_id(req_id)))
		return;
	
	prs->status |= RSS_RANK;
	prs->rank = rank;

	return;
}

/**
 ** md_wakeup()
 **/

void
#ifdef	__STDC__
md_wakeup (
	SSTATUS *		pss
)
#else
md_wakeup (pss)
	SSTATUS *		pss;
#endif
{
	DEFINE_FNNAME (md_wakeup)

	WAITING *		w;

	int			wakeup	= 0;

	SUSPENDED *		susp;
	SUSPENDED *		newlist	= 0;


	while (pss->waiting) {
		w = pss->waiting;
		pss->waiting = w->next;
		if (--(w->md->wait) <= 0)
			wakeup = 1;
		w->md = (MESG *)0;
		Free (w);
	}

	if (wakeup) {
		while (Suspend_List) {
			susp = Suspend_List;
			Suspend_List = susp->next;
			if (susp->md->wait <= 0) {
				susp->md->wait = -1;
				Redispatch = 1;
				dispatch (mtype(susp->message), susp->message, susp->md);
				Redispatch = 0;
				susp->md = (MESG *)0;
				Free (susp->message);
				Free (susp);
			} else {
				susp->next = newlist;
				newlist = susp;
			}
		}
		Suspend_List = newlist;
	}
}

/**
 ** mkreq()
 **/

RSTATUS *
#ifdef	__STDC__
mkreq (
	SSTATUS *		pss,
	PSTATUS	*		pps
)
#else
mkreq (pss, pps)
	SSTATUS *		pss;
	PSTATUS *		pps;
#endif
{
	DEFINE_FNNAME (mkreq)

	char			idno[STRSIZE(BIGGEST_REQID_S) + 1];

	RSTATUS *		prs;
	RSTATUS *		r;


	/*
	 * Create a fake request with enough information to
	 * fool the various request handling routines.
	 */
	prs = allocr();
	(void) sprintf (idno, "%ld", _alloc_req_id());
	prs->secure->req_id = makestr("(fake)", "-", idno, (char *)0);
	prs->system = pss;
	if (pps)
		prs->printer = pps;

	if (Status_List) {
		for (r = Status_List; r->next; r = r->next)
			;
		r->next = prs;
		prs->prev = r;
	} else
		Status_List = prs;

	return	prs;
}

/**
 ** rmreq()
 **/

void
#ifdef	__STDC__
rmreq (
	RSTATUS *		prs
)
#else
rmreq (prs)
	RSTATUS *		prs;
#endif
{
	DEFINE_FNNAME (rmreq)

	RSTATUS *		old_Request_List;


	/*
	 * UGLY: Rename the "Request_List" to "Status_List",
	 * so that "freerstatus()" (actually "remove()") will
	 * unlink the correct list.
	 */
	old_Request_List = Request_List;
	Request_List = Status_List;
	freerstatus (prs);
	Status_List = Request_List;
	Request_List = old_Request_List;

	return;
}
