/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acomp:i386/mddefs.h	55.2.1.7"
/* i386/mddefs.h */


/* Machine-dependent ANSI C definitions. */

/* These definitions will eventually be in CG's macdefs.h */
#define C_CHSIGN		/* plain chars default to signed */
#define	C_SIGNED_RS		/* right shifts of ints are signed */

/* Produce debugging register number corresponding to internal
** register number.
*/
#define	DB_OUTREGNO(i)	(outreg[SY_REGNO(i)])

/* Only want for-loop code tests at bottom. */
#define	FOR_LOOP_CODE	LL_BOT
#define WH_LOOP_CODE	LL_BOT

/* Enable #pragma pack; maximum value is 4 */
#define	PACK_PRAGMA	4

/* Enable section mapping */
#define SECTION_MAP_PRAGMA

/* HACK:  register numbers to represent ELF debugging.
** Both autos and args are accessed via %ebp.
*/
#define	DB_FRAMEPTR(sid)	5	/* %ebp number */
#define	DB_ARGPTR(sid)		5	/* %ebp number */

#define DB_ARGOFFSET(sid) 	SY_OFFSET(sid)

/* Enable #pragma weak.  The two strings are for the 1 and 2
** identifier forms of the pragma.
*/
#define	WEAK_PRAGMA "\t.weak\t%1\n", "\t.weak\t%1\n\t.set\t%1,%2\n"

#define FAT_ACOMP
#define RA_DEFAULT RA_GLOBAL
#define GENERATE_LOOP_INFO
extern chars_signed;
