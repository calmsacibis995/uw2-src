/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.gated/recvmsg.c	1.2"
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
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */
/*
 *  $Header: /users/jch/src/gated/src/compat/RCS/recvmsg.c,v 2.0 90/04/16 16:55:47 jch Exp $
 */

/********************************************************************************
*										*
*	GateD, Release 2							*
*										*
*	Copyright (c) 1990 by Cornell University				*
*	    All rights reserved.						*
*										*
*	    Royalty-free licenses to redistribute GateD Release 2 in		*
*	    whole or in part may be obtained by writing to:			*
*										*
*	    Center for Theory and Simulation in Science and Engineering		*
*	    Cornell University							*
*	    Ithaca, NY 14853-5201.						*
*										*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR		*
*	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED		*
*	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.	*
*										*
*	GateD is based on Kirton's EGP, UC Berkeley's routing daemon		*
*	(routed), and DCN's HELLO routing Protocol.  Development of Release	*
*	2 has been supported by the National Science Foundation.		*
*										*
*	The following acknowledgements and thanks apply:			*
*										*
*	    Mark Fedor (fedor@psi.com) for the development and maintenance	*
*	    up to release 1.3.1 and his continuing advice.			*
*										*
*********************************************************************************
*      Portions of this software may fall under the following			*
*      copyrights: 								*
*										*
*	Copyright (c) 1988 Regents of the University of California.		*
*	All rights reserved.							*
*										*
*	Redistribution and use in source and binary forms are permitted		*
*	provided that the above copyright notice and this paragraph are		*
*	duplicated in all such forms and that any documentation,		*
*	advertising materials, and other materials related to such		*
*	distribution and use acknowledge that the software was developed	*
*	by the University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote products derived	*
*	from this software without specific prior written permission.		*
*	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR		*
*	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED		*
*	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.	*
********************************************************************************/


#include "include.h"

static caddr_t buf_base;
static int buf_size;

int
recvmsg(s, msg, flags)
int s;
struct msghdr *msg;
int flags;
{
    register int i, cc, acc;
    extern int errno;
    extern char etext;
    extern char *sbrk();
    char *buf_ptr;

    if ((msg->msg_iovlen < 1) || (msg->msg_iovlen > MSG_MAXIOVLEN)) {
	errno = EINVAL;
	return (-1);
    }
#ifdef	SCM_RIGHTS
    if (msg->msg_control) {
	*msg->msg_control = (char) 0;
    }
#else				/* SCM_RIGHTS */
    if (msg->msg_accrights) {
	*msg->msg_accrights = (char) 0;
    }
#endif				/* SCM_RIGHTS */

    /* only 1 buffer - receive the data directly */
    if (msg->msg_iovlen == 1) {
	acc = recvfrom(s,
		       msg->msg_iov->iov_base,
		       msg->msg_iov->iov_len,
		       flags,
		       msg->msg_name,
		       &msg->msg_namelen);
	return (acc);
    }
    /* Scan through the iovec's to check lengths and figure out */
    /* maximum buffer size */
    for (acc = i = 0; i < msg->msg_iovlen; i++) {
	register int len = msg->msg_iov[i].iov_len;
	register char *base = msg->msg_iov[i].iov_base;

	if ((len < 0) || (base < &etext) || ((base + len) > sbrk(0))) {
	    errno = EINVAL;
	    return (-1);
	}
	acc += msg->msg_iov[i].iov_len;
    }

    /* Allocate a receive buffer */
    if (acc > buf_size) {
	if (buf_base != NULL) {
	    (void) free(buf_base);
	}
	buf_base = (caddr_t) malloc(acc);
	buf_size = acc;
    }
    if (!buf_base) {
	buf_size = 0;
	errno = ENOMEM;
	return (-1);
    }
    acc = cc = recvfrom(s,
			buf_base,
			acc,
			flags,
			msg->msg_name,
			&msg->msg_namelen);

    /* Return if error */
    if (cc < 0) {
	return (cc);
    }
    /* Distribute the data as specified in the iovec */
    for (i = 0, buf_ptr = buf_base; acc && (i < msg->msg_iovlen); i++) {
	register int len = msg->msg_iov[i].iov_len;

	(void) memcpy(msg->msg_iov[i].iov_base, buf_ptr, len);
	buf_ptr += len;
	acc -= len;
    }

    /* Return the length read */
    return (cc);
}
