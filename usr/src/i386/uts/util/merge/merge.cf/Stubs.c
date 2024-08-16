/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:util/merge/merge.cf/Stubs.c	1.2"
#ident	"$Header: $"

/* Stubs for MERGE386 */

int portalloc() { return 1; }
int portfree() { return 1; }
int floppy_free() { return 1; }

int isdosexec() { return 0; }
int asy_is_assigned() { return 0; }
int com_ppiioctl() { return 0; }
int com_ppi_ioctl() { return 0; }
int com_ppi_strioctl() { return 0; }
int flp_for_dos() { return 0; }
int kdppi_ioctl() { return 0; }
int mrg_in_use() { return 0; }
