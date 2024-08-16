/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* 
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ident	"@(#)stand:i386sym/standalone/sys/conf_devsw.c	1.1"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/saio.h>

off_t	nulllseek();
void	scsidisk_ssmopen(), scsidiskclose();
int	scsidiskstrategy(), scsidiskioctl();
void	tmopen(), tmclose();
int	tmstrategy(), tmioctl();

struct devsw devsw[] = {
	{ "wd", scsidiskstrategy, scsidisk_ssmopen, scsidiskclose, 
		scsidiskioctl, nulllseek, D_DISK },
	{ "tm", tmstrategy, tmopen, tmclose, tmioctl, nulllseek, D_TAPE },
};

int n_devsw = sizeof(devsw)/sizeof(devsw[0]);
