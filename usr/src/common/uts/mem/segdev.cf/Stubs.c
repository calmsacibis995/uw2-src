/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/segdev.cf/Stubs.c	1.2"
#ident	"$Header: $"

/* Stubs for segdev module */

#include <sys/errno.h>

int segdev_create() { return ENOSYS; }
int drv_mmap() { return ENOSYS; }
void drv_munmap() {}
