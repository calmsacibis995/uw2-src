/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/tm/tm.cf/Space.c	1.1"

/*
 * SSM/Streaming tape driver (tm) binary configuration parameters.
 */

#include <sys/types.h>
#include <config.h>
#include <sys/tm.h>

/*
 * Group assignments of statically/binary configurable
 * data to the global data structure for the tm-driver.
 * Arrange for remaining fields to start out zeroed.
 */
tm_bconf_t tm_global = {
        TM_CMAJOR_0,                            /* Major device # from Master */
        5,                               	/* SLIC bin for tm-interrupts */
};
