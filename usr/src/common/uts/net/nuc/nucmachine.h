/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nucmachine.h	1.10"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nucmachine.h,v 2.51.2.1 1994/12/12 01:25:55 stevbam Exp $"

#ifndef _NET_NUC_NUCMACHINE_H
#define _NET_NUC_NUCMACHINE_H

/*
 *  Netware Unix Client
 */

/*	Define machine type definitions. */
#ifdef	IAPX386
#define	LO_HI_MACH_TYPE
#define PERMISSIVE_ALIGNMENT
#endif


#ifdef _KERNEL_HEADERS
#include <net/nw/nwtdr.h>
#else
#include <sys/nwtdr.h>
#endif /* _KERNEL_HEADERS */

#endif /* _NET_NUC_NUCMACHINE_H */
