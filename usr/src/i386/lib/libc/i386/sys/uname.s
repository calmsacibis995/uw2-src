#ident	"@(#)libc-i386:sys/uname.s	1.5"


	.file	"uname.s"

	.text

	.set	UNAME,0

_fwdef_(`uname'):
	MCOUNT			/ subroutine entry counter if profiling
	pushl	$UNAME		/ type
	pushl	$0		/ mv flag
	pushl	12(%esp)	/ utsname address (retaddr+$UNAME+0)
	subl	$4,%esp		/ where return address would be.
	movl	$UTSSYS,%eax
	lcall	$0x7,$0
	jc	.cerror
	addl	$16,%esp
	ret

.cerror:
	addl	$16,%esp
	jmp	_cerror
	ret
