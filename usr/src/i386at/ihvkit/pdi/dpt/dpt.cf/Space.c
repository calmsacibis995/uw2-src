/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ihvkit:pdi/dpt/dpt.cf/Space.c	1.1"
#ident	"@(#)kern-i386at:io/hba/dpt/dpt.cf/Space.c	1.6"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/dpt.h>
#include <sys/conf.h>
#include "config.h"

int	 dpt_gtol[MAX_HAS];	/* map global hba # to local # */
int	 dpt_ltog[MAX_HAS];	/* map local hba # to global # */

int dpt_hba_max = 42;	/** Maximum jobs concurrently on HBA **/
			/** The hardware supports up to 64 concurrent jobs,      **/
			/** but due to the "large" size of the DPT CCB structure **/
			/** we can only fit 42 onto a single page, which is      **/
			/** reuired to avoid corruption from non-contiguous      **/
int dpt_lu_max = 6;	/** Maximum jobs per LUN concurrently, it is critcal that **/
			/** this number be less than or equal to:                 **/
			/**                                                       **/
			/**      ((dpt_hba_max / Number_Devices) - 1)             **/
			/**                                                       **/
			/** If the value is greater than or equal to this, a hang **/
			/** can result on any given LUN.                          **/

int dpt_lu_max_sched = 2; /* Maximum scheduled jobs per LUN concurrently */

int	dpt_sc_hacnt = DPT_CNTLS;	/* Total number of controllers	*/

struct	ver_no    dpt_sdi_ver;

int	dpt_ctlr_id = SCSI_ID;	/* HBA's SCSI id number		*/

#ifdef	DPT_CPUBIND

#define	DPT_ID_DIM	DPT_CNTLS+1
#define	DPT_VER		HBA_SVR4_2MP | HBA_IDATA_EXT

struct	hba_ext_info	dpt_extinfo = {
	0, DPT_CPUBIND
};

#else

#define	DPT_ID_DIM	DPT_CNTLS
#define	DPT_VER		1

#endif	/* DPT_CPUBIND */

struct	hba_idata	dptidata[DPT_ID_DIM]	= {
#ifdef	DPT_0
	{ DPT_VER, "(dpt,1st) SCSI HBA",
	  7, DPT_0_SIOA, DPT_0_CHAN, DPT_0_VECT, DPT_0, 0 }
#endif
#ifdef	DPT_1
	,
	{ DPT_VER, "(dpt,2nd) SCSI HBA",
	  7, DPT_1_SIOA, DPT_1_CHAN, DPT_1_VECT, DPT_1, 0 }
#endif
#ifdef	DPT_2
	,
	{ DPT_VER, "(dpt,3rd) SCSI HBA",
	  7, DPT_2_SIOA, DPT_2_CHAN, DPT_2_VECT, DPT_2, 0 }
#endif
#ifdef	DPT_3
	,
	{ DPT_VER, "(dpt,4th) SCSI HBA",
	  7, DPT_3_SIOA, DPT_3_CHAN, DPT_3_VECT, DPT_3, 0 }
#endif
#ifdef	DPT_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&dpt_extinfo }
#endif
};

int	dpt_cntls	= DPT_CNTLS;

int	dpt_flush_time_out = 30000;	/** This is the maximum number of msec's    **/
					/** the driver will wait for the controller **/
					/** to flush it's cache for a given LUN.    **/