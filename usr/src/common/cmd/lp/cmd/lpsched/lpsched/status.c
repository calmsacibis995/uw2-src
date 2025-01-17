/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:cmd/lpsched/lpsched/status.c	1.7.8.2"
#ident  "$Header: status.c 1.2 91/06/27 $"

#include "stdlib.h"
#include "string.h"
#include "unistd.h"

#include "lpsched.h"

#define WHO_AM_I	I_AM_LPSCHED
#include "oam.h"

#define NCMP(X,Y)	(STRNEQU((X), (Y), sizeof(Y)-1))

#ifdef	__STDC__

static void		load_pstatus (void);
static void		load_cstatus (void);
static void		put_multi_line ( FILE * , char * );

#else

static	void		put_multi_line();
static	void		load_pstatus (),
			load_cstatus ();

#endif

static char		*pstatus	= 0,
			*cstatus	= 0;
/**
 ** load_status() - LOAD PRINTER/CLASS STATUS FILES
 **/

void
#ifdef	__STDC__
load_status (
	void
)
#else
load_status ()
#endif
{
	DEFINE_FNNAME (load_status)

	load_pstatus ();
	load_cstatus ();
	return;
}

/*
 * Procedure:     load_pstatus
 *
 * Restrictions:
 *               open_lpfile: None
 *               fgets: None
 *               close_lpfile: None
 * Notes - LOAD PRINTER STATUS FILE
 */

static void
#ifdef	__STDC__
load_pstatus (
	void
)
#else
load_pstatus ()
#endif
{
	DEFINE_FNNAME (load_pstatus)

	PSTATUS			*pps;

	char			*rej_reason,
				*dis_reason,
				*pwheel_name,
				buf[BUFSIZ],
				*name,
				*p;

	time_t			rej_date,
				dis_date;

	short			status;

	FSTATUS			*pfs;

	PWSTATUS		*ppws;

	int			i,
				len,
				total;

	time_t			now;

	register FILE		*fp;

	register int		f;


	(void) time(&now);

	if (!pstatus)
		pstatus = makepath(Lp_System, PSTATUSFILE, (char *)0);
	if ((fp = open_lpfile(pstatus, "r", 0)))
		while (!feof(fp)) {

			status = 0;

			total = 0;
			name = 0;
			rej_reason = 0;
			dis_reason = 0;

			for (f = 0; f < PST_MAX && fgets(buf, BUFSIZ, fp); f++) {
				if (p = strrchr(buf, '\n'))
					*p = '\0';

				switch (f) {
				case PST_BRK:
					break;

				case PST_NAME:
					name = Strdup(buf);
					break;

				case PST_STATUS:
					if (NCMP(buf, NAME_DISABLED))
						status |= PS_DISABLED;
					p = strchr(buf, ' ');
					if (!p || !*(++p))
						break;
					if (NCMP(p, NAME_REJECTING))
						status |= PS_REJECTED;
					break;

				case PST_DATE:
					dis_date = (time_t)atol(buf);
					p = strchr(buf, ' ');
					if (!p || !*(++p))
						break;
					rej_date = (time_t)atol(p);
					break;

				case PST_DISREAS:
					len = strlen(buf);
					if (buf[len - 1] == '\\') {
						buf[len - 1] = '\n';
						f--;
					}
					if (dis_reason) {
						total += len;
						dis_reason = Realloc(
							dis_reason,
							total+1
						);
						(void) strcat (dis_reason, buf);
					} else {
						dis_reason = Strdup(buf);
						total = len;
					}
					break;

				case PST_REJREAS:
					len = strlen(buf);
					if (buf[len - 1] == '\\') {
						buf[len - 1] = '\n';
						f--;
					}
					if (rej_reason) {
						total += len;
						rej_reason = Realloc(
							rej_reason,
							total+1
						);
						(void) strcat (rej_reason, buf);
					} else {
						rej_reason = Strdup(buf);
						total = len;
					}
					break;

				case PST_PWHEEL:
					if (*buf) {
						ppws = search_pwtable(buf);
						pwheel_name = Strdup(buf);
					} else {
						ppws = 0;
						pwheel_name = 0;
					}
					break;

				case PST_FORM:
					if (*buf)
						pfs = search_ftable(buf);
					else
						pfs = 0;
					break;
				}
			}

			if (ferror(fp) || f && f != PST_MAX) {
				(void) close_lpfile (fp);
				lpnote (ERROR, E_SCH_TROUBLE, pstatus);
				return;
			}

			if (!feof(fp) && name && (pps = search_ptable(name))) {
				pps->rej_date = rej_date;
				pps->status |= status;
				if ((pps->form = pfs))
					pfs->mounted++;
				pps->pwheel_name = pwheel_name;
				if ((pps->pwheel = ppws))
					ppws->mounted++;
				pps->rej_reason = rej_reason;
				if (pps->printer->login) {
					pps->dis_date = now;
					pps->dis_reason = Strdup(CUZ_LOGIN_PRINTER);
				} else {
					pps->dis_date = dis_date;
					pps->dis_reason = dis_reason;
				}

			} else {
				if (dis_reason)
					Free (dis_reason);
				if (rej_reason)
					Free (rej_reason);
			}
			if (name)
				Free (name);
		}

	if (fp) {
		if (ferror(fp)) {
			(void) close_lpfile (fp);
			lpnote (ERROR, E_SCH_TROUBLE, pstatus);
			return;
		}
		(void) close_lpfile (fp);
	}

	for (i = 0; i < PT_Size; i++)
		if (PStatus[i].printer->name && !PStatus[i].rej_reason) {
			PStatus[i].dis_reason = Strdup(CUZ_NEW_PRINTER);
			PStatus[i].rej_reason = Strdup(CUZ_NEW_DEST);
			PStatus[i].dis_date = now;
			PStatus[i].rej_date = now;
			PStatus[i].status |= PS_DISABLED | PS_REJECTED;
		}

	return;
}

/*
 * Procedure:     load_cstatus
 *
 * Restrictions:
 *               open_lpfile: None
 *               fgets: None
 *               close_lpfile: None
 * notes - LOAD CLASS STATUS FILE
 */

static void
#ifdef	__STDC__
load_cstatus (
	void
)
#else
load_cstatus ()
#endif
{
	DEFINE_FNNAME (load_cstatus)

	CSTATUS			*pcs;

	char			*rej_reason,
				buf[BUFSIZ],
				*name,
				*p;

	time_t			rej_date;

	short			status;

	int			i,
				len,
				total;

	time_t			now;

	register FILE		*fp;

	register int		f;


	(void) time(&now);

	if (!cstatus)
		cstatus = makepath(Lp_System, CSTATUSFILE, (char *)0);
	if ((fp = open_lpfile(cstatus, "r", 0)))
		while (!feof(fp)) {
			status = 0;

			total = 0;
			name = 0;

			rej_reason = 0;
			for (f = 0; f < CST_MAX && fgets(buf, BUFSIZ, fp); f++) {
				if (p = strrchr(buf, '\n'))
					*p = '\0';
				switch (f) {
				case CST_BRK:
					break;

				case CST_NAME:
					name = Strdup(buf);
					break;

				case CST_STATUS:
					if (NCMP(buf, NAME_REJECTING))
						status |= PS_REJECTED;
					break;

				case CST_DATE:
					rej_date = (time_t)atol(buf);
					break;

				case CST_REJREAS:
					len = strlen(buf);
					if (buf[len - 1] == '\\') {
						buf[len - 1] = '\n';
						f--;
					}
					if (rej_reason) {
						total += len;
						rej_reason = Realloc(
							rej_reason,
							total+1
						);
						(void) strcat (rej_reason, buf);
					} else {
						rej_reason = Strdup(buf);
						total = len;
					}
					break;
				}
			}

			if (ferror(fp) || f && f != CST_MAX) {
				(void) close_lpfile (fp);
				lpnote (ERROR, E_SCH_TROUBLE, cstatus);
				return;
			}

			if (!feof(fp) && name && (pcs = search_ctable(name))) {
				pcs->rej_reason = rej_reason;
				pcs->rej_date = rej_date;
				pcs->status |= status;

			} else
				if (rej_reason)
					Free (rej_reason);

			if (name)
				Free (name);
		}

	if (fp) {
		if (ferror(fp)) {
			(void) close_lpfile (fp);
			lpnote (ERROR, E_SCH_TROUBLE, cstatus);
			return;
		}
		(void) close_lpfile (fp);
	}

	for (i = 0; i < CT_Size; i++)
		if (CStatus[i].class->name && !CStatus[i].rej_reason) {
			CStatus[i].status |= CS_REJECTED;
			CStatus[i].rej_reason = Strdup(CUZ_NEW_DEST);
			CStatus[i].rej_date = now;
		}

	return;
}

/**
 ** put_multi_line() - PRINT OUT MULTI-LINE TEXT
 **/

static void
#ifdef	__STDC__
put_multi_line (
	FILE *			fp,
	char *			buf
)
#else
put_multi_line (fp, buf)
	FILE			*fp;
	char			*buf;
#endif
{
	DEFINE_FNNAME (put_multi_line)

	register char		*cp,
				*p;

	if (!buf) {
		(void)fprintf (fp, "\n");
		return;
	}

	for (p = buf; (cp = strchr(p, '\n')); ) {
		*cp++ = 0;
		(void)fprintf (fp, "%s\\\n", p);
		p = cp;
	}
	(void)fprintf (fp, "%s\n", p);
	return;
}

/*
 * Procedure:     dump_pstatus
 *
 * Restrictions:
 *               open_lpfile: None
 *               close_lpfile: None
 * Notes - DUMP PRINTER STATUS FILE
 */

void
#ifdef	__STDC__
dump_pstatus (
	void
)
#else
dump_pstatus ()
#endif
{
	DEFINE_FNNAME (dump_pstatus)

	PSTATUS			*ppsend;

	FILE			*fp;

	register PSTATUS	*pps;

	register int		f;


	if (!pstatus)
		pstatus = makepath(Lp_System, PSTATUSFILE, (char *)0);
	if (!(fp = open_lpfile(pstatus, "w", MODE_READ))) {
		lpnote (ERROR, E_SCH_CANTOPEN, pstatus, PERROR);
		return;
	}

	for (pps = PStatus, ppsend = &PStatus[PT_Size]; pps < ppsend; pps++)
		if (pps->printer->name)
			for (f = 0; f < PST_MAX; f++) switch (f) {
			case PST_BRK:
				(void)fprintf (fp, "%s\n", STATUS_BREAK);
				break;
			case PST_NAME:
				(void)fprintf (fp, "%s\n", NB(pps->printer->name));
				break;
			case PST_STATUS:
				(void)fprintf (
					fp,
					"%s %s\n",
					(pps->status & PS_DISABLED? NAME_DISABLED : NAME_ENABLED),
					(pps->status & PS_REJECTED? NAME_REJECTING : NAME_ACCEPTING)
				);
				break;
			case PST_DATE:
				(void)fprintf (
					fp,
					"%ld %ld\n",
					pps->dis_date,
					pps->rej_date
				);
				break;
			case PST_DISREAS:
				put_multi_line (fp, pps->dis_reason);
				break;
			case PST_REJREAS:
				put_multi_line (fp, pps->rej_reason);
				break;
			case PST_PWHEEL:
				(void)fprintf (fp, "%s\n", NB(pps->pwheel_name));
				break;
			case PST_FORM:
				(void)fprintf (
					fp,
					"%s\n",
					(pps->form) ?
					pps->form->form->name : ""
				);
				break;
			}

	(void) close_lpfile (fp);

	return;
}

/*
 * Procedure:     dump_cstatus
 *
 * Restrictions:
 *               open_lpfile: None
 *               close_lpfile: None
 * notes - DUMP CLASS STATUS FILE
 */

void
#ifdef	__STDC__
dump_cstatus (
	void
)
#else
dump_cstatus ()
#endif
{
	DEFINE_FNNAME (dump_cstatus)

	CSTATUS			*pcsend;

	FILE			*fp;

	register CSTATUS	*pcs;

	register int		f;


	if (!cstatus)
		cstatus = makepath(Lp_System, CSTATUSFILE, (char *)0);
	if (!(fp = open_lpfile(cstatus, "w", MODE_READ))) {
		lpnote (ERROR, E_SCH_CANTOPEN, cstatus, PERROR);
		return;
	}

	for (pcs = CStatus, pcsend = &CStatus[CT_Size]; pcs < pcsend; pcs++)
		if (pcs->class->name)
			for (f = 0; f < CST_MAX; f++) switch (f) {
			case CST_BRK:
				(void)fprintf (fp, "%s\n", STATUS_BREAK);
				break;
			case CST_NAME:
				(void)fprintf (fp, "%s\n", NB(pcs->class->name));
				break;
			case CST_STATUS:
				(void)fprintf (
					fp,
					"%s\n",
					(pcs->status & CS_REJECTED? NAME_REJECTING : NAME_ACCEPTING)
				);
				break;
			case CST_DATE:
				(void)fprintf (fp, "%ld\n", pcs->rej_date);
				break;
			case CST_REJREAS:
				put_multi_line (fp, pcs->rej_reason);
				break;
			}

	(void) close_lpfile (fp);

	return;
}

/**
 ** dump_status() - DUMP PRINTER/CLASS STATUS FILES
 **/

#ifdef	DEBUG
void
#ifdef	__STDC__
dump_status (
	void
)
#else
dump_status ()
#endif
{
	DEFINE_FNNAME (dump_status)

	dump_pstatus ();
	dump_cstatus ();
	return;
}
#endif	/*  DEBUG  */
