/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _national_h
#define _national_h
#ident	"@(#)debugger:libedit/common/national.h	1.3"

/*
 *  national.h -  definitions for international character sets
 *
 */

/*
 * This data must be defined for each country in defs.c
 */

#ifndef HIGHBIT
# define HIGHBIT		0x80
#endif /* HIGHBIT */

#ifndef ESS_MAXCHAR	/* allow multiple includes */

/*
 *  This section may change from country to country
 */

#define ESS_MAXCHAR	2	/* Maximum number of non-escape bytes
				   for any and all character sets */

#endif /* ESS_MAXCHAR */

#define	CCS1_IN_SIZE	2
#define	CCS1_OUT_SIZE	2
#define	CCS2_IN_SIZE	1
#define	CCS2_OUT_SIZE	1
#define	CCS3_IN_SIZE	2
#define	CCS3_OUT_SIZE	2

/*
 * This part is generic
 */

#define MARKER		0x100	/* Must be invalid character */
#define ESS2		0x8e	/* Escape to char set 2 */
#define ESS3		0x8f	/* Escape to char set 3 */
#define ESS_SETMASK	(3<<(7*ESS_MAXCHAR))	/* character set bits */

#define	echarset(c)	((c)==ESS3?3:((c)==ESS2)?2:((c)>>7)&1)
#define icharset(i)	((i)>>(7*ESS_MAXCHAR)&3)

#define in_csize(s)	int_charsize[s]
#define out_csize(s)	int_charsize[s+4]

#ifdef __cplusplus
extern "C" 
{
#endif
extern char int_charsize[8];
#ifdef __cplusplus
}
#endif

#endif
