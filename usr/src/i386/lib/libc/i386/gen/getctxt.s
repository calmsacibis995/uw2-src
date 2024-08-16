#ident	"@(#)libc-i386:gen/getctxt.s	1.6"

	.file	"getctxt.s"

	.globl	_cerror

_m4_define_(`GETCONTEXT', `0')
/ UR_foo macros are offset into ucontext_t of register foo
_m4_define_(`UR_EAX', `80')
_m4_define_(`UR_EIP', `92')
_m4_define_(`UR_ESP', `104')


/ This implementation of getcontext() attempts to get every register
/ stored properly in the ucontext structure, including the ones that
/ aren't strictly meaningful.  These "scratch" registers don't really
/ need to be saved since they have undefined values upon return from
/ getcontext(), but they might be useful for debugging.

_fwdef_(`getcontext'):
	pushl	%eax
	pushl	%edx
	MCOUNT
	popl	%edx
	movl	8(%esp),%eax
	pushl	%eax		/ push ucp arg
	pushl	$GETCONTEXT	/ push flag arg
	pushl	$0		/ push dummy return address

	/ at this point, all registers contain the caller's values
	/ except for EAX, EIP, and ESP.
	movl	$UCONTEXT,%eax
	lcall	$0x7,$0		/ make syscall
	popl	%eax		/ discard dummy return address
	popl	%eax		/ discard flag arg
	popl	%eax		/ get ucp in EAX
	popl	%edx		/ caller's EAX value in EDX
	jc	_cerror

	/ Now we patch up the three incorrect registers
	movl	%edx,UR_EAX (%eax) / patch EAX value
	popl	%edx		   / get return address (EIP)
	movl	%edx,UR_EIP (%eax) / patch EIP value
	movl	%esp,UR_ESP (%eax) / patch ESP value
	movl	$0,%eax
	jmp	*%edx
