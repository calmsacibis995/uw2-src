/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/gsd/gsddrv.c	1.2"
#ident	"$Header: $"

/*
 * gsddrv.c, Graphics System Driver Module driver code.
 */

#include <util/types.h>
#include <io/ldterm/euc.h>
#include <io/ldterm/eucioctl.h>
#include <io/ws/ws.h>
#include <io/gsd/gsd.h>
#include <util/cmn_err.h>

extern wstation_t Kdws;

/* int	channel_ref_count = 0;	/* number of active graphical channels */

char	gsdid_string[] = "Graphics System Driver Version 1.0";
char	gsdcopyright[] = "Copyright (c) 1994 Novell, Inc.";

/*
 * int gsdinit(void) - called from gsd_load()
 *	Initializes gsd.
 *
 * Calling/Exit status:
 *
 */
int
gsdinit(void)
{
	cmn_err(CE_CONT, "%s\n%s\n\n", gsdid_string, gsdcopyright);

	Kdws.w_consops->cn_gcl_norm = gcl_norm;
	Kdws.w_consops->cn_gcl_handler = gcl_handler;
	Kdws.w_consops->cn_gdv_scrxfer = gdv_scrxfer;
	Kdws.w_consops->cn_gs_chinit = gs_chinit;
	Kdws.w_consops->cn_gs_alloc = gs_alloc;
	Kdws.w_consops->cn_gs_free = gs_free;
	Kdws.w_consops->cn_gs_seteuc = gs_seteuc;
	Kdws.w_consops->cn_gs_ansi_cntl = gs_ansi_cntl;

	gs_init_flg = 1;
}

