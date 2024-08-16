.ident	"@(#)libc-i386:sys/forkall.s	1.2"

/ pid_t forkall()
/	forkall() sys call stub. Clones the calling LWP's process. 
/
/ Calling/Exit State:
/ 	%edx == 0 in parent process, %edx = 1 in child process.
/ 	%eax == pid of child in parent, %eax == pid of parent in child.

	.file	"forkall.s"

	.text

	.set	ERESTART,91

	.globl	_cerror

_fwdef_(`forkall'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$FORKALL,%eax
	lcall	$0x7,$0
	jc	.error
	testl	%edx,%edx
	jz	.parent
	xorl	%eax,%eax
.parent:
	ret
.error:
	cmpl	$ERESTART,%eax
	je	forkall
	jmp	_cerror
