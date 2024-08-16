.ident	"@(#)libc-i386:sys/syncmbox.s	1.1"

	.file	"syncmbox.s"

	.text

	.globl	_cerror

_fwdef_(`sync_mailbox'):
	MCOUNT			/ subroutine entry counter if profiling
	movl	$SYNC_MAILBOX,%eax
	lcall	$0x7,$0
	jc	_cerror
	ret
