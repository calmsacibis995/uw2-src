/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/sad/sad.cf/Space.c	1.2"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/stream.h>
#include <sys/sad.h>

#include <config.h>	/* to collect tunable parameters */

struct	saddev saddev[NUMSAD];
struct	autopush autopush[NAUTOPUSH];
struct autopush *strpcache[NSTRPHASH];	/* autopush hash list */

int	sadcnt = NUMSAD;
int	nstrphash = NSTRPHASH;
int	nautopush = NAUTOPUSH;
int	strpmask = NSTRPHASH-1;
