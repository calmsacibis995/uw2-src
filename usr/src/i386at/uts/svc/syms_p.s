/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386at:svc/syms_p.s	1.7"
	.ident	"$Header: $"

/ Platform-specific symbol definitions; concatenated onto syms.s

	SYM_DEF(bootinfo, _A_BOOTINFO)
	SYM_DEF(plsti, _A_KVPLOCAL + _A_L_PLSTI)
	SYM_DEF(shadow_if, _A_KVPLOCAL + _A_L_SHADOW_IF)
	SYM_DEF(nmi_handler, _A_KVPLOCAL + _A_L_NMI_HANDLER)
	SYM_DEF(fpu_external, _A_KVPLOCAL + _A_L_FPU_EXTERNAL)

/ On behalf of PSMs which need plocal variables:

	SYM_DEF(ipl, _A_KVPLOCAL + _A_L_IPL)
	SYM_DEF(noprmpt, _A_KVPLOCAL + _A_L_NOPRMPT);
	SYM_DEF(prmpt_count, _A_KVPLOCAL + _A_L_PRMPT_STATE);
	SYM_DEF(picipl, _A_KVPLOCAL + _A_L_PICIPL)
	SYM_DEF(intr_count, _A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR)
	SYM_DEF(xclock_pending, _A_KVPLOCAL + _A_L_XCLOCK_PENDING)
	SYM_DEF(prf_pending, _A_KVPLOCAL + _A_L_PRF_PENDING)
	SYM_DEF(kvpage0, _A_KVPAGE0)
