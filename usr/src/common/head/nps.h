/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nps.h	1.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:nps/include/nps.h	1.2"
#ident	"$Id: nps.h,v 1.3 1994/03/04 18:46:18 vtag Exp $"
/*
 * Copyright 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 *
 */

#ifndef __NPS_H__
#define __NPS_H__

/* lib/libnps/npslib.c */

extern int SetSocket(int, uint16 *, int);
extern int BindSocket(int, uint16, int);
extern int SetHiLoWater(int, uint32, uint32, int);
extern int Error(char *, ...);
extern int PrivateIoctl(int, int, char *, int);
extern int killNPSD();

/*
** This is well-defined, and probably defined elsewhere.
*/
#ifndef FAILURE
#define	FAILURE						0xFF
#endif
/*
** This is well-defined, and probably defined elsewhere.
*/
#ifndef SUCCESS
#define SUCCESS						0x00
#endif

#endif /* __NPS_H__ */
