/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.named/tools/nslookup/res.h	1.3.8.2"
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
 *      System V STREAMS TCP - Release 4.0
 *
 *      Copyright 1990 Interactive Systems Corporation,(ISC)
 *      All Rights Reserved.
 *
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *	@(#)res.h	5.10 (Berkeley) 6/1/90
 */

/*
 *******************************************************************************
 *
 *  res.h --
 *
 *	Definitions used by modules of the name server lookup program.
 *
 *	Copyright (c) 1985
 *	Andrew Cherenson
 *	U.C. Berkeley
 *	CS298-26  Fall 1985
 * 
 *******************************************************************************
 */

#define TRUE	1
#define FALSE	0
typedef int Boolean;

/*
 *  Define return statuses in addtion to the ones defined in namserv.h
 *   let SUCCESS be a synonym for NOERROR
 *
 *	TIME_OUT	- a socket connection timed out.
 *	NO_INFO		- the server didn't find any info about the host.
 *	ERROR		- one of the following types of errors:
 *			   dn_expand, res_mkquery failed
 *			   bad command line, socket operation failed, etc.
 *	NONAUTH		- the server didn't have the desired info but
 *			  returned the name(s) of some servers who should.
 *	NO_RESPONSE	- the server didn't respond.
 *
 */

#define  SUCCESS		0
#define  TIME_OUT		-1
#define  NO_INFO		-2
#define  ERROR			-3
#define  NONAUTH		-4
#define  NO_RESPONSE		-5

/*
 *  Define additional options for the resolver state structure.
 *
 *   RES_DEBUG2		more verbose debug level
 */

#define RES_DEBUG2	0x80000000

/*
 *  Maximum length of server, host and file names.
 */

#define NAME_LEN 256


/*
 * Modified struct hostent from <netdb.h>
 *
 * "Structures returned by network data base library.  All addresses
 * are supplied in host order, and returned in network order (suitable
 * for use in system calls)."
 */

typedef struct	{
	char	*name;		/* official name of host */
	char	**domains;	/* domains it serves */
	char	**addrList;	/* list of addresses from name server */
} ServerInfo;

typedef struct	{
	char	*name;		/* official name of host */
	char	**aliases;	/* alias list */
	char	**addrList;	/* list of addresses from name server */
	int	addrType;	/* host address type */
	int	addrLen;	/* length of address */
	ServerInfo **servers;
} HostInfo;


/*
 *  FilePtr is used for directing listings to a file.
 *  It is global so the Control-C handler can close it.
 */

extern FILE *filePtr;

/*
 * TCP/UDP port of server.
 */
extern unsigned short nsport;

/*
 *  External routines:
 */

extern Boolean IsAddr();
extern int  Print_query();
extern char *Print_cdname();
extern char *Print_cdname2();	/* fixed width */
extern char *Print_rr();
extern char *DecodeType();	/* descriptive version of p_type */
extern char *DecodeError();
extern char *Calloc();
extern char *Malloc();
extern void NsError();
extern void PrintServer();
extern void PrintHostInfo();
extern void ShowOptions();
extern void FreeHostInfoPtr();
extern FILE *OpenFile();
extern char *res_skip();

/*
 * Put here a #define that should go in <resolv.h>
 */
#define MAXDFLSRCH      3               /* # default domain levels to try */


