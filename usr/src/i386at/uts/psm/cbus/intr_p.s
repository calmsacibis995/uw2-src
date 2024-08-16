/	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386at:psm/cbus/intr_p.s	1.9"
	.ident	"$Header: $"

/
/ Machine dependent low-level kernel entry points for interrupt
/ and trap handling.
/
include(KBASE/svc/asm.m4)
include(assym_include)
include(KBASE/svc/intr.m4)
include(KBASE/util/debug.m4)

FILE(`intr_p.s')

ENTRY(devint1)
	INTR_ENTER(1)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(, %edx, 4)	/ common handling

ENTRY(devint2)
	INTR_ENTER(2)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint3)
	INTR_ENTER(3)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint4)
	INTR_ENTER(4)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint5)
	INTR_ENTER(5)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint6)
	INTR_ENTER(6)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint7)
	INTR_ENTER(7)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint8)
	INTR_ENTER(8)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint9)
	INTR_ENTER(9)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint10)
	INTR_ENTER(10)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint11)
	INTR_ENTER(11)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint12)
	INTR_ENTER(12)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint13)
	INTR_ENTER(13)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint14)
	INTR_ENTER(14)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint15)
	INTR_ENTER(15)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(softint)
	INTR_ENTER
	movl	myengnum, %edx
	jmp	*ci_softint(,%edx,4)		/ common handling for softint

ENTRY(devint63)
	INTR_ENTER(63)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint96)
	INTR_ENTER(96)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint97)
	INTR_ENTER(97)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint98)
	INTR_ENTER(98)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint99)
	INTR_ENTER(99)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint100)
	INTR_ENTER(100)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint101)
	INTR_ENTER(101)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint102)
	INTR_ENTER(102)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint103)
	INTR_ENTER(103)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint104)
	INTR_ENTER(104)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint105)
	INTR_ENTER(105)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint106)
	INTR_ENTER(106)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint107)
	INTR_ENTER(107)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint108)
	INTR_ENTER(108)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint109)
	INTR_ENTER(109)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint110)
	INTR_ENTER(110)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint111)
	INTR_ENTER(111)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint112)
	INTR_ENTER(112)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint113)
	INTR_ENTER(113)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint114)
	INTR_ENTER(114)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint115)
	INTR_ENTER(115)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint116)
	INTR_ENTER(116)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint117)
	INTR_ENTER(117)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint118)
	INTR_ENTER(118)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint119)
	INTR_ENTER(119)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint120)
	INTR_ENTER(120)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint121)
	INTR_ENTER(121)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint122)
	INTR_ENTER(122)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint123)
	INTR_ENTER(123)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint124)
	INTR_ENTER(124)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint125)
	INTR_ENTER(125)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint126)
	INTR_ENTER(126)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint127)
	INTR_ENTER(127)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint128)
	INTR_ENTER(128)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint129)
	INTR_ENTER(129)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint130)
	INTR_ENTER(130)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint131)
	INTR_ENTER(131)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint132)
	INTR_ENTER(132)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint133)
	INTR_ENTER(133)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint134)
	INTR_ENTER(134)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint135)
	INTR_ENTER(135)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint136)
	INTR_ENTER(136)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint137)
	INTR_ENTER(137)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint138)
	INTR_ENTER(138)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint139)
	INTR_ENTER(139)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint140)
	INTR_ENTER(140)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint141)
	INTR_ENTER(141)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint142)
	INTR_ENTER(142)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint143)
	INTR_ENTER(143)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint144)
	INTR_ENTER(144)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint145)
	INTR_ENTER(145)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint146)
	INTR_ENTER(146)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint147)
	INTR_ENTER(147)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint148)
	INTR_ENTER(148)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint149)
	INTR_ENTER(149)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint150)
	INTR_ENTER(150)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint151)
	INTR_ENTER(151)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint152)
	INTR_ENTER(152)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint153)
	INTR_ENTER(153)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint154)
	INTR_ENTER(154)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint155)
	INTR_ENTER(155)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint156)
	INTR_ENTER(156)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint157)
	INTR_ENTER(157)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint158)
	INTR_ENTER(158)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint159)
	INTR_ENTER(159)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint160)
	INTR_ENTER(160)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint161)
	INTR_ENTER(161)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint162)
	INTR_ENTER(162)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint163)
	INTR_ENTER(163)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint164)
	INTR_ENTER(164)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint165)
	INTR_ENTER(165)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint166)
	INTR_ENTER(166)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint167)
	INTR_ENTER(167)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint168)
	INTR_ENTER(168)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint169)
	INTR_ENTER(169)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint170)
	INTR_ENTER(170)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint171)
	INTR_ENTER(171)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint172)
	INTR_ENTER(172)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint173)
	INTR_ENTER(173)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint174)
	INTR_ENTER(174)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint175)
	INTR_ENTER(175)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint176)
	INTR_ENTER(176)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint177)
	INTR_ENTER(177)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint178)
	INTR_ENTER(178)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint179)
	INTR_ENTER(179)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint180)
	INTR_ENTER(180)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint181)
	INTR_ENTER(181)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint182)
	INTR_ENTER(182)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint183)
	INTR_ENTER(183)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint184)
	INTR_ENTER(184)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint185)
	INTR_ENTER(185)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint186)
	INTR_ENTER(186)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint187)
	INTR_ENTER(187)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint188)
	INTR_ENTER(188)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint189)
	INTR_ENTER(189)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint190)
	INTR_ENTER(190)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint191)
	INTR_ENTER(191)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint192)
	INTR_ENTER(192)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint193)
	INTR_ENTER(193)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint194)
	INTR_ENTER(194)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint195)
	INTR_ENTER(195)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint196)
	INTR_ENTER(196)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint197)
	INTR_ENTER(197)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint198)
	INTR_ENTER(198)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint199)
	INTR_ENTER(199)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint200)
	INTR_ENTER(200)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint201)
	INTR_ENTER(201)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint202)
	INTR_ENTER(202)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint203)
	INTR_ENTER(203)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint204)
	INTR_ENTER(204)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint205)
	INTR_ENTER(205)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint206)
	INTR_ENTER(206)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint207)
	INTR_ENTER(207)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint208)
	INTR_ENTER(208)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint209)
	INTR_ENTER(209)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint210)
	INTR_ENTER(210)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint211)
	INTR_ENTER(211)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint212)
	INTR_ENTER(212)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint213)
	INTR_ENTER(213)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint214)
	INTR_ENTER(214)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint215)
	INTR_ENTER(215)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint216)
	INTR_ENTER(216)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint217)
	INTR_ENTER(217)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint218)
	INTR_ENTER(218)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint219)
	INTR_ENTER(219)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint220)
	INTR_ENTER(220)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint221)
	INTR_ENTER(221)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint222)
	INTR_ENTER(222)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint223)
	INTR_ENTER(223)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint224)
	INTR_ENTER(224)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint225)
	INTR_ENTER(225)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint226)
	INTR_ENTER(226)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint227)
	INTR_ENTER(227)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint228)
	INTR_ENTER(228)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint229)
	INTR_ENTER(229)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint230)
	INTR_ENTER(230)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint231)
	INTR_ENTER(231)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint232)
	INTR_ENTER(232)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint233)
	INTR_ENTER(233)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint234)
	INTR_ENTER(234)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint235)
	INTR_ENTER(235)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint236)
	INTR_ENTER(236)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint237)
	INTR_ENTER(237)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint238)				/ spurious interrupt
	INTR_ENTER(238)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint239)
	INTR_ENTER(239)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint240)
	INTR_ENTER(240)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint241)
	INTR_ENTER(241)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint242)
	INTR_ENTER(242)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint243)
	INTR_ENTER(243)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint244)
	INTR_ENTER(244)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint245)
	INTR_ENTER(245)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint246)
	INTR_ENTER(246)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint247)
	INTR_ENTER(247)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint248)
	INTR_ENTER(248)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint249)
	INTR_ENTER(249)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint250)
	INTR_ENTER(250)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint251)
	INTR_ENTER(251)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint252)
	INTR_ENTER(252)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint253)
	INTR_ENTER(253)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint254)
	INTR_ENTER(254)
	incl	intr_count			/ count the device interrupt
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling

ENTRY(devint255)				/ spurious interrupt
	iret

/
/ RST	- clocks won't necessarily come in in on devint0
/	- fix
/
ENTRY(devint0)
	INTR_ENTER(0)				/ Don't count clock ticks
	cmpl	$0, prfstat			/ Is profiler on?
	jne	.prf				/ Yes, go call it

ifdef(`NEVER',`
	//  this used to be _MPSTATS gated.  But now its off.
	// STARTFIXCOMMENT
	// The code in the defered interrupt handler for _MPSTATS is broken
	// so this switch had to be turned off until that code can be fixed.
	/  ENDFIXCOMMENT
	/
	/ The following code provides a free-flowing, unblockable
	/ ulbolt counter inspite of the locks being held at PLHI.
	/ The ulbolt counter is incremented every time a clock
	/ interrupt is received. If the current priority level is
	/ PLHI, then the clock interrupt is deferred, otherwise
	/ the clock handler is called to service the interrupt.
	/ However, if a clock interrupt is already deferred, the
	/ ulbolt counter will only be incremented and without the
	/ clock interrupt handler being called, the interrupt will
	/ return immediately after acknowledging the interrupt. The
	/ clock interrupt handler will NOT be called.
	/
	/ Increment free-flowing microsec ulbolt counter.
	/
	/ Since the clock interrupt rate is HZ (100 per sec.), we
	/ increment ulbolt by 10000 for each clock interrupt. This
	/ prevents executing a mul instruction in pit_usec_time()
	/ while doing the unit coversion of ulbolt from 10 millisec
	/ to microsec. The reason for doing so is the following:
	/	- mul instruction is more expensive than add
	/	  instruction.
	/	- Execution rate of pit_usec_time() is much
	/	  greater than clock interrupt rate because
	/	  every time a spin lock is acquired/released
	/	  pit_usec_time() is called.
	/	- The cost of add instruction is comparable to
	/	  inc instruction.
	/
.devint0c:
	addl	$10000, ulbolt		/ increment free-flowing ulbolt counter

	movl	myengnum, %edx
	movl	picdeferndx(, %edx, 4), %eax	/ %eax = deferred stack index
	movl	picdeferred(, %edx, 4), %edx	/ %edx = pointer to defer table
	movl	(%edx, %eax, 4), %eax		

	cmpl	$0, %eax		/ if (%eax == 0)
	je	.dev_clkdeferred	/	goto dev_clkdeferred
	/ ------ RST fix me
')
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling


/ -----------------------------------------------------------------------------
/ Interrupt entry point for CBUS-I Base Processor
/ -----------------------------------------------------------------------------
ENTRY(ci_base_intr)
	xorl	%eax, %eax		/ save current level
	movb	ipl, %al
	pushl	%eax
	pushl	%ecx			/ push irqline

	ASSERT(ul,`%eax',<=,`$_A_PLHI')	/ old pl <= PLHI

	/
	/ check to see if this is a spurious interrupt
	/
	testb	$_A_IRQ_CHKSPUR, irqtab+_A_IRQ_FLAGS(,%ecx,4)
	jne	.check_spurious

	/
	/ a real (non-spurious) hardware interrupt
	/	(1) set ipl and PIC masks to service level of interrupt
	/	(2) acknowledge interrupt
	/	(3) see if interrupt should be deferred
	/
.real_int:
	/
	/ get priority level of interrupt and set PIC masks to it
	/ realize that ecx is going to be the irqline,  for IPI, we don't 
	/ necessarily have an irqline to pretend with, so we set it equal 
	/ to the IPI vector.
	/
ifdef(`DEBUG',`
	movzbl	intpri(%ecx),%eax
	ASSERT(ul,%eax,<=,`$_A_PLMAX')		/ interrupt pl <= PLMAX

	/
	/ The assert operator must be changed from > to >= since now at 
	/ ipl == PLHI == picipl, we can still get clock interrupts but that
	/ will be defferred immediately.
	/
	ASSERT(ul,%eax,>=,`picipl')		/ interrupt pl > picipl
')
	/
	/ set current pl to service pl for interrupt
	/
	movl	svcpri(,%ecx,4), %eax	/ %eax = service level
	ASSERT(ul,%eax,<=,`$_A_PLMAX')	/ service level <= PLMAX

	movb	%al, ipl		/ ipl = service level

	call	ci_setpicmasks_base

	movl	(%esp), %ecx		/ restore %ecx (== irqline, not vector)

	/
	/ acknowledge interrupt by sending EOI to master PIC and, if necessary,
	/	to slave as well
	/
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
	.align	4
.eoi_done:

	/
	/ see if interrupt should be deferred
	/
	movzbl	intpri(%ecx), %eax	/ %eax = interrupt pl
	cmpl	4(%esp), %eax		/ if (interrupt pl <= old pl) then ...
	jbe	.picdefer		/   .. defer the interrupt

	/
	/ Handle the interrupt.  This code is reached via two paths:
	/	(1) Falls through from .dev_common for non-deferred interrupts
	/	(2) Branched to from deferred_int to handle deferred interrupts
	/

.int_handle_base:
	sti
	cld			/ clear direction flag

	BEGIN_INT(ivect(,%ecx,4), _A_INTR_SP_IP+4(%esp), %eax)
				/ keep stats if enabled and increment
				/	interrupt depth counter
	call	*ivect(,%ecx,4)	/ call registered handler
	addl	$4, %esp	/ clear int number off stack upon return
	END_INT			/ keep stats if enabled and decrement
				/	interrupt depth counter

	popl	%eax		/ get saved level from stack
	cli			/ disable interrupts at the processor
	movb	%al, ipl	/ set new ipl

	/
	/ Check to see if there are any deferred interrupts whose priority 
	/ is higher than the priority level of the interrupted context.
	/
	/ This code is reached via two paths:
	/	(1) Falls through from above for deferred and non-deferred 
	/	    interrupts
	/	(2) Branched to by softint if picipl > ipl
	/
.check_defer_base:
	movl	myengnum, %edx
	movl	picdeferndx(, %edx, 4), %ecx	/ %ecx = deferred stack index
	movl	picdeferred(, %edx, 4), %edx	
	movl	(%edx, %ecx, 4), %ecx		

	movb	intpri(%ecx), %dl	/ %dl = interrupt of topmost def int
	cmpb	ipl, %dl		/ if deferred ipl > current ipl ...
	ja	deferred_int_base	/   ... undefer the interrupt

	/
	/ Not handling any deferred interrupts, so just set pic mask to the
	/	return pl (which is still in %eax)
	/
	call	ci_setpicmasks_base

	/
	/ Do interrupt return processing.
	/
	jmp	.check_event_base
	SIZE(ci_base_intr)

/
/ deferred_int_base
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
ENTRY(deferred_int_base)
	xorl	%eax, %eax		/ save current level
	movb	ipl, %al
	pushl	%eax
	pushl	%ecx			/ push interrupt number arg

ifdef(`DEBUG',`
	movl	myengnum, %edx
	movl	picdeferndx(, %edx, 4), %eax 	/ %eax = deferred stack index
	movl	picdeferred(, %edx, 4), %edx	
	ASSERT(ul,%ecx,==,(%edx, %eax, 4))

	ASSERT(ub,`intpri(%ecx)',<=,`$_A_PLMAX')
					/ interrupt above PLMAX should never
					/	be deferred
')
	movl	myengnum, %edx
	decl	picdeferndx(, %edx, 4) 	/ pop the deferred interrupt

	ASSERT(sl,picdeferndx(, %edx, 4),>=,$0);
	movl	svcpri(,%ecx,4), %eax	/ %eax = service priority
	movb	%al, ipl		/ set new pl
	cmpl	%eax, picipl		/ if (pl == picipl) then
	je	.int_handle_base	/	handle the interrupt

	call	ci_setpicmasks_base

	movl	(%esp), %ecx		/	reload %ecx with int number
if(`defined(`DEBUG') || defined(`SPINDEBUG')',`
	xorl	%eax, %eax		/ for DEBUG, mov intpri into %eax
	movb	intpri(%ecx), %al
')
	jmp	.int_handle_base	/	handle the interrupt
	SIZE(deferred_int_base)

/
/ Check for spurious interrupts.  Assumes PIC is in READ ISR mode.
/
	.align	8
.check_spurious:
	movl	irqtab+_A_IRQ_CMDPORT(,%ecx,4), %edx
					/ get status port address
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
	jmp	intr_return


/ -----------------------------------------------------------------------------
/ Interrupt entry point for CBUS-I Additional Processor
/ -----------------------------------------------------------------------------
ENTRY(ci_acpu_intr)
	xorl	%eax, %eax		/ save current level
	movb	ipl, %al
	pushl	%eax
	pushl	%ecx			/ push irqline

	ASSERT(ul,`%eax',<=,`$_A_PLHI')	/ old pl <= PLHI

	/ get priority level of interrupt and set PIC masks to it
	/ realize that ecx is going to be the irqline,  for IPI, we don't 
	/ necessarily have an irqline to pretend with, so we set it equal 
	/ to the IPI vector.
ifdef(`DEBUG',`
	movzbl	intpri(%ecx),%eax
	ASSERT(ul,%eax,<=,`$_A_PLMAX')		/ interrupt pl <= PLMAX

	/
	/ The assert operator must be changed from > to >= since now at 
	/ ipl == PLHI == picipl, we can still get clock interrupts but that
	/ will be defferred immediately.
	/
	ASSERT(ul,%eax,>=,`picipl')	/ interrupt pl > picipl
')

	/
	/ set current pl to service pl for interrupt
	/
	movl	svcpri(,%ecx,4), %eax	/ %eax = service level
	ASSERT(ul,%eax,<=,`$_A_PLMAX')	/ service level <= PLHI

	movb	%al, ipl	/ ipl = service level

	cld			/ clear direction flag

	BEGIN_INT(ivect(,%ecx,4), _A_INTR_SP_IP+4(%esp), %eax)
				/ keep stats if enabled and increment
				/	interrupt depth counter
	call	*ivect(,%ecx,4)	/ call registered handler
	addl	$4, %esp	/ clear int number off stack upon return
	END_INT			/ keep stats if enabled and decrement
				/	interrupt depth counter

	popl	%eax		/ get saved level from stack
	movb	%al, ipl	/ set new ipl

	/
	/ Do interrupt return processing.
	/
	jmp	.check_event_acpu
	SIZE(ci_acpu_intr)


/ -----------------------------------------------------------------------------
/ Interrupt entry point for CBUS-I APIC Processor
/ -----------------------------------------------------------------------------
ENTRY(ci_apic_intr)
	xorl	%eax, %eax		/ save current level
	movb	ipl, %al
	pushl	%eax
	pushl	%ecx			/ push interrupt vector arg

	shll	$4, %ecx
	movl	cbus_apic_vector_data(%ecx), %ecx
	/
	/ ecx is now the irqline 
	/
						/  (NOT vector)
	movl	%ecx, (%esp)			/ save irqline on stack 
						/  (overwrite vec)

	ASSERT(ul,`%eax',<=,`$_A_PLHI')		/ old pl <= PLHI

	/
	/ a real (non-spurious) hardware interrupt
	/	(1) set ipl and PIC masks to service level of interrupt
	/	(2) acknowledge interrupt
	/	(3) see if interrupt should be deferred
	/
	/ get priority level of interrupt and set PIC masks to it
	/ realize that ecx is going to be the irqline,  for IPI, we don't 
	/ necessarily have an irqline to pretend with, so we set it equal 
	/ to the IPI vector.
	/

ifdef(`DEBUG',`
	movzbl	intpri(%ecx),%eax
	ASSERT(ul,%eax,<=,`$_A_PLMAX')		/ interrupt pl <= PLMAX

	/
	/ The assert operator must be changed from > to >= since now at 
	/ ipl == PLHI == picipl, we can still get clock interrupts but that
	/ will be defferred immediately.
	/
	ASSERT(ul,%eax,>=,`picipl')		/ interrupt pl > picipl
')

	/
	/ set current pl to service pl for interrupt
	/
	movl	svcpri(,%ecx,4), %eax	/ %eax = service level
	ASSERT(ul,%eax,<=,`$_A_PLMAX')	/ service level <= PLMAX

	movb	%al, ipl		/ ipl = service level

	call	ci_setpicmasks_apic
	movl	(%esp), %ecx		/ restore %ecx (== irqline, not vector)

	/
	/ see if interrupt should be deferred
	/
	movzbl	intpri(%ecx), %eax	/ %eax = interrupt pl
	cmpl	4(%esp), %eax		/ if (interrupt pl <= old pl) then ...
	jbe	.picdefer		/   .. defer the interrupt

	/
	/ Handle the interrupt.  This code is reached via two paths:
	/	(1) Falls through from .dev_common for non-deferred interrupts
	/	(2) Branched to from deferred_int to handle deferred interrupts
	/

.int_handle_apic:
	sti
	cld			/ clear direction flag

	BEGIN_INT(ivect(,%ecx,4), _A_INTR_SP_IP+4(%esp), %eax)
				/ keep stats if enabled and increment
				/	interrupt depth counter
	call	*ivect(,%ecx,4)	/ call registered handler
	addl	$4, %esp	/ clear int number off stack upon return
	END_INT			/ keep stats if enabled and decrement
				/	interrupt depth counter

	/
	/ the APIC requires a write to its EOI register to dismiss 
	/ the interrupt...
	/
	/ WARNING: this will automatically lower the internal priority
	/ register to the level it was prior to receipt of the interrupt.
	/
	movl	cbus_apic_eoi_reg, %eax	/ get address
	movl	%eax, (%eax)		/ must _write_ EOI

	popl	%eax		/ get saved level from stack
	cli			/ disable interrupts at the processor
	movb	%al, ipl	/ set new ipl

	/
	/ Check to see if there are any deferred interrupts whose priority 
	/ is higher than the priority level of the interrupted context.
	/
	/ This code is reached via two paths:
	/	(1) Falls through from above for deferred and non-deferred 
	/	    interrupts
	/	(2) Branched to by softint if picipl > ipl
	/
.check_defer_apic:
	movl	myengnum, %edx
	movl	picdeferndx(, %edx, 4), %ecx	/ %ecx = deferred stack index
	movl	picdeferred(, %edx, 4), %edx	
	movl	(%edx, %ecx, 4), %ecx		

	movb	intpri(%ecx), %dl	/ %edx = interrupt of topmost def int
	cmpb	ipl, %dl		/ if deferred ipl > current ipl ...
	ja	deferred_int_apic	/   ... undefer the interrupt

	/
	/ Not handling any deferred interrupts, so just set pic mask to the
	/	return pl (which is still in %eax)
	/
	call	ci_setpicmasks_apic

	/
	/ Do interrupt return processing.
	/
	jmp	.check_event_apic
	SIZE(ci_apic_intr)

/
/ deferred_int_apic
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
ENTRY(deferred_int_apic)
	xorl	%eax, %eax		/ save current level
	movb	ipl, %al
	pushl	%eax
	pushl	%ecx			/ push interrupt number arg

ifdef(`DEBUG',`
	movl	myengnum, %edx
	movl	picdeferndx(, %edx, 4), %eax 	/ %eax = deferred stack index
	movl	picdeferred(, %edx, 4), %edx	
	ASSERT(ul,%ecx,==,(%edx, %eax, 4))

	movzbl	intpri(%ecx), %eax	/ pl of interrupt
	ASSERT(ul,%eax,<=,`$_A_PLHI')	/ interrupt above PLHI should never
					/	be deferred
')
	movl	myengnum, %edx
	decl	picdeferndx(, %edx, 4) 	/ pop the deferred interrupt

	ASSERT(sl,picdeferndx(, %edx, 4),>=,$0);
	movl	svcpri(,%ecx,4), %eax	/ %eax = service priority
	movb	%al, ipl		/ set new pl
	cmpl	%eax, picipl		/ if (pl == picipl) then
	je	.int_handle_apic	/	handle the interrupt

	call	ci_setpicmasks_apic

	movl	(%esp), %ecx		/	reload %ecx with int number
if(`defined(`DEBUG') || defined(`SPINDEBUG')',`
	movzbl	intpri(%ecx), %eax	/ for DEBUG, mov intpri into %eax
')
	jmp	.int_handle_apic	/	handle the interrupt
	SIZE(deferred_int_apic)

ifdef(`DEBUG',`
define(`DBGOUT',`
	pushl	%eax
	movl	$1, %eax
	outb	`$'0x80
	popl	%eax
')

define(`DBGFLGS',`
	pushl	%eax
	movl	%esp, %eax
	addl	`$'0x28, %eax
	movl	(%eax), %eax
	outw	`$'0x92
	popl	%eax
')


define(`DBGIPL',`
	pushl	%eax
	movl	ipl, %eax
	outb	`$'0x90
	popl	%eax
')

define(`DBGPICIPL',`
	pushl	%eax
	movl	picipl, %eax
	outb	`$'0x91
	popl	%eax
')
')

/ -----------------------------------------------------------------------------
/ Interrupt entry point for CBUS2 Processor
/ -----------------------------------------------------------------------------
ENTRY(ci_cbus2_intr)
	xorl	%eax, %eax		/ save current level
	movb	ipl, %al
	pushl	%eax
	pushl	%ecx			/ push interrupt vector arg

	shll	$4, %ecx
	movl	cbus2_vector_data(%ecx), %ecx	/ ecx is now the irqline 
						/  (NOT vector)
	movl	%ecx, (%esp)			/ save irqline on stack 
						/  (overwrite vec)

	ASSERT(ul,`%eax',<=,`$_A_PLMAX')	/ old pl <= PLMAX

	/
	/ a real (non-spurious) hardware interrupt
	/	(1) set ipl and PIC masks to service level of interrupt
	/	(2) acknowledge interrupt
	/	(3) see if interrupt should be deferred
	/
	/ get priority level of interrupt and set PIC masks to it
	/ realize that ecx is going to be the irqline,  for IPI, we don't 
	/ necessarily have an irqline to pretend with, so we set it equal 
	/ to the IPI vector.
	/

ifdef(`DEBUG',`
	movzbl	intpri(%ecx),%eax
	ASSERT(ul,%eax,<=,`$_A_PLMAX')		/ interrupt pl <= PLMAX

	/
	/ The assert operator must be changed from > to >= since now at 
	/ ipl == PLHI == picipl, we can still get clock interrupts but that
	/ will be defferred immediately.
	/
	ASSERT(ul,%eax,>=,`picipl')		/ interrupt pl > picipl
')
	/
	/ set current pl to service pl for interrupt
	/
	movl	svcpri(,%ecx,4), %eax	/ %eax = service level
	ASSERT(ul,%eax,<=,`$_A_PLMAX')	/ service level <= PLMAX

	movb	%al, ipl		/ ipl = service level

	call	ci_setpicmasks_cbus2
	movl	(%esp), %ecx		/ restore %ecx (== irqline, not vector)

	/
	/ see if interrupt should be deferred
	/
	movzbl	intpri(%ecx), %eax	/ %eax = interrupt pl
	cmpl	4(%esp), %eax		/ if (interrupt pl <= old pl) then ...
	jbe	.picdefer		/   .. defer the interrupt

	/
	/ Handle the interrupt.  This code is reached via two paths:
	/	(1) Falls through from .dev_common for non-deferred interrupts
	/	(2) Branched to from deferred_int to handle deferred interrupts
	/

.int_handle_cbus2:
	sti
	cld			/ clear direction flag

	BEGIN_INT(ivect(,%ecx,4), _A_INTR_SP_IP+4(%esp), %eax)
				/ keep stats if enabled and increment
				/	interrupt depth counter
	call	*ivect(,%ecx,4)	/ call registered handler

	/
	/ the CBC requires a write to the EISA bridge offset by the irqline
	/ that generated the interrupt...
	/

	popl	%ecx			/ get int number off stack upon return

	cmpl	$0x10, %ecx		/ if irqline not on EISA, no EOI needed
	jae	noeoi			/ skip the EOI

	movl	cbus2_fix_level_interrupts, %edx
	cmpl	$0, %edx
	jne	fix_level

	movl	myengnum, %eax
	movl	cbus2_eoi_needed(,%eax,4), %eax	/ check if EOI is needed
	addl	%ecx, %eax
	movb	(%eax), %al
	cmpb	$0, %al
	je	noeoi
cbus2_eoi:
	movl	cbus2_bridge_eoi, %edx	/ get address
	shl	$3, %ecx		/ convert to CSR offset
	addl	%ecx, %edx
	movl	$0, (%edx)		/ must _write_ EOI
	jmp	noeoi

	/
	/ currently work-around a bug in the CBC by
	/ clearing the hwintrmap entry and restoring it.
	/ this will clear the phantom interrupt generated
	/ by a level-triggered interrupt.
	/
fix_level:
	movl	myengnum, %eax
	movl	cbus2_eoi_needed(,%eax,4), %eax	/ check if EOI is needed
	addl	%ecx, %eax
	movb	(%eax), %al
	cmpb	$0, %al				/ if EOI_NONE, no EOI
	je	noeoi
	cmpb	$1, %al				/ if EOI_EDGE, only EOI
	je	cbus2_eoi
	movl	cbus2_bridge_eoi, %edx		/ else EOI_LEVEL, R/W INTRCFG
	subl	cbus2_eoitohwmap, %edx
	movl	cbus2_irqtohwmap(,%ecx,4), %eax	/ get contents of hwmap
	shl	$3, %ecx			/ convert to CSR offset
	addl	%ecx, %edx
	movl	$0, (%edx)			/ clear the hwintrmap entry
	movl	%eax, (%edx)			/ restore the hwintrmap entry

noeoi:

	END_INT			/ keep stats if enabled and decrement
				/	interrupt depth counter

	popl	%eax		/ get saved level from stack
	cli			/ disable interrupts at the processor
	movb	%al, ipl	/ set new ipl

	/
	/ Check to see if there are any deferred interrupts whose priority 
	/ is higher than the priority level of the interrupted context.
	/
	/ This code is reached via two paths:
	/	(1) Falls through from above for deferred and non-deferred 
	/	    interrupts
	/	(2) Branched to by softint if picipl > ipl
	/
.check_defer_cbus2:

	movl	myengnum, %edx
	movl	picdeferndx(, %edx, 4), %ecx	/ %ecx = deferred stack index
	movl	picdeferred(, %edx, 4), %edx	
	movl	(%edx, %ecx, 4), %ecx		

	movb	intpri(%ecx), %dl	/ %dl = interrupt of topmost def int
	cmpb	ipl, %dl		/ if deferred ipl > current ipl ...
	ja	deferred_int_cbus2	/   ... undefer the interrupt

	/
	/ Not handling any deferred interrupts, so just set pic mask to the
	/	return pl (which is still in %eax)
	/
	call	ci_setpicmasks_cbus2

	/
	/ Do interrupt return processing.
	/
	jmp	.check_event_cbus2
	SIZE(ci_cbus2_intr)


/
/ deferred_int_cbus2
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
ENTRY(deferred_int_cbus2)

	xorl	%eax, %eax		/ save current level
	movb	ipl, %al
	pushl	%eax
	pushl	%ecx			/ push interrupt number arg

ifdef(`DEBUG',`
	movl	myengnum, %edx
	movl	picdeferndx(, %edx, 4), %eax 	/ %eax = deferred stack index
	movl	picdeferred(, %edx, 4), %edx	
	ASSERT(ul,%ecx,==,(%edx, %eax, 4))

	movzbl	intpri(%ecx), %eax	/ pl of interrupt
	ASSERT(ul,%eax,<=,`$_A_PLMAX')	/ interrupt above PLMAX should never
					/	be deferred
')
	movl	myengnum, %edx
	decl	picdeferndx(, %edx, 4) 	/ pop the deferred interrupt

	ASSERT(sl,picdeferndx(, %edx, 4),>=,$0);
	movl	svcpri(,%ecx,4), %eax	/ %eax = service priority
	movb	%al, ipl		/ set new pl
	cmpl	%eax, picipl		/ if (pl == picipl) then
	je	.int_handle_cbus2	/	handle the interrupt

	call	ci_setpicmasks_cbus2

	movl	(%esp), %ecx		/	reload %ecx with int number
if(`defined(`DEBUG') || defined(`SPINDEBUG')',`
	movzbl	intpri(%ecx), %eax	/ for DEBUG, mov intpri into %eax
')
	jmp	.int_handle_cbus2	/	handle the interrupt
	SIZE(deferred_int_cbus2)


/ -----------------------------------------------------------------------------
/ Sofware interrupt entry point for CBUS-I Base Processor
/ -----------------------------------------------------------------------------
/
/ Calling/Exit State:
/	None.
/
/ Note:
/	Only applicable to AT architectures with softspls.
/
/	softint_hdlr must be exported to the mhal/oem module.
/
ENTRY(softint_base)
	/
	/ macro to do softint. Must be used in conjunction with
	/ check_event macro.
	/
	/ internal entry point; used when starting softint during interrupt 
	/ return sequence
	/
.align	4
.dosoftint_base:
	call	spl1
	pushl	%eax			/ save old level
	sti
	cld				/ clear direction flag

	BEGIN_INT($softint_hdlr,_A_INTR_SP_IP(%esp), $_A_PL1)
	call	softint_hdlr
	END_INT

	popl	%eax			/ get saved level from stack
	cli				/ disable interrupts at the processor
	movb	%al, ipl		/ set new ipl
	cmpl	%eax, picipl		/ if (picipl > new ipl) then ..
	ja	.check_defer_base	/	check deferred interrupt
	jmp	.check_event_base	/ else check events
	SIZE(softint_base)

/ -----------------------------------------------------------------------------
/ Software interrupt entry point for CBUS-I additional Processor
/ -----------------------------------------------------------------------------
/
/ Calling/Exit State:
/	None.
/
/ Note:
/	Only applicable to AT architectures with softspls.
/
/	softint_hdlr must be exported to the mhal/oem module.
/
ENTRY(softint_acpu)
	/
	/ macro to do softint. Must be used in conjunction with
	/ check_event macro.
	/
	/ internal entry point; used when starting softint during interrupt 
	/ return sequence
	/
.align	4
.dosoftint_acpu:
	call	spl1
	pushl	%eax			/ save old level
	cld				/ clear direction flag

	BEGIN_INT($softint_hdlr,_A_INTR_SP_IP(%esp), $_A_PL1)
	call	softint_hdlr
	END_INT
					/ %eax is already pushed
	call	splx			/ should be spl0 on entry

	popl	%eax
	cli
	jmp	.check_event_acpu
	SIZE(softint_acpu)

/ -----------------------------------------------------------------------------
/ Software interrupt entry point for CBUS-I APIC Processor
/ -----------------------------------------------------------------------------
/
/ Calling/Exit State:
/	None.
/
/ Note:
/	Only applicable to AT architectures with softspls.
/
/	softint_hdlr must be exported to the mhal/oem module.
/
ENTRY(softint_apic)
	/
	/ macro to do softint. Must be used in conjunction with
	/ check_event macro.
	/
	/ internal entry point; used when starting softint during interrupt 
	/ return sequence
	/
.align	4
.dosoftint_apic:
	call	spl1
	pushl	%eax			/ save old level
	sti
	cld				/ clear direction flag

	BEGIN_INT($softint_hdlr,_A_INTR_SP_IP(%esp), $_A_PL1)
	call	softint_hdlr
	END_INT

	popl	%eax			/ get saved level from stack
	cli				/ disable interrupts at the processor
	movb	%al, ipl		/ set new ipl
	cmpl	%eax, picipl		/ if (picipl > new ipl) then ..
	ja	.check_defer_apic	/	check deferred interrupt
	jmp	.check_event_apic	/ else check events
	SIZE(softint_apic)

/ -----------------------------------------------------------------------------
/ Software interrupt entry point for CBUS2 Base Processor
/ -----------------------------------------------------------------------------
/
/ Calling/Exit State:
/	None.
/
/ Note:
/	Only applicable to AT architectures with softspls.
/
/	softint_hdlr must be exported to the mhal/oem module.
/
ENTRY(softint_cbus2)
	/
	/ macro to do softint. Must be used in conjunction with
	/ check_event macro.
	/
	/ internal entry point; used when starting softint during interrupt 
	/ return sequence
	/
.align	4
.dosoftint_cbus2:
	call	spl1
	pushl	%eax			/ save old level
	sti
	cld				/ clear direction flag

	BEGIN_INT($softint_hdlr,_A_INTR_SP_IP(%esp), $_A_PL1)
	call	softint_hdlr
	END_INT

	popl	%eax			/ get saved level from stack
	cli				/ disable interrupts at the processor
	movb	%al, ipl		/ set new ipl
	cmpl	%eax, picipl		/ if (picipl > new ipl) then ..
	ja	.check_defer_cbus2	/	check deferred interrupt
	jmp	.check_event_cbus2	/ else check events
	SIZE(softint_cbus2)



/ -----------------------------------------------------------------------------
/ Common code for deferring interrupts
/ -----------------------------------------------------------------------------
/
/ defer the current interrupt.
/
	.align	8
.picdefer:
	movl	myengnum, %edx
	incl	picdeferndx(, %edx, 4)	/ picdeferred[++picdeferndx] = intnum
	movl	picdeferndx(, %edx, 4), %eax 	/ %eax = deferred stack index
	movl	picdeferred(, %edx, 4), %edx	
	movl	%ecx, (%edx, %eax, 4)

	addl	$4, %esp		/ clear interrupt number off the stack
	popl	%eax			/ restore the previous ipl
	movb	%al, ipl
	jmp	intr_return



ifdef(`NEVER',`
//////  was _MPSTATS gated: BROKEN CODE, turned off for now.  Needs fixin
//////
/ ----- RST fix me
/
/ acknowledge interrupt by sending EOI to master PIC only.
/
.dev_clkdeferred:
	movl	myengnum, %edx
	movl	ci_intsw(, %edx, 4), %edx
	/ addl	$EOI, %edx
	call	*%edx

	/
	/ return to kernel mode because we cannot have a deferred
	/ clock interrupt if we are in user mode. In other words,
	/ we have to be in kernel mode to have deferred clock
	/ interrupts and reach here.
	/
	jmp     intr_return
')

/ -----------------------------------------------------------------------------
/ Common code for profiling
/ -----------------------------------------------------------------------------
/
/ Handle profiling before deferring interrupts from clock; this allows for
/ profiling of PLHI code when using soft spls
/
.prf:
	movl	$1, %eax
	IF_USERMODE(_A_INTR_SP_IP-4(%esp), .prf_usermode)
	xorl	%eax, %eax
.prf_usermode:
	pushl	%eax				/ prfintr(pc, usermode)
	pushl	_A_INTR_SP_IP(%esp)
	call	prfintr
	addl	$8, %esp
	xorl	%ecx, %ecx			/ set %ecx to 0 (int. no.)
ifdef(`NEVER',`
////// was _MPSTATS gated: 
////// This code was ok, but other _MPSTATS code is broken, so this is off also
	jmp	.devint0c
',`
	movl	myengnum, %edx
	jmp	*ci_intsw(,%edx,4)		/ common handling
')

/ -----------------------------------------------------------------------------
/ Check event for CBUS-I base processor
/ -----------------------------------------------------------------------------
/
/ .check_event_base
/	Check for events.  softint has joined by now, and so has .devxcall
/
/ Calling/Exit State:
/	Returns to kernel mode if
/		- not at plbase
/		- pending soft interrupts
/		- preemption flag is set
/		- user preemption is set
/		- pending trap event processing 
/	To user mode if
/		- None of the above conditons are true.
/
/
.check_event_base:

/ if there are soft interrupts pending, then handle them
	testl	$_A_EVT_SOFTINTMASK, engine_evtflags
	jz	intr_return
	cmpb	$_A_PLBASE, ipl
	je	.dosoftint_base
	jmp	intr_return


/ -----------------------------------------------------------------------------
/ Check event for CBUS-I additional processor
/ -----------------------------------------------------------------------------
/
/ .check_event_acpu
/	Check for events.  softint has joined by now, and so has .devxcall
/
/ Calling/Exit State:
/	Returns to kernel mode if
/		- not at plbase
/		- pending soft interrupts
/		- preemption flag is set
/		- user preemption is set
/		- pending trap event processing 
/	To user mode if
/		- None of the above conditons are true.
/
/
.check_event_acpu:
	testl	$_A_EVT_SOFTINTMASK, engine_evtflags
	jz	intr_return
	cmpb	$_A_PLBASE, ipl
	je	.dosoftint_acpu
	jmp	intr_return


/ -----------------------------------------------------------------------------
/ Check event for CBUS-I apic processor
/ -----------------------------------------------------------------------------
/
/ .check_event_apic
/	Check for events.  softint has joined by now, and so has .devxcall
/
/ Calling/Exit State:
/	Returns to kernel mode if
/		- not at plbase
/		- pending soft interrupts
/		- preemption flag is set
/		- user preemption is set
/		- pending trap event processing 
/	To user mode if
/		- None of the above conditons are true.
/
/
.check_event_apic:
	testl	$_A_EVT_SOFTINTMASK, engine_evtflags
	jz	intr_return
	cmpb	$_A_PLBASE, ipl
	je	.dosoftint_apic
	jmp	intr_return

/ -----------------------------------------------------------------------------
/ Check event for CBUS-2 processor
/ -----------------------------------------------------------------------------
/
/ .check_event_cbus2
/	Check for events.  softint has joined by now, and so has .devxcall
/
/ Calling/Exit State:
/	Returns to kernel mode if
/		- not at plbase
/		- pending soft interrupts
/		- preemption flag is set
/		- user preemption is set
/		- pending trap event processing 
/	To user mode if
/		- None of the above conditons are true.
/
/
.check_event_cbus2:
	testl	$_A_EVT_SOFTINTMASK, engine_evtflags
	jz	intr_return
	cmpb	$_A_PLBASE, ipl
	je	.dosoftint_cbus2
	jmp	intr_return


/ -----------------------------------------------------------------------------
/ Read the specified register in the CSR space.  This routine is
/ coded in assembly because the register must be read/written 32
/ bits at a time, and we don't want the compiler "optimizing" our
/ accesses into byte-enabled operations which the hardware won't
/ understand.
/ -----------------------------------------------------------------------------
ENTRY(cbusreadcsr)
	movl	4(%esp), %ecx			/ CSR register address
	movl	(%ecx), %eax			/ return CSR register contents
	SIZE(cbusreadcsr)

/ -----------------------------------------------------------------------------
/ Write the specified register in the CSR space.  This routine is
/ coded in assembly because the register must be read/written 32
/ bits at a time, and we don't want the compiler "optimizing" our
/ accesses into byte-enabled operations which the hardware won't
/ understand.
/ -----------------------------------------------------------------------------
ENTRY(cbuswritecsr)
	movl	4(%esp), %ecx			/ CSR register address
	movl	8(%esp), %eax			/ new contents
	movl	%eax, (%ecx)			/ set the new value
	SIZE(cbuswritecsr)

