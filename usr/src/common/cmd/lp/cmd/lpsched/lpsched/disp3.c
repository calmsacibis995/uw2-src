/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:cmd/lpsched/lpsched/disp3.c	1.10.8.3"
#ident  "$Header: disp3.c 1.2 91/06/27 $"
# include	"dispatch.h"

/**
 ** remount_form() - MOUNT A FORM WHERE ANOTHER WAS MOUNTED
 **/

#ifdef	__STDC__
static void
remount_form (register PSTATUS *pps, FSTATUS *pfs)
#else
static void
remount_form (pps, pfs)
register
PSTATUS	*pps;
FSTATUS	*pfs;
#endif
{
	DEFINE_FNNAME (remount_form)

	if (pps->form == pfs)
		return;	/* nothing to do */

	/*
	 * Unmount the old form.
	 */
	if (pps->form) {
		register FSTATUS	*Opfs	= pps->form;

		pps->form = 0;
		Opfs->mounted--;

		/*
		 * Unmounting the form may make some print requests
		 * no longer printable, because they were accepted
		 * only because the form was already mounted.
		 * Unmounting the form will also force some requests
		 * to another printer (where the form is mounted)
		 * so they can print.
		 */
    		/*
    		**  ES Note:
		**  'queue_repel' uses 'requeue_request'  which
		**  uses 'validate_request'.
    		**  'validate_request' will check to see if the user can
		**  print on the requested destination.
    		*/
		form_in_question = Opfs;
		(void)queue_repel (pps, 0, qchk_form);

		/*
		 * Maybe an alert is due.
		 */
		check_form_alert (Opfs, (_FORM *)0);
	}

	/*
	 * Mount the new form?
	 */
	if (pfs) {
		pps->form = pfs;
		pfs->mounted++;

		/*
		 * Attract all the requests needing this newly mounted
		 * form. This may cause some unnecessary shuffling, but
		 * we have to ensure requests aren't assigned to a printer
		 * without the form mounted, so that the alert check is
		 * correct.
		 */
		if (pfs->requests) {
			form_in_question = pfs;
			queue_attract (pps, qchk_form, 0);

			/*
			 * Maybe an alert can be shut off.
			 */
			check_form_alert (pfs, (_FORM *)0);
		}

	} else {
		/*
		 * Attract first request that doesn't need a form mounted.
		 * We only need to get one request printing, because it
		 * completing will cause the next request to be attracted.
		 */
		form_in_question = 0;
		queue_attract (pps, qchk_form, 1);
	}

	dump_pstatus ();

	return;
}

/**
 ** remount_pwheel() - MOUNT A PRINT-WHEEL WHERE ANOTHER WAS MOUNTED
 **/

static void
#ifdef	__STDC__
remount_pwheel (
	register PSTATUS *	pps,
	char *			pwheel_name
)
#else
remount_pwheel (pps, pwheel_name)
	register PSTATUS	*pps;
	char			*pwheel_name;
#endif
{
	DEFINE_FNNAME (remount_pwheel)

	PWSTATUS		*ppws;


	if (SAME(pps->pwheel_name, pwheel_name))
		return;	/* nothing to do */

	/*
	 * Unmount the old print wheel
	 */
	if (pps->pwheel_name) {
		register PWSTATUS	*Oppws	= pps->pwheel;

		pps->pwheel = 0;
		if (Oppws)
			Oppws->mounted--;

		/*
		 * Unmounting the print wheel may make some print
		 * requests no longer printable, because they were
		 * accepted only because the print wheel was already
		 * mounted. Unmounting the print wheel will also force
		 * some requests to another printer (where the print wheel
		 * is mounted) so they can print.
		 */
    		/*
    		**  ES Note:
		**  'queue_repel' uses 'requeue_request'  which
		**  uses 'validate_request'.
    		**  'validate_request' will check to see if the user can
		**  print on the requested destination.
    		*/
		pwheel_in_question = pps->pwheel_name;
		(void)queue_repel (pps, 0, qchk_pwheel);

		unload_str (&pps->pwheel_name);

		/*
		 * Maybe an alert is due.
		 */
		if (Oppws)
			check_pwheel_alert (Oppws, (PWHEEL *)0);
	}

	/*
	 * Mount the new print wheel?
	 */
	if (pwheel_name) {
		load_str (&pps->pwheel_name, pwheel_name);
		if (ppws = search_pwtable(pwheel_name)) {
			pps->pwheel = ppws;
			ppws->mounted++;

			/*
			 * Attract all requests needing this newly
			 * mounted print wheel. This may cause some
			 * unnecessary shuffling, but we have to ensure
			 * requests aren't assigned to a printer without
			 * the print-wheel mounted, so that the alert
			 * check is correct.
			 */
			if (ppws->requests) {
				pwheel_in_question = pwheel_name;
				queue_attract (pps, qchk_pwheel, 0);

				/*
				 * Maybe an alert can be shut off.
				 */
				check_pwheel_alert (ppws, (PWHEEL *)0);
			}

		} else {
			/*
			 * Attract the first request that needs this newly
			 * mounted print wheel. If no alert has been
			 * defined for the print wheel, we don't know how
			 * many requests are queued waiting for it, so we
			 * have to do this unconditionally.
			 */
			pwheel_in_question = pwheel_name;
			queue_attract (pps, qchk_pwheel, 1);
		}

	} else {
		/*
		 * Attract the first request that doesn't need a
		 * print wheel mounted.
		 * We only need to get one request printing, because it
		 * completing will cause the next request to be attracted.
		 */
		pwheel_in_question = 0;
		queue_attract (pps, qchk_pwheel, 1);
	}

	dump_pstatus ();

	return;
}

/*
 * Procedure:     s_mount
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_mount (char *m, MESG *md)
#else
int
s_mount (m, md)
char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_mount)

	char	*printer,
		*form,
		*pwheel_name;
	ushort	status;

	register PSTATUS	*pps;
	register FSTATUS	*pfs;


	(void)getmessage (m, S_MOUNT, &printer, &form, &pwheel_name);

	if (!*form && !*pwheel_name)
	{
		status = MNOMEDIA;
		goto	Return;
	}

	/*
	 * Have we seen this printer before?
	 */
	if (!*printer || !(pps = search_ptable(printer)))
	{
		status = MNODEST;
		goto	Return;
	}
	/*
	 * How about the form?
	 */
	if (*form && !(pfs = search_ftable(form)))
	{
		status = MNOMEDIA;
		goto	Return;
	}
	/*
	 * If the printer is currently printing a request,
	 * we can't disturb it.
	 */
	if (pps->request)
	{
		status = MBUSY;
		goto	Return;
	}
	/*
	 * Mount them.
	 */
	if (*form)
		remount_form (pps, pfs);
	if (*pwheel_name)
		remount_pwheel (pps, pwheel_name);

	status = MOK;

Return:
	(void) mputm (md, R_MOUNT, status);
	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_unmount
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_unmount (char *m, MESG *md)
#else
int
s_unmount (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_unmount)

	char			*printer,
				*form,
				*pwheel_name;

	ushort			status;

	register PSTATUS	*pps;

	(void)getmessage (m, S_UNMOUNT, &printer, &form, &pwheel_name);

	if (!*form && !*pwheel_name)
	{
		status = MNOMEDIA;
		goto	Return;
	}
	/*
	 * Have we seen this printer before?
	 */
	if (!*printer || !(pps = search_ptable(printer)))
	{
		status = MNODEST;
		goto	Return;
	}
	/*
	 * If the printer is currently printing a request,
	 * we can't unmount the current form/pwheel.
	 */
	if (pps->request)
	{
		status = MBUSY;
		goto	Return;
	}
	/*
	 * Unmount them.
	 */
	if (*form)
		remount_form (pps, (FSTATUS *)0);
	if (*pwheel_name)
		remount_pwheel (pps, (char *)0);

	status = MOK;

Return:
	(void) mputm (md, R_UNMOUNT, status);
	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_load_form
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_load_form (char *m, MESG *md)
#else
int
s_load_form (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_load_form)

	char	*form;
	ushort	status;

	register _FORM		*pf;
	register FSTATUS	*pfs;


	(void)getmessage (m, S_LOAD_FORM, &form);

	if (!*form)
	{
		status = MNODEST;
		goto	Return;
	}
	/*
	 * Strange or missing form?
	 */
	if (!(pf = Getform(form)))
	{
		switch (errno) {
		case EBADF:
			status = MERRDEST;
			break;
		case ENOENT:
		case ENODATA:
		default:
			status = MNODEST;
			break;
		}
		goto	Return;
	}
	/*
	 * Have we seen this form before?
	 */
	if ((pfs = search_ftable(form)))
	{
		unload_list (&pfs->users_allowed);
		unload_list (&pfs->users_denied);
		(void) load_userform_access (
			pf->name,
			&pfs->users_allowed,
			&pfs->users_denied
		);

		load_sdn (&pfs->cpi, pf->cpi);
		load_sdn (&pfs->lpi, pf->lpi);
		load_sdn (&pfs->plen, pf->plen);
		load_sdn (&pfs->pwid, pf->pwid);


		/*
		 * These have to be done in the order shown,
		 * and after the assignments above, so that all
		 * the new information is in place for the
		 * checks. An unfortunate side effect is that
		 * it is possible for the alert to shut off
		 * and then come on again, if (1) enough requests
		 * are canceled to drop the level below the old
		 * alert threshold, but (2) the new alert threshold
		 * is even lower. The final alert will be correct,
		 * though.
		 */

		form_in_question = pfs;
		queue_check (qchk_form);

		check_form_alert (pfs, pf);


		status = MOK;
		goto	Return;
	}
	/*
	 * Room for a new form?
	 */
	if ((pfs = search_ftable((char *)0)))
	{
		pfs->alert->active = 0;
		pfs->requests = pfs->requests_last = 0;
		pfs->mounted = 0;

		/*
		 * No alert is possible for a new form, of course,
		 * but this routine does a bit more than just check
		 * the alert.
		 */
		check_form_alert (pfs, pf);

		unload_list (&pfs->users_allowed);
		unload_list (&pfs->users_denied);
		(void) load_userform_access (
			pf->name,
			&pfs->users_allowed,
			&pfs->users_denied
		);

		load_sdn (&pfs->cpi, pf->cpi);
		load_sdn (&pfs->lpi, pf->lpi);
		load_sdn (&pfs->plen, pf->plen);
		load_sdn (&pfs->pwid, pf->pwid);

		status = MOK;
		goto	Return;
	}
	free_form (pf);
	status = MNOSPACE;

Return:
	(void) mputm (md, R_LOAD_FORM, status);
	return	status == MOK ? 1 : 0;
}

/**
 ** s_unload_form()
 **/

#ifdef	__STDC__
static void
_unload_form (register FSTATUS *pfs)
#else
static void
_unload_form (pfs)
register FSTATUS	*pfs;
#endif
{
	DEFINE_FNNAME (_unload_form)

	register PSTATUS	*pps	= &PStatus[0],
				*ppsend	= &PStatus[PT_Size];

	/*
	 * Unmount this form everywhere and get rid of it.
	 */
	for (; pps < ppsend; pps++)
		if (pps->form && pps->form == pfs)
			pps->form = NULL;

	free_form (pfs->form);
	pfs->form->name = 0;

	return;
}

/*
 * Procedure:     s_unload_form
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_unload_form (char *m, MESG *md)
#else
int
s_unload_form (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_unload_form)

	char	*form;
	ushort	status;
	RSTATUS	*prs;

	register FSTATUS	*pfs;


	(void)getmessage (m, S_UNLOAD_FORM, &form);

	if (!*form || STREQU(form, NAME_ALL))
	{
		/*
		 * If we have a request queued for ANY form,
		 * we can't do it.
		 */
		status = MOK;
		for (
			pfs = walk_ftable(1);
			pfs && status == MOK;
			pfs = walk_ftable(0)
		)
			BEGIN_WALK_BY_FORM_LOOP (prs, pfs)
				status = MBUSY;
				break;
			END_WALK_LOOP

		if (status == MOK)
			for (pfs = walk_ftable(1); pfs; pfs = walk_ftable(0))
				_unload_form (pfs);
		goto	Return;
	}
	/*
	 * Have we seen this form before?
	 */
	if (!*form || !(pfs = search_ftable(form)))
	{
		status = MNODEST;
		goto	Return;
	}
	/*
	 * Is there even one request waiting for this form?
	 * If not then we can remove it.
	 */
	status = MOK;
	BEGIN_WALK_BY_FORM_LOOP (prs, pfs)
		status = MBUSY;
		break;
	END_WALK_LOOP
	if (status == MOK)
		_unload_form (pfs);

Return:
	(void) mputm (md, R_UNLOAD_FORM, status);
	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_load_printwheel
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_load_printwheel (char *m, MESG *md)
#else
int
s_load_printwheel (m, md)

char *m;
MESG *md;
#endif
{
	DEFINE_FNNAME (s_load_printwheel)

	char	*pwheel_name;
	ushort	status;

	register PWHEEL		*ppw;
	register PWSTATUS	*ppws;


	(void)getmessage (m, S_LOAD_PRINTWHEEL, &pwheel_name);

	if (!*pwheel_name)
	{
		status = MNODEST;
		goto	Return;
	}

	/*
	 * Strange or missing print wheel?
	 */
	if (!(ppw = Getpwheel(pwheel_name)))
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
	 * Print wheel we already know about?
	 */
	if ((ppws = search_pwtable(pwheel_name)))
	{

		check_pwheel_alert (ppws, ppw);
		status = MOK;
		goto	Return;
	}
	/*
	 * Room for a new print wheel?
	 */
	if ((ppws = search_pwtable((char *)0)))
	{
		register RSTATUS	*prs;

		ppws->alert->active = 0;
		ppws->requests = ppws->requests_last = 0;
		ppws->mounted = 0;

		/*
		 * Because of the quirky nature of the print wheel
		 * structures, i.e. no structure unless an alert has
		 * been defined, we have to run through the requests
		 * and see which ones are waiting for this print wheel,
		 * so we can assign alerts and count pending requests.
		 */
		BEGIN_WALK_BY_PWHEEL_LOOP (prs, pwheel_name)
			if (!one_printer_with_charsets(prs)) {
				prs->pwheel = ppws;
				ppws->requests++;
			}
		END_WALK_LOOP
		check_pwheel_alert (ppws, ppw);

		status = MOK;
		goto	Return;
	}

	freepwheel (ppw);
	status = MNOSPACE;

Return:
	(void) mputm (md, R_LOAD_PRINTWHEEL, status);
	return	status == MOK ? 1 : 0;
}

/**
 ** s_unload_printwheel()
 **/

#ifdef	__STDC__
static void
_unload_pwheel (register PWSTATUS *ppws)
#else
static void
_unload_pwheel (ppws)

register
PWSTATUS	*ppws;
#endif
{
	DEFINE_FNNAME (_unload_pwheel)

	register PSTATUS	*pps	= &PStatus[0],
				*ppsend	= &PStatus[PT_Size];

	register RSTATUS	*prs;


	/*
	 * ``Unmount'' the alert part of this print wheel everywhere.
	 * THIS IS NOT A COMPLETE UNMOUNT, JUST THE ALERT STRUCTURE
	 * IS REMOVED.
	 */
	for (; pps < ppsend; pps++)
		if (pps->pwheel == ppws)
			pps->pwheel = 0;

	/*
	 * Remove the alert part from all requests.
	 */
	for (prs = Request_List; prs; prs = prs->next)
		if (prs->pwheel == ppws)
			prs->pwheel = 0;

	/*
	 * Cancel any alert pending. Here we're different from the
	 * similar code for unloading a form, because, to be able to 
	 * unload a form we first require NO requests pending. If no
	 * requests are pending there should be no alert to cancel.
	 * Print wheels, on the other hand, only exist as names and
	 * alerts. We can always unload a ``print wheel'' because
	 * all we're really unloading is an alert. Thus, there can
	 * be requests queued for the print wheel (the name), and
	 * thus there can be an alert running.
	 */
	if (ppws->alert->active)
		cancel_alert (A_PWHEEL, ppws);

	freepwheel (ppws->pwheel);
	ppws->pwheel->name = 0;		/* freeprinter() doesn't */

	return;
}

/*
 * Procedure:     s_unload_printwheel
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_unload_printwheel (char *m, MESG *md)
#else
int
s_unload_printwheel (m, md)

char	*m;
MESG	*md;
#endif
{
	DEFINE_FNNAME (s_unload_printwheel)

	char	*pwheel_name;
	ushort	status;

	register PWSTATUS	*ppws;


	/*
	 * We don't care if any requests are waiting for the print
	 * wheel(s)--what we're removing here is (are) just the alert(s)!
	 */

	(void)getmessage (m, S_UNLOAD_PRINTWHEEL, &pwheel_name);

	/*
	 * Remove all print wheel alerts?
	 */
	if (!*pwheel_name || STREQU(pwheel_name, NAME_ALL))
	{
		for (ppws = walk_pwtable(1); ppws; ppws = walk_pwtable(0))
			_unload_pwheel (ppws);
		status = MOK;
		goto	Return;
	}
	/*
	 * Have we seen this print wheel before?
	 */
	if (!(ppws = search_pwtable(pwheel_name)))
	{
		status = MNODEST;
		goto	Return;
	}
	_unload_pwheel (ppws);
	status = MOK;

Return:
	(void) mputm (md, R_UNLOAD_PRINTWHEEL, status);
	return  status == MOK ? 1 : 0;
}
