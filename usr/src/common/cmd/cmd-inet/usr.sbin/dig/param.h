/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/dig/param.h	1.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *      System V STREAMS TCP - Release 4.0
 *
 *      Copyright 1990 Interactive Systems Corporation,(ISC)
 *      All Rights Reserved.
 *
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */
/*
** Modified version distributed with 'dig' version 2.0 from 
** University of Southern California Information Sciences Institute
** (USC-ISI). 9/1/90
*/

#ifndef __PARAM_HEADER__
#define __PARAM_HEADER__

/*
 * Fundamental constants of the implementation.
 */
#define	NBBY	8		/* number of bits in a byte */
#define	NBPW	sizeof(int)	/* number of bytes in an integer */

#define	NULL	0
#define	CMASK	0		/* default mask for file creation */

#include	<sys/types.h>

/*
 * bit map related macros
 */
#define	setbit(a,i)	((a)[(i)/NBBY] |= 1<<((i)%NBBY))
#define	clrbit(a,i)	((a)[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define	isset(a,i)	((a)[(i)/NBBY] & (1<<((i)%NBBY)))
#define	isclr(a,i)	(((a)[(i)/NBBY] & (1<<((i)%NBBY))) == 0)

/*
 * Macros for fast min/max.
 */
#ifndef MIN
#define	MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define	MAX(a,b) (((a)>(b))?(a):(b))
#endif


#endif !__PARAM_HEADER__
