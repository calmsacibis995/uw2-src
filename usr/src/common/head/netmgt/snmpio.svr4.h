/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:netmgt/snmpio.svr4.h	1.4"
#ident	"$Header: /SRCS/esmp/usr/src/nw/head/netmgt/snmpio.svr4.h,v 1.4 1994/08/02 23:33:36 cyang Exp $"
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991, 1992,    *
 *                 1993, 1994  Novell, Inc. All Rights Reserved.            *
 *                                                                          *
 ****************************************************************************
 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	    *
 *      The copyright notice above does not evidence any   	            *
 *      actual or intended publication of such source code.                 *
 ****************************************************************************/

/*      @(#)snmpio.svr4	1.1 STREAMWare TCP/IP SVR4.2  source        */
/*      SCCS IDENTIFICATION        */
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989 INTERACTIVE Systems Corporation
 * All rights reserved.
 */

/*      @(#)snmpio.svr3	6.5 INTERACTIVE SNMP  source        */

#ifndef _SNMP_SNMPIO_SVR4
#define _SNMP_SNMPIO_SVR4

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key
 */
/*
snmpio.svr4

     snmpio.svr4 contains those data elements common to the routines
     which are commonly called by SNMP applications for net input
     and output but which are specific to System V Release 4 UNIX
*/

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/stream.h>
#include <sys/tiuser.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "snmp.h"
#include "snmpuser.h"

#define TRUE 1
#define FALSE 0

struct hostent *hp;
struct sockaddr_in sin;
struct sockaddr_in from;

#ifdef __cplusplus
}
#endif

#endif /* _SNMP_SNMPIO_SVR4 */
