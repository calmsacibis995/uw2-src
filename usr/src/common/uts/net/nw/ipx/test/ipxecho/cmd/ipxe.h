/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ipx-test:common/uts/net/nw/ipx/test/ipxecho/cmd/ipxe.h	1.1"
/*  Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.  */
/*  Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Righ
ts Reserved.    */
/*    All Rights Reserved   */

/*  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.  */
/*  The copyright notice above does not evidence any    */
/*  actual or intended publication of such source code. */

#ident  "$Id"
/******************************************************************************
 *	NAME: ipxe.h
 *
 *	DATE: 18 MAY 99	
 *
 ******************************************************************************/

/* INCLUDES */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <tiuser.h>
#include <errno.h>
#include <sys/types.h>
#include <stropts.h>
#include <sys/nwportable.h>
#include <sys/ipx_app.h>

/* DEFINES */

#define IPX				"/dev/ipx"		/* ipx device. */
#define KERNEL_SOCKET	0x8130			/* Kernel echo socket */
#define USER_SOCKET		0x8131			/* User echo socket */

#define ECHO_MODULE		"ipxecho"		/* IPXECHO module name */
