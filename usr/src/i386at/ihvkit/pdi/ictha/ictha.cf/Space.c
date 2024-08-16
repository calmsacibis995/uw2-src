/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ihvkit:pdi/ictha/ictha.cf/Space.c	1.2"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/st01.h>
#include <sys/ictha.h>
#include "config.h"

ulong	ictha_rdwr_duration 	 = 60*1;   /* 2 minute */
ulong	ictha_cmds_duration 	 = 60*1;   /* 2 minute */
ulong	ictha_retention_duration = 60*15;  /* 15 minutes */
ulong	ictha_rewind_duration 	 = 60*15;  /* 15 minutes */
ulong	ictha_erase_duration 	 = 60*15;  /* 15 minutes */
ulong	ictha_space_duration 	 = 60*15;  /* 15 minutes */

long	ictha_blocksize = ICTHA_BLOCKSIZE;

ulong	ictha_init_wait_limit = ICTHA_INIT_WAIT_LIMIT;
ulong	ictha_wait_poll_limit = ICTHA_WAIT_POLL_LIMIT;
ulong	ictha_wr_maxout       = ICTHA_WR_MAXTOUT;
ulong	ictha_waitcnt	      = 250;

struct	ictha_ctrl 	ictha_ctrl[ICTHA_CNTLS];

#ifdef	ICTHA_CPUBIND

#define	ICTHA_ID_DIM	ICTHA_CNTLS+1
#define	ICTHA_VER	HBA_SVR4_2MP | HBA_IDATA_EXT

struct	hba_ext_info	ictha_extinfo = {
	0, ICTHA_CPUBIND
};

#else

#define	ICTHA_ID_DIM	ICTHA_CNTLS
#define	ICTHA_VER	1

#endif	/* ICTHA_CPUBIND */

struct	hba_idata	icthaidata[ICTHA_ID_DIM]	= {
#ifdef	ICTHA_0
	{ ICTHA_VER, "(ictha,1st) ICT TAPE HBA",
	  7, ICTHA_0_SIOA, ICTHA_0_CHAN, ICTHA_0_VECT, ICTHA_0, 0 }
#endif
#ifdef	ICTHA_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&ictha_extinfo }
#endif	/* ICTHA_CPUBIND */
};

int	ictha_cntls	= ICTHA_CNTLS;

