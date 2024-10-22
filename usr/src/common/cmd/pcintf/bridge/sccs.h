/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/sccs.h	1.1.1.2"
/* SCCSID(@(#)sccs.h	6.4	LCC);	/* Modified: 21:15:07 11/19/90 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#ifndef _SCCS_
#define _SCCS_
#define H_SCCSID(arg)
#define LCC_SCCSID(arg)

#ifndef lint

#ifdef  NOSCCSIDS
#ifndef	__STDC__
#	define	SCCSID(arg)
#	define	SCCSIDPUFF(arg)
#else	/* __STDC__ */
#	define	SCCSID(arg)		char __LCCSCCSID
#	define	SCCSIDPUFF(arg)		char __LCCSCCSID
#endif	/* __STDC__ */
#else
#ifndef	__STDC__
#	define SCCSID(arg)		static char Sccsid[] = "arg"
#	define SCCSIDPUFF(arg)		static char Sccsidpuff[] = "arg"
#else	/* __STDC__ */
#	define SCCSID(arg)		static char Sccsid[] = # arg
#	define SCCSIDPUFF(arg)		static char Sccsidpuff[] = # arg
#endif	/* __STDC__ */
#endif  /* NOSCCSIDS */

#else /* lint */
#	define	SCCSID(arg)
#	define	SCCSIDPUFF(arg)
#endif /* lint */

#endif /* _SCCS_ */
