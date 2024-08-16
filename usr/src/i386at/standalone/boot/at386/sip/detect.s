/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.


/
/	 Copyrighted as an unpublished work.
/	 (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
/	 All rights reserved.
/	
/	 RESTRICTED RIGHTS
/	
/	 These programs are supplied under a license.  They may be used,
/	 disclosed, and/or copied only as permitted under such license
/	 agreement.  Any copy must contain the above copyright notice and
/	 this restricted rights notice.  Use, copying, and/or disclosure
/	 of the programs is strictly prohibited unless otherwise provided
/	 in the license agreement.

	.file	"detect.s"

.ident	"@(#)stand:i386at/standalone/boot/at386/sip/detect.s	1.1"
	.ident	"$Header: $"

	/ flags word bits
	.set	EFL_AC, 0x40000		/ alignment check (1->check)

	/ CR0 control register bits
	.set	CR0_CD_ENA, 0xBFFFFFFF		/ cache enabled
	.set	CR0_CD_DIS, 0x40000000		/ cache disabled

	.set	MAP_VADDR, 0x402000	/ Magic address for cd_mapin mappings
	.text
/
/ int
/ detect_cpu(void)
/	Detect CPU type.
/
/ Exit :
/	0 : i386
/	1 : i486 or newer
/

	.globl detect_cpu
	/
	/ Identify the cpu we are running on
	/

	/ Identify i386 and i486 by the inability to set certain flag bits
detect_cpu:

	pushfl				/ push FLAGS value on stack
	movl	(%esp), %ecx		/ save copy of FLAGS

	xorl	$EFL_AC, (%esp)		/ flip AC bit for new FLAGS
	popfl				/ attempt changing FLAGS.AC
	pushfl				/ push resulting FLAGS on stack
	cmpl	(%esp), %ecx		/ succeeded in flipping AC?
	je	.cpu_is_386		/ no, must be an i386

.cpu_is_486:
	movl	$1, %eax		
	popfl				/ restore original FLAGS
	ret

.cpu_is_386:
	movl	$0, %eax		
	popfl				/ restore original FLAGS
	ret

/
/ cache_on
/ 	enable cache 
/
	.globl cache_on
cache_on:
	movl %cr0,%eax
	andl	$CR0_CD_ENA,%eax
	movl %eax,%cr0
	ret

/
/ cache_off
/ 	disenable cache 
/
	.globl cache_off
cache_off:
	movl %cr0,%eax
	orl	$CR0_CD_DIS,%eax
	movl %eax,%cr0
	ret

/
/ invalidate cache w/ write-back
/
	.globl inv_cache
inv_cache:
	wbinvd
	ret
