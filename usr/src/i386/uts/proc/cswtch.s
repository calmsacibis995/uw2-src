/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:proc/cswtch.s	1.46"
	.ident	"$Header: $"
	.file	"proc/cswtch.s"

include(../svc/asm.m4)
include(assym_include)
include(../util/ksynch.m4)
include(../proc/cswtch_p.m4)

/
/ save(lwp_t *lwp)
/
/	Save the current context onto the stack for "lwp".
/
/ Calling/Exit State:
/
/	Called with l_mutex held; returns (the first time) with it still held.
/
/	Returns twice.  Once prior to switch-out and once from the
/	switch in.
/
/ Description:
/
/	Save the non-C temporaries into u_kcontext.  This is done
/	prior to idling or prior to duplicating the u-area in fork.
/	We set up the saved registers so it looks like we already
/	returned from the call.  Eax is saved as "1" so the caller
/	may distinguish between the original and switch-in returns.
/
ENTRY(save)
	movl	upointer,%eax	/ Check for stack overflow
				/ +8 skip the return address and argument
	leal	_A_UAREA_OFFSET+8(%esp),%ecx
	subl	%eax,%ecx
	jb	.save_esp_overflow

.save_esp_ok:
	addl	$_A_U_KCONTEXT,%eax	
	movl	%ebx,_A_KCTX_EBX(%eax)	/ %ebx
	movl	%ebp,_A_KCTX_EBP(%eax)	/ %ebp
	movl	%edi,_A_KCTX_EDI(%eax)	/ %edi
	movl	%esi,_A_KCTX_ESI(%eax)	/ %esi
	movl	$1,_A_KCTX_EAX(%eax)	/ save return code, we save a 1 here
	movl	(%esp),%edx
	movl	%edx,_A_KCTX_EIP(%eax)	/ %eip
	leal	4(%esp),%edx		/ "pop" the PC
	movl	%edx,_A_KCTX_ESP(%eax)	/ %esp
	movw	%fs, _A_KCTX_FS(%eax)	/ %fs
	movw	%gs, _A_KCTX_GS(%eax)	/ %gs

	/
	/ If the FPU's being used, save it and disable the FPU.
	/
	xorl	%eax,%eax
	cmpl	%eax,using_fpu
	jne	save_fpu_l

	ret				/ return 0

.save_esp_overflow:
	/
	/ An lwp is attempting to save its context while still executing on
	/ the stack extension page. We try to save the piece of the stack
	/ which resides on the extension page in some memory we are holding
	/ just for this purpose. If ublock_save_extension() returns, then it
	/ has succeeded. If it fails, it will panic. If it succeeds, and this
	/ is a fork(), then ublock_restore_extension() will panic in the
	/ child. Luckily, forks() should use deterministic stack [since
	/ device drivers and other variables are not involved], hopefully
	/ precluding such a case from the observable failures.
	/
	/ This design assumes that this code will rarely [if ever] be
	/ executed. No one should rely upon this code.
	/
	negl	%ecx			/ size of extension stack in use
	leal	8(%esp),%eax		/ beginning of in use extension
	pushl	%ecx
	pushl	%eax
	call	ublock_save_extension
	addl	$8,%esp
	movl	upointer,%eax		/ restore %eax
	jmp	.save_esp_ok

	SIZE(save)

/
/ void
/ save_fpu_l(void)
/	Save the FPU state and disable the FPU.
/
/ Calling/Exit State:
/	Called with l_mutex held.
/
/ Remarks:
/	Even though save_fpu returns void, save() jumps here, so we
/	must return 0 on its behalf.
/

ENTRY(save_fpu_l)
	incl	prmpt_state		/ disable preemption
	movl	upointer,%edx
	cmpl	$_A_FP_SW,fp_kind	/ see if using emulator
	je	.save_fpu_emul
	fnsave	_A_U_KCONTEXT+_A_KCTX_FPREGS+_A_FPCHIP_STATE(%edx)
	.globl	save_fpu_l_fwait
save_fpu_l_fwait:		/ Label the fwait so trap() can find it
	fwait				/ wait for save to complete
	movl	l+_A_L_FPUOFF,%eax	/ want to turn it off
	movl	%eax,%cr0		/ it's off now!
.save_fpu_common:
	xorl	%eax,%eax
	movl	%eax,using_fpu		/ processor no longer using FPU
	decl	prmpt_state		/ enable preemption
	ret				/ return 0

	.align	4
.save_fpu_emul:
	pushl	%esi
	pushl	%edi
	leal	l+_A_L_FPE_KSTATE+_A_FPE_STATE,%esi
	leal	_A_U_KCONTEXT+_A_KCTX_FPREGS+_A_FP_EMUL_SPACE(%edx),%edi
	movl	$_A_SIZEOF_FP_EMUL\/4,%ecx
	rep; smovl
	leal	_A_U_FPE_RESTART(%edx),%edi
	movl	$_A_SIZEOF_FPE_RESTART\/4,%ecx
	rep; smovl
	popl	%edi
	popl	%esi
	movl	cur_idtp,%ecx
	movl	fpuoff_noextflt,%eax
	movl	%eax,[_A_NOEXTFLT\*8](%ecx)
	movl	fpuoff_noextflt+4,%eax
	movl	%eax,[_A_NOEXTFLT\*8+4](%ecx)
	jmp	.save_fpu_common

	SIZE(save_fpu_l)

/
/ void
/ save_fpu(void)
/	Save the FPU state and disable the FPU.
/
/ Calling/Exit State:
/	Called w/o l_mutex held.
/

ENTRY(save_fpu)
	incl	prmpt_state		/ disable preemption
	movl	upointer,%edx
	cmpl	$_A_FP_SW,fp_kind	/ see if using emulator
	je	.save_fpu_emul
	fnsave	_A_U_KCONTEXT+_A_KCTX_FPREGS+_A_FPCHIP_STATE(%edx)
	.globl	save_fpu_fwait
save_fpu_fwait:			/ Label the fwait so trap() can find it
	fwait				/ wait for save to complete
	movl	l+_A_L_FPUOFF,%eax	/ want to turn it off
	movl	%eax,%cr0		/ it's off now!
	jmp	.save_fpu_common

	SIZE(save_fpu)

/
/ void *
/ saveregs(kcontext_t *kcp)
/
/ Calling/Exit State:
/
/	Save registers for debugger prior to a panic.
/
/ Description:
/
/	Save the register set into the kcontext structure pointed to by
/	kcp and return a pointer to kcp.
/
/	This routine is only used from panic.
/	This routine is NOT preemptable.  The callers of this routine
/	MUST guarantee the non-preemptability.
/
ENTRY(saveregs)
	pushl	%eax			/ save eax
	movl	8(%esp),%eax		/ kcontext pointer
	movl	%ecx,_A_KCTX_ECX(%eax)	/ %ecx
	movl	%edx,_A_KCTX_EDX(%eax)	/ %edx
	popl	_A_KCTX_EAX(%eax)	/ %eax
	movl	%ebx,_A_KCTX_EBX(%eax)	/ %ebx
	movl	%ebp,_A_KCTX_EBP(%eax)	/ %ebp
	movl	%edi,_A_KCTX_EDI(%eax)	/ %edi
	movl	%esi,_A_KCTX_ESI(%eax)	/ %esi
	movl	(%esp),%ecx
	movl	%ecx,_A_KCTX_EIP(%eax)	/ %eip
	leal	4(%esp),%ecx		/ "pop" the PC
	movl	%ecx,_A_KCTX_ESP(%eax)	/ %esp
	movw	%fs, _A_KCTX_FS(%eax)	/ %fs
	movw	%gs, _A_KCTX_GS(%eax)	/ %gs
	
	/
	/ If the FPU's being used, save it.
	/
	cmpl	$0,using_fpu
	jne	.saveregs_fpu

	ret

	.align	4
.saveregs_fpu:
	cmpl	$_A_FP_SW,fp_kind		/ see if using emulator
	je	.saveregs_fpu_emul
	/
	/ Save the fpu.
	/
	fnsave	_A_KCTX_FPREGS+_A_FPCHIP_STATE(%eax)
	.globl	saveregs_fwait
saveregs_fwait:			/ Label the fwait so trap() can find it
	fwait
	ret

	.align	4
.saveregs_fpu_emul:
	pushl	%esi
	pushl	%edi
	leal	l+_A_L_FPE_KSTATE+_A_FPE_STATE,%esi
	leal	_A_KCTX_FPREGS+_A_FP_EMUL_SPACE(%eax),%edi
	movl	$_A_SIZEOF_FP_EMUL\/4,%ecx
	rep; smovl
	popl	%edi
	popl	%esi
	ret

	SIZE(saveregs)

/
/ resume(lwp_t *new, lwp_t *old)
/
/	Switch context from one lwp to another.
/
/ Calling/Exit State:
/
/	Should be called holding the l_mutex of both new and old (if non-NULL).
/	Returns at pl0, with both locks released.
/
/ Description:
/
/	Save the context of "old" (if non-NULL) and load the context of "new".
/	It does this by copying pte's around.
/
ENTRY(resume)
	incl	prmpt_state	/ Mark the engine non-preemptable

ifdef(`DEBUG',`
	cmpl	$0, l+_A_L_INTR_DEPTH	/ intr. depth must be 0
	je	.intr_ok
	pushl	.resume_intrpanic
	pushl	$_A_CE_PANIC
	call	cmn_err

	/ NOTREACHED (adjust %esp anyway, so stack traces will work)
	addl	$8, %esp	/ for proper stack traces 
.intr_ok:
')
	movl	upointer,%ecx	/ Check for stack overflow
	leal	_A_UAREA_OFFSET+8(%esp), %eax
				/ +8 ignores the return address and 1 argument
	subl	%ecx,%eax
	jb	.resume_esp_overflow

.resume_esp_ok:
	movl	8(%esp),%edx		/ %edx = old
	testl	%edx,%edx
	jz	.resume_nosave

	/
	/ Save the context of "old".
	/
	movl	%ebx,_A_U_KCONTEXT+_A_KCTX_EBX(%ecx)	/ %ebx

	movl	%ebp,_A_U_KCONTEXT+_A_KCTX_EBP(%ecx)	/ %ebp

	movl	%edi,_A_U_KCONTEXT+_A_KCTX_EDI(%ecx)	/ %edi
	movl	%esi,_A_U_KCONTEXT+_A_KCTX_ESI(%ecx)	/ %esi
	movl	(%esp),%eax
	movl	%eax,_A_U_KCONTEXT+_A_KCTX_EIP(%ecx)	/ %eip
	leal	4(%esp),%eax				/ "pop" the PC
	movl	%eax,_A_U_KCONTEXT+_A_KCTX_ESP(%ecx)	/ %esp
	movw	%fs,_A_U_KCONTEXT+_A_KCTX_FS(%ecx)	/ %fs
	movw	%gs,_A_U_KCONTEXT+_A_KCTX_GS(%ecx)	/ %gs

	movl	4(%esp),%ebx		/ %ebx = new LWP
	movl	%edx,%ebp		/ %ebp = old LWP

	/
	/ Save and disable the FPU, if necessary.
	/
	cmpl	$0,using_fpu
	je	.resume_no_fpu
	cmpl	$_A_FP_SW,fp_kind		/ see if using emulator
	jne	.resume_no_emul

	/ %ecx has a pointer to the user structure of the old LWP.
	/ Copy the outgoing state from plocal to the uarea.
	leal	l+_A_L_FPE_KSTATE+_A_FPE_STATE,%esi
	leal	_A_U_KCONTEXT+_A_KCTX_FPREGS+_A_FP_EMUL_SPACE(%ecx),%edi
	movl	$_A_SIZEOF_FP_EMUL\/4,%ecx
	rep; smovl
	movl	upointer,%ecx
	leal	_A_U_FPE_RESTART(%ecx),%edi
	movl	$_A_SIZEOF_FPE_RESTART\/4,%ecx
	rep; smovl
	/ Cause subsequent FP access to fault to the kernel.
	movl	cur_idtp,%ecx
	movl	fpuoff_noextflt,%eax
	movl	%eax,[_A_NOEXTFLT\*8](%ecx)
	movl	fpuoff_noextflt+4,%eax
	movl	%eax,[_A_NOEXTFLT\*8+4](%ecx)

	jmp	.resume_fpu_common

	.align 4
.resume_no_emul:
	fnsave	_A_U_KCONTEXT+_A_KCTX_FPREGS+_A_FPCHIP_STATE(%ecx)
	.globl	cswtch_fwait
cswtch_fwait:			/ Label the fwait so trap() can find it
	fwait
	movl	l+_A_L_FPUOFF,%eax
	movl	%eax,%cr0
.resume_fpu_common:
	movl	$0,using_fpu
.resume_no_fpu:

	/
	/ Bag the user-mode address space of old.
	/
	movl	_A_LWP_PROCP(%ebp),%eax		/ %eax = old procp
	cmpl	%eax, _A_LWP_PROCP(%ebx)	/ check if == new procp
	je	.resume_fromsave
	movl	_A_P_AS(%eax),%ecx
	testl	%ecx,%ecx
	jz	.resume_fromsave

	pushl   $1		/ B_TRUE flag; hat_asunload must do TLB flush
	pushl	%ecx
	call	hat_asunload	/ hat_asunload(old->l_procp->p_as, B_TRUE)
	addl	$8,%esp

	jmp	.resume_fromsave

.resume_nosave:
	/
	/ Load the context of new.
	/ Save the pointers into callee-saved registers so we can count on
	/ them over the unlock.
	/
	movl	4(%esp),%ebx		/ %ebx = new LWP
	movl	%edx,%ebp		/ %ebp = old LWP (NULL)

.resume_fromsave:

	/
	/ We're now off the context of the old lwp, initialize the stack
	/ pointer of the new and release the old lwp if necessary.
	/

	/
	/ Save the u area address of the context being switched in
	/ in the engine structure.            
	movl	_A_LWP_UP(%ebx),%eax
	movl	%eax,upointer
	movl	_A_U_PROCP(%eax),%edx
	movl	%edx,uprocp
	movl	_A_U_KCONTEXT+_A_KCTX_ESP(%eax),%esp
	/
	/ Map in the kernel stack extension page.
	/
	movl	l+_A_L_KSE_PTE,%edx
	movl	_A_U_KSE_PTEP(%eax),%eax
	movl	%edx,(%eax)		/ *u.u_kse_ptep = l.kse_pte;

	__ENABLE_ASM			/ enable interrupts as per current ipl

	/
	/ Are we restoring from a stack overflow (onto the extension stack)?
	/ If so, we need to restore the extension piece as well. 
	/
	movl	upointer,%ecx
	leal	_A_UAREA_OFFSET+4(%esp),%eax
					/ +4 because return address has
					/ already been popped
	subl	%eax,%ecx
	ja	.resume_from_overflow

.resume_unlock:
	/
	/ Unlock the old lwp.
	/
	testl	%ebp,%ebp
	jz	.resume_nounlock

	leal	_A_LWP_MUTEX(%ebp),%eax
	__UNLOCK_ASM(%eax, $_A_PLHI)

.resume_nounlock:
	/
	/ Unlock the new lwp; as long as our %esp is restored, we're
	/ in business.
	/
	leal	_A_LWP_MUTEX(%ebx),%edx
	__UNLOCK_ASM(%edx, $_A_PL0)

	/
	/ Call down into the hat layer to load the user-mode address
	/ space.
	/
	movl	_A_LWP_PROCP(%ebx),%eax
	testl	%ebp,%ebp
	jz	.old_null
	cmpl	%eax,_A_LWP_PROCP(%ebp)
	je	.no_asload
.old_null:
	movl	_A_P_AS(%eax),%eax
	testl	%eax,%eax
	jz	.no_asload
	pushl	%eax
	call	hat_asload		/ hat_asload(new->l_procp->p_as)
	addl	$4,%esp
.no_asload:

	movl	upointer,%ecx

	/
	/ Set the kernel stack pointer in the global TSS to point to
	/ this context's kernel stack, in case it's a standard process
	/ without a private TSS.  Private TSS's always have pre-set
	/ stack pointers.
	/
	subl	$_A_KSTACK_RESERVE,%ecx
	movl	%ecx,l+_A_L_TSS+_A_T_ESP0	/ tss.t_esp0 for global tss
	addl	$_A_KSTACK_RESERVE,%ecx

	/
	/ Do platform-specific part of resume()
	/ The following registers must be preserved:
	/	%ebp	- "old" LWP pointer
	/	%ebx	- "new" LWP pointer
	/	%ecx	- "new" uarea pointer
	/
	__RESUME_P()

	/
	/ See if we need special processing.
	/	(Special processing preserves %ebx and %ecx;
	/	 it expects "old" LWP in %ebp, but doesn't preserve it.)
	/
	movb	_A_LWP_SPECIAL(%ebx),%al
	orb	l+_A_L_SPECIAL_LWP,%al
	jnz	.special
.done_special:

	/
	/ Fill in the uvwin user-visible window with:
	/	the per-LWP private data pointer
	/	the per-LWP FP-used flag
	/
	movl	_A_U_PRIVATEDATAP(%ecx),%eax
	movl	%eax,uvwin+_A_UV_PRIVATEDATAP
	movl	_A_U_FP_USED(%ecx),%eax
	movl	%eax,uvwin+_A_UV_FP_USED

	/
	/ If we're using an FP emulator, do some special processing.
	/
	cmpl	$_A_FP_SW,fp_kind
	je	.resume_fpemul
.resume_fpedone:

	/
	/ Restore the new LWP's register set. %ecx has the pointer to the 
	/ new context's u area.
	/
	movl	_A_U_KCONTEXT+_A_KCTX_EBX(%ecx),%ebx	/ %ebx
	movl	_A_U_KCONTEXT+_A_KCTX_EBP(%ecx),%ebp	/ %ebp
	movl	_A_U_KCONTEXT+_A_KCTX_EDI(%ecx),%edi	/ %edi
	movl	_A_U_KCONTEXT+_A_KCTX_ESI(%ecx),%esi	/ %esi
	movw	_A_U_KCONTEXT+_A_KCTX_FS(%ecx),%fs	/ %fs
	movw	_A_U_KCONTEXT+_A_KCTX_GS(%ecx),%gs	/ %gs
	movl	_A_U_KCONTEXT+_A_KCTX_EAX(%ecx),%eax	/ %eax (for save)
	movl	_A_U_KCONTEXT+_A_KCTX_EIP(%ecx),%ecx	/ %eip

	decl	prmpt_state		/ enable preemption
	jmp	*%ecx			/ jump to the appropriate location

	/
	/ Handle FP emulator-related incoming processing.
	/
	.align	8
.resume_fpemul:
	/ Check if the saved state indicates FP emulation in progress.
	movl	_A_U_AR0(%ecx),%eax
	testl	%eax,%eax
	jz	.resume_fpedone
	cmpw	$_A_FPESEL,[_A_T_CS\*4](%eax)
	jne	.resume_fpedone

	/ The new context was in the middle of an FP instruction when
	/ it switched out.  Restore the saved state, because we won't
	/ get a fault since we're already in the emulator.

	call	fpu_restore
	movl	upointer,%ecx
	jmp	.resume_fpedone

	/
	/ Special handling for special processes.
	/	(Must preserve %ebx and %ecx)
	/
	.align	8
.special:
	movb	_A_LWP_SPECIAL(%ebx),%ah
	movb	%ah,l+_A_L_SPECIAL_LWP

	/ GDT and LDT checks must be first.
	/ They depend on the value in %al, which is the logical OR
	/ of the incoming and outgoing special flags.

	/
	/ If the new LWP has a different GDT than currently loaded, load it.
	/
	testb	$_A_SPECF_PRIVGDT,%al
	jz	.gdt_ok				/ no change in GDT
	lgdt	_A_U_GDT_DESC(%ecx)		/ load the pre-made descriptor
.gdt_ok:

	/
	/ If the new LWP has a different LDT than currently loaded, load it.
	/
	testb	$_A_SPECF_PRIVLDT,%al
	jz	.ldt_ok				/ no change in LDT
	testb	$_A_SPECF_PRIVLDT,%ah		/ switching back to global LDT?
	jz	.global_ldt			/    yes, just load KLDTSEL
	movl	_A_U_GDT_INFOP(%ecx),%eax
	movl	_A_DI_TABLE(%eax),%esi
	movl	_A_U_LDT_DESC(%ecx),%eax
	movl	%eax,_A_KPRIVLDTSEL(%esi)	/ copy u.u_ldt_desc into
	movl	_A_U_LDT_DESC+4(%ecx),%eax	/    gdt[KPRIVLDTSEL]
	movl	%eax,_A_KPRIVLDTSEL+4(%esi)
	movl	$_A_KPRIVLDTSEL,%eax		/ load KPRIVLDTSEL into LDTR
	jmp	.new_ldt
.global_ldt:
	movl	$_A_KLDTSEL,%eax		/ load global KLDTSEL into LDTR
.new_ldt:
	lldt	%ax
.ldt_ok:

	/ See if new LWP has different u_fpintgate for IDT[0xF0].
	/ This assumes that the only IDT entries that might be special
	/ are 0xF0 through 0xFF, and they all have the same value as
	/ u_fpintgate.  Therefore, if the new u_fpintgate is exactly
	/ equal to IDT[0xF0], no changes are needed.
	movl	cur_idtp,%edx
	movl	_A_U_FPINTGATE(%ecx),%eax
	cmpl	%eax,[0xF0\*8](%edx)
	jne	.needfpintgate
	movl	_A_U_FPINTGATE+4(%ecx),%eax
	cmpl	%eax,[0xF0\*8+4](%edx)
	je	.noneedfpintgate
.needfpintgate:
	leal	[0xF0\*8](%edx),%edi		/ point to IDT area
	movl	%edi,%esi
	movl	_A_U_FPINTGATE(%ecx),%eax	/ move first word
	sstol
	movl	_A_U_FPINTGATE+4(%ecx),%eax	/ move second word
	sstol
	pushl	%ecx
	movl	$30,%ecx			/ 15 entries * 2 words each
	rep; smovl				/ fill in the rest
	popl	%ecx
.noneedfpintgate:

	/
	/ If the old or the new LWP has a (different) private TSS,
	/ load or unload it here.
	/
	/ While we're at it, get the address of the esp0 register for the
	/ appropriate tss.
	/
	movl	%ebp,%edx		/ old LWP
	orl	%edx,%edx
	jz	.old_null2
	movl	_A_LWP_TSSP(%edx),%edx	/ old->l_tssp
.old_null2:
	cmpl	%edx,_A_LWP_TSSP(%ebx)	/ if old->l_tssp == new->l_tssp
	je	.tss_ok			/    TSS change not needed
	movl	_A_LWP_TSSP(%ebx),%edx	/ new->l_tssp
	orl	%edx,%edx
	movl	_A_U_GDT_INFOP(%ecx),%eax
	movl	_A_DI_TABLE(%eax),%esi
	jz	.global_tss		/ no private TSS, restore global one
	movl	_A_ST_TSSDESC(%edx),%eax
	movl	%eax,_A_KPRIVTSSSEL(%esi)	/ load GDT entry from l_tssp
	movl	_A_ST_TSSDESC+4(%edx),%eax
	movl	%eax,_A_KPRIVTSSSEL+4(%esi)
	movl	$_A_KPRIVTSSSEL,%eax	/ we're now using KPRIVTSSSEL
	jmp	.load_tr
.global_tss:
	movb	$_A_TSS3_KACC1,_A_KTSSSEL+5(%esi)	/ clear busy bit
	movl	$_A_KTSSSEL,%eax	/ we're now using KTSSSEL
.load_tr:
	ltr	%ax			/ load the Task Register
.tss_ok:

ifdef(`MERGE386',`
	/
	/ Do special MERGE386 processing.  Call a Merge function if the
	/ old or the new LWP is part of a Merge process.
	/
	testl	%ebp,%ebp
	jz	.not_merge_old
	testl	$_A_SPECF_VM86,_A_LWP_SPECIAL(%ebp)
	jz	.not_merge_old
	movl	_A_LWP_UP(%ebp),%eax
	movl	_A_U_VM86P(%eax),%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%eax			/ pass vm86p as argument
	call	*0(%eax)		/ call Merge switch-out routine
	addl	$4,%esp
	popl	%ecx
	popl	%ebx
.not_merge_old:
	testl	$_A_SPECF_VM86,_A_LWP_SPECIAL(%ebx)
	jz	.not_merge_new
	movl	_A_U_VM86P(%ecx),%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%eax			/ pass vm86p as argument
	call	*4(%eax)		/ call Merge switch-in routine
	addl	$4,%esp
	popl	%ecx
	popl	%ebx
.not_merge_new:
')

	/
	/ Turn off debug registers unless a kernel debugger owns them.
	/
	xorl	%ebp,%ebp
ifdef(`NODEBUGGER',`',`
	cmpl	%ebp,user_debug_count
	je	.done_special
')
	movl	%ebp,%db7

	/
	/ Reload debug registers for "new" LWP if necessary.
	/
	cmpl    %ebp,_A_U_KCONTEXT+_A_KCTX_DEBUGON(%ecx)
	je	.done_special
	movl	_A_U_KCONTEXT+_A_KCTX_DBREGS(%ecx),%eax
	movl	%eax,%db0
	movl	_A_U_KCONTEXT+_A_KCTX_DBREGS+4(%ecx),%eax
	movl	%eax,%db1
	movl	_A_U_KCONTEXT+_A_KCTX_DBREGS+8(%ecx),%eax
	movl	%eax,%db2
	movl	_A_U_KCONTEXT+_A_KCTX_DBREGS+12(%ecx),%eax
	movl	%eax,%db3
	movl	_A_U_KCONTEXT+_A_KCTX_DBREGS+28(%ecx),%eax
	movl	%eax,%db7
	jmp	.done_special

.resume_esp_overflow:
	/
	/ An lwp is attempting to switch-out while it was still on the kernel
	/ stack extension page. We try to save the piece of the stack which
	/ resides on the extension page in some memory we are holding just
	/ for this purpose. If ublock_save_extension() returns, then it has
	/ succeeded. If it fails, it will panic.
	/
	/ This design assumes that this code will rarely [if ever] be
	/ executed. No one should rely upon this code.
	/
	negl	%eax			/ size of extension stack in use
	leal	8(%esp),%ecx		/ beginning of in use extension
	pushl	%eax
	pushl	%ecx
	call	ublock_save_extension
	addl	$8,%esp
	movl	upointer,%ecx		/ restore %ecx
	jmp	.resume_esp_ok

.resume_from_overflow:
	/
	/ We are restoring from a stack overflow (onto the extension stack),
	/ so we need to restore the extension piece as well. If this
	/ is a fork, and the extension was therefore not saved, then
	/ ublock_restore_extension() will panic. Luckily, forks() should
	/ use deterministic space [since device drivers and other
	/ variables are not involved], hopefully precluding that case
	/ from the observable failures.
	/
	leal	4(%esp),%eax		/ beginning of in use extension
	pushl	%ecx 			/ size of extension stack in use
	pushl	%eax
	call	ublock_restore_extension
	addl	$8,%esp
	jmp	.resume_unlock

ifdef(`DEBUG',`
.resume_intrpanic: .string "resume: switch-out while servicing interrupt"
')

	SIZE(resume)

/
/ void use_private(lwp_t *lwp, void (*funcp)(lwp_t *), lwp_t *arg)
/
/	Get off of lwp's context (both kernel and user) onto per-processor
/	stack and execute the specified function.
/
/ Calling/Exit State:
/
/	Called holding lwp->l_mutex.  The lock is released after we've
/	cleared off of lwp's context prior to executing the function.
/
/ Description:
/
/	Switch back to the per-processor data structures (stack, uarea).
/	After we've switched off the lwp, we unlock the lwp mutex.
/	We then execute the specified function.
/	Since we are short on registers, we will save some of the args to 
/	this function in the plocal structure.
/	Following is
/	the layout of this save area:
/
/	_______________________
/	|       passed in arg  |
/	|	(funcp)	       |
/	|______________________|
/	|       passed in arg  |
/	|	(argp)	       |
/	|______________________|
/
ENTRY(use_private)
	movl	$l+_A_L_ARGSAVE,%eax
	movl	SPARG1,%ecx		/ funcp.
	movl	%ecx,0(%eax)
	movl	SPARG2,%ecx		/ argp
	movl	%ecx,4(%eax)
	movl	SPARG0,%edi		/ lwpp
	xorl	%ebx,%ebx		/ keep 0 in %ebx for use below
	/
	/ Turn off the FPU if necessary.
	/
	cmpl	%ebx,using_fpu
	je	.use_private_no_fpu
	movl	cur_idtp,%ecx
	movl	fpuoff_noextflt,%eax
	movl	%eax,[_A_NOEXTFLT\*8](%ecx)
	movl	fpuoff_noextflt+4,%eax
	movl	%eax,[_A_NOEXTFLT\*8+4](%ecx)
	movl	l+_A_L_FPUOFF,%eax
	movl	%eax,%cr0
	movl	%ebx,using_fpu
.use_private_no_fpu:

	/
	/ Turn off debug registers unless a kernel debugger owns them.
	/
ifdef(`NODEBUGGER',`',`
	cmpl	%ebx,user_debug_count
	je	.use_private_no_dbreg
')
	movl	%ebx,%db7
.use_private_no_dbreg:

	/
	/ Map out the user-mode portion of the address space.
	/ %edi has the pointer to LWP.
	/
	movl	_A_LWP_PROCP(%edi),%eax
	movl	_A_P_AS(%eax),%eax
	testl	%eax,%eax
	jz	.private_no_as
	pushl   $1		/ B_TRUE flag; hat_asunload must do TLB flush
	pushl	%eax
	call	hat_asunload	/ hat_asunload(lwp->l_procp->p_as, B_TRUE)
	addl	$8,%esp
.private_no_as:
	movl	%edi,%edx		/ edx = lwp

	/
	/ If the outgoing LWP has a private TSS, unload it here.
	/
	testb	$_A_SPECF_PRIVTSS,l+_A_L_SPECIAL_LWP	/ test special flag
	jz	.no_tss			/    TSS change not needed
	/ restore the global TSS
	movl	_A_LWP_UP(%edx),%eax	/ dp = lwp->l_up->u_dt_infop[DT_GDT]
	movl	_A_U_GDT_INFOP(%eax),%eax
	movl	_A_DI_TABLE(%eax),%eax	/ dp->di_table (base of GDT)
	movb	$_A_TSS3_KACC1,_A_KTSSSEL+5(%eax)	/ clear busy bit
	movl	$_A_KTSSSEL,%eax	/ we're now using KTSSSEL
	ltr	%ax			/ load the Task Register
	xorb	$_A_SPECF_PRIVTSS,l+_A_L_SPECIAL_LWP	/ clear special flag
.no_tss:

ifdef(`MERGE386',`
	/
	/ Do special MERGE386 processing.  Call a Merge function if the
	/ old LWP is part of a Merge process.
	/
	testl	$_A_SPECF_VM86,_A_LWP_SPECIAL(%edx)
	jz	.private_not_merge
	movl	_A_LWP_UP(%edx),%eax
	movl	_A_U_VM86P(%eax),%eax
	pushl	%edx
	pushl	%eax			/ pass vm86p as argument
	call	*0(%eax)		/ call Merge switch-out routine
	addl	$4,%esp
	popl	%edx
.private_not_merge:
')
	/
	/ Prior to enabling interrupts, we need to set %esp to the 
	/ stack base of the per-engine stack area.
	movl	$ueng,%esp
	movl	%esp,upointer
	movl	_A_U_PROCP(%esp),%eax
	movl	%eax,uprocp
	incl	prmpt_state		/ Idle stack - non-preemptable
	__ENABLE_ASM			/ enable interrupts as per current ipl

	/ Unlock the entry lock. edx points to the lwp.

	leal	_A_LWP_MUTEX(%edx),%edx
	__UNLOCK_ASM(%edx, $_A_PL0)

	/ Get the function pointer and the arg pointer and execute 
	/ the specified function. 
	movl	$l+_A_L_ARGSAVE,%eax
	movl	0(%eax),%edx		/ funcp. 
	pushl	4(%eax)			/ push the argp.
	call	*%edx			/ execute the function 
	/
	/ We should never get here.
	/+ We returned from the function passed into use_private().
	/+ This is a kernel software problem.
	pushl	$.useprv_panic
	pushl	$_A_CE_PANIC
	call	cmn_err
	/ NOTREACHED (following loop is for stack traces)
	jmp	.

.useprv_panic:	.string	"use_private: returned from the function"

	SIZE(use_private)

/
/ int setjmp(label_t save)
/
/	Setup a non-local goto.
/
/ Calling/Exit State:
/
/	No locks need be held.
/
/ Description:
/
/	Setjmp stores the current register set in the area pointed to
/	by "save".  It returns zero.  Subsequent calls to "longjmp" will
/	restore the registers and return non-zero to the same location.
/
ENTRY(setjmp)
	movl	4(%esp),%edx		/ address of save area
	movl	%edi,(%edx)
	movl	%esi,4(%edx)
	movl	%ebx,8(%edx)
	movl	%ebp,12(%edx)
	movl	%esp,16(%edx)
	movl	(%esp),%ecx		/ %eip (return address)
	movl	%ecx,20(%edx)
	subl	%eax,%eax		/ retval <- 0
	ret

	SIZE(setjmp)

/
/ void longjmp(label_t save)
/
/	Perform a non-local goto.
/
/ Calling/Exit State:
/
/	No locks need be held.
/
/ Description:
/
/	Longjmp initializes the register set to the values saved by a
/	previous setjmp and jumps to the return location saved by that
/	setjmp.  This has the effect of unwinding the stack and returning
/	for a second time to the setjmp.
/
ENTRY(longjmp)
	movl	4(%esp),%edx		/ address of save area
	movl	(%edx),%edi
	movl	4(%edx),%esi
	movl	8(%edx),%ebx
	movl	12(%edx),%ebp
	movl	16(%edx),%esp
	movl	20(%edx),%ecx		/ %eip (return address)
	movl	$1,%eax
	addl	$4,%esp			/ pop ret adr
	jmp	*%ecx			/ indirect

	SIZE(longjmp)

/
/ void fpu_restore(void)
/
/	Restore FPU context in calling process.
/
/ Calling/Exit State:
/
/	Called with the floating point unit disabled.  Returns with
/	the LWP's floating point context restored and the floating point
/	unit enabled.
/
/	The caller must guarantee that this routine is NOT PREEMPTABLE.
/
ENTRY(fpu_restore)
	movl	$1,using_fpu			/ indicate to the context
						/ switch routines that the
						/ processor is currently
						/ using its FPU
	movl	upointer,%edx
	cmpl	$_A_FP_SW,fp_kind		/ see if using emulator
	je	.fpu_restore_emul
	movl	l+_A_L_FPUON,%eax		/ turn on the fpu
	movl	%eax,%cr0
	frstor	_A_U_KCONTEXT+_A_KCTX_FPREGS+_A_FPCHIP_STATE(%edx) / restore FPU
	ret

	.align 4
.fpu_restore_emul:
	pushl	%esi
	pushl	%edi
	leal	_A_U_KCONTEXT+_A_KCTX_FPREGS+_A_FP_EMUL_SPACE(%edx),%esi
	leal	l+_A_L_FPE_KSTATE+_A_FPE_STATE,%edi
	movl	$_A_SIZEOF_FP_EMUL\/4,%ecx
	rep; smovl
	leal	_A_U_FPE_RESTART(%edx),%esi
	movl	$_A_SIZEOF_FPE_RESTART\/4,%ecx
	rep; smovl
	popl	%edi
	popl	%esi
	movl	cur_idtp,%ecx
	movl	fpuon_noextflt,%eax
	movl	%eax,[_A_NOEXTFLT\*8](%ecx)
	movl	fpuon_noextflt+4,%eax
	movl	%eax,[_A_NOEXTFLT\*8+4](%ecx)
	ret

	SIZE(fpu_restore)

/
/ void fpu_disable(void)
/
/	Turn the fpu off.
/
/ Calling/Exit State:
/
/	Called to turn the fpu off without saving any of the fpu
/	context.
/
ENTRY(fpu_disable)
	incl	prmpt_state			/ Disable preemption
	movl	l+_A_L_FPUOFF,%eax
	movl	%eax,%cr0			/ turn off the fpu
	movl	cur_idtp,%ecx
	movl	fpuoff_noextflt,%eax
	movl	%eax,[_A_NOEXTFLT\*8](%ecx)
	movl	fpuoff_noextflt+4,%eax
	movl	%eax,[_A_NOEXTFLT\*8+4](%ecx)
	movl	$0,using_fpu			/ processor not using FPU
	decl	prmpt_state			/ Enable preemption
	ret

	SIZE(fpu_disable)

/
/ void fpu_init(void)
/
/ Calling/Exit State:
/
/	Called when an LWP uses the FPU (or emulator) for the first time.
/	Initializes the floating-point state and sets the control word
/	to the system-wide default.
/
/	The caller must guarantee that this routine is NOT PREEMPTABLE.
/
ENTRY(fpu_init)
	movl	l+_A_L_FPUON,%eax
	movl	%eax,%cr0			/ turn on the fpu
	fninit					/ initialize the FPU
	subl	$4,%esp				/ allocate temp space
	fstcw	(%esp)
	orw	$_A_FPU_INIT_ORMASK,(%esp)	/ set bits from init
	fldcw	(%esp)
	addl	$4,%esp
	ret

	SIZE(fpu_init)

/
/ void fpu_init_old(void)
/	Version of fpu_init for old user binaries
/
ENTRY(fpu_init_old)
	movl	l+_A_L_FPUON,%eax
	movl	%eax,%cr0			/ turn on the fpu
	fninit					/ initialize the FPU
	subl	$4,%esp				/ allocate temp space
	fstcw	(%esp)
	andw	$_A_FPU_INIT_OLD_ANDMASK,(%esp)	/ cleared bits from init
	orw	$_A_FPU_INIT_OLD_ORMASK,(%esp)	/ set bits from init
	fldcw	(%esp)
	addl	$4,%esp
	ret

	SIZE(fpu_init_old)

/
/ user_fpu_init is used when the FPU emulator needs to be initialized.
/ Since the emulator is only accessible from user mode, this code must
/ be executed from user mode.  We accomplish this by copying this code
/ out to the user stack, and arranging for it to be executed there.
/
/ On entry to user_fpu_init, the user stack looks like:
/
/	|		...		|
/	|	original user stack	|
/	+-------------------------------+
/	|	user_fpu_init code	|
/	+-------------------------------+
/	|	scratch word		|
/	+-------------------------------+
/	|	saved user eip		|
/  esp->+-------------------------------+
/
ENTRY(user_fpu_init)
	pushfl
	fninit					/ initialize the FPU
	fstcw	8(%esp)
	orw	$_A_FPU_INIT_ORMASK,8(%esp)	/ set bits from init
	fldcw	8(%esp)
	popfl
	ret	$_user_fpu_size+4	/ return and pop code + scratch word

	.align	4
	.set	_user_fpu_size,.-user_fpu_init

	.data
	.align	4
	.globl	user_fpu_size
user_fpu_size:	.long	_user_fpu_size

	.text

	SIZE(user_fpu_init)

/
/ void user_fpu_init_old(void)
/	Version of user_fpu_init for old user binaries
/
ENTRY(user_fpu_init_old)
	pushfl
	fninit					/ initialize the FPU
	fstcw	8(%esp)
	andw	$_A_FPU_INIT_OLD_ANDMASK,8(%esp) / cleared bits from init
	orw	$_A_FPU_INIT_OLD_ORMASK,8(%esp)	/ set bits from init
	fldcw	8(%esp)
	popfl
	ret	$_user_fpu_o_size+4	/ return and pop code + scratch word

	.align	4
	.set	_user_fpu_o_size,.-user_fpu_init_old

	.data
	.align	4
	.globl	user_fpu_old_size
user_fpu_old_size:	.long	_user_fpu_o_size

	.text

	SIZE(user_fpu_init_old)

/
/ ushort_t fpu_stsw_clex(void)
/
/ Calling/Exit State:
/
/	Called to clear pending FPU exceptions.
/	First saves the current status word, then clears the exceptions.
/	The saved status word is returned.
/
ENTRY(fpu_stsw_clex)
	fnstsw	%ax
	fnclex
	ret

	SIZE(fpu_stsw_clex)


/
/ void
/ init_fpu(void)
/
/	Called at online time to init FPU (387 needs an FNINIT to
/	turn off ERROR input -- this is used to differentiate from a 287).
/
/ Calling State/Exit State:
/
/	Called after l.fpuon, l.fpuoff are set up.
/
/	The caller must guarantee that this routine is NOT PREEMPTABLE.
/

ENTRY(init_fpu)
	movl	l+_A_L_FPUON, %eax		/ want to turn it ON.
	movl	%eax, %cr0			/ it_s on now!
	fninit					/ init FPU state.
	movl	l+_A_L_FPUOFF, %eax		/ No longer using FPU.
	movl	%eax, %cr0			/ it_s off now!
	ret

	SIZE(init_fpu)

/
/ boolean_t
/ check_preemption_f(void)
/
/	Check if the family level code allows preemption. Preemption might
/	be disalowed if %esp is on (or too close to) the extension stack page
/	(depending upon the tunable OVSTACK_PREEMPT).
/
/ Calling/Exit State:
/
/	Returns B_TRUE if preemption is dissallowed, and B_FALSE otherwise.
/

ENTRY(check_preemption_f)
	movl	upointer,%eax
	movl	ovstack_margin,%edx
	leal	_A_UAREA_OFFSET(%esp,%edx,1),%ecx
	cmpl	%eax,%ecx
	jb	.preempt_disable
	xorl	%eax,%eax
	ret
.preempt_disable:
	movl	$1,%eax
	ret

	.data
	.align	4
	.globl	ovstack_margin
ovstack_margin:
	.long	0

	.text

	SIZE(check_preemption_f)

/
/ void
/ setup_seg_regs(void)
/
/	Setup kernel segment registers and task-state register.
/
/ Calling State/Exit State:
/
/	None.
/

ENTRY(setup_seg_regs)
	movl	$_A_KDSSEL, %eax	/ official kernel DS selector.
	movw	%ax, %ds		/ ds, es, and ss
	movw	%ax, %es		/	all use
	movw	%ax, %ss		/	the DS selector.
	xorl	%eax, %eax
	movw	%ax, %fs		/ fs and gs are unused,
	movw	%ax, %gs		/	so set them to NULL.
	/
	/ Arrange long-return, which re-loads code-segment register.
	/
	popl	%eax			/ return EIP.
	pushl	$_A_KCSSEL		/ set up...
	pushl	%eax			/	...long return.
	lret				/ and return, re-loading CS.

	SIZE(setup_seg_regs)
