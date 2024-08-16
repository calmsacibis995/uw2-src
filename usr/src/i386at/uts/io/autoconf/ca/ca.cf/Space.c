/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/autoconf/ca/ca.cf/Space.c	1.1"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>

#define	LOW_TO_HIGH	0x00
#define	HIGH_TO_LOW	0x01

uint_t ca_config_order = LOW_TO_HIGH;
