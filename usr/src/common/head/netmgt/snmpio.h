/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:netmgt/snmpio.h	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/head/netmgt/snmpio.h,v 1.7 1994/09/15 21:49:36 cyang Exp $"
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

/*      @(#)snmpio.h	1.1 STREAMWare TCP/IP SVR4.2  source        */
/*      SCCS IDENTIFICATION        */
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
 * All rights reserved.
 */

/*      @(#)snmpio.h	6.4 INTERACTIVE SNMP  source        */

#ifndef _SNMP_SNMPIO_H
#define _SNMP_SNMPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */


/*
snmpio.h

     snmpio.h contains those data elements common to the routines
     which are commonly called by SNMP applications for net input
     and output
*/

#include "snmpio.svr4.h"	/* include file for use on SVR3 with Lachman TCP */

#define WAITING 0
#define TIMEOUT 1
#define RECEIVED 2
#define ERROR  -1
#define SECS 20  /* time to wait for reply before declaring timeout in seconds */



#ifdef SVR4
void time_out(int);
#else
int time_out();
#endif

#ifdef __STDC__
void initialize_io(char *program_name, char *name);
void close_up(int fd);    /* close the transport */
int send_request(int socket, AuthHeader *auth_pointer);
int get_response(int seconds);
long make_req_id(void);  /* returns a request-id */
#else
void initialize_io();
void close_up();
int send_request();
int get_response();
long make_req_id();      /* returns a request-id */
#endif

long packet_len;
unsigned char packet[2048];

int fd;

char imagename[128];		       /* place to hold name of image */

#define Min(a, b) ((a) < (b) ? (a) : (b))

#ifdef __cplusplus
}
#endif

#endif /* _SNMP_SNMPIO_H */
