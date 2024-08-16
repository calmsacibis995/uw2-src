/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nuctool.h	1.13"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nuctool.h,v 2.53.2.4 1995/02/09 03:46:17 doshi Exp $"

#ifndef _UTIL_NUC_TOOLS_NUCTOOL_H
#define _UTIL_NUC_TOOLS_NUCTOOL_H

/*
 *  Netware Unix Client 
 *
 *  FILE:	nuctool.h
 *
 *  ABSTRACT:   Include header files for toolkit functions supported
 *              as macros.
 *
 */

#ifdef _KERNEL_HEADERS
#include	<svc/systm.h>
#include	<util/ksynch.h>
#include	<util/param.h>
#else
#include	<sys/systm.h>
#include	<sys/ksynch.h>
#include	<sys/param.h>
#endif

extern void wakeup();
extern int sleep();

#define NWtlCopyToPhys(x,y,z)			ASSERT(0)
#define NWtlCopyFromPhys(x,y,z)			ASSERT(0)

/*
 *	SYSTEM SPECIFIC MANIFEST CONSTANTS
 *	THESE WILL BE IN ANOTHER FILE
 */
#define NUCSPILSLEEPPRI		primed
#define NUCSPITASKSLEEPPRI	primed
#define NUCCONNSLEEPPRI		primed
#define NUCSTRSLEEPPRI		primed

#define	CRED_BOTH	0
#define CRED_UID	1
#define CRED_GID	2
#define CRED_PID	3

/*
 *	NUC Semaphore structure
 */
typedef struct {
	int		number;
	int		initialValue;
	int 	value;
	sv_t	*semaSV;
} nucSema_t;

typedef struct {
	int	psuedoSemaValue;      /* the semaphore value */
	int	psuedoSemaWaitCount;  /* number of contexts waiting */
	int	psuedoSemaDraining;   /* 1 if reset ("Wakeup") is in progress */
	lock_t	*psuedoSemaLock;
	sv_t	*psuedoSemaSV;
} psuedoSema_t;

#ifdef NUC_DEBUG9
#define PRINT_REPLY(replyPack)											\
{																		\
	int 	cnt = 0;													\
	uchar_t	*p = (uchar_t *)replyPack;									\
																		\
	cmn_err ( CE_CONT, "NEWPACKET: ");									\
	while ( cnt < 131 ) {												\
		if ( *p > 0x2f && *p < 0x7f ) 									\
			cmn_err ( CE_CONT, "[%c]%2x ",(char *)*p, (char *)*p++);	\
		else															\
			cmn_err ( CE_CONT, "%2x ",(char *)*p++);					\
																		\
		cnt++;															\
	}																	\
	cmn_err ( CE_CONT, "DONE\n");										\
}
#else NUC_DEBUG9
#define PRINT_REPLY(replyPack)
#endif NUC_DEBUG9
#endif /* _UTIL_NUC_TOOLS_NUCTOOL_H */
