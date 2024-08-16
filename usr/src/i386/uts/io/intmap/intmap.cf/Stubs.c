/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:io/intmap/intmap.cf/Stubs.c	1.1"
#ident	"$Header: $"

void chanmap_close() { return; }
void chanmap_data() { return; }
void chanmap_output_msg() { return; }
int chanmap_do_iocdata() { return(0); }
int chanmap_do_ioctl() { return(0); }
