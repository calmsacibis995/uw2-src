/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/asy/asyc/asyc.cf/Space.c	1.14"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/stream.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/termio.h>
#include <sys/asyc.h>
#include <sys/inline.h>
#include <sys/cmn_err.h>
#include <sys/stropts.h>
#include <sys/strtty.h>
#include <sys/debug.h>
#include <sys/eucioctl.h>
#include <sys/ddi.h>
#include "config.h"

#define		ASYC_CONSOLE_YES	1
#define		ASYC_CONSOLE_NO		0

unsigned int asyc_sminor = 0;		/* Starting minor number */

asyc_base_t asyc_base[4] =  {
	{ 0x3F8,4 },
	{ 0x2F8,3 },
	{ 0x3E8,5 },
	{ 0x2E8,5 },
};
