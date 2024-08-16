/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/cpqsc/cpqsc.cf/Space.c	1.14"
#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include "config.h"

int	cpqsc_lowat = 3;	/* LU queue low water mark	*/
int	cpqsc_hiwat = 16;	/* LU queue high water mark	*/

struct	ver_no    cpqsc_sdi_ver;


int             cpqsc_gtol[MAX_HAS];	/* global to local */
int             cpqsc_ltog[MAX_HAS];	/* local to global */

struct	hba_idata_v4	_cpqscidata[] = {
	{ 1, "(cpqsc,1) Compaq SCSI-II",
	  7, 0, -1 ,0, -1, 0, 0 }
};

int	cpqsc_cntls	= 1;

/*
 * Normally the system will look at all LUNs of the device.  If
 * cpqsc_lun_fix is set to 1 then only the first LUN of all devices
 * will ever be accessed.  This is here as a possible work around
 * for incompatiblity with devices that have only one LUN.
 */
int     cpqsc_lun_fix = 0;

/*
 * If cpqsc_stats is set to 0, then the statistics will NOT be gathered
 */
int	cpqsc_stats = 1;

/*
 * If cpqsc_verbose is set to 1, then the more obscure cmn_err 
 * messages will be printed to putbuf.
 */
int	cpqsc_verbose = 0;
