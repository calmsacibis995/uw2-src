/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:lib/lp/alerts.c	1.23.2.4"
#ident  "$Header: alerts.c 1.2 91/06/27 $"

#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "limits.h"
#include "unistd.h"
#include "ulimit.h"
#include "stdlib.h"

#include "lp.h"
#include "printers.h"

#define WHO_AM_I        I_AM_LPADMIN
#include "oam.h"

extern char		**environ;

#if	defined(__STDC__)
static void		envlist ( FILE * , char ** );
#else
static void		envlist();
#endif

/*
 * We recognize the following key phrases in the alert prototype
 * file, and replace them with appropriate values.
 */
#define NALRT_KEYS	7
# define ALRT_ENV		0
# define ALRT_PWD		1
# define ALRT_ULIMIT		2
# define ALRT_UMASK		3
# define ALRT_INTERVAL		4
# define ALRT_CMD		5
# define ALRT_USER		6

static struct {
	char			*v;
	short			len;
}			shell_keys[NALRT_KEYS] = {
#define	ENTRY(X)	X, sizeof(X)-1
	ENTRY("-ENVIRONMENT-"),
	ENTRY("-PWD-"),
	ENTRY("-ULIMIT-"),
	ENTRY("-UMASK-"),
	ENTRY("-INTERVAL-"),
	ENTRY("-CMD-"),
	ENTRY("-USER-"),
};

/*
 * These are used to bracket the administrator's command, so that
 * we can find it easily. We're out of luck if the administrator
 * includes an identical phrase in his or her command.
 */
#define ALRT_CMDSTART "## YOUR COMMAND STARTS HERE -- DON'T TOUCH ABOVE!!"
#define ALRT_CMDEND   "## YOUR COMMAND ENDS HERE -- DON'T TOUCH BELOW!!"

/**
 ** putalert() - WRITE ALERT TO FILES
 **/

int
#if	defined(__STDC__)
putalert (
	char *			parent,
	char *			name,
	FALERT *		alertp
)
#else
putalert (parent, name, alertp)
	char			*parent,
				*name;
	FALERT			*alertp;
#endif
{
	char			*path,
	                        *outpath,
				cur_dir[PATH_MAX + 1],
				buf[BUFSIZ];

	int			n,
                                cur_umask;

	FILE			*fpout,
				*fpin;

	level_t			lid = PR_SYS_PUBLIC;

	if (!parent || !*parent || !name || !*name) {
		errno = EINVAL;
		return (-1);
	}

	if (!alertp->shcmd) {
		errno = EINVAL;
		return (-1);
	}

	if (STREQU(alertp->shcmd, NAME_NONE))
		return (delalert(parent, name));

	/*
	 * See if the form/printer/print-wheel exists.
	 */

	if (!(path = makepath(parent, name, (char *)0)))
		return (-1);

	if (Access(path, F_OK) == -1) {
		if (errno == ENOENT)
			errno = ENOTDIR; /* not quite, but what else? */
		Free (path);
		return (-1);
	}
	Free (path);

	/*
	 * First, the shell command file.
	 */

	if (!(outpath = makepath(parent, name, ALERTSHFILE, (char *)0)))
		return (-1);

	if (!(fpout = open_lpfile(outpath, "w", MODE_NOEXEC))) {
		Free (outpath);
		return (-1);
	}

	/*
	 * We use a prototype file to build the shell command,
	 * so that the alerts are easily customized. The shell
	 * is expected to handle repeat alerts and failed alerts,
	 * because the Spooler doesn't. Also, the Spooler runs
	 * each alert with the UID and GID of the administrator
	 * who defined the alert. Otherwise, anything goes.
	 */

	/* CONSTCOND */
	if (!Lp_Bin) {
		getpaths ();
		/* CONSTCOND */
		if (!Lp_Bin)
			return (-1);
	}
	if (!(path = makepath(Lp_Bin, ALERTPROTOFILE, (char *)0)))
		return (-1);

	if (!(fpin = open_lpfile(path, "r", 0))) {
		Free (path);
		return (-1);
	}
	Free (path);

	while (fgets(buf, BUFSIZ, fpin)) {
		int			key;
		char			*cp,
					*dash;

		cp = buf;
		while ((dash = strchr(cp, '-'))) {

		    *dash = 0;
		    (void)fputs (cp, fpout);
		    *(cp = dash) = '-';

		    for (key = 0; key < NALRT_KEYS; key++)
			if (STRNEQU(
				cp,
				shell_keys[key].v,
				shell_keys[key].len
			)) {
				register char	*newline =
						(cp != buf)? "\n" : "";

				cp += shell_keys[key].len;

				switch (key) {

				case ALRT_ENV:
					(void)fprintf (fpout, newline);
					envlist (fpout, environ);
					break;

				case ALRT_PWD:
					(void)getcwd (cur_dir, PATH_MAX);
					(void)fprintf (fpout, "%s", cur_dir);
					break;

				case ALRT_ULIMIT:
					(void)fprintf (fpout, "%ld", ulimit(1, (long)0));
					break;

				case ALRT_UMASK:
					(void)umask (cur_umask = umask(0));
					(void)fprintf (fpout, "%03o", cur_umask);
					break;

				case ALRT_INTERVAL:
					(void)fprintf (fpout, "%ld", (long)alertp->W);
					break;

				case ALRT_CMD:
					(void)fprintf (fpout, newline);
					(void)fprintf (fpout, "%s\n", ALRT_CMDSTART);
					(void)fprintf (fpout, "%s\n", alertp->shcmd);
					(void)fprintf (fpout, "%s\n", ALRT_CMDEND);
					break;

				case ALRT_USER:
					(void)fprintf (fpout, "%s", getname());
					break;

				}

				break;
			}
		    if (key >= NALRT_KEYS)
			(void)fputc (*cp++, fpout);

		}
		(void)fputs (cp, fpout);

	}
	if (feof(fpout) || ferror(fpout) || ferror(fpin)) {
		int			save_errno = errno;

		(void)close_lpfile (fpin);
		(void)close_lpfile (fpout);
		while (lvlfile (outpath, MAC_SET, &lid) < 0 &&
		       errno == EINTR)
		    continue;
		Free (outpath);
		errno = save_errno;
		return (-1);
	}
	(void)close_lpfile (fpin);
	(void)close_lpfile (fpout);
	while ((n=lvlfile (outpath, MAC_SET, &lid)) < 0 && errno == EINTR)
	    continue;

	Free (outpath);
	if (n < 0 && errno != ENOSYS)
	    return -1;

	/*
	 * Next, the variables file.
	 */

	if (!(path = makepath(parent, name, ALERTVARSFILE, (char *)0)))
		return (-1);

	if (!(fpout = open_lpfile(path, "w", MODE_NOREAD))) {
		Free (path);
		return (-1);
	}

	(void)fprintf (fpout, "%d\n", alertp->Q > 0? alertp->Q : 1);
	(void)fprintf (fpout, "%d\n", alertp->W >= 0? alertp->W : 0);

	(void)close_lpfile (fpout);
	while ((n=lvlfile (path, MAC_SET, &lid)) < 0 && errno == EINTR)
	    continue;

	Free (path);
	if (n < 0 && errno != ENOSYS)
	    return -1;

	return (0);
}

/**
 ** getalert() - EXTRACT ALERT FROM FILES
 **/

FALERT *
#if	defined(__STDC__)
getalert (
	char *			parent,
	char *			name
)
#else
getalert (parent, name)
	char			*parent,
				*name;
#endif
{
	static FALERT		alert;

	register char		*path;

	char			buf[BUFSIZ];

	FILE			*fp;

	int			len;


	if (!parent || !*parent || !name || !*name) {
		errno = EINVAL;
		return (0);
	}

	/*
	 * See if the form/printer/print-wheel exists.
	 */

	if (!(path = makepath(parent, name, (char *)0)))
		return (0);

	if (Access(path, F_OK) == -1) {
		if (errno == ENOENT)
			errno = ENOTDIR; /* not quite, but what else? */
		Free (path);
		return (0);
	}
	Free (path);

	/*
	 * First, the shell command file.
	 */

	if (!(path = makepath(parent, name, ALERTSHFILE, (char *)0)))
		return (0);

	if (!(fp = open_lpfile(path, "r", 0))) {
		Free (path);
		return (0);
	}
	Free (path);

	/*
	 * Skip over environment setting stuff, while loop, etc.,
	 * to find the beginning of the command.
	 */
	while (
		fgets(buf, BUFSIZ, fp)
	     && !STRNEQU(buf, ALRT_CMDSTART, sizeof(ALRT_CMDSTART)-1)
	)
		;
	if (feof(fp) || ferror(fp)) {
		int			save_errno = errno;

		(void)close_lpfile (fp);
		errno = save_errno;
		return (0);
	}

	alert.shcmd = sop_up_rest(fp, ALRT_CMDEND);
	if (!alert.shcmd) {
		(void)close_lpfile (fp);
		return (0);
	}

	/*
	 * Drop terminating newline.
	 */
	if (alert.shcmd[(len = strlen(alert.shcmd)) - 1] == '\n')
		alert.shcmd[len - 1] = 0;

	(void)close_lpfile (fp);

	/*
	 * Next, the variables file.
	 */

	if (!(path = makepath(parent, name, ALERTVARSFILE, (char *)0)))
		return (0);

	if (!(fp = open_lpfile(path, "r", 0))) {
		Free (path);
		return (0);
	}
	Free (path);

	(void)fgets (buf, BUFSIZ, fp);
	if (ferror(fp)) {
		int			save_errno = errno;

		(void)close_lpfile (fp);
		errno = save_errno;
		return (0);
	}
	alert.Q = atoi(buf);

	(void)fgets (buf, BUFSIZ, fp);
	if (ferror(fp)) {
		int			save_errno = errno;

		(void)close_lpfile (fp);
		errno = save_errno;
		return (0);
	}
	alert.W = atoi(buf);

	(void)close_lpfile (fp);

	return (&alert);
}

/**
 ** delalert() - DELETE ALERT FILES
 **/

int
#if	defined(__STDC__)
delalert (
	char *			parent,
	char *			name
)
#else
delalert (parent, name)
	char			*parent,
				*name;
#endif
{
	char			*path;


	if (!parent || !*parent || !name || !*name) {
		errno = EINVAL;
		return (-1);
	}

	/*
	 * See if the form/printer/print-wheel exists.
	 */

	if (!(path = makepath(parent, name, (char *)0)))
		return (-1);

	if (Access(path, F_OK) == -1) {
		if (errno == ENOENT)
			errno = ENOTDIR; /* not quite, but what else? */
		Free (path);
		return (-1);
	}
	Free (path);

	/*
	 * Remove the two files.
	 */

	if (!(path = makepath(parent, name, ALERTSHFILE, (char *)0)))
		return (-1);
	if (rmfile(path) == -1) {
		Free (path);
		return (-1);
	}
	Free (path);

	if (!(path = makepath(parent, name, ALERTVARSFILE, (char *)0)))
		return (-1);
	if (rmfile(path) == -1) {
		Free (path);
		return (-1);
	}
	Free (path);

	return (0);
}

/**
 ** envlist() - PRINT OUT ENVIRONMENT LIST SAFELY
 **/

static void
#if	defined(__STDC__)
envlist (
	FILE *			fp,
	char **			list
)
#else
envlist (fp, list)
	register FILE		*fp;
	register char		**list;
#endif
{
	register char		*env,
				*value;

	if (!list || !*list)
		return;

	while ((env = *list++)) {
		if (!(value = strchr(env, '=')))
			continue;
		*value++ = 0;
		if (!strchr(value, '\''))
			(void)fprintf (fp, "export %s; %s='%s'\n", env, env, value);
		*--value = '=';
	}
}

/**
 ** printalert() - PRINT ALERT DESCRIPTION
 **/

/* ARGSUSED0 */
void
#if	defined(__STDC__)
printalert (
	FILE *			fp,
	FALERT *		alertp,
	int			isfault
)
#else
printalert (fp, alertp, isfault)
	register FILE		*fp;
	register FALERT		*alertp;
	int			isfault;
#endif
{
	if (!alertp->shcmd) {
		if (isfault)
                        LP_OUTMSG(MM_NOSTD, E_ADM_ALERT13);
		else
                        LP_OUTMSG(MM_NOSTD, E_ADM_ALERT14);

	} else {
		register char	*copy = Strdup(alertp->shcmd),
				*cp;

		if (copy && (cp = strchr(copy, ' ')))
			while (*cp == ' ')
				*cp++ = 0;

		if (
			copy
		     && syn_name(cp)
		     && (
				STREQU(copy, NAME_WRITE)
			     || STREQU(copy, NAME_MAIL)
			)
		)
		   if (isfault)
		      if (alertp->W > 0)
                         LP_OUTMSG3(MM_NOSTD, E_ADM_ALERT1,
					copy, cp, alertp->W);
		      else
                         LP_OUTMSG2(MM_NOSTD, E_ADM_ALERT2, copy, cp);
                   else
                      if (alertp->Q > 1)
		         if (alertp->W > 0)
                            LP_OUTMSG4(MM_NOSTD, E_ADM_ALERT5,
					alertp->Q, copy, cp, alertp->W);
                         else
                            LP_OUTMSG3(MM_NOSTD, E_ADM_ALERT6, 
					alertp->Q, copy, cp);
                      else
		         if (alertp->W > 0)
                            LP_OUTMSG3(MM_NOSTD, E_ADM_ALERT9,
					copy, cp, alertp->W);
                         else
                            LP_OUTMSG2(MM_NOSTD, E_ADM_ALERT10,
					copy, cp);
                else
		   if (isfault)
		      if (alertp->W > 0)
                         LP_OUTMSG2(MM_NOSTD, E_ADM_ALERT3,
					alertp->shcmd, alertp->W);
		      else
                         LP_OUTMSG1(MM_NOSTD, E_ADM_ALERT4,
					alertp->shcmd);
                   else
                      if (alertp->Q > 1)
		         if (alertp->W > 0)
                            LP_OUTMSG3(MM_NOSTD, E_ADM_ALERT7,
				alertp->Q, alertp->shcmd, alertp->W);
                         else
                            LP_OUTMSG2(MM_NOSTD, E_ADM_ALERT8,
					alertp->Q, alertp->shcmd);
                      else
		         if (alertp->W > 0)
                            LP_OUTMSG2(MM_NOSTD, E_ADM_ALERT11,
					alertp->shcmd, alertp->W);
                         else
                            LP_OUTMSG1(MM_NOSTD, E_ADM_ALERT12,
							alertp->shcmd);

		Free (copy);
	}
	return;
}
