/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:svc/start.s	1.2"
	.ident	"$Header: $"
	.file	"svc/start.s"

/ void
/ _start(void)
/
/	This code is not really a C callable procedure but instead
/	is code called by the low level startup or online code.
/
/ Calling State/Exit State:
/
/	This function never returns since there is no context to return
/	to, but either initializes the system or just calls swtch()
/	when the system is first booted or a procesor is onlined respectively.
/
/	The first call to this function is with only one processor running.
/
/	Subsequent calls are the result of online of other processors and
/	they are serialized at higher levels of code (ie. the online code),
/	thus we have simplified mutex issues to deal with.  These callers
/	pass the logical engine number (engine[] index) of this engine
/	in %edx.
/
/ Description:
/
/	If we are the first processor, do:
/		1. call sysinit
/		2. call p0init to handcraft the first process;
/		   p0init in turn invokes main to complete initialization.
/
/	If we are not the first processor, do:
/		1. call selfinit
/		2. call swtch.
/

include(../svc/asm.m4)
include(assym_include)

	.data
	.globl	upyet
	.align	4
upyet:	.long	0			/ upyet: non-zero when system is "up"

 	.text

ENTRY(_start)
	pushl	$0			/ clear all flags
	popfl

	cmpl	$0, upyet
	jne	sysup

	/
	/ System not already up.  Call sysinit().
	/
	call	sysinit

	incl	upyet			/ indicate the system is initialized

	jmp	p0init			/ handcraft process 0.

sysup:
	/
	/ System already up, so this must be the result
	/ of an online() call.  Just call selfinit and swtch.
	/
	incl	prmpt_state		/ executing on the idle stack
	pushl	%edx			/ %edx contains logical processor #
	call	selfinit		/ call selfinit(procid)
	movl	$0, (%esp)
	call	swtch			/ swtch((lwp_t *)NULL)
	/ NOTREACHED

	SIZE(_start)
