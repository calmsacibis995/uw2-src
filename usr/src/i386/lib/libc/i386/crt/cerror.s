	.file	"cerror.s"
	.ident	"@(#)libc-i386:crt/cerror.s	1.7"

/ C return sequence which sets errno, returns -1.

/ _m4_define(`ERESTART', 0x5b)		/ 91 decimal: ERESTART
/ _m4_define(`EINTR', 0x4)		/  4 decimal: EINTR

	.globl	_cerror

_fgdef_(_cerror):
	_prologue_
	/
	/ If the error is ERESTART, then this is a system call that wants
	/ to EINTR out on ERESTART.  We replace ERESTART with EINTR.
	/
	cmpl	$0x5b, %eax
	jne	.L1
	movl	$0x4, %eax
.L1:
	/
	/ (*__thr_errno()) = err
	/
	pushl	%eax
	call	_fref_(__thr_errno)
	popl	%ecx
	movl	%ecx, (%eax)
	movl	$-1,%eax
	_epilogue_
	ret
