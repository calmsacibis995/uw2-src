/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.


/
/	 Copyrighted as an unpublished work.
/	 (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
/	 All rights reserved.
/	
/	 RESTRICTED RIGHTS
/	
/	 These programs are supplied under a license.  They may be used,
/	 disclosed, and/or copied only as permitted under such license
/	 agreement.  Any copy must contain the above copyright notice and
/	 this restricted rights notice.  Use, copying, and/or disclosure
/	 of the programs is strictly prohibited unless otherwise provided
/	 in the license agreement.

	.file	"priboot.s"

.ident	"@(#)stand:i386at/standalone/boot/at386/priboot.s	1.2.2.7"
	.ident  "$Header: $"

/ 	UNIX primary boot program

include(priboot.m4)

/ 	Here we are running where we are originally loaded:
/	cs = 0x0, ip = 0x7c00.

	.text	
	.set	BOOTSECTS,PROG_SECT+1	/ # of secondary boot sectors 	
	.set	RETRIES, 3		/ max I/O error retries.
	.set	SECBOOT_MEM_LOC, SECBOOT_ADDR   / secondary boot memory location
	.set	PRIBOOT_STACK, SECBOOT_ADDR	/ primary boot stack base
ifdef(`WINI',`
/ from fdisk.h
	.set	IPART,0x600+B_BOOTSZ
	.set	BOOTIND,0
	.set	RELSECT,8
	.set	IPART_SZ,16
	.set	BOOTDRV,BOOTDRV_HD
',`
	.set	BOOTDRV,BOOTDRV_FP
')

	.globl	_start
ZERO:
_start:
	cli
	jmp past_spt
	.align	4
drv_mark:
	.long	0		/ reserved space

past_spt:
	movw	%cs, %ax

	movw	%ds, %bx	/ Save ds for access to fdisk entry
	movw	%ax, %ds

	movw	%ax, %es
	movw	%ax, %ss	/ sanitize the stack segment register
	data16
	movl	$pstack+BOOT_STACKSIZ, %esp
	sti			/ allow interrupts

/	save ds:si entry address
	data16
	shll	$4, %ebx
	data16
	addl	%esi, %ebx
	addr16
	data16
	movl	%ebx, ZERO

/ 	Get the boot drive parameters

	movb	$8, %ah			/ Call the BIOS to get drive parms
	movb	$BOOTDRV, %dl		/ ...for drive 0
	int	$0x13
	jnc	gotparm			/ jump if no error
ifdef(`WINI',`
	data16
	mov	$hdperr, %esi
	jmp	fatal
',`
	addr16
	data16
	mov	$FDB_ADDR, %eax		/ get floppy parameter table
	mov	%eax, %edi
	shrw	$16, %ax		/ 32 bit shift
	movw	%ax, %es
	movb	$1, %dh			/ max head, 0 based
	xor	%eax, %eax
	addr16
	movb	%es:0x4(%edi), %al	/ sectors per track
	xor	%ecx, %ecx
	addr16
	movb	%es:0xB(%edi), %cl	/ max cylinders
	jmp	setparm
')

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

ifdef(`WINI',`
',`
/	This code checks if we have a lower density diskette in
/	a higher density drive eg: 1.44MB disk in a 2.88MB drive.
/	We do this by verifying sector 18 of track 0, cylibder 0.
/	If the verify succeeds, we have a 1.44Mb boot media, otherwise
/	we assume we have a 1.2Mb boot media. I would have liked to use
/	the verify with the Max_SPT returned by the get parameters logic,
/	then just look at how many sectors verified to determine the
/	real SPT for the media. However, there is not enough room in
/	this bootstrap to include a full tracks worth of address mark
/	information.

	pusha
	pushf
	push	%es

	clrl	%esi			/ This is needed for an old Olivetti
					/ with a B1 Stepping i386.
ifdef(`COMPAQ',`
',`
	data16
	addr16
	mov	adrmrk, %eax		/ pickup the buffer phys addr
	xor	%ebx,%ebx		/ prep
	addr16
	movb	%al, %bl		/ offset part
	andb	$0x0f, %bl		/ only a nibbles worth
	shrl	$4, %eax		/ paragraph part ....
	addr16
	movw	%ax, %es		/ ... to seg register es

	movb	$1, %al
	movb	$4, %ah			/ Call the BIOS to verify the sector
	movb	$18, %cl		/ Try for 18-SPT
	movb	$0, %ch			/ Track 0
	movb	$BOOTDRV, %dl		/ Boot drive
	movb	$0, %dh			/ Head 0

	int	$0x13			/ Ask BIOS to verify the sector
	jnc	spt18

	addr16
	mov	$15, spt		/ We assume 15-SPT if not 18-SPT
	addr16
	mov	$30, spc		/ Thus 30-SPC
	jmp	sptdone
')
spt18:
	addr16
	mov	$18, spt		/ Found 15-SPT
	addr16
	mov	$36, spc		/ Thus 36-SPC
sptdone:
	pop	%es
	popf
	popa
')
ifdef(`WINI',`
/	BIOS entry address pointed by ds:si specifies the fdisk partition
/	that was used to locate this primary boot program.
/	(1) check for valid ds:si entry address. If valid, use that as
/	    active partition address. Otherwise, reset to zero.
/	(2) record the active partition address; zero indicates no active
/	    partition.

	addr16
	data16
	movl	ZERO, %esi			/ address for ds:si entry
	data16
	xorl	%ebx, %ebx			/ default active partition addr
	addr16
	data16					/ a full 32 bit address for
	movl	$IPART, %eax			/ partition table address
	addr16
	data16
	mov	$B_FD_NUMPART, %ecx		/ number of fdisk partitions
ostry:
	addr16
	cmpb	$B_ACTIVE, BOOTIND(%eax)	/ check active partition
	jne	not_actpart			/ branch if not active partition
	data16
	movl	%eax, %ebx			/ save active partition
not_actpart:
	data16
	cmpl	%eax, %esi			/ check for valid ds:si entry 
	jne	nomatch				/ branch if not match
	data16
	movl	%esi, %ebx		/ get saved ds:si entry location
	jmp	osfound			
nomatch:	
	addr16
	data16
	addl	$IPART_SZ, %eax			/ next partition
	loop	ostry

bad_entry:
	data16
	xorl	%edx, %edx			/ set invalid flag for ds:si 
	addr16
	data16
	movl	%edx, ZERO		/ set invalid ds:si entry address
	data16
	testl	%ebx, %ebx		/ if valid active partition exists
	jnz	osfound			/ then found active partition	

        data16				
        movl    $nopart, %esi           / error: no active partition found
        data16
        jmp     fatal

osfound:
	addr16
	data16
	movl	RELSECT(%ebx), %eax	/ save relative sector number
	data16
	addr16
	movl	%eax, part_start
	data16
	inc	%eax		/ plus one to get over primary
	data16
	addr16
	movl	%eax, readsect
')

/ 	call the BIOS to read the remainder of the bootstrap from disk
/	all the data is set in the diskblock

ifdef(`WINI',`

	data16
	call	RDdisk			/ do the i/o
',`
	data16
	mov	$BOOTSECTS, %ecx
doiof:
	push	%ecx
	data16
	call	RDdisk			/ read a floppy sector
	addr16
	mov	bps, %eax		/ get bytes per sector (16 bits)
	data16
	addr16
	add	%eax, destadrs		/ advance read address (32 bits)
	data16
	addr16
	inc	readsect		/ bump the sector to read
	pop	%ecx
	loop	doiof
')

/	pass data to the secondary boot program through stack
setstack:
	data16
	xorl	%eax, %eax		/ clear eax
	addr16
	data16
	movl	part_start, %eax	/ start of active partition
	data16
	pushl	%eax

	addr16
	mov	bps, %eax		/ bytes per sector
	data16
	pushl	%eax

	addr16
	movl	spt, %eax		/ sectors per track
	data16
	pushl	%eax

	addr16
	movl	spc, %eax		/ sector per cylinder
	data16
	pushl	%eax

	data16
	movl	$SECBOOT_MEM_LOC, %eax	/ load address for sec boot program
	data16
	pushl	%eax

/	make an indirect jump to the secondary boot program
/	%eax got the address of the secondary boot program
	addr16				/ add bps as word to ax
	addl	bps, %eax		/ skip 1 sector for boot control blk
	data16
	addr16
	call	*%eax

/	no return here

/	----------------------------------------------------
/
/	Stop everything, an error occured.
/
fatal:
	data16
	call	strput			/ print error message
					/ fall through to...
halt:	sti				/ allow int's
	hlt				/ stop.
	jmp	halt			/ STOP.

/	----------------------------------------------------
/ 	strput:		put null-terminated string at si to console
/
strput:
	movb	$1, %bl		/ select a normal video attribute
stploop:
	cld			/ make sure we are going the right dir
	lodsb			/ pickup a message byte
	orb	%al,%al
	jz	pexit		/ zero is end of the string

	movb	$14, %ah	/ Write TTY function
	int	$0x10
	jmp	stploop
okread:
pexit:
	data16
	ret			


/	----------------------------------------------------
/
/	RDdisk makes bios calls to read from the boot drive based
/	on the information in the disk parameters block.
/	Here we are in real mode.
/	the segment:offset destination.  secno is 0 for the
/	first sector on the disk.  count is the number of
/	sectors to read.
/
RDdisk:	
	movb	$RETRIES, %cl		/ set retry counter
retry:
	push	%ecx			/ save the retry counter
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
	movb	$BOOTDRV, %dl		/ get boot drive number
	addr16
	data16
	mov	destadrs, %eax		/ pickup the destination phys addr
	xor	%ebx,%ebx		/ prep
	movb	%al, %bl		/ offset part
	andb	$0x0f, %bl		/ only a nibbles worth
	shrl	$4, %eax			/ paragraph part
	movw	%ax, %es		/ to seg register es
	addr16
	movb	numsect, %al		/ sectors to read
	movb	$2, %ah			/ read sectors function

	int	$0x13			/ BIOS disk support
	pop	%ecx

	jnb	okread			/ retry if error
	cmpb	0x11, %ah		/ ECC corrected
	je	okread			/ jump yes

	push	%ecx
	movb	$0, %ah			/ reset controller
	int	$0x13
	pop	%ecx
	decb	%cl
	jnz	retry

ifdef(`WINI',`
nopart:		.string		"No active partition on HD\r\n"
hdperr:		.string		"No HD params\r\n"
',`

')


	.align	4

/
/	disk I/O stuff, used to pass info for disk parameters and
/	I/O requests.
/
destadrs:	.long	SECBOOT_MEM_LOC	/ address(physical) for next read
readsect:	.long	1		/ starting sector to read (0 based)
part_start:	.long	0		/ starting sector for active partition

ifdef(`WINI',`
numsect:	.value	BOOTSECTS
',`
numsect:	.value	1		/ read 1 sector at a time for floppy
')

/	The following values are floppy defaults in case the BIOS does
/	not properly support the "read disk parameters" function.

bps:		.value	512		/ bytes per sector
spt:		.value	15		/ disk sectors per track
spc:		.value	15\*2		/ disk sectors per cylinder
tpc:		.value	2		/ tracks(heads) per cylinder

/ The loader of this bootstrap (ROM BIOS or "fdisk" boot) expects a
/ 0x55AA pattern to indicate a valid bootstrap.  For the hard disk,
/ this pattern is expected right at the end of the sector (offset 510).
/ For the floppy, it is expected at offset 506 (the remaining 4 bytes
/ are reserved for use by the ROM).
ifdef(`WINI',`
	. = ZERO + 510
',`
	. = ZERO + 506
')
dskmark:
	.byte	0x55
	.byte	0xAA

ifdef(`WINI',`
',`
adrmrk:
	.byte	0
	.byte	0
	.byte	18
	.byte	2
')
/ allocated stack area
	.align	4
	.comm	pstack, BOOT_STACKSIZ

