/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/dak/dak.cf/Space.c	1.7"
#ident	"$Header: $"

#include "sys/types.h"
#include "sys/sdi_edt.h"
#include "sys/sdi.h"
#include "sys/scsi.h"
#include "sys/conf.h"
#include "config.h"

#define DAK_SCSI_ID 7

#ifndef DAK_CNTLS
#define DAK_CNTLS 1
#endif

int	dak_gtol[SDI_MAX_HBAS];		/* xlate global hba# to loc */
int	dak_ltog[SDI_MAX_HBAS];		/* local hba# to global     */

int	dak_cntls	= DAK_CNTLS;	/* # of controllers to be determined by
				 * autoconfig.
				 */
struct	ver_no    dak_sdi_ver;

int	dak_do_conc_io[8] = {1,0};	/* do not do concurrent IO by default
				 * change to 1 to send simultaneous
				 * commands to the host adaptor hardware.
				 */

#ifdef	DAK_CPUBIND
				/* 
				 * Even though HBA_SVR4_2MP is set in
				 * the version number field, the real
				 * version number used in the idata
				 * structure is the one that gets changed 
				 * dynamically by the HBA_IDATA macro, 
				 * to the sdi.h PDI_VERSION the driver was 
				 * compiled with.
				 */
#define	DAK_VER		HBA_SVR4_2MP | HBA_IDATA_EXT
struct	hba_ext_info	dak_extinfo = {
	0, DAK_CPUBIND
};
#else
#define	DAK_VER		HBA_SVR4_2MP
#endif	/* DAK_CPUBIND */

/*
 * Change the name of the original idata array in space.c
 * We do this so we don't have to change all references
 * to the original name in the driver code.
 */
struct	hba_idata_v4	_dakidata[]	= {
	{ DAK_VER, "(dak,1) SCSI HBA",
	  DAK_SCSI_ID, 0, -1, 0, -1, 0 }
#ifdef	DAK_CPUBIND
	,
	{ HBA_EXT_INFO, (char *)&dak_extinfo }
#endif
};
