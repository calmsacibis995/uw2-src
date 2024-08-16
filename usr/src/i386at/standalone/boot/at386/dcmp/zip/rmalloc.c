/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386at/standalone/boot/at386/dcmp/zip/rmalloc.c	1.3"

/*
 * rmalloc.c -- routines to manage memory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * You may contact UNIX System Laboratories by writing to
 * UNIX System Laboratories, 190 River Road, Summit, NJ 07901, USA
 */

#include <sys/types.h>
#include <sys/bootinfo.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/immu.h>
#include <sys/cram.h>

#include <boothdr/boot.h>
#include <boothdr/initprog.h>
#include <boothdr/bootlink.h>
#include <boothdr/libfm.h>
#include "dcmp.h"

char *rmalloc_header;
static char *next = NULL;
static int freeleft;

void *rmalloc(int amount)
{
	char *ret;

	if ( next == NULL ){
		next = rmalloc_header;
		freeleft = HUFT_BUF_SIZE;
	}
	amount += (amount % 16);
	if (amount > freeleft) {
		printf("out of memory for rmalloc\n");
		return 0;
	}
	freeleft -= amount;
	ret = next;
	next += amount;
	return ret;
}

bt_free(void)
{
	next = rmalloc_header;
	freeleft = HUFT_BUF_SIZE;
}
