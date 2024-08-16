.ident	"@(#)libc-i386:sys/truncate.s	1.2"

	.file	"truncate.s"

	.text

	.set	ERESTART,91

	.globl	_cerror

_fwdef_(`truncate'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$TRUNCATE,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je 	truncate
	jmp	_cerror

.noerror:
	ret
