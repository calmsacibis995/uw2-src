/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)head.usr:deflt.h	1.1.10.2"
#ident  "$Header: deflt.h 1.3 91/06/21 $"
/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/*
 *	@(#) deflt.h 1.1 86/10/07 
 */
/***	deflt.h -- include file for deflt(3).
 *
 */

#define	DEFLT	"/etc/default"

/*
 * Following for defcntl(3).
 * If you add new args, make sure that the default is:
 *	OFF	new-improved-feature-off, i.e. current state of affairs
 *	ON	new-improved-feature-on
 * or that you change the code for deflt(3) to have the old value as the
 * default.  (for compatibility).
 */

/* ... cmds */
#define	DC_GETFLAGS	0	/* get current flags */
#define	DC_SETFLAGS	1	/* set flags */

/* ... args */
#define	DC_CASE		0001	/* ON: respect case; OFF: ignore case */

#define	DC_STD		((0) | (DC_CASE))


#define	TURNON(flags, mask)	flags |= mask
#define	TURNOFF(flags, mask)	flags &= ~(mask)
#define	ISON(flags, mask)	(((flags) & (mask)) == (mask))
#define	ISOFF(flags, mask)	(((flags) & (mask)) != (mask))

#define	DEF_WRITE	0
#define	DEF_DEL		1

#if defined(__STDC__)

extern FILE *defopen(char * );
extern char *defread(FILE *ptr, char *defname);
extern int defclose(FILE * );

#else
extern FILE *defopen();
extern char *defread();
extern int defclose();

#endif
