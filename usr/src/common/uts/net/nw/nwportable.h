/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/nwportable.h	1.5"
#ident	"$Id: nwportable.h,v 1.4 1994/09/24 12:57:46 vtag Exp $"

/*
 * Copyright 1991, 1992 Novell, Inc.
 * All Rights Reserved.
 *
 * This work is subject to U.S. and International copyright laws and
 * treaties.  No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

/*
**	Max length of a netware server name, must stay outside of wrapper
*/
#ifndef NWMAX_SERVER_NAME_LENGTH
#define NWMAX_SERVER_NAME_LENGTH      48
#endif

#ifndef _NET_NW_NWPORTABLE_H  /* wrapper symbol for kernel use */
#define _NET_NW_NWPORTABLE_H  /* subject to change without notice */

/*
**	Definitions required for application include files
*/

/*	Define base types. */
typedef	unsigned long		uint32;	/* 32-bit unsigned type */
typedef	unsigned short		uint16;	/* 16-bit unsigned type */
typedef	unsigned char		uint8;	/* 8-bit unsigned type */ 	

typedef	long				int32;	/* 32-bit signed type */
typedef	short				int16;	/* 16-bit signed type */
typedef char				int8;	/* 8-bit signed type */

/* For compatibility with native NetWare source code. */
#ifndef LONG
#define LONG	unsigned long
#endif
#ifndef WORD
#define WORD	unsigned short
#endif
#ifndef BYTE
#define BYTE	unsigned char
#endif

/* Standard constants */
#ifndef TRUE
#define	TRUE		1	
#endif
#ifndef FALSE
#define	FALSE		0
#endif

#ifndef NULL
#define NULL                     0
#endif

/* these defines are used at the NCP layer as response values to the requestors.
** Some of the other layers have been using these defines, which is ok, but
** do not change their values.
*/
#ifndef SUCCESS
#define SUCCESS                  0x00
#endif
#ifndef FAILURE
#define FAILURE                  0xFF
#endif

#endif /* _NET_NW_NWPORTABLE_H */
