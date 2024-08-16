/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:svc/detect.s	1.6"
	.ident	"$Header: $"

include(../svc/asm.m4)
include(assym_include)

	/ flags word bits
	.set	EFL_T, 0x100		/ trace enable bit
	.set	EFL_AC, 0x40000		/ alignment check (1->check)
	.set	EFL_ID, 0x200000	/ cpuid opcode (flippable->supported)

	/ CR0 control register bits
	.set	CR0_MP, 0x02		/ math coprocessor present
	.set	CR0_EM, 0x04		/ use math emulation
	.set	CR0_TS, 0x08		/ task switched

	.set	MAP_VADDR, 0x402000	/ Magic address for cd_mapin mappings

/
/ void
/ detect_cpu(void)
/	Detect CPU type.
/
/ Calling/Exit State:
/	No locks needed because action is entirely processor-local.
/	Called when a processor is initialized.
/	Fills out fields in the local engine structure which identify
/	the type and stepping of the CPU.
/
/	Output is in l.cpu_id, l.cpu_model, l.cpu_stepping, and l.cpu_features.
/	Assumes these fields are initially zeroed.
/

ENTRY(detect_cpu)
	/
	/ Identify the cpu we are running on
	/

	/ Identify i386 and i486 by the inability to set certain flag bits

	pushfl				/ push FLAGS value on stack
	movl	(%esp), %ecx		/ save copy of FLAGS

	xorl	$EFL_AC, (%esp)		/ flip AC bit for new FLAGS
	popfl				/ attempt changing FLAGS.AC
	pushfl				/ push resulting FLAGS on stack
	cmpl	(%esp), %ecx		/ succeeded in flipping AC?
	je	.cpu_is_386		/ no, must be an i386

	xorl	$EFL_AC+EFL_ID, (%esp)	/ restore AC bit and flip ID bit
	popfl				/ attempt changing FLAGS.ID
	pushfl				/ push resulting FLAGS on stack
	cmpl	(%esp), %ecx		/ succeeded in flipping ID?
	je	.cpu_is_486		/ no, must be an i486

	/ Since we were able to flip the ID bit, we are running on
	/ a processor which supports the cpuid instruction

	xchgl	%ecx, (%esp)		/ save original FLAGS on stack

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	$0, %eax		/ parameter for cpuid op
	cpuid				/ get CPU hv & vendor

	andl	$0x7, %eax		/ limit the cpuid-hv to 7
	movl	%eax, cpuidparm		/ save that high parm value
	movl	%ebx, cpuvendor		/ save the vendor ID string
	movl	%edx, cpuvendor+4	/	vendor ID continued
	movl	%ecx, cpuvendor+8	/	vendor ID continued

	leal	cpuvendor, %esi		/ compare vendor ID string
	leal	intel_cpu_id, %edi	/  against the Intel ID string
	movl	$12, %ecx
	repe; cmpsb
	jne	.other_cpu		/ not Intel; ignore the rest

	movl	$1, %eax		/ parameter for cpuid op
	cpuid				/ get CPU family-model-stepping
					/     and first 96 feature bits

	movl	%edx, l+_A_L_CPU_FEATURES	/ save feature bits
	movl	%ecx, l+_A_L_CPU_FEATURES+4
	movl	%ebx, l+_A_L_CPU_FEATURES+8

	movl	%eax, %ebx		/ extract stepping id
	andl	$0x00F, %ebx		/     from bits [3:0]
	movl	%ebx, l+_A_L_CPU_STEPPING

	movl	%eax, %ebx		/ extract model
	andl	$0x0F0, %ebx		/     from bits [7:4]
	shrl	$4, %ebx
	movl	%ebx, l+_A_L_CPU_MODEL

	movl	%eax, %ebx		/ extract family
	andl	$0xF00, %ebx		/     from bits [11:8]
	shrl	$8, %ebx
	movl	%ebx, l+_A_L_CPU_ID

	movl	$2, %esi		/ initial index for remaining features
	leal	l+_A_L_CPU_FEATURES+12, %edi
.other_features:
	cmpl	%esi, cpuidparm		/ check loop variable against limit
	jb	.cpu_identified
	movl	%esi, %eax		/ set next cpuid parameter
	cpuid				/ get next set of features
	movl	%edx, (%edi)		/ save next dword of feature bits
	movl	%ecx, 4(%edi)		/ save next dword of feature bits
	movl	%ebx, 8(%edi)		/ save next dword of feature bits
	movl	%eax, 12(%edi)		/ save next dword of feature bits
	incl	%esi			/ increment loop variable
	addl	$16, %edi		/ adjust pointer to features
	jmp	.other_features

.other_cpu:	/ assume non-Intel CPUs behave like i486 CPUs
	movl	$_A_CPU_486, l+_A_L_CPU_ID
	/ FALLTHROUGH

.cpu_identified:
	popl	%edi
	popl	%esi
	popl	%ebx

	popfl				/ restore original FLAGS
	ret

.cpu_is_486:
	movl	$_A_CPU_486, l+_A_L_CPU_ID
	popfl				/ restore original FLAGS
	ret

.cpu_is_386:
	movl	$_A_CPU_386, l+_A_L_CPU_ID
	popfl				/ restore original FLAGS

/
/ Do 80386 B1 stepping detection.
/
/ The detection is done by looking for the presence of the Errata 5 bug,
/ which causes a single step of REP MOVS to go through 2 iterations, not 1.
/
	pushl	%esi
	pushl	%edi
	subl	$8, %esp
	sidt	2(%esp)			/ get pointer to IDT into %edx
	movl	4(%esp), %edx
	pushl	4+8(%edx)		/ save current debug trap
	pushl	8(%edx)
	movl	$.b1_ss_trap, %eax
	movw	%ax, 8(%edx)		/ set debug trap to b1_ss_trap
	shrl	$16, %eax
	movw	%ax, 6+8(%edx)
	pushfl				/ save flags register
	movl	$2, %ecx		/ set up for a REP MOVS w/count of 2
	movl	$l, %esi		/   (to and from an arbitrary addr)
	movl	%esi, %edi
	pushl	$EFL_T			/ set the single step flag
	popfl
	rep
	movsb
.b1_ss_trap:
	addl	$12, %esp		/ skip the iret
	xorl	$1, %ecx		/ ECX has 1 if no bug, else 0 -
	movl	%ecx, l+_A_L_CPU_STEPPING /   store in l.cpu_stepping
	popfl				/ restore flags
	popl	8(%edx)			/ restore debug trap
	popl	4+8(%edx)
	addl	$8, %esp
	popl	%edi
	popl	%esi

	ret

	.data
	.align	4
	/ local variables
cpuidparm:
	.long	0	/ highest cpuid parameter #
cpuvendor:
	.long	0,0,0	/ CPU vendor ID string
intel_cpu_id:
	.string	"GenuineIntel"	/ ID string for Intel CPUs

	.text

	SIZE(detect_cpu)


/
/ int
/ detect_fpu(void)
/	Detect FPU type.
/
/ Calling/Exit State:
/	No locks needed because action is entirely processor-local.
/	Called when a processor is initialized.
/
/	Returns the "fp_kind" value identifying the type of FPU.
/

ENTRY(detect_fpu)
/
/ Check for any chip at all by tring to do a reset.  If that succeeds,
/ differentiate via cr0.
/
	subl	$4, %esp		/ reserve space for local variable
	movl	%cr0, %edx		/ save CR0 in %edx
	movl	%edx, %eax
	orl	$CR0_MP, %eax		/ set CR0 bits to indicate FPU h/w
	andl	$~[CR0_EM|CR0_TS], %eax
	movl	%eax, %cr0
	fninit				/ initialize chip, if any
	movw	$0x5A5A, (%esp)
	fnstsw	(%esp)			/ get status
	cmpb	$0, (%esp)		/ status zero? 0 = chip present
	jne	.nomathchip
/
/ See if ones can be correctly written from the control word
/
	fnstcw	(%esp)			/ look at the control word
	movw	(%esp), %ax
	andw	$0x103F, %ax		/ see if selected parts of cw look ok
	cmpw	$0x3F, %ax		/ 0x3F = chip present
	jne	.nomathchip
/
/ At this point we know we have a chip of some sort; 
/ NPX and WAIT instructions are now safe.  Distinguish the type of chip
/ by its behavior w.r.t. infinity.
/
/ Note: must use default control word from fninit.
/
	fsetpm				/ in case it's an 80286
	fld1				/ form infinity
	fldz				/   by dividing 1./0.
	fdivr
	fld	%st(0)			/ form negative infinity
	fchs
	fcompp				/ compare +inf with -inf
	fstsw	(%esp)			/    287 considers +inf == -inf
	movw	(%esp), %ax		/    387 considers +inf != -inf
	sahf				/ get the fcompp status
	movl	$_A_FP_287, %eax	/ we have a 287 chip
	je	.fpu_ret		/    if compare equal
	movl	$_A_FP_387, %eax	/ else, we have a 387 chip
.fpu_ret:
	movl	%edx, %cr0		/ restore original CR0
	addl	$4, %esp		/ "pop" local variable off stack
	ret

/
/ No chip was found
/
.nomathchip:
	movl	$_A_FP_NO, %eax		/ indicate no hardware
	jmp	.fpu_ret

	SIZE(detect_fpu)

ifdef(`BUG386B1',`
/
/ int
/ detect_387cr3(void)
/	Detect if system supports the 387cr3 workaround for 386B1 stepping
/	Errata #21.
/
/ Calling/Exit State:
/	No locks needed because action is entirely processor-local.
/	Called when a processor is initialized.
/
/	Returns the non-zero if the 387cr3 workaround can be used.
/
define(NEED_MAPPING,1)
ENTRY(detect_387cr3)
	movl	$_A_KVENG_L1PT, %edx	/ %edx <- vaddr for L1 page table
	xorl	%ecx, %ecx		/ %ecx holds return value, init to 0
	movl	%cr3, %eax
	orl	$0x80000000, %eax	/ 2G alias for L1PT paddr
	call	cd_mapin		/ map MAP_VADDR to L1PT alias
	/ CATCH_FAULTS(CATCH_ALL_FAULTS)
	movl	$_A_CATCH_ALL_FAULTS, ueng+_A_U_FAULT_CATCH+_A_FC_FLAGS
	movl	$.cr3_disable, ueng+_A_U_FAULT_CATCH+_A_FC_FUNC
	movl	MAP_VADDR, %eax		/ try reading from alias
	cmpl	%eax, (%edx)		/ see if alias read matches real read
	jne	.cr3_disable		/ no, we cannot use workaround
	incl	MAP_VADDR		/ change the entry via the alias
	incl	%eax
	cmpl	%eax, (%edx)		/ did the kl1pt entry change also?
	jne	.cr3_disable		/ no, we cannot use workaround
	decl	(%edx)			/ alias was successful, restore kl1pt
	incl	%ecx			/ indicate 387cr3 workaround ok
.cr3_disable:
	movl	$0, ueng+_A_U_FAULT_CATCH+_A_FC_FLAGS	/ END_CATCH()
        movl    $fc_jmpjmp, ueng+_A_U_FAULT_CATCH+_A_FC_FUNC
	call	cd_mapout		/ undo the mapping
	movl	%ecx, %eax		/ return value
	ret

	SIZE(detect_387cr3)
')


ifdef(`WEITEK',`
/
/ int
/ detect_weitek(void)
/	Detect if a weitek math chip is present.
/
/ Calling/Exit State:
/	No locks needed because action is entirely processor-local.
/	Called when a processor is initialized.
/
/	Returns the "weitek_kind" value indicating the presence or absence
/	of a weitek unit.
/
/
/ extern paddr_t weitek_paddr;	/* chip physical address, 0 if not supported */
/
define(NEED_MAPPING,1)
ENTRY(detect_weitek)
	movl	weitek_paddr, %eax
	orl	%eax, %eax		/ if (weitek_paddr == 0)
	jz	.weitek_ret		/	return 0;

	xorl	%ecx, %ecx		/ %ecx holds return value, init to 0

	movl	$_A_KVENG_L1PT, %edx	/ %edx <- vaddr for L1 page table
	call	cd_mapin		/ map MAP_VADDR to weitek registers
	xorl	%eax, %eax
	/ CATCH_FAULTS(CATCH_ALL_FAULTS)
	movl	$_A_CATCH_ALL_FAULTS, ueng+_A_U_FAULT_CATCH+_A_FC_FLAGS
	movl	$.weitek_done, ueng+_A_U_FAULT_CATCH+_A_FC_FUNC
	movl	$0x3b3b3b3b, MAP_VADDR+0x404 / store a value into a register
	movl	MAP_VADDR+0xc04, %eax	/      and read it back out.
	cmpl	$0x3b3b3b3b, %eax
	jne	.weitek_done		/ no chip
	call	cd_mapout		/ undo the mapping

	/ clear weitek exceptions so that floating point exceptions
	/ are reported correctly from here out
	/ initialize the 1167 timers
	movl	weitek_paddr, %eax
	addl	$0xc000, %eax		/ map MAP_VADDR to weitek off 0xc000
	call	cd_mapin
	movl	$0xB8000000, MAP_VADDR
	movl	MAP_VADDR+0x400, %eax	/ Check for 20 MHz 1163
	andl	$_A_WEITEK_20MHz, %eax
	jnz	.w_init_20MHz
	movl 	$0x16000000, MAP_VADDR	/ 16 MHz 1164/1165 flowthrough timer
	jmp	.w_init_wt1

.w_init_20MHz:
	movl	$0x56000000, MAP_VADDR	/ 20 MHz 1164/1165 flowthrough
	movl	$0x98000000, MAP_VADDR	/ timer
	
.w_init_wt1:
	movl 	$0x64000000, MAP_VADDR	/ 1164 accumulate timer
	movl 	$0xA0000000, MAP_VADDR	/ 1165 accumulate timer
	movl 	$0x30000000, MAP_VADDR	/ Reserved mode bits (set to 0).
	movl 	weitek_cfg, %eax	/ Rounding modes and Exception
	movl 	%eax, MAP_VADDR		/ enables.
	movb	$0, %al
	outb	%al, $0xF0		/ clear the fp error flip-flop

	movl	$_A_WEITEK_HW, %ecx	/ indicate that there is a chip

.weitek_done:
	movl	$0, ueng+_A_U_FAULT_CATCH+_A_FC_FLAGS	/ END_CATCH()
        movl    $fc_jmpjmp, ueng+_A_U_FAULT_CATCH+_A_FC_FUNC
	call	cd_mapout		/ undo the mapping

	movl	%ecx, %eax		/ return value
.weitek_ret:
	ret

	SIZE(detect_weitek)
')


ifdef(`NEED_MAPPING',`

	.bss	save_pte1, 4
	.bss	save_pte2, 4

	.set	PG_V, 0x1
	.set	PG_RW, 0x2

/
/ cd_mapin -- map physical address given by %eax at virtual address MAP_VADDR
/
/ called with vaddr of L1 page table in %edx; all regs except %eax preserved
/
	.type	cd_mapin, @function
	.align	4
cd_mapin:
	orl	$[PG_V|PG_RW], %eax
	xchgl	%eax, 8(%edx)
	movl	%eax, save_pte2
	movl	%cr3, %eax		/ %eax <- paddr for L1 page table
	orl	$[PG_V|PG_RW], %eax
	xchgl	%eax, 4(%edx)
	movl	%eax, save_pte1
	movl	%cr3, %eax
	movl	%eax, %cr3
	ret

	.size	cd_mapin, .-cd_mapin

/
/ cd_mapout -- undo a mapping established by cd_mapin
/
/ called with vaddr of L1 page table in %edx; all regs except %eax preserved
/
	.type	cd_mapout, @function
	.align	4
cd_mapout:
	movl	save_pte1, %eax
	movl	%eax, 4(%edx)
	movl	save_pte2, %eax
	movl	%eax, 8(%edx)
	movl	%cr3, %eax
	movl	%eax, %cr3
	ret

	.size	cd_mapout, .-cd_mapout

')
