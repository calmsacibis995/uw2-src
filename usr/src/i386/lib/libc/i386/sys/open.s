.ident	"@(#)libc-i386:sys/open.s	1.5"

	.file	"open.s"
	
	.text

	.set    ERESTART,91

	.globl	_cerror

_fwdef_(`open'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$OPEN,%eax
	lcall	$0x7,$0
	jae	.noerror
	cmpb	$ERESTART,%al
	je	open
	jmp	_cerror

.noerror:
	ret
