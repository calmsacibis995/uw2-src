/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:cmd/lpsched/lpsched/disp2.c	1.9.8.6"
#ident  "$Header: disp2.c 1.2 91/06/27 $"

#include "dispatch.h"

extern long	time();

/**
 ** untidbit_all() - CALL untidbit() FOR A LIST OF TYPES
 **/

#ifdef	__STDC__
static void
untidbit_all (char **printer_types)
#else
static void
untidbit_all (printer_types)

char **printer_types;
#endif
{
	DEFINE_FNNAME (untidbit_all)

	char **	pl;

	for (pl = printer_types; *pl; pl++)
		untidbit (*pl);
	return;
}

/*
 * Procedure:     s_load_printer
 *
 * Restrictions:
 *               Unlink: None
 *               mputm: None
*/

#ifdef	__STDC__
int
s_load_printer (char *m, MESG *md)
#else
int
s_load_printer (m, md)

char * m;
MESG * md;
#endif
{
	char	*printer;
	ushort	status;
	PRINTER	op;

	register PRINTER	*pp;
	register PSTATUS	*pps;

	DEFINE_FNNAME (s_load_printer)

	ENTRYP
	(void)getmessage (m, S_LOAD_PRINTER, &printer);

	if (!*printer)
	{
		TRACEP ("No printer specified in message.")
		status = MNODEST;
		goto	Return;
	}
	/*
	 * Strange or missing printer?
	 */
	if (!(pp = Getprinter (printer)))
	{
		switch (errno) {
		case EBADF:
			TRACEP ("Getprinter failed with EBADF.")
			status = MERRDEST;
			break;
		case ENOENT:
		case ENODATA:
			TRACEP ("Getprinter failed with ENOENT.")
			status = MNODEST;
			break;
		default:
			TRACEP ("Getprinter failed.")
			TRACEd (errno)
			status = MNODEST;
			break;
		}
		goto	Return;
	}
	/*
	 * Printer we know about already?
	 */
	if ((pps = search_ptable(printer)))
	{
		op = *(pps->printer);
		*(pps->printer) = *pp;

		/*
		 * Ensure that an old Terminfo type that's no longer 
		 * needed gets freed, and that an existing type gets
		 * reloaded (in case it has been changed).
		 */
		untidbit_all (op.printer_types);
		untidbit_all (pp->printer_types);

		/*
		 * Does an alert get affected?
		 *	- Different command?
		 *	- Different wait interval?
		 */
		if (pps->alert->active)

#define	OALERT	op.fault_alert
#define NALERT	pp->fault_alert
			if (!SAME(NALERT.shcmd, OALERT.shcmd)
				|| NALERT.W != OALERT.W)
			{
				/*
				 * We can't use "cancel_alert()" here
				 * because it will remove the message.
				 * We'll do half of the cancel, then
				 * check if we need to run the new alert,
				 * and remove the message if not.
				 */
				pps->alert->active = 0;
				terminate (pps->alert->exec);
				if (NALERT.shcmd)
					alert (A_PRINTER, pps,
						(RSTATUS *)0, (char *)0);
				else
					(void) Unlink (pps->alert->msgfile);
			}
#undef	OALERT
#undef	NALERT

		freeprinter (&op);

		unload_list (&pps->users_allowed);
		unload_list (&pps->users_denied);
		unload_list (&pps->forms_allowed);
		unload_list (&pps->forms_denied);
		(void) load_userprinter_access (
				pp->name,
				&pps->users_allowed,
				&pps->users_denied
		);
		(void) load_formprinter_access (
				pp->name,
				&pps->forms_allowed,
				&pps->forms_denied
		);

		load_sdn (&pps->cpi, pp->cpi);
		load_sdn (&pps->lpi, pp->lpi);
		load_sdn (&pps->plen, pp->plen);
		load_sdn (&pps->pwid, pp->pwid);

		pps->last_dial_rc = 0;
		pps->nretry = 0;

		init_remote_printer (pps, pp);

		/*
		 * Evaluate all requests queued for this printer,
		 * to make sure they are still eligible. They will
		 * get moved to another printer, get (re)filtered,
		 * or get canceled.
		 */
		(void)queue_repel (pps, 0, (qchk_fnc_type)0);

		status = MOK;
		goto	Return;
	}
	/*
	 * Room for new printer?
	 */
	if ((pps = search_ptable((char *)0)))
	{
		pps->status = PS_DISABLED | PS_REJECTED;
		pps->request = 0;
		pps->alert->active = 0;

		pps->form = 0;
		pps->pwheel_name = 0;
		pps->pwheel = 0;

		load_str (&pps->dis_reason, CUZ_NEW_PRINTER);
		load_str (&pps->rej_reason, CUZ_NEW_DEST);
		(void) time (&pps->dis_date);
		(void) time (&pps->rej_date);

		*(pps->printer) = *pp;

		untidbit_all (pp->printer_types);

		unload_list (&pps->users_allowed);
		unload_list (&pps->users_denied);
		unload_list (&pps->forms_allowed);
		unload_list (&pps->forms_denied);
		(void) load_userprinter_access (
				pp->name,
				&pps->users_allowed,
				&pps->users_denied
		);
		(void) load_formprinter_access (
				pp->name,
				&pps->forms_allowed,
				&pps->forms_denied
		);

		load_sdn (&pps->cpi, pp->cpi);
		load_sdn (&pps->lpi, pp->lpi);
		load_sdn (&pps->plen, pp->plen);
		load_sdn (&pps->pwid, pp->pwid);

		pps->last_dial_rc = 0;
		pps->nretry = 0;

		init_remote_printer (pps, pp);

		dump_pstatus ();

		status = MOK;

		goto	Return;
	}
	freeprinter (pp);
	status = MNOSPACE;

Return:
	(void) mputm (md, R_LOAD_PRINTER, status);
	EXITP
	return	status == MOK ? 1 : 0;
}

/**
 ** s_unload_printer()
 **/

#ifdef	__STDC__
static void
_unload_printer (PSTATUS *pps)
#else
static void
_unload_printer (pps)

PSTATUS	*pps;
#endif
{
	DEFINE_FNNAME (_unload_printer)

	register CSTATUS	*pcs;


	if (pps->alert->active)
		cancel_alert (A_PRINTER, pps);

	/*
	 * Remove this printer from the classes it may be in.
	 * This is likely to be redundant, i.e. upon deleting
	 * a printer the caller is SUPPOSED TO check all the
	 * classes; any that contain the printer will be changed
	 * and we should receive a S_LOAD_CLASS message for each
	 * to reload the class.
	 *
	 * HOWEVER, this leaves a (small) window where someone
	 * can sneak a request in destined for the CLASS. If
	 * we have deleted the printer but still have it in the
	 * class, we may have trouble!
	 */
	for (pcs = walk_ctable(1); pcs; pcs = walk_ctable(0))
		(void)dellist (&(pcs->class->members), pps->printer->name);

	untidbit_all (pps->printer->printer_types);
	freeprinter (pps->printer);
	pps->printer->name = 0;		/* freeprinter() doesn't */

	return;
}

/*
 * Procedure:     s_unload_printer
 *
 * Restrictions:
 *               mputm: None
*/
#ifdef	__STDC__
int
s_unload_printer (char *m, MESG *md)
#else
int
s_unload_printer (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_unload_printer)

	char	*printer;
	ushort	status;

	register PSTATUS	*pps;


	(void)getmessage (m, S_UNLOAD_PRINTER, &printer);

	/*
	 * Unload ALL printers?
	 */
	if (!*printer || STREQU(printer, NAME_ALL))

		/*
		 * If we have ANY requests queued, we can't do it.
		 */
		if (!Request_List)
		{
			status = MBUSY;
			goto	Return;
		}
		else
		{
			for (pps = walk_ptable(1); pps; pps = walk_ptable(0))
				_unload_printer (pps);
			status = MOK;
			goto	Return;
		}

	/*
	 * Have we seen this printer before?
	 */
	if (!(pps = search_ptable(printer)))
	{
		status = MNODEST;
		goto	Return;
	}
	/*
	 * Can we can move all requests queued for this printer
	 * to another printer?
	 */
	/*
	 * Note: This routine WILL MOVE requests to another
	 * printer. It will not stop until it has gone through
	 * the entire list of requests, so all requests that
	 * can be moved will be moved. If any couldn't move,
	 * however, we don't unload the printer.
	 */
	/*
	**  ES Note:
	**  'queue_repel' uses 'requeue_request'  which
	**  uses 'validate_request'.
	**  'validate_request' will check to see if the user can
	**  print on the requested destination.
	*/
	if (queue_repel(pps, 1, (qchk_fnc_type)0))
		status = MOK;
	else
		status = MBUSY;

	if (status == MOK)
		_unload_printer (pps);

Return:
	if (status == MOK)
		dump_pstatus ();

	(void) mputm (md, R_UNLOAD_PRINTER, status);
	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_inquire_printer_status
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_inquire_printer_status (char *m, MESG *md)
#else
int
s_inquire_printer_status (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_inquire_printer_status)

	char	*printer;
	short	status;

	register PSTATUS	*pps,
				*ppsnext;

	/*
	**  ES Note:
	**  This message is not an ADMIN type message.
	**  It is merely a status request.
	*/
	(void)getmessage (m, S_INQUIRE_PRINTER_STATUS, &printer);

	/*
	 * Inquire about ALL printers?
	 */
	if (!*printer || STREQU(printer, NAME_ALL))
	{
		if ((pps = walk_ptable(1)))
			for (; (ppsnext = walk_ptable(0)); pps = ppsnext)
				(void) send (
					md,
					R_INQUIRE_PRINTER_STATUS,
					MOKMORE,
					pps->printer->name,
					(pps->form ?
						pps->form->form->name : ""),
					(pps->pwheel_name ?
						pps->pwheel_name : ""),
					pps->dis_reason,
					pps->rej_reason,
					pps->status,
					(pps->request ?
						pps->request->secure->req_id :
						""),
					pps->dis_date,
					pps->rej_date);

	}
	/*
	 * Inquire about a specific printer?
	 */
	else
		pps = search_ptable(printer);

	if (pps)
		(void) send (
			md,
			R_INQUIRE_PRINTER_STATUS,
			status = MOK,
			pps->printer->name,
			(pps->form ? pps->form->form->name : ""),
			(pps->pwheel_name ? pps->pwheel_name : ""),
			pps->dis_reason,
			pps->rej_reason,
			pps->status,
			(pps->request ? pps->request->secure->req_id : ""),
			pps->dis_date,
			pps->rej_date);

	else
		(void) mputm (md, R_INQUIRE_PRINTER_STATUS, status = MNODEST,
			"", "", "", "", "", 0, "", 0L, 0L);

	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_inquire_remote_printer
 *
 * Restrictions:
 *               mwrite: None
 *               mputm: None
*/

#ifdef	__STDC__
int
s_inquire_remote_printer (char *m, MESG *md)
#else
int
s_inquire_remote_printer (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_inquire_remote_printer)

	int		x;
	char		*printer;
	char		*found = NULL;
	char		**ppm;
	char		*s1, *s2, *s3, *s4, *s5, *s6;
	long		l1, l2;
	short		h1, h2;
	short		status;
	PSTATUS		*pps;
	SSTATUS		*pss;
	
	/*
	**  ES Note:
	**  This message is not an ADMIN type message.
	**  It is merely a status request.
	*/
	(void)getmessage (m, S_INQUIRE_REMOTE_PRINTER, &printer);

	schedlog("INQUIRE_REMOTE_PRINTER for %s\n", printer);
	
	/*
	 * Inquire about ALL printers?
	 */
	if (!*printer || STREQU(printer, NAME_ALL))
	{
		for (x = 0; (pss = SStatus[x]) != NULL; x++)
		{
			schedlog("Want status for printers on %s\n",
				pss->system->name);
			
			askforstatus(pss, md);
		}
		if (waitforstatus(m, md))
			return	0;
			
		for (x = 0; (pss = SStatus[x]) != NULL; x++)
		{
			for (ppm = pss->pmlist; *ppm; ppm++)
			{
				if (found)
					(void) mwrite(md, found);
				found = *ppm;
			}
		}
		if (found)
		{
			(void) getmessage(found, R_INQUIRE_REMOTE_PRINTER,
				&h1, &s1, &s2, &s3, &s4, &s5, &h2, &s6, &l1,
				&l2);
			(void) mputm (md, R_INQUIRE_REMOTE_PRINTER, status = MOK,
				s1, s2, s3, s4, s5, h2, s6, l1, l2);
			goto	Return;
		}
		(void) mputm (md, R_INQUIRE_REMOTE_PRINTER, status = MNOINFO,
			"", "", "", "", "", 0, "", 0L, 0L);
		goto	Return;
	}
	if ((pps = search_ptable(printer)) != NULL)
	{
		if (!(pps->status & PS_REMOTE) || !pps->system)
		{
			(void) mputm (md, R_INQUIRE_REMOTE_PRINTER, 
				status = MERRDEST, "", "", "", "",
				"", 0, "", 0L, 0L);
			goto	Return;
		}
	}
	else
	{
		(void) mputm (md, R_INQUIRE_REMOTE_PRINTER, 
			status = MNODEST, "", "", "", "",
			"", 0, "", 0L, 0L);
		goto	Return;
	}
	if (!Redispatch)
		askforstatus(pps->system, md);
		
	if (waitforstatus(m, md))
		return	0;
		
	if (pps->system->pmlist == NULL)
	{
		(void) mputm(md, R_INQUIRE_REMOTE_PRINTER,
			status = MNOINFO, "", "", "",
			  "", "", 0, "", 0L, 0L);
		goto	Return;
	}
	for (ppm = pps->system->pmlist; *ppm; ppm++)
		if (mtype(*ppm) == R_INQUIRE_REMOTE_PRINTER)
		{
			(void) getmessage(*ppm, R_INQUIRE_REMOTE_PRINTER,
				&h1, &s1, &s2, &s3, &s4, &s5,
				&h2, &s6, &l1, &l2);
			(void) mputm(md, R_INQUIRE_REMOTE_PRINTER, status = MOK,
				s1, s2, s3, s4, s5, h2, s6, l1, l2);
		}
Return:
	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_load_class
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_load_class (char *m, MESG *md)
#else
int
s_load_class (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_load_class)

	char			*class;

	ushort			status;

	register CLASS		*pc;

	register CSTATUS	*pcs;


	(void)getmessage (m, S_LOAD_CLASS, &class);

	if (!*class)
	{
		status = MNODEST;
		goto	Return;
	}
	/*
	 * Strange or missing class?
	 */
	if (!(pc = Getclass(class)))
	{
		switch (errno) {
		case EBADF:
			status = MERRDEST;
			break;
		case ENOENT:
		default:
			status = MNODEST;
			break;
		}
		goto	Return;
	}
	/*
	 * Class we already know about?
	 */
	if ((pcs = search_ctable(class)))
	{
		register RSTATUS	*prs;

		freeclass (pcs->class);
		*(pcs->class) = *pc;

		/*
		 * Here we go through the list of requests
		 * to see who gets affected.
		 */
		BEGIN_WALK_BY_DEST_LOOP (prs, class)
			/*
			 * If not still eligible for this class...
			 */
			switch (validate_request(prs, (char **)0, 1)) {
			case MOK:
			case MERRDEST:	/* rejecting (shouldn't happen) */
				break;
			case MDENYDEST:
			case MNODEST:   /*  new possible return from VR  */
			case MNOMOUNT:
			case MNOMEDIA:
			case MNOFILTER:
			default:
				/*
				 * ...then too bad!
				 */
				(void) cancel (prs, 1);
				break;
			}
		END_WALK_LOOP
		status = MOK;
		goto	Return;
	}
	/*
	 * Room for new class?
	 */
	if ((pcs = search_ctable((char *)0)))
	{
		pcs->status = CS_REJECTED;

		load_str (&pcs->rej_reason, CUZ_NEW_DEST);
		(void) time (&pcs->rej_date);

		*(pcs->class) = *pc;

		dump_cstatus ();

		status = MOK;
		goto	Return;
	}
	freeclass (pc);
	status = MNOSPACE;

Return:
	(void) mputm (md, R_LOAD_CLASS, status);
	return	status == MOK ? 1 : 0;
}

/**
 ** s_unload_class()
 **/

#ifdef	__STDC__
static void
_unload_class (CSTATUS *pcs)
#else
static void
_unload_class (pcs)

CSTATUS	*pcs;
#endif
{
	DEFINE_FNNAME (_unload_class)

	freeclass (pcs->class);
	pcs->class->name = 0;		/* freeclass() doesn't */

	return;
}

/*
 * Procedure:     s_unload_class
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_unload_class (char *m, MESG *md)
#else
int
s_unload_class (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_unload_class)

	char	*class;
	ushort	status;
	RSTATUS	*prs;

	register CSTATUS	*pcs;


	(void)getmessage (m, S_UNLOAD_CLASS, &class);

	/*
	 * Unload ALL classes?
	 */
	if (!*class || STREQU(class, NAME_ALL))
	{

		/*
		 * If we have a request queued for a member of ANY
		 * class, we can't do it.
		 */
		status = MOK;
		for (
			pcs = walk_ctable(1);
			pcs && status == MOK;
			pcs = walk_ctable(0)
		)
			BEGIN_WALK_BY_DEST_LOOP (prs, pcs->class->name)
				status = MBUSY;
				break;
			END_WALK_LOOP

		if (status == MOK)
			for (pcs = walk_ctable(1); pcs; pcs = walk_ctable(0))
				_unload_class (pcs);
		goto	Return;
	}
	/*
	 * Have we seen this class before?
	 */
	if (!(pcs = search_ctable(class)))
	{
		status = MNODEST;
		goto	Return;
	}
	/*
	 * Is there even one request queued for this class?
	 * If not, we can safely remove it.
	 */
	status = MOK;
	BEGIN_WALK_BY_DEST_LOOP (prs, class)
		status = MBUSY;
		break;
	END_WALK_LOOP
	if (status == MOK)
		_unload_class (pcs);
Return:
	if (status == MOK)
		dump_cstatus ();

	(void) mputm (md, R_UNLOAD_CLASS, status);
	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_inquire_class
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_inquire_class (char *m, MESG *md)
#else
int
s_inquire_class (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_inquire_class)

	char	*class;
	short	status;

	register CSTATUS	*pcs,
				*pcsnext;

	/*
	**  ES Note:
	**  This message is not an ADMIN type message.
	**  It is merely a status request.
	*/
	(void)getmessage (m, S_INQUIRE_CLASS, &class);

	/*
	 * Inquire about ALL classes?
	 */
	if (!*class || STREQU(class, NAME_ALL))
	{
		if ((pcs = walk_ctable(1)))
			for (; (pcsnext = walk_ctable(0)); pcs = pcsnext)
				(void) send (
					md,
					R_INQUIRE_CLASS,
					MOKMORE,
					pcs->class->name,
					pcs->status,
					pcs->rej_reason,
					pcs->rej_date
				);

	}
	/*
	 * Inquire about a single class?
	 */
	else
		pcs = search_ctable(class);

	if (pcs)
		(void) send (
			md,
			R_INQUIRE_CLASS,
			status = MOK,
			pcs->class->name,
			pcs->status,
			pcs->rej_reason,
			pcs->rej_date
		);

	else
		(void) mputm (md, R_INQUIRE_CLASS, status = MNODEST, "", 0, "", 0L);

Return:
	return	status == MOK ? 1 : 0;
}