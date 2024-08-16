/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _STROPTS_H
#define _STROPTS_H

#ident	"@(#)sgs-head:common/head/stropts.h	1.10"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Streams user options definitions.
 */

#ifndef _SYS_STROPTS_H
#include <sys/stropts.h>
#endif

#if defined(__STDC__)

extern int getmsg(int, struct strbuf *, struct strbuf *, int *);
extern int putmsg(int, const struct strbuf *, const struct strbuf *, int);

extern int getpmsg(int, struct strbuf *, struct strbuf *, int *, int *);
extern int putpmsg(int, const struct strbuf *, const struct strbuf *, int, int);

extern int isastream(int);

#else

extern int getmsg();
extern int putmsg();

extern int getpmsg();
extern int putpmsg();

extern int isastream();

#endif

#if defined(__cplusplus)
        }
#endif
#endif /* _STROPTS_H */
