/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:util/ccnv/ccnv.cf/Stubs.c	1.2"
#ident	"$Header: $"
#include <sys/errno.h>

ccnv_dos2unix(char **a, unsigned char *b, unsigned int c) { return(ENOLOAD); }
ccnv_unix2dos(char **a, unsigned char **b, unsigned int c) { return(ENOLOAD); }
dos2unixfn(unsigned char *a, unsigned char *b, int *c) { return(ENOLOAD); }
unix2dosfn(unsigned char *a, unsigned char *b, int c) { return(ENOLOAD); }
