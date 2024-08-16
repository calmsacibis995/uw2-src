/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:io/intmap/intmap.cf/Space.c	1.1"
#ident	"$Header: $"

#include <sys/emap.h>	/* channel mapping include file from XENIX */
#include <sys/nmap.h>	/* channel mapping include file from XENIX */

#include "config.h" 	/* to get NEMAP tunaebale from mtune */

struct	emap	emap[NEMAP];	/* channel mapping data struct table */
struct	nxmap	nxmap[NEMAP];	/* channel mapping data struct table */


