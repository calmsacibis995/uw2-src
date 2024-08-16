/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:io/ddi_misc.s	1.10"
	.ident	"$Header: $"

/	ddi routines

include(../svc/asm.m4)
include(assym_include)

/
/ The following routines read and write I/O address space:
/	inb, inl, inw, outb, outl, outw
/

/
/ uchar_t
/ inb(int port)
/	read a byte from an 8 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(inb)
	movl	SPARG0, %edx
	subl    %eax, %eax
	inb	(%dx)
	ret
	SIZE(inb)

/
/ ulong_t
/ inl(int port)
/	read a 32 bit word from a 32 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(inl)
	movl	SPARG0, %edx
	inl	(%dx)
	ret
	SIZE(inl)

/
/ ushort_t
/ inw(int port)
/	read a 16 bit short word from a 16 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(inw)
	movl	SPARG0, %edx
	subl	%eax, %eax
	data16
	inl	(%dx)
	ret
	SIZE(inw)

/ void
/ outb(int port, uchar_t data)
/	write a byte to an 8 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(outb)
	movl	SPARG0, %edx
	movl	SPARG1, %eax
	outb	(%dx)
	ret
	SIZE(outb)

/ void
/ outl(int port, ulong_t data)
/	write a 32 bit long word to a 32 bit I/O port
/
/ Calling/Exit State:
/	none
ENTRY(outl)
	movl	SPARG0, %edx
	movl	SPARG1, %eax
	outl	(%dx)
	ret
	SIZE(outl)

/ void
/ outw(int port, ushort_t data)
/	write a 16 bit short word to a 16 bit I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(outw)
	movl	SPARG0, %edx
	movl	SPARG1, %eax
	data16
	outl	(%dx)
	ret
	SIZE(outw)

/
/ The following routines move data between buffer and I/O port:
/	repinsb, repinsd, repinsw, repoutsb, repoutsd, repoutsw
/

/
/ void
/ repinsb(int port, uchar_t *addr, int cnt)
/	read bytes from I/O port to buffer
/
/ Calling/Exit State:
/	none
/
ENTRY(repinsb)
	pushl	%edi

	movl	4+SPARG0, %edx
	movl	4+SPARG1, %edi
	movl	4+SPARG2, %ecx		/ is BYTE count

	rep
	insb

	popl	%edi
	ret
	SIZE(repinsb)

/
/ void
/ repinsd(int port, ulong_t *addr, int cnt)
/	read 32 bit words from I/O port to buffer
/
/ Calling/Exit State:
/	none
/
ENTRY(repinsd)
	pushl	%edi

	movl	4+SPARG0, %edx
	movl	4+SPARG1, %edi
	movl	4+SPARG2, %ecx		/ is DWORD count

	rep
	insl

	popl	%edi
	ret
	SIZE(repinsd)

/
/ void
/ repinsw(int port, ushort_t *addr, int cnt)
/	read 16 bit words from I/O port to buffer
/
/ Calling/Exit State:
/	none
/
ENTRY(repinsw)
	pushl	%edi

	movl	4+SPARG0, %edx
	movl	4+SPARG1, %edi
	movl	4+SPARG2, %ecx		/ is SWORD count

	rep
	data16
	insl

	popl	%edi
	ret
	SIZE(repinsw)

/
/ void
/ repoutsb(int port, uchar_t *addr, int cnt)
/	write bytes from buffer to an I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(repoutsb)
	pushl	%esi

	movl	4+SPARG0, %edx
	movl	4+SPARG1, %esi
	movl	4+SPARG2, %ecx		/ is BYTE count

	rep
	outsb

	popl	%esi
	ret
	SIZE(repoutsb)

/
/ void
/ repoutsd(int port, ulong_t *addr, int cnt)
/	write 32 bit words from buffer to an I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(repoutsd)
	pushl	%esi

	movl	4+SPARG0, %edx
	movl	4+SPARG1, %esi
	movl	4+SPARG2, %ecx		/ is DWORD count

	rep
	outsl

	popl	%esi
	ret
	SIZE(repoutsd)

/
/ void
/ repoutsw(int port, ushort_t *addr, int cnt)
/	write 16 bit words from buffer to an I/O port
/
/ Calling/Exit State:
/	none
/
ENTRY(repoutsw)
	pushl	%esi

	movl	4+SPARG0, %edx
	movl	4+SPARG1, %esi
	movl	4+SPARG2, %ecx		/ is SWORD count

	rep
	data16
	outsl

	popl	%esi
	ret
	SIZE(repoutsw)


/
/ void
/ _tenmicrosec(void)
/	Generate a ten microsecond delay.
/
/ Calling/Exit State:
/	none
/
/ Remarks:
/	microdata is defined and initialized in platform-specific code.
/	See pit.c for AT platform, clock_mdep.c for SYM platform.
/
ENTRY(_tenmicrosec)
	movl	microdata, %ecx		/ Loop uses ecx.
microloop:
	loop	microloop
	ret
	SIZE(_tenmicrosec)

/
/ int
/ intr_disable(void)
/
/ Calling/Exit State:
/	None.
/
/ Remarks:
/	Disable all interrupts, with minimal overhead.
/	Returns the previous value of the EFLAGS register
/	for use in a subsequent call to intr_restore.
/	Normally an inline asm version of intr_disable
/	is used; this function version exists for the
/	benefit of callers who are unable to include
/	inline.h, or for implementations not supporting
/	inline asm functions.
/
ENTRY(intr_disable)
        pushfl
        cli
        popl    %eax
        ret
	SIZE(intr_disable)

/
/ void
/ intr_restore(int efl)
/
/ Calling/Exit State:
/	Argument is the EFLAGS value from previous call to intr_disable.
/
/ Remarks:
/	Restore interrupt enable state, with minimal overhead.
/
ENTRY(intr_restore)
        pushl   4(%esp)
        popfl
        ret
	SIZE(intr_restore)
