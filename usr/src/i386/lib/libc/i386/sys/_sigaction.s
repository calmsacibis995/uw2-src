.ident	"@(#)libc-i386:sys/_sigaction.s	1.2"

/ SYS library -- _sigaction
/ error = _sigaction(sig, act, oact, handler);

	
	.file "_sigaction.s"
	
	.text

	.globl	__sigaction
	.globl	_cerror

_fgdef_(__sigaction):
	_prologue_
	MCOUNT
	movl	$SIGACTION,%eax
	_epilogue_
	lcall	$0x7,$0
	jc	_cerror
	ret
