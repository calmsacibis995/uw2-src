/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)fdisk:i386at/cmd/fdisk/bootstrap.s	1.2"
	.ident  "$Header: $"

	.file	"bootstrap.s"


	.text	
	.set	BOOT_DRIVE, 0x80
	.set	ACTIVE_DRIVE, 128
	.set	BOOTSTRAP_SIZE, 446

	.set	PART_ENTRY_SIZE, 15
	.set	SECTOR_SIZE, 512

	.set	ORIGINAL_CODE, 0x7C00
	.set	SANITY_WORD, ORIGINAL_CODE + 510
	.set	BOOTSTRAP_CODE, 0x600
	.set	BOOTIND, 0
	.set	PARTITION_TABLE, BOOTSTRAP_CODE + BOOTSTRAP_SIZE
	.set	BOOTSTRAP_STACK, ORIGINAL_CODE - 4
	.set	VARIABLE_BASE, ORIGINAL_CODE
	.set	SPT, 0
	.set	SPC, 4
	.set	RETRIES, 3			/ max I/O error retries.
	.set	RELSECT, 8			/ 

	.globl	spc
	.globl	spt
	.globl	readsect
	.globl	ioerr
	.globl	testbios
	.globl	nopart
	.globl	hdperr
	.globl	sanityerr
	.globl	_start
	.globl	main
	.globl	dispmsg
	.align	4
ZERO:
_start:
	cli
	xor	%eax, %eax
	movw	%cs, %ax
	movw	%ax, %ss
	movw	%ax, %ds
	movw	%ax, %es
	movl	$BOOTSTRAP_STACK, %esp
	sti			/ allow interrupts
	mov	$ORIGINAL_CODE, %esi
	mov	$BOOTSTRAP_CODE, %edi
	mov	$SECTOR_SIZE, %ecx
	cld
	repz	
	smovb

	/	When we boot, the firmware reads in this code at address 0x7c00
	/	and jump there. This code is assembled assuming it starts
	/	at address 0x600. Now that we have just moved ourselves to 0x600
	/	let's continue execution there.

	mov	$[next_instruction - ZERO + BOOTSTRAP_CODE], %eax
	jmp	*%eax
	.align 4
next_instruction:
/enable_cirrus_video:
/	movb	$0x12, %ah
/	movb	$0x92, %bl
/	movb	$0x02, %al
/	int	$0x10
	data16
	addr16
	mov	$testbios, %esi
	.align 4
main:
	xor	%eax, %eax
	data16
	addr16
	call	dispmsg

					/ Get the boot drive parameters
	movb	$8, %ah			/ Call the BIOS to get drive parms
	movb	$BOOT_DRIVE, %dl	/ ...for drive 0
	int	$0x13
	jnc	gotparm			/ jump if no error

	data16
	addr16
	mov	$hdperr, %esi

	jmp	main
	.align 4
gotparm:
	xor	%eax, %eax
	movb	%cl, %al
	andb	$0x3f, %al		/ extract spt value
	shrb	$6, %cl			/ align cyl hi bits
	xchgb	%cl, %ch		/ make into a short
setparm:
	addr16
	mov	%eax, spt		/ and save it
	incb	%dh			/ convert track to 1 base
	addr16
	movb	%dh, tpc		/ save tracks per cyl(heads)
	mulb	%dh			/ make sectors per cyl
	addr16
	mov	%eax, spc		/ and save it

					
	movl	$PARTITION_TABLE, %esi	/ load primary bootstrap
	mov	$4, %ecx

	.align 4
check_active_part:	
	mov	%esi, %edi
	lodsb				/ get a byte into %al and 
					/ increment %esi by one
	cmpb	$ACTIVE_DRIVE, %al
	je	active_partition_found
	addl	$PART_ENTRY_SIZE, %esi
	dec	%ecx
	jnz	check_active_part

	data16
	addr16
	mov	$nopart, %esi

	jmp	main

	.align 4
active_partition_found:
	movl	%edi, %esi
	addr16
	data16
	movl	RELSECT(%esi), %eax
	addr16
	data16
	movl	%eax, readsect
	addr16
	mov	readsect,%eax		/ get the relative sector to read
	addr16
	mov	readsect+2,%edx		/ get the relative sector to read
	addr16
	div	spc			/ long divide to get cyl
	movw	%ax, %cx		/ cyl to cx
	xchgb	%cl, %ch		/ low number to high
	shlb	$6, %cl			/ align cyl upper 2 bits
	movw	%dx, %ax		/ remainder from divide to ax
	cltd				/ make a long in ax,dx
	addr16
	div	spt			/ divide to get track(head)
	movb	%al, %dh		/ to dh for int call
	orb	%dl, %cl		/ starting sector to cl
	incb	%cl			/ convert to 1 based
	movb	$BOOT_DRIVE, %dl	/ get boot drive number
	addr16
	data16
	mov	$ORIGINAL_CODE, %ebx	/ pickup the destination phys addr
	addr16
	movb	$1, %al			/ sectors to read
	movb	$2, %ah			/ read sectors function

	int	$0x13			/ BIOS disk support
	jnb	execute_pri_boot	/ retry if error
	movb	$0, %ah			/ reset controller
	int	$0x13
	data16
	addr16
	mov	$ioerr, %esi

	jmp	main

	.align 4
execute_pri_boot:
	addr16
	data16
	movl	$SANITY_WORD, %edi
	addr16
	data16
	movw	(%edi), %ax
	addr16
	data16
	movw	0xaa55, %bx
	addr16
	data16
	cmpw	%ax, %bx
	jz	sanity_check_failed	
	addr16
	data16
	movl	$BOOTSTRAP_CODE, %esi
	addr16
	data16
	movl	$ORIGINAL_CODE, %eax
	jmp	*%eax			/ execute primary bootstarp

	.align 4
sanity_check_failed:
	data16
	addr16
	mov	$sanityerr, %esi

	jmp	main


/	----------------------------------------------------
/ 	dispmsg:		put null-terminated string at si to console
/
	.align	4
	.globl	dispmsg
dispmsg:
	movb	$1, %bl		/ select a normal video attribute
dispchar:
	cld
	lodsb
	orb	%al,%al
	jz	done		/ zero is end of the string

	movb	$14, %ah	/ Write TTY function
	int	$0x10
	jmp	dispchar
done:
	ret			

/
/	program variables
/
	.align 4
readsect:	.long	0		/ starting sector to read (0 based)
bps:		.value	512		/ bytes per sector
spt:		.value	15		/ disk sectors per track
spc:		.value	15\*2		/ disk sectors per cylinder
tpc:		.value	2		/ tracks(heads) per cylinder

/
/	various	error messages
/

ioerr:		.string		"Boot read error\r\n"
nopart:		.string		"No active partition\r\n"
hdperr:		.string		"No HD params\r\n"
sanityerr:	.string		"sanity error\r\n"
testbios:	.string		"\r\n"
