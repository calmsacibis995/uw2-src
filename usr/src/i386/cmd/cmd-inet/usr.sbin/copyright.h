/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _COPYRIGHT_H
#define _COPYRIGHT_H

#ident	"@(#)cmd-inet:i386/cmd/cmd-inet/usr.sbin/copyright.h	1.7"

/*
 * copyright for i386 machines
 */

#if 0

#define	COPYRIGHT(un) \
        (void) printf("UNIX(r) System V Release %s Version %s\n%s\n\
Copyright 1984-1993 Novell, Inc.  All Rights Reserved.\n\
Copyright 1987, 1988 Microsoft Corp.  All Rights Reserved.\n\
U.S. Pat. No. 5,349,642\n",\
	un.release, un.version, un.nodename)

#else				/* UnixWare licensee */

#define	COPYRIGHT(un) \
        (void) printf("UnixWare %s\n%s\n\
Copyright 1984-1995 Novell, Inc.  All Rights Reserved.\n\
Copyright 1987, 1988 Microsoft Corp.  All Rights Reserved.\n\
U.S. Pat. No. 5,349,642\n",\
	un.version, un.nodename)

#endif

#endif
