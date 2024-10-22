/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libcmd:common/lib/libcmd/deflt.c	1.1.7.1"
/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */


/*
 *	@(#) deflt.c 1.1 88/03/30 libcmd:deflt.c
 */
/***	deflt.c - Default Reading Package
 *
 *      this package consists of the routines:
 *
 *              defopen()
 *              defread()
 *		defcntl()		M000
 *
 *      These routines allow one to conveniently pull data out of
 *      files in
 *                      /etc/default/
 *
 *      and thus make configuring a utility's defopens standard and
 *      convenient.
 *
 *	MODIFICATION HISTORY
 *	M000	06 Sep 83	andyp
 *	- Added new routine defcntl().  The idea here is that we need
 *	  to maintain compatibility for people who are already using
 *	  these routines and /etc/default, but we want to have case-
 *	  independence for our default files.
 *	M001	22 Jul 86	greggo
 *	- Set dfile to NULL in defopen() when called with NULL to close
 *	  default file. 
 *	M002	10 Mar 87	jeffro
 *	- Upped defread's static buffer from 80 to 256 characters (from SCO).
 *	M003	20 Jan 89	ATT
 *	added a defclose() routine.
 *  	- defopen() takes a filename as an argument not a full pathname
 *		and does a chdir to DEFLT(/etc/default) directory then opens
 *		the file.
 *
 *	NOTE:	defopen() no longer does a chdir() since that caused problems
 *		on a system designed to use the MAC feature (Mandatory Access
 *		Control).
 *
 *	- defread() takes an additional argument the FILE * of the file
 *		    you want to read. Also if the char * is NULL it
 *		    will sequentially read the file.
 */

#include <stdio.h>
#include <deflt.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>

#define	FOREVER			for ( ; ; )
#define	TSTBITS(flags, mask)	(((flags) & (mask)) == (mask))

extern	int	errno,
		tolower();

extern 	char	*strcat();

static	void	strlower(), strnlower();

static	char	*sname();

/*
 * M000
 * Following for defcntl(3).
 * If things get more complicated we probably will want to put these
 * in an include file.
 * If you add new args, make sure that the default is
 *	OFF	new-improved-feature-off, i.e. current state of affairs
 *	ON	new-improved-feature-on
 * (for compatibility).
 */
/* ... flags */
static	int	Dcflags;	/* [re-]initialized on each call to defopen() */


/*      defopen() - declare defopen filename
 *
 *      defopen(cp)
 *              char *cp
 *
 *      see defread() for more details.
 *
 *      EXIT    returns FILE * if ok
 *              returns NULL if error
 */

FILE *
defopen(fn)
	char *fn;
{
	FILE	*dfile	= NULL;
	char    fname[MAXPATHLEN],
		*filep = &fname[0];

	fname[0] = '\0';
	(void) strcat(filep, DEFLT);
	(void) strcat(filep, (char *) "/");
	(void) strcat(filep, sname(fn));

	if ((dfile = fopen(filep, "r")) == (FILE *) NULL) {
		return (FILE *) NULL;
	}

	Dcflags = DC_STD;	/* M000 */

	return dfile;
}


/*      defread() - read an entry from the defaults file
 *
 *      defread(fp, cp)
 *		FILE *fp
 *              char *cp
 *
 *      The file pointed to by fp must have been previously opened by
 *      defopen().  defread scans the data file looking for a line
 *      which begins with the string '*cp'.  If such a line is found,
 *      defread returns a pointer to the first character following
 *      the matched string (*cp).  If no line is found or no file
 *      is open, defread() returns NULL.
 *
 *      If *cp is NULL then defread returns the first line in the file
 *	so subsequent calls will sequentially read the file.
 */

char *
defread(fp, cp)
FILE *fp;
char *cp;
{
	static char buf[256];	/* M002 */
	register int len;
	register int patlen;

	if (fp == (FILE *) NULL) {
		errno = EINVAL;
		return((char *) NULL);
	}

	if (cp) {

		patlen = strlen(cp);
		rewind(fp);
		while (fgets(buf, sizeof(buf), fp)) {
			len = strlen(buf);
			if (buf[len-1] == '\n')
				buf[len-1] = 0;
			else
				return((char *) NULL);		/* line too long */
			if (!TSTBITS(Dcflags, DC_CASE)) {	/* M000 ignore case */
				strlower(cp, cp);
				strnlower(buf, buf, patlen);
			}
		
		if ((strncmp(cp, buf, patlen) == 0) && (buf[patlen] == '='))
				return(&buf[++patlen]);           /* found it */
		}
		return((char *) NULL);

	} else {
	
skip:
		if(fgets(buf, sizeof(buf), fp) == NULL)
			return((char *) NULL);
		else {
			if (buf[0] == '#')
				goto skip;
			len = strlen(buf);
			if (buf[len-1] == '\n')
				buf[len-1] = 0;
			else
				return((char *) NULL);		/* line too long */
			return(&buf[0]);
		}
	}

}

int
defclose(fp)
FILE *fp;
{
	return(fclose(fp));
}
	
	/* M000 new routine */
/***	defcntl -- default control
 *
 *	SYNOPSIS
 *	  oldflags = defcntl(cmd, arg);
 *
 *	ENTRY
 *	  cmd		Command.  One of DC_GET, DC_SET.
 *	  arg		Depends on command.  If DC_GET, ignored.  If
 *		DC_GET, new flags value, created by ORing the DC_* bits.
 *	RETURN
 *	  oldflags	Old value of flags.  -1 on error.
 *	NOTES
 *	  Currently only one bit of flags implemented, namely respect/
 *	  ignore case.  The routine is as general as it is so that we
 *	  leave our options open.  E.g. we might want to specify rewind/
 *	  norewind before each defread.
 */

int
defcntl(cmd, newflags)
register int  cmd;
int  newflags;
{
	register int  oldflags;

	switch (cmd) {
	case DC_GETFLAGS:		/* query */
		oldflags = Dcflags;
		break;
	case DC_SETFLAGS:		/* set */
		oldflags = Dcflags;
		Dcflags = newflags;
		break;
	default:			/* error */
		oldflags = -1;
		break;
	}

	return(oldflags);
}

/* M000 new routine */
/***	strlower -- convert upper-case to lower-case.
 *
 *	Like strcpy(3), but converts upper to lower case.
 *	All non-upper-case letters are copied as is.
 *
 *	ENTRY
 *	  from		'From' string.  ASCIZ.
 *	  to		'To' string.  Assumed to be large enough.
 *	EXIT
 *	  to		filled in.
 */

static	void
strlower(from, to)
register char  *from, *to;
{
	register int  ch;

	FOREVER {
		ch = *from++;
		if ((*to = tolower(ch)) == '\0')
			break;
		else 
			++to;

	}

	return;
}

/* M000 new routine */
/***	strnlower -- convert upper-case to lower-case.
 *
 *	Like strncpy(3), but converts upper to lower case.
 *	All non-upper-case letters are copied as is.
 *
 *	ENTRY
 *	  from		'from' string.  May be ASCIZ.
 *	  to		'to' string.
 *	  cnt		Max # of chars to copy.
 *	EXIT
 *	  to		Filled in.
 */

static	void
strnlower(from, to, cnt)
register char  *from, *to;
register int  cnt;
{
	register int  ch;

	while (cnt-- > 0) {
		ch = *from++;
		if ((*to = tolower(ch)) == '\0')
			break;
		else
			++to;
	}
}


/*
	Return pointer to the last element of a pathname.
	(same as basename())
*/
static	char *
sname(s)
	char	*s;
{
	register char	*p;

	if (!s || !*s)			/* zero or empty argument */
		return  ".";

	p = s + strlen(s);

	while (p != s && *--p == '/')	/* skip trailing /s */
		*p = '\0';
	
	if (p == s && *p == '\0')	/* all slashes */
		return "/";

	while (p != s)
		if (*--p == '/')
			return ++p;

	return p;
}
