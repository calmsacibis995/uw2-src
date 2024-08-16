/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386at:svc/pstart.s	1.22"
	.ident	"$Header: $"
	.file	"svc/pstart.s"

include(../svc/asm.m4)
include(assym_include)

	.set	KCS32, 0x08
	.set	KDS, 0x10
	.set	KCS16, 0x18
	.set	KDS64K, 0x20
	.set	STARTSTK, 0x1000
	.set	PROTMASK, 0x1                     /* MSW protection bit */
	.set	NOPROTMASK, 0xfffffffe
	.set	RESET_STKSIZ, 32	/ size of stack needed by reset_code

/
/ void
/ reset_code(void)
/
/	This code is not really a C callable procedure but instead
/	is the first kernel code executed after the hardware input
/	RESET is de-asserted.
/
/ Calling State/Exit State:
/
/	This function is only entered in response to another engine
/	executing online_engine().
/
/	online_engine() puts the physical address of our level 1 page
/	table into online_kl1pt and the logical engine number (engine[]
/	index) for our engine into online_engno.  We get these values by
/	knowing that these variables are in static data memory with the
/	virtual address equal to physical address.
/
/       This function never returns since there is no context to return
/       to, but jumps to `_start' at the end of the code.  We pass the
/       logical engine number (engine[] index) of this engine to `_start'
/       in %edx.
/
/ Description:
/
/       The 80386/80486 family start executing in REAL-MODE (16-bit mode)
/       after RESET is removed.  This code is run in REAL-MODE and turns
/       on protected mode (segmentation only, no paging) so that the rest
/       of the system can be brought up.  The segment setup is BASE equal 0
/       with the limit set to 4 Gig.  This turns the 386/486 into a 32-bit
/       non-paged processor.
/
/       Next, the engine table structure is searched looking for our entry.
/       If found, we pick up our page table base value and turn on paging.
/	Once mapping is one, we have full access to all kernel resources.
/
/	One major motivation for turning on protection immediately is
/	to be able to access greater than 1 MB addresses, and to minimize
/	assembly language programming in 16-bit mode (the assembler does
/	not really support 16-bit mode, one must fake it).
/
/ Remarks:
/
/       The code is executed only once per processor on AT-MP systems because
/       the offline/online sequence merely asserts HOLD to the processor
/       but does not reset it.  Also, the boot processor (usually processor
/       zero) does not execute this code since it already is running in
/       32 bit mode when started by the /boot code (i.e., /boot starts /unix
/       by jumping to the symbol "start").
/
/       The location of the gdt table in this file must not move as it
/       exactly matches the "/boot" version which is overlayed in the
/       process of loading the kernel.
/
/       This code makes the assumption that physical == virtual address.
/       If this notion is broken, then this code must be revisited.
/

	.align	4096			/ newer cpu's require this for 
					/ startup ipi!
ENTRY(reset_code)
        movl    %cr0, %eax              / Find out which mode
        testb   $_A_CR0_PE, %al         / test PE bit
        jz      reset_real
        / Protect mode, access relative to eip until we get our GDT
        call    reset_prot
reset_prot:
        popl    %eax
        lgdt    %cs:[GDTptr - reset_prot](%eax)
        ljmp    $KCS32, $.next

reset_real:
	xor	%eax, %eax
	movw	%ax, %ds
	/ Load the GDTR
	addr16				/ override 16-bit addr addressing
	data16				/ override 16-bit data addressing
	lgdt	GDTptr			/ access rel. to start until we 
					/ get our GDT
	movl    %cr0, %ebx		/ get CR0 value
	orb     $_A_CR0_PE, %bl		/ turn on PE bit
	movl    %ebx, %cr0		/ turn on protected 32-bit mode
	data16				/ Flush prefetch q by doing an inter-
	ljmp	$KCS32, $.next		/ segment jump, which also loads %cs

.next:
	movw	$KDS, %ax		/ select GDT entry for 32-bit data seg
	movw    %ax, %es		/ load misc segment registers
	movw    %ax, %ds
	movw    %ax, %ss
	/
	/ VIRT = PHYS for the entire 4 Gig virtual address space.
	/
	/ online_engine() puts the physical address of our level 1 page table
	/ into online_kl1pt.  We can access online_kl1pt because everything
	/ in this file has physical == virtual.
	/
	movl	online_kl1pt, %eax	/ get physical address of my kl1pt
	movl	%eax, %cr3              / write page table root register
	movl	$stext, %ecx		/ find pde mapping stext
	shrl	$22, %ecx		/ compute page directory index
	movl	(%eax, %ecx, 4), %ecx	/ %ecx = pde value
	testl	$0x80, %ecx		/ see if pde has PS bit on
	je	.loadcr0		/ PS bit off, just load cr0
	/
	/	if this cpu supports PSE then
	/		set the bit in cr4
	/	else
	/		convert page tables to non PS page tables
	/

	/ first, need stack to call pse_supported
	movl	$.reset_stkbase + RESET_STKSIZ, %esp
	call	pse_supported
	orl	%eax, %eax
	je	.problems
	movl	%cr4, %eax
	orl	$_A_CR4_PSE, %eax
	movl	%eax, %cr4
	
.loadcr0:
	movl	%cr0, %eax              / get current cr0 value
	orl     $_A_CR0_PG, %eax	/ set desired bits
	movl	%eax, %cr0		/ turn on mapping
	movl	$ueng, %ebp		/ running mapped so ...
	movl	%ebp, %esp		/ set stack pointer to engine stack
	/
	/ online_engine() puts the logical engine number (engine[] index)
	/ for our engine into online_engno.
	/
	movl	online_engno, %edx	/ get logical engine number for _start
	jmp	_start			/ continue startup

/ what to do if PSE support needed, but none available
.msg:
	.string	"\r\nPSE not supported on this processor\r\n"
.problems:
	pushl	$.msg
	call	psm_phys_puts
	jmp	.
	

SIZE(reset_code)
/ Stack for calling pse_supported
.align	4
.reset_stkbase:	.zero	RESET_STKSIZ

/
/ void
/ start(void)
/
/	This code is not really a C callable procedure but instead
/	is the first kernel code executed after the kernel is loaded
/	in memory by /boot.
/
/ Calling State/Exit State:
/
/	This function never returns since there is no context to return
/	to, but jumps to `_start' at the end of the code.
/
/ Description:
/
/	Since this is the first code executed by the kernel, we are
/	running 32-bit mode with PHYS == VIRT for 0-4 GIG.  We set
/	our stack pointer to a temporary location, call init_mmu()
/	to build us some temporary page tables in what should be unused
/	memory and to turn on paging, and then jump to _start.
/
/	When we first enter, we're using the code and data segments set up
/	by the bootstrap, which are both based at "start".  Since our
/	virtuals are relative to 0, we switch to our own GDT.
/

ENTRY(start)

	/ access relative to eip until we get our GDT
	call	.push_eip
.push_eip:
	popl	%eax
	lgdt	%cs:[GDTptr - .push_eip](%eax)
	ljmp	$KCS32, $start2

	.align	8
GDTstart:
nulldesc:			/ offset = 0x0

	.value	0x0	
	.value	0x0	
	.byte	0x0	
	.byte	0x0	
	.byte	0x0	
	.byte	0x0	

kcs32desc:			/ offset = 0x8

	.value	0xFFFF		/ segment limit 0..15
	.value	0x0000		/ segment base 0..15
	.byte	0x0		/ segment base 16..23
	.byte	0x9A		/ flags; R=1, A=0, Type=110, DPL=00, P=1
	.byte	0xCF		/ flags; Limit (16..19)=0xF, AVL=0, G=1, D=1
	.byte	0x0		/ segment base 24..31

kdsdesc:			/ offset = 0x10

	.value	0xFFFF		/ segment limit 0..15
	.value	0x0000		/ segment base 0..15
	.byte	0x0		/ segment base 16..23
	.byte	0x92		/ flags; A=0, Type=001, DPL=00, P=1
	.byte	0xCF		/ flags; Limit (16..19)=0xF, AVL=0, G=1, B=1
	.byte	0x0		/ segment base 24..31

kcs16desc:			/ offset = 0x18

	.value	0xFFFF		/ segment limit 0..15
	.value	0x0000		/ segment base 0..15
	.byte	0x0		/ segment base 16..23
	.byte	0x9A		/ flags; R=1, A=0, Type=110, DPL=00, P=1
	.byte	0x0		/ flags; Limit (16..19)=0, AVL=0, G=0, D=0
	.byte	0x0		/ segment base 24..31

kds64kdesc:			/ offset = 0x20

	.value	0xFFFF		/ segment limit 0..15
	.value	0x0000		/ segment base 0..15
	.byte	0x0		/ segment base 16..23
	.byte	0x92		/ flags; A=0, Type=001, DPL=00, P=1
	.byte	0x40		/ flags; Limit (16..19)=0, AVL=0, G=0, B=1
	.byte	0x0		/ segment base 24..31

	.set	gdtsize,.-GDTstart

	/ In-memory GDT pointer for the lgdt call

	.globl	GDTptr
	.type	GDTptr,@object
GDTptr:	
gdtlimit:
	.value	gdtsize - 1
gdtbase:
	.long	GDTstart
	.size	GDTptr,.-GDTptr

	.align	8
start2:
	movw	$KDS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movl	$STARTSTK, %esp		/ set temp stack pointer

ifdef(`PHYS_DEBUG',`
        call    dbg_puts
        .string "\r\nEntered kernel startup at start2.\r\n"
')

ifdef(`PHYS_DEBUG',`
        call    dbg_puts
        .string "\r\ninit_mmu.\r\n"
')
	call	psm_pstart0		/ hooks to allow non-AT platforms
					/ to modify the bootinfo structure.
	call	init_mmu		/ build temporary page tables

	/
	/ Perform platform specific functions that can only be
	/ executed in real mode or protected mode but not
	/ after paging is enabled.
	/ e.g. BIOS calls
	/

	pushl	%edi			/ Pass boot arg to psm_pstart
	call 	psm_pstart
	addl	$4, %esp		/ Pop %edi

	call	enable_paging

	movl	$ueng, %ebp		/ running mapped so ...
	movl	%ebp, %esp		/ set stack pointer to engine stack
	jmp	_start			/ continue startup

SIZE(start)


	.align	4
	.globl	online_kl1pt
online_kl1pt:	.long	0


/
/ EGA font pointers (these start as real mode pointers)
/ The pointers point to the 8x8, 8x14, 9x14, 8x16 and 
/ the 9x16 fonts, respectively
/

	.align	8	
	.globl	egafontptr
	.type	egafontptr,@object
egafontptr:
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.size	egafontptr,.-egafontptr
	
	.globl	egafontsz
	.type	egafontsz,@object
egafontsz:
	.value	0
	.value	0
	.value	0
	.value	0
	.value	0
	.size	egafontsz,.-egafontsz

/
/ void
/ loadegafonts(void)
/
/	Code to find font locations from the bios
/	and to put them in egafonptr[] where the kd driver can find them.
/
/ Calling/Exit State:
/
/	Called once during system initialization while the linear address
/	is equal to physical/real address.
/
/ Note:
/
/	The system is NOT in real-mode. Must call goreal to switch to
/	real-mode and then switch back to protected-mode on exit.
/
/	Not a C callable funcion.
/
/	The name of the function is a misnomer. It is being used as
/	a base kernel entry point to make int 0x10 (video) bios call.
/

ENTRY(loadegafonts)
	/
	/ Do the BIOS call to determine the monitor type.
	/
	call	phys_vdc_mon_type

	call	goreal

	data16
	movl	$0x1130, %eax	/ set up bios call
	data16
	movl	$0x0300, %ebx	/ get pointer to 8x8 font
	int	$0x10

	addr16
	movl	%ebp, %cs:egafontptr
	addr16
	movw	%es, %cs:egafontptr + 2
	addr16
	movl	%ecx, %cs:egafontsz

	data16
	movl	$0x1130, %eax	/ set up bios call
	data16
	movl	$0x0200, %ebx	/ get pointer to 8x14 font
	int	$0x10

	addr16
	movl	%ebp, %cs:egafontptr + 4
	addr16
	movw	%es, %cs:egafontptr + 6
	addr16
	movl	%ecx, %cs:egafontsz + 2

	data16
	movl	$0x1130, %eax	/ set up bios call
	data16
	movl	$0x0500, %ebx	/ get pointer to 9x14 font
	int	$0x10

	addr16
	movl	%ebp, %cs:egafontptr + 8
	addr16
	movw	%es, %cs:egafontptr + 10
	addr16
	movl	%ecx, %cs:egafontsz + 4

	data16
	movl	$0x1130, %eax	/ set up bios call
	data16
	movl	$0x0600, %ebx	/ get pointer to 8x16 font
	int	$0x10

	addr16
	movl	%ebp, %cs:egafontptr + 12
	addr16
	movw	%es, %cs:egafontptr + 14
	addr16
	movl	%ecx, %cs:egafontsz + 6

	data16
	movl	$0x1130, %eax	/ set up bios call
	data16
	movl	$0x0700, %ebx	/ get pointer to 9x16 font
	int	$0x10

	addr16
	movl	%ebp, %cs:egafontptr + 16
	addr16
	movw	%es, %cs:egafontptr + 18
	addr16
	movl	%ecx, %cs:egafontsz + 8

	/ Finished using BIOS calls, back to protected mode.
	data16
	call	goprot

	ret
SIZE(loadegafonts)


/ 
/ void
/ at_phys_puts(const char *str)
/
/	i386at-specific print-string function (calls BIOS print chr)
/	Print a string argument on the console running in physical mode.
/
/ Calling/Exit State:
/
/	<str> is a string pointer to be printed.
/ 

ENTRY(at_phys_puts)
	subl	$8, %esp
	sgdt	(%esp)
	pushl	%ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	movl	24+SPARG0, %edi
	movl	%esp, %ecx
	cmpl	$STARTSTK, %esp
	jbe	.L1
	movl	$STARTSTK, %esp
.L1:	
	pushl	%ecx			/ push old %esp on new stack
.ploop:
	movb	(%edi), %bl
	orb	%bl, %bl
	jne	.pmore
	popl	%ecx
	movl	%ecx, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	lgdt	(%esp)
	addl	$8, %esp
	ret
.pmore:
	incl	%edi
	push	%ds
	push	%es
	movl	%cr0, %edx
	pushl	%edx			/ save %cr0
	andl	$0x7FFFFFFF, %edx	/ turn off paging
	movl	%edx, %cr0
	call	goreal
	addr16
	data16
	movb	%bl, %al	/ %al = char
	movb	$1, %bl		/ foreground color
	movb	$14, %ah	/ setup call to bios
	data16
	pushfl
	int	$0x10		/ print chr
	data16
	popfl
	data16
	call	goprot
	popl	%edx
	movl	%edx, %cr0		/ restore %cr0
	pop	%es
	pop	%ds
	jmp	.ploop

SIZE(at_phys_puts)


/ 
/ void
/ goprot(void)
/
/ 	Enter protected mode.
/
/ Calling/Exit State:
/
/ 	When we enter this routine, 	ss == ds == cs == "codebase", 
/		when we leave,  	ss == ds = es = KDS cs = KCS32
/
/ Note:
/
/ 	We must set up the GDTR
/
/ 	Trashes %ax, %bx.
/
/	Not a C callable funcion.
/

ENTRY(goprot)

	data16
	popl	%ebx			/ get return %eip, for later use

	/ load the GDTR

	addr16
	data16
	lgdt	%cs:GDTptr

	movl	%cr0, %eax

	data16
	orl	$PROTMASK, %eax

	movl	%eax, %cr0 

	jmp	qflush			/ flush the prefetch queue

	/
	/ Set up the segment registers, so we can continue like before;
	/ if everything works properly, this shouldn't change anything.
	/ Note that we're still in 16 bit operand and address mode, here, 
	/ and we will continue to be until the new %cs is established. 
	/

qflush:
	data16
	movl	$KDS, %eax
	movw	%ax, %es
	movw	%ax, %ds
	movw	%ax, %ss		/ don't need to set %sp

	/ Now, set up %cs by fiddling with the return stack and doing an lret

	data16
	pushl	$KCS32			/ push %cs

	data16
	pushl	%ebx			/ push %eip

	data16
	lret

SIZE(goprot)


/ 
/ void
/ goreal(void)
/
/ 	Enter real mode.
/
/ Calling/Exit State:
/
/	None.
/ 
/ Remarks:
/
/ 	We assume that we are executing code in a segment that
/ 	has a limit of 64k. Thus, the CS register limit should
/ 	be set up appropriately for real mode already. We also
/ 	assume that paging has *not* been turned on.
/ 	Set up %ss, %ds, %es, %fs, and %gs with a selector that
/ 	points to a descriptor containing the following values
/
/	Limit = 64k
/	Byte Granular 	( G = 0 )
/	Expand up	( E = 0 )
/	Writable	( W = 1 )
/	Present		( P = 1 )
/	Base = any value
/
/ Note:
/
/	Not a C callable funcion.
/

ENTRY(goreal)

	/ Switch to 64K-limit data segment

	movw	$KDS64K, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %ss
	movw	$0, %ax
	movw	%ax, %fs
	movw	%ax, %gs

	/ Now transfer control to a 16 bit code segment

	ljmp	$KCS16, $set16cs

set16cs:			

	/ 16 bit addresses and operands 

	movl	%cr0, %eax

	data16
	andl 	$NOPROTMASK, %eax	/ clear the protection bit

	movl	%eax, %cr0

	/ We want to do a long jump here, to reestablish %cs in real mode

	data16
	ljmp	$0, $restorecs

	/
	/ Now we've returned to real mode, so everything is as it 
	/ should be. Set up the segment registers and so on.
	/ The stack pointer can stay where it was, since we have fiddled
	/ the segments to be compatible.
	/

restorecs:

	movw	%cs, %ax
	movw	%ax, %ss
	movw	%ax, %ds
	movw	%ax, %es

	data16
	ret			/ return to whence we came; it was a 32 bit call

SIZE(goreal)


ifdef(`PHYS_DEBUG',`

/ 
/ void
/ dbg_puts(void)
/
/	Used for PHYS_DEBUG string printing.
/	Pauses for a short time after each string.
/
/ Calling/Exit State:
/
/	Callable only from assembly language, with null-terminated string
/	immediately following call instruction.
/
ENTRY(dbg_puts)
	/ call at_phys_puts() with pushed return address as argument
	call	at_phys_puts
	/ advance return address past string
	pushl	%edi
	movl	4(%esp), %edi
	movl	$-1, %ecx
	xorb	%al, %al
	repne; scasb
	movl	%edi, 4(%esp)
	popl	%edi
	/ delay
	movl	$3000000, %ecx
	loop	.
	ret

SIZE(dbg_puts)
')

/
/ boolean_t
/ pse_supported(void)
/	Indicate whether chip supports page size extension (PSE), i.e.,
/	4MB page.
/
/ Calling/Exit State:
/	Return value is TRUE/FALSE if chip does/does not support pse.
/
/ Remarks:
/	A simplified version of chip_detect routine; assumes only Intel
/	Pentium support pse.
/
	/ flags word bits
	.set	EFL_AC, 0x40000		/ alignment check (1->check)
	.set	EFL_ID, 0x200000	/ cpuid opcode (flippable->supported)
	.set	_A_CPUFEAT_PSE, 0x08

ENTRY(pse_supported)
	/ Identify i386 and i486 by the inability to set certain flag bits

	xorl	%eax, %eax		/ zero out %eax
	pushfl				/ push FLAGS value on stack
	movl	(%esp), %ecx		/ save copy of FLAGS

	xorl	$EFL_AC, (%esp)		/ flip AC bit for new FLAGS
	popfl				/ attempt changing FLAGS.AC
	pushfl				/ push resulting FLAGS on stack
	cmpl	(%esp), %ecx		/ succeeded in flipping AC?
	je	.no4mb			/ no it's a 386, no 4mb support

	xorl	$EFL_AC+EFL_ID, (%esp)	/ restore AC bit and flip ID bit
	popfl				/ attempt changing FLAGS.ID
	pushfl				/ push resulting FLAGS on stack
	cmpl	(%esp), %ecx		/ succeeded in flipping ID?
	je	.no4mb			/ no, must be an i486, no 4mb support

	/ Since we were able to flip the ID bit, we are running on
	/ a processor which supports the cpuid instruction

	xchgl	%ecx, (%esp)		/ save original FLAGS on stack

	/ %eax is already zero'd out at this point, parameter to cpuid
	cpuid				/ get CPU hv & vendor

	xorl	%eax, %eax		/ zero out %eax
	cmpl	%ebx, intel_cpu_id
	jne	.no4mb
	cmpl	%edx, intel_cpu_id+4
	jne	.no4mb
	cmpl	%ecx, intel_cpu_id+8
	jne	.no4mb

	movl	$1, %eax		/ parameter for cpuid op
	cpuid				/ get CPU family-model-stepping
					/     and first 96 feature bits
	xorl	%eax, %eax		/ zero it out
	testl	$_A_CPUFEAT_PSE, %edx
	je	.no4mb
	incl	%eax			/ set %eax to 1 (TRUE)

.no4mb:
	popfl
	ret
intel_cpu_id:
	.string	"GenuineIntel"	/ ID string for Intel CPUs
SIZE(pse_supported)


/
/ void
/ doint(struct intregs *param)
/	Do a BIOS int call.
/
/ Calling/Exit State:
/	<param> is a pointer to intregs structure shown below.
/	
/	struct intregs {
/	ushort	intval, 	/* 0x0 */
/		ax,		/* 0x2 */ 
/		bx,		/* 0x4 */
/		cx,		/* 0x6 */
/		dx,		/* 0x8 */
/		bp,		/* 0xa */
/		es,		/* 0xc */
/		si,		/* 0xe */
/		ds,		/* 0x10 */
/		di;		/* 0x12 */
/	};
/
/ Note:
/	The int 15 BIOS routines require about 1536 allocated from
/	the stack for temporary RAM variables.

ENTRY(doint)
	pushl	%ebp			/ 0x04: save non-volatile %ebp register
	pushl	%ebx			/ 0x08: save non-volatile %ebx register
	pushl	%edi			/ 0x0C: save non-volatile %edi register
	pushl	%esi			/ 0x10: save non-volatile %esi register

	movl	0x14(%esp), %eax	/ %eax = param
	movw	(%eax), %ax		/ %ax = param->intval 
					/ (pick up the int code to use)
	movb	%al, intcode+1	

	call	goreal			/ get real

	sti

	/
	/ Set up registers for the call.
	/

	addr16
	data16
	mov	0x14(%esp), %edi	/ move intregs fields to registers
					/ %di = param
	addr16
	mov	4(%edi), %ebx		/ %bx = param->bx
	addr16
	mov	6(%edi), %ecx		/ %cx = param->cx
	addr16
	mov	8(%edi), %edx		/ %dx = param->dx
	addr16
	mov	10(%edi), %ebp		/ %bp = param->bp
	addr16
	movw	12(%edi), %es		/ %es = param->es
	addr16
	mov	14(%edi), %esi		/ %si = param->si

	push	%ds			/ 0x12: save real mode segment register
	addr16
	push	2(%edi)			/ 0x14: push ax because of %ds change
	addr16
	mov	16(%edi), %eax		/ %ax = param->ds
	movw	%ax, %ds		/ %ds = %ax = param->ds
	clc				/ clear CARRY to see BIOS call effect
	pop	%eax			/ 0x12: restore ax
	addr16
	mov	18(%edi), %edi		/ %di = param->di

intcode:
	int	$0			/ It is self modified

	cli
	cld
	pop	%ds			/ 0x10: restore real mode segment reg.

	/
	/ Save carry for later and stash the returned registers
	/ that are going to get trashed by goport call.
	/
	pushf				/ 0x12: save the eflags register
	push	%eax			/ 0x14: saved %eax scratch register
	push	%ebx			/ 0x16: save %ebx scratch register
	push	%es			/ 0x18: save %es segment register
	push	%edi			/ 0x1a: saved %edi index register

	data16
	call	goprot			/ protect mode

	/
	/ Transferring the regs to the int call param block.
	/

	movl	0x1e(%esp), %edi	/ %di = param

	popw	%ax			/ 0x18: pop the %di saved register
	movw	%ax, 18(%edi)		/ param->di = %di

	popw	%ax			/ 0x16: pop the %es saved register
	movw	%ax, 12(%edi)		/ param->es = %es

	popw	%ax			/ 0x14: pop the %bx saved register
	movw	%ax, 4(%edi)		/ param->bx = %bx

	popw	%ax			/ 0x12: pop the %ax saved register
	movw	%ax, 2(%edi)		/ param->ax = %ax

	movw	%cx, 6(%edi)		/ param->cx = %cx
	movw	%dx, 8(%edi)		/ param->dx = %dx
	movw	%bp, 10(%edi)		/ param->bp = %bp
	movw	%si, 14(%edi)		/ param->si = %si

        xorl    %eax, %eax		/ initalize return to zero
	popfw				/ 0x10: get carry

	jnc     dixt
	incl    %eax			/ return != 0 for carry set
dixt:
	/
	/ Restore registers.
	/
	popl    %esi			/ 0x0C: restore %esi register
	popl    %edi			/ 0x08: restore %edi register
	popl    %ebx			/ 0x04: restore %ebx register
	popl    %ebp			/ 0x00: restore %ebp register
	ret

SIZE(doint)
