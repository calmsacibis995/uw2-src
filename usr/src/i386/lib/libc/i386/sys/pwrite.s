.ident	"@(#)libc-i386:sys/pwrite.s	1.2"


	.file	"pwrite.s"

	.text

	.set	ERESTART,91

_fwdef_(`pwrite'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PWRITE,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	pwrite
	jmp	_cerror

.noerror:
	ret
