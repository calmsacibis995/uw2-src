/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/sc/sc.cf/Space.c	1.2"

#include <sys/types.h>	
#include <config.h>	
#include <sys/sc.h>		/* Contains flow control selectors */
#include <sys/termios.h>		/* Contains cflags definitions */

/*
 * Group assignments of statically/binary configurable
 * data to the global data structure for the sc-driver.
 * Arrange for remaining fields to start out zeroed.
 */
struct sc_bin_conf sc_global = {
        SC_0_IPL,                               /* SLIC bin for sc-interrupts */
	(CS8 | CREAD | HUPCL | B9600),          /* Initial cflags */
        IGNPAR,                                 /* Initial iflags */
        20,                                     /* Rx timeout 20 ms */
        3,                                      /* Additional wait time */
        0,                                      /* 0=don't print allocb fail */
	CCF_XOFF,                               /* Flow control */
        SC_CMAJOR_0,                            /* Major device # from Master */
};
