/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)fmli:sys/terror.c	1.14.3.7"
/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */

#include	<stdio.h>
#include	<errno.h>
#include	"inc.types.h"	/* abs s14 */
#include	"wish.h"
#include	"message.h"
#include	"vtdefs.h"
#include	"terror.h"
#include	"retcodes.h"
#include	"sizes.h"
#include	"terrmess.h"
#include        <unistd.h>
#include	<time.h>
#include	<string.h>

/* dmd TCB
extern char	*Errlist[];
extern char	*What[];
*/
extern int	Vflag;
extern void	exit();		/* fmli's exit not the C lib call */
void
_terror(sev, what, name, file, line, child)
int	sev;	/* flags to see if we should log or exit */
int	what;	/* What we were trying to do */
char	*name;	/* What we were trying to do it to */
char	*file;	/* __FILE__ */
int	line;	/* __LINE__ */
bool 	child;	/* TRUE if called by a child of fmli. abs k15 */
{
	if (errno == ENOTTY)
		errno = 0;
	notify(what, child);	/* abs k15 */
	if (Vflag && (sev & TERR_LOG))
		log(sev, what, name, file, line);
	if (sev & TERR_EXIT)
		exit(R_ERR);	/* fmli's exit not the C lib call */
	errno = 0;
}

static
notify(what, child)
int	what;
bool 	child;	 /* TRUE if called by a child of fmli. abs k15 */
{
	register char	*mymess;
	register int	length;
	register char	*screen;
	register int	mywin;
	char	messbuf[PATHSIZ];
/* dmd TCB
	extern bool	Use_errno[];
*/
	char	*push_win();

	if ((mymess = What[what]) == nil &&
	    (Use_errno[what] == FALSE || Errlist[errno] == nil))
		return;
	if (mymess == nil)
		mymess = gettxt(":276","error");

        mymess = gettxt(Whatid[what],What[what]);
	strncpy(messbuf, mymess, PATHSIZ - 1);
	length = strlen(messbuf);
	if (Use_errno[what] && Errlist[errno] != nil) {
	    strncat(messbuf, ": ", PATHSIZ - 1 - length);
	    length += 2;

            strncat(messbuf, gettxt(Errlistid[errno],Errlist[errno]), PATHSIZ - 1 - length);
	}
	/* to ensure '\0' termination of string */
	messbuf[PATHSIZ -1] = '\0';

	/* if message generated by a child of fmli, print it on line
	 * following cursor. if generated by fmli then use message line
	 */
	if (child == TRUE)		     	/* abs k15 */
	{
	    printf("\r\n%s\r\n", messbuf); 	/* abs k15 */
	    fflush(stdout);			/* abs k15 */
	}
	else
	{
	    (void)mess_err(messbuf);	
	    mess_flash(messbuf);		/* abs f15 */
	    doupdate();		       		/* abs f15 */
	}
}

#define LOGFILE		0
#define TERMINAL	1
#define MAILADM		2

/*
 * FACE application ONLY ....  log problems in the TERRLOG file
 * and/or send mail
 */
static
log(sev, what, name, file, line)
int	sev;
int	what;
char	*name;
char	*file;
int	line;
{
	char	path[PATHSIZ];
	register int	method;
      	time_t	t;     /* EFT abs k16 */
	register FILE	*fp;
	extern char	*Oasys;
	extern char	*Progname;
	extern char	*sys_errlist[];
	char	*getenv();
	char	 tmp1 = NULL;
	char	*tmp2;

	time_t	time();   /* EFT abs k16 */
	char	*ictime();

	/*
	 * construct path of error log file
	 */
	method = LOGFILE;
	if (name == NULL)
		name = nil;
	strcat(strcpy(path, Oasys), TERRLOG);
	if ((fp = fopen(path, "a")) == NULL && errno == EMFILE) {
		close(4);
		fp = fopen(path, "a");
	}
	if (fp == NULL)
/***		if ((fp = popen("mail $LOGNAME", "w")) == NULL)
 *** abs s18 
 ***/
	        {
			method = TERMINAL;
			fp = stderr;
	        }
/***		else
 ***			method = MAILADM;
 *** abs s18
 ***/
	(void) time(&t);
	setbuf(fp, NULL);
	tmp2 = getenv("LOGNAME");
	fprintf(fp, "%16.16s %-8s %-14s %-14s %3d %s%-*s %-24s %s\n", ictime(&t),
		(tmp2 == NULL ? &tmp1 : tmp2), Progname, file, line,
		(sev & TERR_EXIT) ? "(FATAL)" : nil,
		(sev & TERR_EXIT) ? 17 : 24,
		gettxt(Whatid[what],What[what]), sys_errlist[errno], name);
	if (method == LOGFILE)
		fclose(fp);
	else if (method == MAILADM)
		pclose(fp);
}

/*** new internationalized ctime */
char *ictime(const time_t *tclock)
{
  static char buf[26];
  char locbuf[50],*ws,*search_pos;

  strftime(locbuf,50,"%a %b %d %T %Y",localtime(tclock));
  memset(buf,' ',25);
  search_pos = locbuf;
  /* locale's abbr. weekday name */
  ws = strchr(search_pos,' '); memcpy(buf,search_pos,(int)(ws - search_pos));
  search_pos = ws + 1;
  /* locale's abbr. monthname */
  ws = strchr(search_pos,' '); memcpy(buf+4,search_pos,(int)(ws - search_pos));
  search_pos = ws + 1;
  /* day of month */
  ws = strchr(search_pos,' '); memcpy(buf+8,search_pos,(int)(ws - search_pos));
  strcpy(buf+11,ws+1);
  return((char *)buf);
}

