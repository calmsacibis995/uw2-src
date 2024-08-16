/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.file	"ee16asm.s"
	.ident	"@(#)kern-i386at:io/dlpi_ether/ee16/ee16asm.s	1.2"
	.ident	"$Header: $"

include(KBASE/svc/asm.m4)
include(assym_include)

	.set	DXREG, 0
	.set	WRPTR, 2
	.set	RDPTR, 4
	.set	AUTOID, 0x0f

/
/	read_word(ushort src, ushort bio)
/
	.align	4

ENTRY(ee16read_word)
	pushl	%edx
/
/	set base io in %dx, make an io from AUTOID to avoid race condition
/
	movw	0x0c(%esp),%dx
	addw	$AUTOID, %dx
	inb	(%dx)
/
/	set suorce word offset in %eax and write to register RDPTR
/
	subw	$0x0b,%dx
	movw	0x08(%esp),%ax
	outw	(%dx)
/
/	input the value	from DXREG which is the base io addr
/
	subw	$0x04,%dx
	inw	(%dx)
	popl	%edx
	ret

/
/	write_word(dst, value, bio)
/
	.align	4

ENTRY(ee16write_word)
	pushl	%edx
/
/	set base io in %dx, make an io from AUTOID to avoid race condition
/
	movw	0x10(%esp),%dx
	addw	$AUTOID,%dx
	inb	(%dx)
/
/	set destination offset in %ax and write to register WRPTR
/
	movw	0x08(%esp),%ax
	subw	$0x0d,%dx
	outw	(%dx)
/
/	set the output value in %ax and write to DXREG
/
	movw	0x0c(%esp),%ax
	subw	$0x02,%dx
	outw	(%dx)

	popl	%edx
	ret

/
/	bcopy_from_buffer(dst, src, count, base_io)
/
	.align	4

ENTRY(ee16bcopy_from_buffer)
	pushl	%ecx
	pushl	%edx
	pushl	%edi
/
/	set base io in %dx and make an io from AUTOID to avoid race condition
/
	movw	0x1c(%esp),%dx
	addw	$AUTOID,%dx
	inb	(%dx)
/
/	write source board buffer offset to register RDPTR
/
	movw	0x14(%esp),%ax
	subw	$0x0b,%dx
	outw	(%dx)
/
/	set up destination host buffer in %edi and byte count in %ecx
/
	subw	$0x04,%dx
	movl	0x10(%esp),%edi
	movl	0x18(%esp),%ecx
	pushl	%ecx
/
/	get short word count, and set for incrementing of %edi
/
	shrl	$1, %ecx
	jz	byte_1
	cld
r_1:
	inw	(%dx)
	stosw
	loop	r_1
/
/	check for odd byte count
/
byte_1:
	popl	%ecx
	andl	$1,%ecx
	jz	done_1
	inb	(%dx)
	stosb
done_1:
	popl	%edi
	popl	%edx
	popl	%ecx
	ret

/
/	bcopy_to_buffer(src, dst, count, base_io)
/
	.align	4

ENTRY(ee16bcopy_to_buffer)
	pushl	%ecx
	pushl	%edx
	pushl	%esi
/
/	load base io into %dx and read from AUTOID to avoid race condition
/
	movw	0x1c(%esp),%dx
	addw	$AUTOID,%dx
	inb	(%dx)
/
/	write destination board buffer offset to register WRPTR
/
	movw	0x14(%esp),%ax
	subw	$0x0d,%dx
	outw	(%dx)
/
/	set up source host buffer in %esi and byte count in %ecx
/
	subw	$0x02,%dx
	movl	0x10(%esp),%esi
	movl	0x18(%esp),%ecx
	pushl	%ecx
/
/	get short word count, and set for incrementing of %esi
/
	shrl	$1, %ecx
	jz	byte_2
	cld
r_2:
	lodsw
	outw	(%dx)
	loop	r_2
/
/	check for odd byte count
/
byte_2:
	popl	%ecx
	andl	$1,%ecx
	jz	done_2
	lodsb
	outb	(%dx)
done_2:
	popl	%esi
	popl	%edx
	popl	%ecx
	ret
