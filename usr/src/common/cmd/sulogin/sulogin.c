/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sulogin:sulogin.c	1.9.15.2"
/***************************************************************************
 * Command: sulogin
 *
 * Inheritable Privileges: P_ALLPRIVS
 *       Fixed Privileges: None
 *
 * Notes:	special login program exec'd from init to let user
 *		come up single user, or go multi straight away.
 *
 *		Explain the scoop to the user, and prompt for
 *		root password or ^D. Good root password gets you
 *		single user, ^D exits sulogin, and init will
 *		go multi-user.
 *
 *		If /etc/passwd is missing, or there's no entry for root,
 *		go single user, no questions asked.
 *
 *      	11/29/82
 *
 ***************************************************************************/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/*
 *	MODIFICATION HISTORY
 *
 *	M000	01 May 83	andyp	3.0 upgrade
 *	- index ==> strchr.
 */

#include <sys/types.h>
#include <termio.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ia.h>
#include <utmpx.h>
#include <unistd.h>
#include <priv.h>
#include <mac.h>
#include <errno.h>
#include <sys/secsys.h>
#include <ctype.h>
#include <pfmt.h>
#include <locale.h>	

#define SCPYN(a, b)	(void) strncpy(a, b, sizeof(a))

static	char	*get_passwd(),
		*ttyn = NULL,
		minus[]	= "-",
		*findttyname(),
		shell[]	= "/sbin/su",
		SHELL[]	= "/sbin/sh";

extern	char	*crypt(),
		*strcpy(),
		*strncpy(),
		*ttyname();

extern	int	errno,
		strcmp(),
		devstat();

extern	void	free(),
		*malloc(),
		ia_closeinfo();

extern	struct	utmpx	*getutxent(),
			*pututxline();

static struct utmpx *u;

static	void	single(),
		consalloc();	/* security code to put console in public state */

/*
 * Procedure:     main
 *
 * Restrictions:
 *		printf:		None
 *		ia_openinfo:	P_MACREAD
*/
main()
{
	uinfo_t	uinfo = NULL;
	char	*r_pass;	/* password read from user */
	char	*c_pass;	/* password to compare */
	char	*ia_pwdp;	/* password from master file */
	char	*ia_shellp;	/* shell from master file */

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxsulogin");
	(void)setlabel("UX:sulogin");


	(void) procprivl(CLRPRV, pm_work(P_MACREAD), (priv_t)0); 

	if ((ia_openinfo("root", &uinfo)) || (uinfo == NULL)) {
		(void) pfmt(stderr,MM_ERROR,
			":1:\n**** NO ENTRY FOR root IN MASTER FILE! ****\n");
		(void) pfmt(stderr,MM_ACTION,
			":2:\n**** RUN /sbin/creatiadb ****\n\n");		
		single(0);
	}
	ia_get_logpwd(uinfo, &ia_pwdp);
	if (ia_pwdp)
		ia_pwdp = strdup(ia_pwdp);
	ia_get_sh(uinfo, &ia_shellp);
	if (ia_shellp)
		ia_shellp = strdup(ia_shellp);
	ia_closeinfo(uinfo);

	(void) procprivl(SETPRV, pm_work(P_MACREAD), (priv_t)0); 

	if (ia_pwdp == NULL)		/* shouldn't happen */
		ia_pwdp = "";		/* but do something reasonable */

	for (;;) {
		(void) pfmt(stdout,MM_NOSTD,
			":3:\nType Ctrl-d to proceed with normal startup,\n(or give root password for Single User Mode): ");

		if((r_pass = get_passwd()) == NULL)
			exit(0);	/* ^D, so straight to multi-user */

		if (ia_pwdp[0])
			c_pass = crypt(r_pass, ia_pwdp);
		else
			c_pass = r_pass;

		if (strcmp(c_pass, ia_pwdp)) {
			(void) pfmt(stderr,MM_ERROR,
				":4:Login incorrect\n");
			continue;
		}

		/*
		 * Password from master file and password read from
		 * stdin were the same, so we have a correct login.
		 *
		 * If login shell from master file doesn't look good,
		 * don't bother invoking su since it will probably fail.
		 * (An empty string is OK here since that will default to
		 * /sbin/sh in su anyway, but a NULL pointer isn't good.)
		 */
		if (ia_shellp == NULL ||
		    ia_shellp[0] && access(ia_shellp, EX_OK) != 0) {
			(void) pfmt(stdout,MM_WARNING,
				":9:No shell; will try %s\n", SHELL);
			single(0);
		}

		/*
		 * All is well, so invoke su.
		 */
		single(1);
	}
	/* NOTREACHED */
}


/*
 * Procedure:     single
 *
 * Restrictions:
 *		getutxent:	P_MACREAD accesses the /var/adm/*tmp* files
 *		pututxline:	P_MACREAD accesses the /var/adm/*tmp* files
 *		printf:		None
 *		execl(2):	P_MACREAD access to the shell
 *
 * Notes:	exec shell for single user mode. 
*/
static	void
single(ok)
	int	ok;
{
	/*
	 * update the utmpx file.
	 */
	ttyn = findttyname(0);

	if (*ttyn) {
		consalloc();
	}
	else {
		ttyn = "/dev/???";
	}

	(void) procprivl(CLRPRV, pm_work(P_MACREAD), (priv_t)0);

	while ((u = getutxent()) != NULL) {
		if (!strcmp(u->ut_line, (ttyn + sizeof("/dev/") - 1))) {
			(void) time(&u->ut_tv.tv_sec);
			u->ut_type = INIT_PROCESS;
			/*
			 * force user to "look" like
			 * "root" in utmp entry.
			*/
			if (strcmp(u->ut_user, "root")) {
				u->ut_pid = getpid();
				SCPYN(u->ut_user, (char *) "root");
			}
			break;
		}
	}
	if (u != NULL) {
		(void) pututxline(u);
	}

	endutxent();		/* Close utmp file */

	(void) pfmt(stdout,MM_NOSTD,
		":5:Entering Single User Mode\n\n");

	if (ok) 
		(void) execl(shell, shell, minus, (char *)0);
	else
		(void) execl(SHELL, SHELL, minus, (char *)0);

	exit(0);
	/* NOTREACHED */
}



/*
 * Procedure:     get_passwd
 *
 * Restrictions:
 *		fopen:		None
 *		setbuf:		None
 *		ioctl(2):	None
 *		fprintf:	None
 *		fclose:		None
 *
 * Notes: 	hacked from the stdio library version so we can
 *		distinguish newline and EOF.  Also don't need this
 *		routine to give a prompt.
 *
 * RETURN:	(char *)address of password string (could be null string)
 *
 *			or
 *
 *		(char *)0 if user typed EOF
*/
static char *
get_passwd()
{
	struct	termio	ttyb;
	static	char	pbuf[9];
	int		flags;
	register char	*p;
	register c;
	FILE	*fi;
	void	(*sig)();
	char	*rval;		/* function return value */

	fi = stdin;
	setbuf(fi, (char *)NULL);

	sig = signal(SIGINT, SIG_IGN);
	(void) ioctl(fileno(fi), TCGETA, &ttyb);
	flags = ttyb.c_lflag;
	ttyb.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);

	(void) ioctl(fileno(fi), TCSETAF, &ttyb);
	p = pbuf;
	rval = pbuf;
	while ((c = getc(fi)) != '\n') {
		if (c == EOF) {
			if (p == pbuf)		/* ^D, No password */
				rval = (char *)0;
			break;
		}
		if (p < &pbuf[8])
			*p++ = (char) c;
	}
	*p = '\0';			/* terminate password string */
	(void) fprintf(stderr, "\n");		/* echo a newline */

	ttyb.c_lflag = (long) flags;

	(void) ioctl(fileno(fi), TCSETAW, &ttyb);
	(void) signal(SIGINT, sig);

	if (fi != stdin)
		(void) fclose(fi);

	return rval;
}


/*
 * Procedure:     findttyname
 *
 * Restrictions:
 *		ttyname:	P_MACREAD
 *		access(2):	P_MACREAD
*/
static	char *
findttyname(fd)
	int	fd;
{
	static	char	*l_ttyn;

	(void) procprivl(CLRPRV, pm_work(P_MACREAD), (priv_t)0);

	l_ttyn = ttyname(fd);

/* do not use syscon or systty if console is present, assuming they are links */
	if (((strcmp(l_ttyn, "/dev/syscon") == 0) ||
		(strcmp(l_ttyn, "/dev/systty") == 0)) &&
		(access("/dev/console", F_OK)))
			l_ttyn = "/dev/console";

	(void) procprivl(SETPRV, pm_work(P_MACREAD), (priv_t)0);

	return l_ttyn;
}


/*
 * Procedure:     consalloc
 *
 * Restrictions:
 *		lvlproc(2):	None
 *		lvlin:		None
 *		devstat:	None
 *		printf:		None
*/
static	void
consalloc()
{
	struct	devstat	mybuf;
	level_t		lid_low,
			lid_user,
			lid_high;
	char		*ttynamep;

	errno = 0;
	ttynamep = ttyname(0);

	if ((lvlproc(MAC_GET, &lid_user) == -1) && errno == ENOPKG)
		return; 

	if (lvlin(SYS_RANGE_MAX, &lid_high) == -1) {
		(void) pfmt(stderr,MM_ERROR,
			":6:LTDB is inacessible or corrupt\n");
		(void) pfmt(stderr,MM_ERROR,
			":7:failed to allocate %s\n", ttynamep);
		return;
	}
	if (lvlin(SYS_PUBLIC, &lid_low) == -1) {
		(void) pfmt(stderr,MM_ERROR,
			":6:LTDB is inacessible or corrupt\n");
		(void) pfmt(stderr,MM_ERROR,
			":7:failed to allocate %s\n", ttynamep);
		return;
	}
	mybuf.dev_relflag = DEV_PERSISTENT;
	mybuf.dev_mode = DEV_STATIC;
	mybuf.dev_hilevel = lid_high;
	mybuf.dev_lolevel = lid_low;
	mybuf.dev_state = DEV_PUBLIC;

	if (fdevstat(0, DEV_SET, &mybuf)){
		(void) pfmt(stderr,MM_ERROR,
			":8:devstat failed on %s\n", ttynamep); 
		(void) pfmt(stderr,MM_ERROR,
			":7:failed to allocate %s\n", ttynamep);
	}
	return;
}
