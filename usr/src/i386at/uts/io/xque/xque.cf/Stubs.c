/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/xque/xque.cf/Stubs.c	1.1"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>

addr_t xq_allocate_scoq() { return NULL; }
void xq_close() {}
int xq_close_scoq() { return 0; }
int xq_enqueue() { return 0; }
caddr_t xq_init() { return NULL; }
