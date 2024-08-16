/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/nvt/nvtd/nvt_util.c	1.6"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: nvt_util.c,v 1.4 1994/07/05 15:50:08 mark Exp $"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
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

/****************************************************************************
 *
 * File Name: util.c
 *
 * Description: Novell Virtual Terminal Daemon common utilities
 *
 ****************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include "nvt_util.h"
#ifndef DEBUG
#include <fcntl.h>
#include "nwmsg.h"
#include "npsmsgtable.h"
#endif	/* !DEBUG*/

void
ErrorOut(const char *fmt, ...)
{
	va_list		ap;
	char		buf[81];
#ifndef DEBUG
	FILE		*fd;
#endif	/* !DEBUG*/

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	strcat(buf, "\n");
	va_end(ap);

#ifdef DEBUG
	SYSLOG(buf);
#else
	fd = fopen("/dev/console", "w");	/* write failures to console */
	fprintf(fd, buf); 
#endif
	exit(-1);
}
