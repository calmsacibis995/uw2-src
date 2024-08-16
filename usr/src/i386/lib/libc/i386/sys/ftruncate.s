.ident	"@(#)libc-i386:sys/ftruncate.s	1.2"

	.file	"ftruncate.s"

	.text

	.set	ERESTART,91

	.globl	_cerror

_fwdef_(`ftruncate'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$FTRUNCATE,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je 	ftruncate
	jmp	_cerror

.noerror:
	ret
