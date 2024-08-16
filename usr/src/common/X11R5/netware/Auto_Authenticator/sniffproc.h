/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoauthent:sniffproc.h	1.1"


/*
 *    Copyright Novell Inc. 1993, 1994
 *    Copyright Univel 1993, 1994
 *    (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
 *
 *    No part of this file may be duplicated, revised, translated, localized
 *    or modified in any manner or compiled, linked or uploaded or
 *    downloaded to or from any computer system without the prior written
 *    consent of Novell, Inc.
 *
 *  Netware Unix Client
 *
 *	Author:  Duck
 *	Created: 1-1-93 (really)
 *
 *	MODULE:
 *		sniffproc.h - Header file for callers of sniffproc functions
 *
 *	ABSTRACT:
 *		contains ANSI prototypes of public sniffing functions.
 *
 */ 


extern char	*sniff_env( const pid_t pid, const char *searchString);
extern int	sniffinfo( const pid_t pid, const int cmd );
extern char *gethost( int pid);

#define	SNIFF_TTYDEV	1
#define	SNIFF_PPID		3
#define	SNIFF_FNAME		4
#define SNIFF_PSARGS    5


#define sniff_ttydev(pid)	sniffinfo( (pid), SNIFF_TTYDEV)
#define sniff_ppid(pid)		sniffinfo( (pid), SNIFF_PPID)
#define sniff_fname(pid)	(char *)sniffinfo( (pid), SNIFF_FNAME)
#define sniff_psargs(pid)   (char *)sniffinfo( (pid), SNIFF_PSARGS)

