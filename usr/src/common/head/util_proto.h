/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:util_proto.h	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Header: /SRCS/esmp/usr/src/nw/head/util_proto.h,v 1.8 1994/08/16 18:13:27 vtag Exp $"
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
 */

/*
 * ANSI C Function prototypes for libnwutil.
 */

#ifndef __UTIL_PROTO_H__
#define __UTIL_PROTO_H__

#if defined(__STDC__) || defined(__cplusplus)

/* header file dependancies */

#include <stddef.h>
#include <sys/types.h>
#include <sys/nwportable.h>

/* lib/libutil/string.c */

#ifdef __cplusplus
extern "C" {
#endif

/* lib/libutil/server.c */
extern int StopNucSapd( void);

/* lib/libutil/logpid.c */

extern int LogPidToFile(char *, char *, pid_t);
extern int DeleteLogPidFile(char *, char *);
extern int LogPidKill(char *, char *, int);

#ifdef __cplusplus
}
#endif

#else /* __STDC__ */

extern int StopNucSapd();
extern int LogPidToFile();
extern int DeleteLogPidFile();
extern int LogPidKill();

#endif /* __STDC__ */

#endif /* __UTIL_PROTO_H__ */
