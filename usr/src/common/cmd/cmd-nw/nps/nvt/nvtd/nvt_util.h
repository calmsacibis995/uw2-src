/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/nvt/nvtd/nvt_util.h	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: nvt_util.h,v 1.3 1994/02/28 15:40:38 vtag Exp $"
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
 * File Name: util.h
 *
 * Description: Novell Virtual Terminal Daemon common utilities header
 *
 ****************************************************************************/
#include <stdarg.h>
#include <syslog.h>

/* CONSTANTS */
#define CNULL		(char*)0	/* character pointer null replaces (char*)NULL*/
#define EOS			'\0'		/* End Of String */

/* MACROS */
#ifdef	DEBUG
#define	SYSLOG(x)			syslog(LOG_NOTICE, x)
#define	SYSLOG1(x, p1)		syslog(LOG_NOTICE, x, p1)
#define	SYSLOG2(x, p1, p2)	syslog(LOG_NOTICE, x, p1, p2)
#define	OPENLOG(x)			openlog(x, LOG_CONS, LOG_USER)
#else
#define	SYSLOG(x)
#define	SYSLOG1(x, p1)
#define	SYSLOG2(x, p1, p2)
#define	OPENLOG(x)
#endif /* DEBUG */
