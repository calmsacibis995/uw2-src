/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:svc/syms.s	1.12"
	.ident	"$Header: $"

include(../svc/asm.m4)
include(assym_include)

	.text
	.globl	stext
	.align	0x1000
	.set	stext, .

	.data
	.globl	sdata
	.align	0x1000
	.set	sdata, .

	.bss
	.globl	sbss
	.align	0x1000
	.set	sbss, .

	SYM_DEF(l, _A_KVPLOCAL)
	SYM_DEF(lm, _A_KVPLOCALMET)
	SYM_DEF(m, _A_KVMET)
	SYM_DEF(upointer, _A_KVPLOCAL+_A_L_USERP)
	SYM_DEF(uprocp, _A_KVPLOCAL+_A_L_CURPROCP)
	SYM_DEF(ueng, _A_KVUENG+_A_UAREA_OFFSET)
	SYM_DEF(uvwin, _A_KVUVWIN)
	SYM_DEF(plocal_intr_depth, _A_KVPLOCAL+_A_L_INTR_DEPTH)
	SYM_DEF(prmpt_state, _A_KVPLOCAL+_A_L_PRMPT_STATE)
	SYM_DEF(engine_evtflags, _A_KVPLOCAL+_A_L_EVENTFLAGS)
	SYM_DEF(using_fpu, _A_KVPLOCAL+_A_L_USINGFPU)
	SYM_DEF(myengnum, _A_KVPLOCAL+_A_L_ENG_NUM)
	SYM_DEF(cur_idtp, _A_KVPLOCAL+_A_L_IDTP)
	SYM_DEF(trap_err_code, _A_KVPLOCAL+_A_L_TRAP_ERR_CODE)
	SYM_DEF(microdata, _A_KVPLOCAL + _A_L_MICRODATA)
	SYM_DEF(mycpuid, _A_KVPLOCAL+_A_L_CPU_ID)
ifdef(`MERGE386', `
	SYM_DEF(vm86_idtp, _A_KVPLOCAL+_A_L_VM86_IDTP)
')

ifdef(`DEBUG', `
ifdef(`_LOCKTEST', `
	SYM_DEF(KVPLOCAL_L_HOLDFASTLOCK, _A_KVPLOCAL+_A_L_HOLDFASTLOCK)
	SYM_DEF(KVPLOCAL_L_FSPIN, _A_KVPLOCAL+_A_L_FSPIN)
')
	SYM_DEF(IS_DEBUG_BASE, 1)	/ indicate to the debugger that the
					/ base-kernel was compiled with DEBUG
')
