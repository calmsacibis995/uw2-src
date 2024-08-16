/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:util/symbols_p.c	1.38"
#ident	"$Header: $"

/*
 * Generate symbols for use by assembly language files in kernel.
 *
 * This file is compiled using "-S"; "symbols.awk" walks over the assembly
 * file and extracts the symbols.
 */

#include <io/cfg.h>
#include <io/slic.h>
#include <io/SGSproc.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <proc/lwp.h>
#include <util/engine.h>
#include <util/ipl.h>
#include <util/plocal.h>

#define	offsetof(x, y)	((int)&((x *)0)->y)
#define	OFFSET(s, st, m) \
	size_t __SYMBOL___A_##s = offsetof(st, m)

#define	DEFINE(s, e) \
	size_t __SYMBOL___A_##s = (size_t)(e)

/*
 * Physical addresses of various hardware components.
 */

DEFINE(PHYS_MBAD, PHYS_MBAD);
DEFINE(PHYS_SSMVME, PHYS_SSMVME);
DEFINE(PHYS_SLIC, PHYS_SLIC);
DEFINE(PHYS_ETC, PHYS_ETC);
DEFINE(PHYS_LED, PHYS_LED);
DEFINE(PHYS_SYNC_POINT, PHYS_SYNC_POINT);

/*
 * Kernel virtual addresses of various hardware components.
 */

DEFINE(KVSLIC, KVSLIC);
DEFINE(KVETC, KVETC);
DEFINE(KVLED, KVLED);
DEFINE(KVSYNC_POINT, KVSYNC_POINT);

/*
 * Misc kernel virtual addresses and constants.
 */

DEFINE(KVSBASE, KVSBASE);

/*
 * SLIC masks and PL values for *spl functions.
 */

DEFINE(PL0, PL0);
DEFINE(PL1, PL1);
DEFINE(PL2, PL2);
DEFINE(PL3, PL3);
DEFINE(PL4, PL4);
DEFINE(PL5, PL5);
DEFINE(PL6, PL6);
DEFINE(PL7, PL7);
DEFINE(PL8, PL8);
DEFINE(PLBASE, PLBASE);
DEFINE(PLHI, PLHI);
DEFINE(PLXCALL, PLXCALL);
DEFINE(PLVM, PLVM);
DEFINE(PLTTY, PLTTY);
DEFINE(PLDISK, PLDISK);
DEFINE(PLSTR, PLSTR);
DEFINE(PLTIMEOUT, PLTIMEOUT);
DEFINE(INVPL, INVPL);

/*
 * Offsets for interrupt vector table.
 */

OFFSET(BH_HDLRTAB, struct bin_header, bh_hdlrtab);

/*
 * Processor local fields ("l.").
 */

OFFSET(L_SLIC_DELAY, struct plocal, slic_delay);
OFFSET(L_SLIC_LONG_DELAY, struct plocal, slic_long_delay);

/*
 * Slic offsets
 */

OFFSET(SL_CMD_STAT, struct cpuslic, sl_cmd_stat);
OFFSET(SL_DEST, struct cpuslic, sl_dest);
OFFSET(SL_SMESSAGE, struct cpuslic, sl_smessage);
OFFSET(SL_B0INT, struct cpuslic, sl_b0int);
OFFSET(SL_BININT, struct cpuslic, sl_binint);
OFFSET(SL_NMIINT, struct cpuslic, sl_nmiint);
OFFSET(SL_LMASK, struct cpuslic, sl_lmask);
OFFSET(SL_GMASK, struct cpuslic, sl_gmask);
OFFSET(SL_IPL, struct cpuslic, sl_ipl);
OFFSET(SL_ICTL, struct cpuslic, sl_ictl);
OFFSET(SL_TCONT, struct cpuslic, sl_tcont);
OFFSET(SL_TRV, struct cpuslic, sl_trv);
OFFSET(SL_TCTL, struct cpuslic, sl_tctl);
OFFSET(SL_SDR, struct cpuslic, sl_sdr);
OFFSET(SL_PROCGRP, struct cpuslic, sl_procgrp);
OFFSET(SL_PROCID, struct cpuslic, sl_procid);
OFFSET(SL_CRL, struct cpuslic, sl_crl);

/*
 * SLIC bits
 */
DEFINE(SL_HARDINT, SL_HARDINT);

/*
 * Misc defines.
 */

DEFINE(CD_LOC, (int)CD_LOC);
OFFSET(E_SLICADDR, struct engine, e_slicaddr);
OFFSET(L_PRI, lwp_t, l_pri);
