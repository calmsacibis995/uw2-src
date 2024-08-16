/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:mem/pse.cf/Space.c	1.1"
#ident	"$Header: $"

#include <sys/types.h>
#include <config.h>

uint_t pse_vmemsize = PSE_VMEMSIZE;
uint_t pse_physmem = PSE_PHYSMEM;
uint_t pse_major = PSE__CMAJOR_0;
