/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:fs/s5fs/s5.cf/Space.c	1.3"

#include <config.h>

int ninode;			/* now autotuned */
int s5_inode_lwm = S5INODELWM;	/* low-water mark */
int s5_tflush = S5FSFLUSH;	/* the frequency of flush daemon */

