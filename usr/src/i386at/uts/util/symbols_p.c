/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:util/symbols_p.c	1.39"
#ident	"$Header: $"

/*
 * Generate symbols for use by assembly language files in kernel.
 *
 * This file is compiled using "-S"; "symbols.awk" walks over the assembly
 * file and extracts the symbols.
 */

#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <proc/disp_p.h>
#include <svc/bootinfo.h>
#include <svc/intr.h>
#include <svc/pic.h>
#include <svc/trap.h>
#include <util/ipl.h>
#include <util/param.h>
#include <util/plocal.h>

#define	offsetof(x, y)	((int)&((x *)0)->y)
#define	OFFSET(s, st, m) \
	size_t __SYMBOL___A_##s = offsetof(st, m)

#define	DEFINE(s, e) \
	size_t __SYMBOL___A_##s = (size_t)(e)

/*
 * Misc kernel virtual addresses and constants.
 */

DEFINE(BOOTINFO, KVBOOTINFO + (BOOTINFO_LOC & MMU_PAGEOFFSET));
DEFINE(KVPAGE0, KVPAGE0);

DEFINE(BKI_MAGIC, BKI_MAGIC);

/*
 * Interrupt Priority Levels
 */

DEFINE(PL0, PL0);
DEFINE(PL1, PL1);
DEFINE(PL4, PL4);
DEFINE(PL5, PL5);
DEFINE(PL6, PL6);
DEFINE(PL7, PL7);
DEFINE(PLTTY, PLTTY);
DEFINE(INVPL, INVPL);
DEFINE(PLHI, PLHI);
DEFINE(PLBASE, PLBASE);
DEFINE(PLTIMEOUT, PLTIMEOUT);
DEFINE(PLDISK, PLDISK);
DEFINE(PLSTR, PLSTR);
DEFINE(PLMAX, PLMAX);
DEFINE(PLXCALL, PLXCALL);

/*
 * PIC related constants and offsets
 */
DEFINE(PIC_NSEOI, PIC_NSEOI);

OFFSET(IRQ_CMDPORT, struct irqtab, irq_cmdport);
OFFSET(IRQ_FLAGS, struct irqtab, irq_flags);

DEFINE(IRQ_CHKSPUR, IRQ_CHKSPUR);
DEFINE(IRQ_ONSLAVE, IRQ_ONSLAVE);

/*
 * Processor local fields ("l.").
 */

OFFSET(L_IPL, struct plocal, prmpt_state.s_prmpt_state.s_ipl);
OFFSET(L_NOPRMPT, struct plocal, prmpt_state.s_prmpt_state.s_noprmpt);
OFFSET(L_PICIPL, struct plocal, picipl);
OFFSET(L_PLSTI, struct plocal, plsti);
OFFSET(L_SHADOW_IF, struct plocal, shadow_if);
OFFSET(L_XCLOCK_PENDING, struct plocal, xclock_pending);
OFFSET(L_PRF_PENDING, struct plocal, prf_pending);
OFFSET(L_FPU_EXTERNAL, struct plocal, fpu_external);


/*
 * Misc defines.
 */

DEFINE(EVT_SOFTINTMASK, EVT_SOFTINTMASK);
