/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/xs/xs.cf/Space.c	1.2"

/*
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 *
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/*
 * Xylogics 781 STREAMS driver (xs) binary configuration parameters.
 */

#include <sys/types.h>
#include <config.h>
#include <sys/termios.h>         /* Contains cflags definitions */
#include <sys/xs.h>

/*
 * Group assignments of statically/binary configurable
 * data to the global data structure for the xs-driver.
 * Arrange for remaining fields to start out zeroed.
 */
xs_bconf_t xs_global = {
        XS_CMAJOR_0,                            /* Major device # from Master */
        4,                               	/* SLIC bin for xs-interrupts */
	(CS8 | CREAD | HUPCL | B9600),          /* Initial cflags */
        IGNPAR,                                 /* Initial iflags */
        1,                                      /* Additional wait time */
	250,					/* Break duration millisecs */
	400000,					/* ARCP command timeout; 4sec */
        20,                                     /* Rx timeout, 0.1 sec */
	255,					/* Minimal input buffer size;
						 * must be <= 255. */
	(XS_OBUF_SZ / 2),			/* Output low water mark */
        0,                                      /* 0=don't print allocb fail */
        0,                                      /* 0=don't print overflow msg */
};

/*
 * The following records below desribes the
 * configuration for a specific 781 board.
 * They are then referenced in the configuration
 * array, xs_conf[].  In the future, these declarations
 * should be output by enhanced idtools to its conf.c file, 
 * based on the driver's Master and System files, which
 * must then be modified to specify the attributes in a 
 * new format.
 */
#define XS_DEF_IPL	2
			  /* tag, Configured, SSM/VMEbus#, CSR,    int-lvl */
static struct xs_conf xs0 = { 0,  1,		0,	   0xC000, XS_DEF_IPL };
static struct xs_conf xs1 = { 1,  1,		0,	   0xC010, XS_DEF_IPL };
static struct xs_conf xs2 = { 2,  1,		0,	   0xC020, XS_DEF_IPL };
static struct xs_conf xs3 = { 3,  1,		0,	   0xC040, XS_DEF_IPL };
static struct xs_conf xs4 = { 4,  1,		0,	   0xC080, XS_DEF_IPL };
static struct xs_conf xs5 = { 5,  1,		0,	   0xC100, XS_DEF_IPL };
static struct xs_conf xs6 = { 6,  1,		0,	   0xC110, XS_DEF_IPL };
static struct xs_conf xs7 = { 7,  1,		0,	   0xC120, XS_DEF_IPL };

struct xs_conf *xs_conf[] = { &xs0, &xs1, &xs2, &xs3, &xs4, &xs5, &xs6, &xs7 };
int xs_nconf = 8;
