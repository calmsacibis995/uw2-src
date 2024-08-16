/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/dpt/dpt.cf/Space.c	1.15"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/dpt.h>
#include <sys/conf.h>
#include "config.h"

int	 dpt_gtol[SDI_MAX_HBAS];	/* map global hba # to local # */
int	 dpt_ltog[SDI_MAX_HBAS];	/* map local hba # to global # */

int dpt_hba_max = 64;	/* Maximum jobs concurrently on HBA 
			 * The hardware supports up to 64 concurrent jobs
			 */

int dpt_lu_max = 7;	/** Maximum jobs per LUN concurrently, it is critcal that **/
			/** this number be less than or equal to:                 **/
			/**                                                       **/
			/**      ((dpt_hba_max / Number_Devices) - 1)             **/
			/**                                                       **/
			/** If the value is greater than or equal to this, a hang **/
			/** can result on any given LUN.                          **/

int dpt_enable_scheduling = 0;	/* Enable only if your disks do not support 
				 * Command Queueing.
				 */
int dpt_lu_max_sched = 2; /* Maximum scheduled jobs per LUN concurrently */

struct	ver_no    dpt_sdi_ver;

int	dpt_ctlr_id = SCSI_ID;	/* HBA's SCSI id number		*/

#ifdef	DPT_CPUBIND

#define	DPT_VER		HBA_SVR4_2MP | HBA_IDATA_EXT

struct	hba_ext_info	dpt_extinfo = {
	0, DPT_CPUBIND
};

#else

#define	DPT_VER		HBA_SVR4_2MP

#endif	/* DPT_CPUBIND */

/*
 * Change the name of the original idata array in space.c.
 * We do this so we don't have to change all references
 * to the original name in the driver code.
 */

struct	hba_idata_v4	_dptidata[]	= {
	{ DPT_VER, "(dpt,1) DPT SCSI",
	  7, 0, -1, 0, -1, 0 }
#ifdef	DPT_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&dpt_extinfo }
#endif
};

int	dpt_cntls	= 1;

int	dpt_flush_time_out = 30000;	/** This is the maximum number of msec's    **/
					/** the driver will wait for the controller **/
					/** to flush it's cache for a given LUN.    **/
