/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:svc/name.cf/Space.c	1.9"

/*
 * This file contains machine dependent information.
 */

#include <sys/types.h>
#include <sys/utsname.h>
#include <config.h>

/* ibcs2 conformance restricts the value assigned to the constant SYSNAME
 * to 8 characters or less.
 */
#define SYSNAME	"UNIX_SV"
#define XSYS	"UNIX_Sys"
#define NODE	"unix"
#define REL	"4.2MP"
#define VER	"2.01"
#define CPU	"i386"

#define MACH		"i386at"
#define ARCHITECTURE	"x86at"
#define HW_SERIAL	""
#define HW_PROVIDER	"Novell"
#define SRPC_DOMAIN	""

#define RESRVD	""
#define ORIGIN	1
#define OEMNUM	0
#define SERIAL	0

struct utsname	utsname = {
	SYSNAME,
	NODE,
	REL,
	VER,
	CPU
};

struct	xutsname xutsname = {
		XSYS,
		NODE,
		REL,
		VER,
		MACH,
		RESRVD,
		ORIGIN,
		OEMNUM,
		SERIAL
};

/* sysinfo information */
char architecture[SYS_NMLN] = ARCHITECTURE;
char hw_serial[SYS_NMLN] = HW_SERIAL;
char hw_provider[SYS_NMLN] = HW_PROVIDER;
char srpc_domain[SYS_NMLN] = SRPC_DOMAIN;
char initfile[SYS_NMLN] = INITFILE;
char *bustype = "AT";
