/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:io/io.cf/Space.c	1.9"

#include <config.h>	/* to collect tunable parameters */
#include <sys/types.h>

int nstrpush = NSTRPUSH;
int strmsgsz = STRMSGSZ;
int strctlsz = STRCTLSZ;
int strthresh = STRTHRESH;
int strnsched = STRNSCHED;

major_t maxmajor = MAXMAJOR;
minor_t maxminor = MAXMINOR;

int console_security = CONSOLE_SECURITY;
