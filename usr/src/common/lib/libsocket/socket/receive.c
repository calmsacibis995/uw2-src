/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)libsocket:common/lib/libsocket/socket/receive.c	1.5.11.5"
#ident	"$Header: $"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stream.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/xti.h>		/* REQUIRED - XPG4 */
#include <sys/sockmod.h>

ssize_t
recvmsg(s, msg, flags)
	int			s;
	struct msghdr		*msg;
	int			flags;
{
	register struct _si_user	*siptr;

	if ((siptr = _s_checkfd(s)) == NULL)
		return (-1);

	return (_s_soreceive(siptr, msg, flags));
}

ssize_t
recvfrom(s, buf, len, flags, from, fromlen)
	int			s;
	void			*buf;
	size_t			len;
	int			flags;
	struct sockaddr		*from;
	size_t			*fromlen;
{
	register struct _si_user	*siptr;
	register int			retlen;
	struct iovec			msg_iov[1];
	struct msghdr			msg;

	if ((siptr = _s_checkfd(s)) == NULL)
		return (-1);

	msg.msg_iovlen = 1;
	msg.msg_iov = msg_iov;
	msg.msg_iov[0].iov_base = buf;
	msg.msg_iov[0].iov_len = len;
	msg.msg_namelen = *fromlen;
	msg.msg_name = (caddr_t)from;
	msg.msg_accrightslen = 0;
	msg.msg_accrights = NULL;

	if ((retlen = _s_soreceive(siptr, &msg, flags)) < 0)
		return (-1);
	if (fromlen != NULL)
		*fromlen = msg.msg_namelen;

	return (retlen);
}

ssize_t
recv(s, buf, len, flags)
	int			s;
	void			*buf;
	size_t			len;
	int			flags;
{
	struct _si_user	*siptr;
	struct msghdr			msg;
	struct iovec			msg_iov[1];

	if ((siptr = _s_checkfd(s)) == NULL)
		return (-1);

	msg.msg_iovlen = 1;
	msg.msg_iov = msg_iov;
	msg.msg_iov[0].iov_base = buf;
	msg.msg_iov[0].iov_len = len;
	msg.msg_namelen = 0;
	msg.msg_name = NULL;
	msg.msg_accrightslen = 0;
	msg.msg_accrights = NULL;

	return (_s_soreceive(siptr, &msg, flags));
}
