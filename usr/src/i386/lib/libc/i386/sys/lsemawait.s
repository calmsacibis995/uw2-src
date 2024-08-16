.ident	"@(#)libc-i386:sys/lsemawait.s	1.1"

	.file	"lwpsemawait.s"

	.text

	.set	ERESTART,91
	.set	EINTR,4

_fwdef_(`_lwp_sema_wait'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$LWPSEMAWAIT,%eax
	lcall	$0x7,$0
	jc	.swerror
	ret
.swerror:
	cmpb	$ERESTART,%al
	je	.reerror
	ret
.reerror:
	movl	$EINTR,%eax
	ret
