/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/asy/iasy.cf/Space.c	1.9"
#ident	"$Header: $"

/*	Copyright (c) 1991 Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	


/*
 * Reserve storage for Generic serial driver
 */

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/termio.h>
#include <sys/strtty.h>
#include <sys/cred.h>
#include <sys/ddi.h>
#include <sys/iasy.h>
#include "config.h"


int	iasy_num = IASY_UNITS;
major_t	iasy_major = IASY_CMAJOR_0;


struct strtty  asy_tty[IASY_UNITS];	/* strtty structs for each device. 
					 * iasy_tty is changed to asy_tty for
					 * merge.
					 */
struct iasy_hw iasy_hw[IASY_UNITS];	/* Hardware support routines */
struct iasy_sv iasy_sv[IASY_UNITS];	/* sync. variables per port */
toid_t iasy_toid[IASY_UNITS];		/* timeout IDs per port */

int strhead_iasy_hiwat = 5120;
int strhead_iasy_lowat = 4096;
