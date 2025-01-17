/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/mitsumi/mitsumi.cf/Space.c	1.12"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/param.h>
#include "config.h"
#include <sys/mitsumi.h>

int	mitsumi_gtol[ MAX_HAS ];		/* global to local */
int	mitsumi_ltog[ MAX_HAS ];		/* local to global */

int	mitsumi_lowat = LOWATER_JOBS;	/* LU queue low water mark	*/
int	mitsumi_hiwat = HIWATER_JOBS;	/* LU queue high water mark	*/

#ifdef  MITSUMI_CPUBIND
#define MITSUMI_VER	HBA_SVR4_2MP | HBA_IDATA_EXT
struct	hba_ext_info mitsumi_extinfo = {
	0, MITSUMI_CPUBIND
};
#else
#define MITSUMI_VER	1
#endif

struct	hba_idata_v4	_mitsumiidata[] = {
	{ MITSUMI_VER, "(mitsumi,1) MITSUMI",
	  7, 0, -1, 0, -1, 0 }
#ifdef  MITSUMI_CPUBIND
	,{ HBA_EXT_INFO, (char *) &mitsumi_extinfo }
#endif
};

int	mitsumi_cntls		= 1;

int 	mitsumi_rdy_timeout 	= 160000;

int	mitsumi_max_idle_time 	= MITSUMI_MAX_IDLE_TIME;   /* 5 minutes */
int	mitsumi_chktime		= MITSUMI_CHKTIME;	  /* 20 seconds */
int	mitsumi_cmd_timeout 	= MITSUMI_CMD_TIMEOUT;  	  /* 10 seconds */
ushort_t mitsumi_max_tries 	= 5;  	  		  /* 5 times */
