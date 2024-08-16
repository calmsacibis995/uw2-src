/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.lib/libtelnet/misc.c	1.1.1.1"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *      System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      The copyright above and this notice must be preserved in all
 *      copies of this source code.  The copyright above does not
 *      evidence any actual or intended publication of this source
 *      code.
 *
 *      This is unpublished proprietary trade secret source code of
 *      Lachman Associates.  This source code may not be copied,
 *      disclosed, distributed, demonstrated or licensed except as
 *      expressly authorized by Lachman Associates.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */

/*      SCCS IDENTIFICATION        */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)misc.c	5.1 (Berkeley) 2/28/91";
#endif /* not lint */

/*
 * Copyright (c) 1988, 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <pfmt.h>
#include "misc.h"

char *RemoteHostName;
char *LocalHostName;
char *UserNameRequested = 0;
int ConnectedCount = 0;

	void
auth_encrypt_init(local, remote, name, server)
	char *local;
	char *remote;
	char *name;
	int server;
{
	RemoteHostName = remote;
	LocalHostName = local;
#if	defined(AUTHENTICATE)
	auth_init(name, server);
#endif
#if	defined(ENCRYPT)
	encrypt_init(name, server);
#endif
	if (UserNameRequested) {
		free(UserNameRequested);
		UserNameRequested = 0;
	}
}

	void
auth_encrypt_user(name)
	char *name;
{
	extern char *strdup();

	if (UserNameRequested)
		free(UserNameRequested);
	UserNameRequested = name ? strdup(name) : 0;
}

	void
auth_encrypt_connect(cnt)
	int cnt;
{
}

	void
printd(data, cnt)
	unsigned char *data;
	int cnt;
{
	if (cnt > 16)
		cnt = 16;
	while (cnt-- > 0) {
		printf(" %02x", *data);
		++data;
	}
}
