/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ident	"@(#)stand:i386sym/standalone/i386/symbols.c	1.1"

#include <sys/types.h>
#include <sys/cfg.h>

#define	offsetof(x, y)	((int)&((x *)0)->y)
#define	OFFSET(s, st, m) \
	size_t __SYMBOL__##s = (size_t)offsetof(st, m)

#define	DEFINE(s, e) \
	size_t __SYMBOL__##s = (size_t)e

/* new firmware */
DEFINE(CD_LOC, CD_LOC);
OFFSET(c_version, struct config_desc, c_version);
OFFSET(c_bottom, struct config_desc, c_bottom);

DEFINE(sd_type, offsetof(struct config_desc, c_sys) +
	offsetof(struct sys_desc, sd_type));

/* system types */

DEFINE(SYSTYP_B8, SYSTYP_B8);
DEFINE(SYSTYP_B21, SYSTYP_B21);
DEFINE(SYSTYP_S27, SYSTYP_S27);
DEFINE(SYSTYP_S81, SYSTYP_S81);
DEFINE(SYSTYP_S16, SYSTYP_S16);
DEFINE(SYSTYP_S1, SYSTYP_S1);
