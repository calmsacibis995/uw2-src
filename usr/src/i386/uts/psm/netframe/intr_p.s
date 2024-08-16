/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:psm/netframe/intr_p.s	1.1"
	.ident	"$Header: $"
	.file	"svc/intr_p.s"

/
/ Machine dependent low-level kernel entry points for interrupt
/ and trap handling.
/
#include "svc/nf_pic.h"
include(../svc/asm.m4)
include(assym_include)
include(../svc/intr.m4)
include(../util/debug.m4)

FILE(`intr_p.s')

/
/ MACRO
/ PL_CHECK
/
/ Description:
/	If DEBUG is defined, PL_CHECK checks that priority level of the
/	interrupt context and the interrupted context meet certain constraints.
/	If DEBUG is not defined, this is a no-op.
/
/ Remarks:
/	Invoked during interrupt return sequence, with old pl on top of stack.
/
define(`PL_CHECK',`
ifdef(`DEBUG',`
	LABEL(`usermode',`less',`done')
/
/ The following should be true regardless of whether interrupted context
/ was kernel or user:
/
	ASSERT(ul,`l+_A_L_IPL',!=,`$_A_PLBASE')	/ current pl != PLBASE
	ASSERT(ul,`l+_A_L_IPL',<=,`$_A_PLHI')	/ current pl <= PLHI
/
/ See if interrupt context was user or kernel
/
	IF_USERMODE(_A_INTR_SP_IP(%esp), usermode)
/
/ If interrupted context was kernel, then either:
/	(1) (return pl == current pl) && (current pl == PLHI)
/ or
/	(2) return pl < current pl
/
	movl	(%esp), %eax			/ %eax = return pl
	cmpl	$_A_PLHI, %eax			/ if return pl == PLHI &&
	jne	less
	cmpl	l+_A_L_IPL, %eax		/ return pl == current pl
	je	done				/	=> OK
less:
	ASSERT(ul,%eax,<,`l+_A_L_IPL')		/ return pl < current pl
	jmp	done
/
/ If interrupted context was user:
/
usermode:
	ASSERT(ul,(%esp),==,`$_A_PLBASE')	/ return pl == PLBASE
done:
	popdef(`kernel',`less',`done')
')
')

ENTRY(devint0)
	INTR_ENTER(0)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint1)
	INTR_ENTER(1)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint2)
	INTR_ENTER(2)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint3)
	INTR_ENTER(3)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint4)
	INTR_ENTER(4)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint5)
	INTR_ENTER(5)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint6)
	INTR_ENTER(6)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint7)
	INTR_ENTER(7)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint9)
	INTR_ENTER(9)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint10)
	INTR_ENTER(10)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint11)
	INTR_ENTER(11)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint12)
	INTR_ENTER(12)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint13)
	INTR_ENTER(13)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint14)
	INTR_ENTER(14)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint15)
	INTR_ENTER(15)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint16)
	INTR_ENTER(16)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint17)
	INTR_ENTER(17)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint18)
	INTR_ENTER(18)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint19)
	INTR_ENTER(19)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint20)
	INTR_ENTER(20)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint21)
	INTR_ENTER(21)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint22)
	INTR_ENTER(22)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint23)
	INTR_ENTER(23)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint24)
	INTR_ENTER(24)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint25)
	INTR_ENTER(25)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint26)
	INTR_ENTER(26)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint27)
	INTR_ENTER(27)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint28)
	INTR_ENTER(28)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint29)
	INTR_ENTER(29)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint30)
	INTR_ENTER(30)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint31)
	INTR_ENTER(31)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(ivct60)
	INTR_ENTER(0x60)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct61)
	INTR_ENTER(0x61)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct62)
	INTR_ENTER(0x62)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct63)
	INTR_ENTER(0x63)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct64)
	INTR_ENTER(0x64)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct65)
	INTR_ENTER(0x65)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct66)
	INTR_ENTER(0x66)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct67)
	INTR_ENTER(0x67)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct68)
	INTR_ENTER(0x68)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct69)
	INTR_ENTER(0x69)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct6A)
	INTR_ENTER(0x6A)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct6B)
	INTR_ENTER(0x6B)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct6C)
	INTR_ENTER(0x6C)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct6D)
	INTR_ENTER(0x6D)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct6E)
	INTR_ENTER(0x6E)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct6F)
	INTR_ENTER(0x6F)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct70)
	INTR_ENTER(0x70)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct71)
	INTR_ENTER(0x71)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct72)
	INTR_ENTER(0x72)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct73)
	INTR_ENTER(0x73)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct74)
	INTR_ENTER(0x74)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct75)
	INTR_ENTER(0x75)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct76)
	INTR_ENTER(0x76)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct77)
	INTR_ENTER(0x77)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct78)
	INTR_ENTER(0x78)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct79)
	INTR_ENTER(0x79)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct7A)
	INTR_ENTER(0x7A)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct7B)
	INTR_ENTER(0x7B)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct7C)
	INTR_ENTER(0x7C)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct7D)
	INTR_ENTER(0x7D)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct7E)
	INTR_ENTER(0x7E)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct7F)
	INTR_ENTER(0x7F)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct80)
	INTR_ENTER(0x80)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct81)
	INTR_ENTER(0x81)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct82)
	INTR_ENTER(0x82)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct83)
	INTR_ENTER(0x83)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct84)
	INTR_ENTER(0x84)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct85)
	INTR_ENTER(0x85)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct86)
	INTR_ENTER(0x86)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct87)
	INTR_ENTER(0x87)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct88)
	INTR_ENTER(0x88)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct89)
	INTR_ENTER(0x89)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct8A)
	INTR_ENTER(0x8A)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct8B)
	INTR_ENTER(0x8B)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct8C)
	INTR_ENTER(0x8C)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct8D)
	INTR_ENTER(0x8D)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct8E)
	INTR_ENTER(0x8E)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct8F)
	INTR_ENTER(0x8F)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct90)
	INTR_ENTER(0x90)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct91)
	INTR_ENTER(0x91)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct92)
	INTR_ENTER(0x92)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct93)
	INTR_ENTER(0x93)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct94)
	INTR_ENTER(0x94)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct95)
	INTR_ENTER(0x95)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct96)
	INTR_ENTER(0x96)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct97)
	INTR_ENTER(0x97)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct98)
	INTR_ENTER(0x98)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct99)
	INTR_ENTER(0x99)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct9A)
	INTR_ENTER(0x9A)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct9B)
	INTR_ENTER(0x9B)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct9C)
	INTR_ENTER(0x9C)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct9D)
	INTR_ENTER(0x9D)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct9E)
	INTR_ENTER(0x9E)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivct9F)
	INTR_ENTER(0x9F)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctA0)
	INTR_ENTER(0xA0)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctA1)
	INTR_ENTER(0xA1)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctA2)
	INTR_ENTER(0xA2)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctA3)
	INTR_ENTER(0xA3)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctA4)
	INTR_ENTER(0xA4)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctA5)
	INTR_ENTER(0xA5)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctA6)
	INTR_ENTER(0xA6)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctA7)
	INTR_ENTER(0xA7)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctA8)
	INTR_ENTER(0xA8)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctA9)
	INTR_ENTER(0xA9)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctAA)
	INTR_ENTER(0xAA)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctAB)
	INTR_ENTER(0xAB)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctAC)
	INTR_ENTER(0xAC)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctAD)
	INTR_ENTER(0xAD)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctAE)
	INTR_ENTER(0xAE)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctAF)
	INTR_ENTER(0xAF)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctB0)
	INTR_ENTER(0xB0)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctB1)
	INTR_ENTER(0xB1)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctB2)
	INTR_ENTER(0xB2)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctB3)
	INTR_ENTER(0xB3)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctB4)
	INTR_ENTER(0xB4)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctB5)
	INTR_ENTER(0xB5)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctB6)
	INTR_ENTER(0xB6)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctB7)
	INTR_ENTER(0xB7)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctB8)
	INTR_ENTER(0xB8)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctB9)
	INTR_ENTER(0xB9)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctBA)
	INTR_ENTER(0xBA)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctBB)
	INTR_ENTER(0xBB)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctBC)
	INTR_ENTER(0xBC)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctBD)
	INTR_ENTER(0xBD)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctBE)
	INTR_ENTER(0xBE)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctBF)
	INTR_ENTER(0xBF)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctC0)
	INTR_ENTER(0xC0)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctC1)
	INTR_ENTER(0xC1)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctC2)
	INTR_ENTER(0xC2)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctC3)
	INTR_ENTER(0xC3)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctC4)
	INTR_ENTER(0xC4)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctC5)
	INTR_ENTER(0xC5)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctC6)
	INTR_ENTER(0xC6)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctC7)
	INTR_ENTER(0xC7)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctC8)
	INTR_ENTER(0xC8)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctC9)
	INTR_ENTER(0xC9)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctCA)
	INTR_ENTER(0xCA)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctCB)
	INTR_ENTER(0xCB)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctCC)
	INTR_ENTER(0xCC)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctCD)
	INTR_ENTER(0xCD)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctCE)
	INTR_ENTER(0xCE)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctCF)
	INTR_ENTER(0xCF)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctD0)
	INTR_ENTER(0xD0)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctD1)
	INTR_ENTER(0xD1)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctD2)
	INTR_ENTER(0xD2)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctD3)
	INTR_ENTER(0xD3)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctD4)
	INTR_ENTER(0xD4)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctD5)
	INTR_ENTER(0xD5)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctD6)
	INTR_ENTER(0xD6)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctD7)
	INTR_ENTER(0xD7)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctD8)
	INTR_ENTER(0xD8)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctD9)
	INTR_ENTER(0xD9)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctDA)
	INTR_ENTER(0xDA)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctDB)
	INTR_ENTER(0xDB)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctDC)
	INTR_ENTER(0xDC)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctDD)
	INTR_ENTER(0xDD)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctDE)
	INTR_ENTER(0xDE)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctDF)
	INTR_ENTER(0xDF)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctE0)
	INTR_ENTER(0xE0)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctE1)
	INTR_ENTER(0xE1)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctE2)
	INTR_ENTER(0xE2)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctE3)
	INTR_ENTER(0xE3)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctE4)
	INTR_ENTER(0xE4)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctE5)
	INTR_ENTER(0xE5)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctE6)
	INTR_ENTER(0xE6)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctE7)
	INTR_ENTER(0xE7)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctE8)
	INTR_ENTER(0xE8)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctE9)
	INTR_ENTER(0xE9)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctEA)
	INTR_ENTER(0xEA)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctEB)
	INTR_ENTER(0xEB)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctEC)
	INTR_ENTER(0xEC)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctED)
	INTR_ENTER(0xED)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctEE)
	INTR_ENTER(0xEE)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctEF)
	INTR_ENTER(0xEF)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctF0)
	INTR_ENTER(0xF0)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctF1)
	INTR_ENTER(0xF1)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctF2)
	INTR_ENTER(0xF2)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(ivctF3)
	INTR_ENTER(0xF3)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.apic_common			/ common handling

ENTRY(apic_ivctSTRAY)
	INTR_ENTER(0xFF)
	/ check if returning to user mode
	IF_USERMODE(_A_INTR_SP_IP-4(%esp), .uintret)
	jmp	.kintret		/ no, do kernel iret

ENTRY(ivctSTRAY)
	INTR_ENTER(0x1F)
	incl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / count the device interrupt
	jmp	.dev_common			/ common handling

ENTRY(devint8)
	INTR_ENTER(8)				/ Don't count clock ticks
	cmpl	$0, prfstat			/ Is profiler on?
	jne	.prf				/ Yes, go call it
						/ Otherwise fall thru
.align	4
.dev_common:
	pushl	l+_A_L_IPL		/ save current level
	pushl	%ecx			/ push interrupt number arg
	ASSERT(ul,`l+_A_L_IPL',<=,`$_A_PLHI')	/ old pl <= PLHI

	/ check to see if this is a spurious interrupt
	testb	$_A_IRQ_CHKSPUR, irqtab+_A_IRQ_FLAGS(,%ecx,4)
	jne	.check_spurious

	/ a real (non-spurious) hardware interrupt
	/	(1) set ipl and PIC masks to service level of interrupt
	/	(2) acknowledge interrupt
	/	(3) see if interrupt should be deferred
.real_int:
	/ get priority level of interrupt and set PIC masks to it
	ASSERT(ul,%ecx,<=,nintr)		/ %ecx has interrupt number
	movl	svcpri(,%ecx,4), %eax	/ %eax = service level
	ASSERT(ul,%eax,<=,`$_A_PLHI')	/ service level <= PLHI
	ASSERT(ul,%eax,>,`$_A_PLBASE')	/ service level > PLBASE
	movl	%eax, l+_A_L_IPL	/ ipl = service level
	call	setpicmasks		/ load pic masks for new level
	movl	(%esp), %ecx		/ restore %ecx (= interrupt number)

	/ acknowledge interrupt by sending EOI to master PIC and, if necessary,
	/ 	to slave as well
	movb	$_A_PIC_NSEOI, %al	/ non-specific EOI
	movl	cmdport, %edx		/ master pic command port addr
	outb	(%dx)			/ send EOI to master pic
	testb	$_A_IRQ_ONSLAVE, irqtab+_A_IRQ_FLAGS(,%ecx,4)
					/ if not on slave
	jz	.eoi_done		/   .. skip slave
	/ send EOI to slave PIC
	movl	irqtab+_A_IRQ_CMDPORT(,%ecx,4), %edx
					/ slave pic command port addr
	outb	(%dx)			/ send EOI to slave pic
	jmp	.eoi_done

.apic_common:
        movl    apic_to_pic(,%ecx,4), %ecx / normalize to pic
	cmpl	$0, %ecx			/ Is it clock?
	jne	.apic_int
	decl	_A_KVPLOCALMET+_A_LM_CNT+_A_V_INTR  / don't count lock
	cmpl	$0, prfstat			/ Is profiler on?
	jne	.apic_prf				/ Yes, go call it

.apic_int:
	pushl	l+_A_L_IPL		/ save current level
	pushl	%ecx			/ push interrupt number arg
	ASSERT(ul,`l+_A_L_IPL',<=,`$_A_PLHI')	/ old pl <= PLHI

	/ set current pl to service pl for interrupt
        movl    svcpri(,%ecx,4), %eax
	ASSERT(ul,%eax,<=,`$_A_PLHI')	/ service level <= PLHI
	ASSERT(ul,%eax,>,`$_A_PLBASE')	/ service level > PLBASE
	movl	%eax, l+_A_L_IPL	/ ipl = service level
	call	setpicmasks		/ load pic masks for new level
	movl	eoi_reg_addr,%ecx
	movl	$0, (%ecx)		/ sending EOI to APIC
	movl	(%esp), %ecx		/ restore %ecx (= interrupt number)

.align	4
.eoi_done:

ifdef(`UNIPROC',`
	/ see if interrupt should be deferred
	movzbl	intpri(%ecx), %eax	/ %eax = interrupt pl
	cmpl	4(%esp), %eax		/ if (interrupt pl <= old pl) then ...
	jbe	.picdefer		/   .. defer the interrupt
')

	/ Handle the interrupt.  This code is reached via two paths:
	/  (1) Falls through from .dev_common for non-deferred interrupts
	/  (2) Branched to from deferred_int to handle deferred interrupts
.int_handle:
	sti			/ enable interrupts at processor
	cld			/ clear direction flag
	BEGIN_INT(ivect(,%ecx,4), _A_INTR_SP_IP+4(%esp), %eax)
				/ keep stats if enabled and increment
				/	interrupt depth counter
	call	*ivect(,%ecx,4)	/ call registered handler
	addl	$4, %esp	/ clear int number off stack upon return
	END_INT			/ keep stats if enabled and decrement
				/	interrupt depth counter
	PL_CHECK		/ check constraints on current and return pl

	popl	%eax		/ get saved level from stack
	cli			/ disable interrupts at the processor
	movl	%eax, l+_A_L_IPL/ set new ipl

ifdef(`UNIPROC',`
	/ Check to see if there are any deferred interrupts whose priority 
	/ is higher than the priority level of the interrupted context.
	/
	/ This code is reached via two paths:
	/  (1) Falls through from above for deferred and non-deferred interrupts
	/  (2) Branched to by softint if picipl > ipl
.check_defer:
	movl	picdeferndx, %edx	/ %edx = deferred stack index
	movl	picdeferred(,%edx,4), %ecx
					/ %ecx = topmost deferred interrupt
	movzbl	intpri(%ecx), %edx	/ %edx = interrupt of topmost def int
	cmpl	ipl, %edx		/ if deferred ipl > current ipl ...
	ja	deferred_int		/   ... undefer the interrupt
	/ Not handling any deferred interrupts, so just set pic mask to the
	/	return pl (which is still in %eax)
')

	call	setpicmasks

	/ Check for events.  softint has joined by now, and so has .dev_xcall
.check_event:
	cmpl	$_A_PLBASE, l+_A_L_IPL	/ if ipl is not PLBASE ...
	jne	.kintret		/   .. must be returning to kernel
					/	without preemption

	/ if there are soft interrupts pending, then handle them
	testl	$_A_EVT_SOFTINTMASK, l+_A_L_EVENTFLAGS
	jnz	.dosoftint

	/ if returning to user mode, check user preemption
	IF_USERMODE(_A_INTR_SP_IP-4(%esp), .ucheck_preempt)

	/ check for pending kernel preemptions.  If any are pending, then
	/	check to see if preempt state allows preemptions
	testl	$_A_EVT_KPRUNRUN, l+_A_L_EVENTFLAGS
	jnz	.kcheck_preempt_state

	/ return from interrupt to kernel mode
.kintret:
	INTR_RESTORE_REGS		/ restore scratch registers
	RESTORE_DSEGREGS
	iret				/ return from interrupt

/
/ Check preemption state to see if a pending kernel preemption should be done.
/	The kernel is not preemptable under the following conditions:
/		a) The engine is servicing an interrupt.
/		b) The engine has been marked non-preemptable (prmpt_state > 0).
/		c) The engine is not at base ipl.
/
/	Since all interrupts are serviced at an ipl greater than base ipl,
/	the check for condition (a) is superfluous.  Further, since we
/	already know the system is at PLBASE because of previous checking
/	(see .check_event above), the check for condition (c) is superfluous.
/	Thus, the only check needed is to see if the engine has been marked
/	non-preemptable.
/
/ If the check for condition (b) passes, then go off to trap_sched to
/	do the preemption.
/
.align	8
.kcheck_preempt_state:
	cmpl	$0, prmpt_state
	jne	.kintret
	
	/ If the context is being preempted, simply return. If this is not
	/ done, we can blow the kernel stack.
	movl	upointer,%eax
	movl	_A_U_LWPP(%eax),%eax
	cmpl	$_A_B_TRUE, _A_LWP_BPT(%eax)
	jne	.kintret
	/ Check if preemption is enabled in kernel
	cmpl 	$0,prmpt_enable
	je	.kintret
	/ Since we are going to preempt ourselves, clear the kernel
	/ preemption pending flag and mark the context as being in the
	/ process of being preempted.
	andl	$~_A_EVT_KPRUNRUN, l+_A_L_EVENTFLAGS
	movl	$_A_B_TRUE, _A_LWP_BPT(%eax) / %eax has the LWP pointer
	jmp	trap_sched	/ Force a preemption.

.align	8
.ucheck_preempt:
	/ check for pending user preemptions
	testl	$_A_EVT_RUNRUN, l+_A_L_EVENTFLAGS
	jnz	trap_sched

	/ Check for trap event processing
	movl	upointer, %eax
	movl	_A_U_LWPP(%eax), %eax
	testl	$_A_TRAPEXIT_FLAGS, _A_LWP_TRAPEVF(%eax)
	jnz	.utrapevt
	/ return from interrupt to user mode
.uintret:
	INTR_RESTORE_REGS		/ restore scratch registers
	RESTORE_UDSEGREGS
	USER_IRET			/ return from interrupt

	/ handle trap event processing
.align	8
.utrapevt:
	INTR_TO_TRAP_REGS	/ Convert interrupt frame to trap frame
	movl	%eax,%ebx	/ Save lwpp in %ebx
	jmp	int_trapret	/ Go handle trap events

ifdef(`UNIPROC',`
/
/ defer the current interrupt.  If an interrupt is to be deferred, then
/ the system was running above PLBASE at interrupt time, which means it
/ was running in the kernel; thus we just jump to .kintret at the bottom
/ here.
/
	.align	8
.picdefer:
	incl	picdeferndx		/ picdeferred[++picdeferndx] = intnum
	movl	picdeferndx, %eax
	movl	%ecx, picdeferred(,%eax,4)
	addl	$4, %esp		/ clear interrupt number off the stack
	popl	l+_A_L_IPL		/ restore the previous ipl
	jmp	.kintret
')

/
/ Check for spurious interrupts.  Assumes PIC is in READ ISR mode.
/
	.align	8
.check_spurious:
	movl	irqtab+_A_IRQ_CMDPORT(,%ecx,4), %edx
	inb	(%dx)			/ read ISR from PIC
	testb	$0x80, %al		/ check IS bit for interrupt 7
	jnz	.real_int		/ if set, it's a real interrupt
	testb	$_A_IRQ_ONSLAVE, irqtab+_A_IRQ_FLAGS(,%ecx,4)
					/ if interrupt is not on slave PIC
	jz	.spurious		/   .. don't clear master
	movb	$_A_PIC_NSEOI, %al	/ send EOI to master PIC
	movl	cmdport, %edx		/ master PIC command port addr
	outb	(%dx)
	.align	4
.spurious:
	addl	$8, %esp
	/ check if returning to user mode
	IF_USERMODE(_A_INTR_SP_IP-4(%esp), .uintret)
	jmp	.kintret		/ no, do kernel iret

ifdef(`UNIPROC',`
/
/ deferred_int.
/	Entry point for handling deferred interrupts.  Interrupt
/	code is passed in %ecx.
/
/ Calling/Exit State:
/	Trap frame is on top of stack.
/	Interrupt code is passed in %ecx.
/	Deferred interrupt is still on top of deferred stack.
/
/ Remarks:
/	Pushes ipl and interrupt number arg
/	Decrement picdeferndx to pop the deferred interrupt stack
/	Set ipl and picipl (call setpicmasks if necessary)
/	Branch to .int_handle
/
ENTRY(deferred_int)
	pushl	l+_A_L_IPL		/ save current level
	pushl	%ecx			/ push interrupt number arg
	decl	picdeferndx		/ pop the deferred interrupt
	ASSERT(sl,picdeferndx,>=,$0);
	movl	svcpri(,%ecx,4), %eax	/ %eax = service priority
	movl	%eax, l+_A_L_IPL	/ set new pl
	cmpl	%eax, l+_A_L_PICIPL	/ if (pl == picipl) then
	je	.int_handle		/	handle the interrupt
	call	setpicmasks		/ else	set PIC to interrupt level
	movl	(%esp), %ecx		/	reload %ecx with int number
	jmp	.int_handle		/	handle the interrupt
	SIZE(deferred_int)
')

/
/ Handle profiling before deferring interrupts from clock; this allows for
/ profiling of PLHI code when using soft spls
/
.prf:
	movl	$1, %eax
	IF_USERMODE(_A_INTR_SP_IP-4(%esp), .prf_usermode)
	xorl    %eax, %eax
.prf_usermode:
	pushl	%eax				/ prfintr(pc, usermode)
	pushl	_A_INTR_SP_IP(%esp)
	call	prfintr
	addl	$8, %esp
	movl	$8, %ecx			/ set %ecx to 8 (int. no.)
	jmp	.dev_common			/ jmp to common handler

.apic_prf:
	movl	$1, %eax
	IF_USERMODE(_A_INTR_SP_IP-4(%esp), .apic_prf_usermode)
	xorl    %eax, %eax
.apic_prf_usermode:
	pushl	%eax				/ prfintr(pc, usermode)
	pushl	_A_INTR_SP_IP(%esp)
	call	prfintr
	addl	$8, %esp
	xorl	%ecx, %ecx			/ set %ecx to 0 (int. no.)
	jmp	.apic_int			/ jmp to common handler

ENTRY(softint)
	INTR_ENTER
/
/ internal entry point; used when starting softint during interrupt return
/	sequence
/
.align	4
.dosoftint:
	ASSERT(ul,`l+_A_L_IPL',==,`$_A_PLBASE')
					/ should be at PLBASE
	call	spl1
	pushl	%eax			/ save old level
	sti
	cld				/ clear direction flag

	BEGIN_INT($softint_hdlr,_A_INTR_SP_IP(%esp), $_A_PL1)
	call	softint_hdlr
	END_INT
	PL_CHECK

ifdef(`UNIPROC',`
	popl	%eax			/ get saved level from stack
	cli				/ disable interrupts at the processor
	movl	%eax, l+_A_L_IPL	/ set new ipl
	cmpl	%eax, l+_A_L_PICIPL	/ if (picipl > new ipl) then ..
	ja	.check_defer		/	check deferred interrupt
',`
	call	splx
	popl	%eax
	cli
')
	jmp	.check_event		/ else check events
	SIZE(softint)
