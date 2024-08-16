/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:net/inet/in_cksum.s	1.4"
	.ident	"$Header: $"
	.file	"net/inet/in_cksum.s"

/
/	STREAMware TCP
/	Copyright 1987, 1993 Lachman Technology, Inc.
/	All Rights Reserved.
/ 

/
/	Copyright (c) 1982, 1986, 1988
/	The Regents of the University of California
/	All Rights Reserved.
/	Portions of this document are derived from
/	software developed by the University of
/	California, Berkeley, and its contributors.
/

/
/	System V STREAMS TCP - Release 5.0
/
/   Copyright 1990 Interactive Systems Corporation,(ISC)
/   All Rights Reserved.
/
/	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
/	All Rights Reserved.
/
/	The copyright above and this notice must be preserved in all
/	copies of this source code.  The copyright above does not
/	evidence any actual or intended publication of this source
/	code.
/
/	This is unpublished proprietary trade secret source code of
/	Lachman Associates.  This source code may not be copied,
/	disclosed, distributed, demonstrated or licensed except as
/	expressly authorized by Lachman Associates.
/
/	System V STREAMS TCP was jointly developed by Lachman
/	Associates and Convergent Technologies.
/
/ SCCS IDENTIFICATION
/	in_cksum computes and returns the one's complement of the
/	16-bit sum of len bytes of data in the streams message blocks
/	headed by mp.
/
/	*** i386 version ***
/
/   Checksum routine for Internet Protocol Headers
/
/   unsigned short int
/   in_cksum (mp, len)
/       mblk_t   *mp;	/* Ptr to 1st message buffer		*/
/       int       len;  /* Length of data			*/
/
/	ebx holds the offset modulo 4 of the last byte in the previous mblock
/	ecx holds the number of bytes in the current mblock
/	edx	holds the current value of the checksum
/	esi holds the current location in the current mblock
/	
	.text
	.align	4
	.globl	in_cksum

in_cksum:
	pushl	%ebp		/ set up stack frame
	movl	%esp,%ebp
	subl	$4, %esp	/ reserve local variable, tmp
	pushl	%ebx		/ save registers
	pushl	%esi
	pushl	%edi
	xorl	%edx,%edx	/ zero checksum
	xorl	%ebx,%ebx	/ zero previous count
	leal	8-8(%ebp),%edi	/ initialize block pointer (b_cont)

.nextblock:
	cmpl	$0,12(%ebp)	/ if (message length <= 0) then exit
	jle	.exit
	movl	8(%edi),%edi	/ if (block pointer = 0) then exit
	testl	%edi,%edi
	jz	.exit
	movl	12(%edi),%esi	/ get read pointer (b_rptr)
	movl	16(%edi),%ecx	/ compute block length (b_wptr - b_rptr)
	subl	%esi,%ecx
	jle	.nextblock	/ if (block length <= 0) then get next block
	subl	%ecx,12(%ebp)	/ message length - block length
	jge	.firstfew	/ if (message length >= block length) then OK
	addl	12(%ebp),%ecx	/ else adjust block length

.firstfew:
	negl	%ebx		/ if (previous block ended on 4 byte count)
	jz	.next64test	/ then get next 4 bytes
	pushl	%ecx		/ save current byte count
	pushl	%ebx		/ save current offset
	cmpl	%ecx,%ebx	/ is byte count > offset?
	jge		.ff			/ no, just move the byte count
	movl	%ebx,%ecx	/ yes, move offset bytes
.ff:
	movl	$0,-4(%ebp)	/ zero tmp storage
	leal	-4(%ebp),%ebx
.ff1:
	cmpl	$0,%ecx
	je		.ff2
	movb	(%esi),%al	/ copy byte to tmp
	movb	%al,(%ebx)
	incl	%esi		/ bump b_rptr
	incl	%ebx		/ bump tmp ptr
	decl	%ecx		/ decrement move count
	jmp		.ff1
.ff2:
	popl	%ebx		/ restore offset
	popl	%ecx		/ restore byte count
	movl	-4(%ebp),%eax	/ move tmp into eax
	andl	.mask(,%ebx,4),%eax
	pushl	%ecx
	movb	.shft(%ebx),%cl
	rorl	%cl,%eax
	popl	%ecx
 	subl	%ebx,%ecx	/ block length - first few (< 4)
	jle	.lastfew2	/ end of block
	addl	%eax,%edx
	adcl	$0,%edx
	xorl	%ebx,%ebx	/ zero last count

.next64test:
	movl	%ecx,%eax	/ compute number of 64 byte blocks
	andl	$0x3f,%ecx
	shrl	$6,%eax
	jz	.last64
.next64:
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	addl	(%esi),%edx	/ add next 4 bytes to checksum
	adcl	$0,%edx
	addl	$4,%esi		/ data pointer + 4
	decl	%eax
	jg	.next64
	jl	.lastfew
.last64:
	movl	%ecx,%ebx	/ compute size of last block (< 64 bytes)
	andl	$0x3,%ecx
	shrl	$2,%ebx
	jz	.lastfew
	jmp	*.jump(,%ebx,4)

.lastfew:
	pushl	%ecx		/ save byte count
	movl	$0,-4(%ebp)	/ zero tmp storage
	leal	-4(%ebp),%ebx	/ set up pointer to tmp storage
.lf:
	cmpl	$0,%ecx		/ check for 2 or 3 bytes (4 done previously)
	je		.lf2	/ only 1 byte to move
	movb	(%esi),%al	/ copy byte to temp storage
	movb	%al,(%ebx)
	incl	%esi		/ bump b_rptr
	incl	%ebx		/ bump tmp pointer
	decl	%ecx		/ decrement byte count
	jmp		.lf
.lf2:
	popl	%ecx		/ restore byte count
	movl	-4(%ebp),%eax
	subl	$4,%ecx		/ generate offset for next block
.lastfew2:
	movl	%ecx,%ebx	/ set last count
	andl	.mask+16(,%ebx,4),%eax
	addl	%eax,%edx
	adcl	$0,%edx
	jmp	.nextblock

.exit:
	cmpl	$0,12(%ebp)	/ if (message length = 0) then OK
	jle	.exit2
	pushl	%edx		/ save checksum
	pushl	$.msg		/ else print error message
	call	printf
	popl	%eax
	popl	%edx		/ restore checksum
.exit2:
	movl	%edx,%eax	/ form a 16 bit checksum by
	shrl	$16,%eax	/ adding two halves of 32 bit checksum
	addw	%dx,%ax
	adcw	$0,%ax
	notw	%ax		/ return 1's complement of checksum
	andl	$0xffff,%eax
	popl	%edi		/ restore registers
	popl	%esi
	popl	%ebx
	leave
	ret

	.data
	.align	4

.mask:
	.long	0x00,0xff,0xffff,0xffffff,0xffffffff,0x00

.shft:
	.byte	0,8,16,24,0

.msg:
	.string	"in_cksum: out of data\n"

.jump:
	.long	.next64+128,.next64+120,.next64+112,.next64+104
	.long	.next64+96,.next64+88,.next64+80,.next64+72
	.long	.next64+64,.next64+56,.next64+48,.next64+40
	.long	.next64+32,.next64+24,.next64+16,.next64+8
