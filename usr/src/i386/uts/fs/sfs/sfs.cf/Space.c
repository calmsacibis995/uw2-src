/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:fs/sfs/sfs.cf/Space.c	1.2"

#include <config.h>

int sfs_ninode;      		 /* now autotuned */
int sfs_inode_lwm = SFSINODELWM; /* low-water mark */
int sfs_timelag = SFSTIMELAG;    /* Heuristic: how many ticks before 
				  * encouraging recycling of an inactive 
				  * inode */
int sfs_tflush = SFSFLUSH;	 /* flush time parameter is NAUTOUP */
int sfs_ndquot = NDQUOT;	 /* size of quota table */
