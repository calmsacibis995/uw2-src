/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:net/osocket/osocket.cf/Space.c	1.2"
#ident	"$Header: $"

/* Enhanced Application Compatibility Support */

#include <sys/osocket.h>

#define ONSOCK 100

int num_osockets = ONSOCK;
struct osocket *osocket_tab[ONSOCK];
char osoc_domainbuf[OMAXHOSTNAMELEN] = { 0 };

/* Enhanced Application Compatibility Support */
