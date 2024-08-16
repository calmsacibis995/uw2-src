/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/wd7000/wd7000.cf/Space.c	1.12"
#ident	"$Header: $"


#include <sys/types.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/immu.h>
#include <sys/had.h>

#include "config.h"

int wd_dbgsize = 11;
char wd_Debug[11] = { 0, 0,0,0,0,0,0,0,0,0,0};

char wd_Board[11] = { 1, 1,1,1,0,0,0,0,0,0,0};

#ifdef	WD__CPUBIND

#define	WD__VER		HBA_SVR4_2MP | HBA_IDATA_EXT

struct	hba_ext_info	wd__extinfo = {
	0, WD__CPUBIND
};

#else

#define	WD__VER		HBA_SVR4_2MP

#endif	/* WD__CPUBIND */

struct	hba_idata_v4	_wd_idata[]	= {
	{ WD__VER, "(wd7000,1) WD7000 SCSI",
	  7, 0, -1, 0, -1, 0 }
#ifdef	WD__CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&wd__extinfo }
#endif	/* WD__CPUBIND */
};

int	wd__cntls	= 1;	/* # of controllers to be determined by
				 * autoconfig.
				 */

int	wd_enable_sync	= 0;	/*
				 * set to 1 to enable syncronous negotiation on all targets
				 */
