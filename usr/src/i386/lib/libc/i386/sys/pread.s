.ident	"@(#)libc-i386:sys/pread.s	1.2"

	.file	"pread.s"

	.text

	.set	ERESTART,91

	.globl	_cerror

_fwdef_(`pread'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$PREAD,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	pread
	jmp	_cerror

.noerror:
	ret
