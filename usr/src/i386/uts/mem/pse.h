/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_PSE_H	/* wrapper symbol for kernel use */
#define _MEM_PSE_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/pse.h	1.1"
#ident	"$Header: $"

#ifdef	_KERNEL

/*
 * is PSE supported on this cpu?
 */
#define	PSE_SUPPORTED()		(l.cpu_features[0] & CPUFEAT_PSE)

/*
 * minimum size to use PSE mappings
 */
#define	KPSE_MIN		(1024*1024)

/*
 * maximum virtual space waste
 */
#define	KPSE_WASTE		(4*1024*1024)

#endif	/* _KERNEL */

#endif /* _MEM_PSE_H */
