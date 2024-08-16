/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/blc/blc.cf/Space.c	1.4"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/conf.h>
#include "config.h"

int	 blc_gtol[MAX_HAS];	/* map global hba # to local # */
int	 blc_ltog[MAX_HAS];	/* map local hba # to global # */

/*
**  blc_hba_max is the maximum jobs that can be run concurrently
**  on the HBA.  The hardware supports much more, however, due
**  to the large size of the ccb structure, we can only fit 44
**  onto a single page, which is required to avoid corruption
**  from non-contiguous DMA.
*/
int blc_hba_max = 44;

/*
**  blc_lu_max is the maximum jobs per LUN concurrently.  It is
**  critical that this number be less than or equal to
**
**	((blc_hba_max / Number_Devices) - 1)
**
**  If the value is greater than or equal to this, a hang can
**  result on any given LUN.
*/
int blc_lu_max = 6;

/*
**  blc_lu_max_sched is the maximum scheduled jobs per LUN
**  concurrently.
*/
int blc_lu_max_sched = 2;

/*
**  blc_dma_max is the maximum number of concurrent DMA requests
**  per host adapter.
*/
int blc_dma_max = 15;

/*
**  blc_mbox_max is the maximum number of mailboxes per HBA.
**  The maximum value that the firmware can is 8192, but it
**  should set it <= blc_hba_max;
*/
int blc_mbox_max = 32;

struct	ver_no    blc_sdi_ver;

/* BLC_CNTLS is not set if autoconfiguration is supported.
 * The driver will set it by calling sdi_hba_autoconf().
 */
#ifndef BLC_CNTLS
#define BLC_CNTLS 1
#endif

#ifdef	BLC_CPUBIND

#define	BLC_VER		HBA_SVR4_2MP | HBA_IDATA_EXT

struct	hba_ext_info	blc_extinfo = {
	0, BLC_CPUBIND
};

#else

#define	BLC_VER		HBA_SVR4_2MP

#endif	/* BLC_CPUBIND */

struct	hba_idata_v4	_blcidata[]	= {
	{ BLC_VER, "(blc,1st) BLC SCSI",
	  7, 0, -1, 0, -1, 0 }
#ifdef BLC_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&blc_extinfo }
#endif
};

int	blc_cntls	= BLC_CNTLS;

/* Early versions of the BusLogic "C" board firmware incorrectly
 * reported data underrun/overrun errors.  Setting blc_fw_overrun
 * to non-zero avoids the bug.
 */
int blc_fw_overrun = 1;	
