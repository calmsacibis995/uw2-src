/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/udev/udev.cf/Space.c	1.1"
#ident	"$Header: $"

#include <config.h>
#include <sys/bitmasks.h>
#include <sys/sysmacros.h>
#include <sys/types.h>

major_t udev_imajor = UDEV__CMAJOR_0;
uint_t udev_nmajors = UDEV__CMAJORS;

#define BITS_MAX	(UDEV__CMAJORS << O_BITSMINOR)
#define BITS_SIZE	BITMASK_NWORDS(BITS_MAX)

uint_t udev_bits[BITS_SIZE];
