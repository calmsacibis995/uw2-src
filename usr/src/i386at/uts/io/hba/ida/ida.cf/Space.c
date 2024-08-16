/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/ida/ida.cf/Space.c	1.12"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include "config.h"

int	ida_gtol[ SDI_MAX_HBAS ];		/* global to local */
int	ida_ltog[ SDI_MAX_HBAS ];		/* local to global */

#ifndef IDA_CNTLS
#define IDA_CNTLS 1
#endif

#ifdef ONEJOB
int	ida_lowat = 1;	/* LU queue low water mark	*/
int	ida_hiwat = 1;	/* LU queue high water mark	*/
#else

#define	LOWATER_JOBS		18	/* low water mark for jobs */
#define	HIWATER_JOBS		22	/* high water mark for jobs */

int	ida_lowat = LOWATER_JOBS;	/* LU queue low water mark	*/
int	ida_hiwat = HIWATER_JOBS;	/* LU queue high water mark	*/
#endif

struct	ver_no    ida_sdi_ver;

#ifdef	IDA_CPUBIND

#define	IDA_VER	HBA_SVR4_2MP | HBA_IDATA_EXT

struct	hba_ext_info	ida_extinfo = {
	0, IDA_CPUBIND
};

#else

#define	IDA_VER	HBA_SVR4_2MP

#endif	/* IDA_CPUBIND */

struct	hba_idata_v4	_idaidata[]	= {
	{ IDA_VER, "(ida,1) Compaq IDA", 7, 0, -1, 0, -1, 0 }
#ifdef IDA_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&ida_extinfo }
#endif
};

int	ida_cntls	= IDA_CNTLS;

