/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:util/symbols.c	1.52"
#ident	"$Header: $"

/*
 * Generate symbols for use by assembly language files in kernel.
 *
 * This file is compiled using "-S"; "symbols.awk" walks over the assembly
 * file and extracts the symbols.
 */

#include <mem/faultcatch.h>
#include <mem/vmmeter.h>
#include <mem/vmparam.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/regset.h>
#include <proc/seg.h>
#include <proc/user.h>
#include <svc/cpu.h>
#include <svc/creg.h>
#include <svc/fp.h>
#include <svc/reg.h>
#include <svc/trap.h>
#include <svc/time.h>
#ifdef WEITEK
#include <svc/weitek.h>
#endif
#include <util/cmn_err.h>
#include <util/engine.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#ifdef DEBUG_TRACE
#include <util/nuc_tools/trace/nwctrace.h>
#endif /* DEBUG_TRACE */

#define	offsetof(x, y)	((int)&((x *)0)->y)
#define	OFFSET(s, st, m) \
	size_t __SYMBOL___A_##s = offsetof(st, m)

#define	DEFINE(s, e) \
	size_t __SYMBOL___A_##s = (size_t)(e)

/*
 * Control Register 0 (CR0) and 4 (CR4) definitions.
 */

DEFINE(CR0_PG, CR0_PG);
DEFINE(CR0_CD, CR0_CD);
DEFINE(CR0_NW, CR0_NW);
DEFINE(CR0_AM, CR0_AM);
DEFINE(CR0_WP, CR0_WP);
DEFINE(CR0_NE, CR0_NE);
DEFINE(CR0_TS, CR0_TS);
DEFINE(CR0_EM, CR0_EM);
DEFINE(CR0_MP, CR0_MP);
DEFINE(CR0_PE, CR0_PE);
DEFINE(CR4_PSE, CR4_PSE);

/*
 * Misc kernel virtual addresses and constants.
 */

DEFINE(KVPLOCAL, KVPLOCAL);
DEFINE(KVPLOCALMET, KVPLOCALMET);
DEFINE(KVMET, KVMET);
DEFINE(KVUENG, KVUENG);
DEFINE(KVUVWIN, KVUVWIN);
DEFINE(KVENG_L1PT, KVENG_L1PT);
DEFINE(KVSYSDAT, KVSYSDAT);
DEFINE(TIMESTR_SIZ, sizeof(timestruc_t));

/*
 * Offset within an LWP ublock where the uarea begins.
 */

DEFINE(UAREA_OFFSET, UAREA_OFFSET);

/*
 * Processor local fields ("l.").
 */

OFFSET(L_ARGSAVE, struct plocal, argsave);
OFFSET(L_FPUON, struct plocal, fpuon);
OFFSET(L_FPUOFF, struct plocal, fpuoff);
OFFSET(L_USINGFPU, struct plocal, usingfpu);
OFFSET(L_MICRODATA, struct plocal, microdata);

/*
 * Processor local metric fields ("lm.").
 */

OFFSET(LM_CNT, struct plocalmet, cnt);

OFFSET(L_EVENTFLAGS, struct plocal, eventflags);
DEFINE(EVT_RUNRUN, EVT_RUNRUN);
DEFINE(EVT_KPRUNRUN, EVT_KPRUNRUN);
DEFINE(EVT_UPREEMPT, EVT_UPREEMPT);

OFFSET(L_IDTP, struct plocal, idtp);
OFFSET(L_TSS, struct plocal, tss);
OFFSET(L_TRAP_ERR_CODE, struct plocal, trap_err_code);
#if defined DEBUG && defined _LOCKTEST
OFFSET(L_HOLDFASTLOCK, struct plocal, holdfastlock);
OFFSET(L_FSPIN, struct plocal, fspin);
#endif /* DEBUG && _LOCKTEST */
OFFSET(L_INTR_DEPTH, struct plocal, intr_depth);
OFFSET(L_ENG, struct plocal, eng);
OFFSET(L_ENG_NUM, struct plocal, eng_num);
OFFSET(L_NMI_HANDLER, struct plocal, nmi_handler);
OFFSET(L_USERP, struct plocal, userp);
OFFSET(L_CURPROCP, struct plocal, procp);
OFFSET(L_CPU_ID, struct plocal, cpu_id);
OFFSET(L_CPU_MODEL, struct plocal, cpu_model);
OFFSET(L_CPU_STEPPING, struct plocal, cpu_stepping);
OFFSET(L_CPU_FEATURES, struct plocal, cpu_features);
OFFSET(L_PRMPT_STATE, struct plocal, prmpt_state);
#ifdef _MPSTATS
OFFSET(L_UPRMPTCNT, struct plocal, prmpt_user);
OFFSET(L_KPRMPTCNT, struct plocal, prmpt_kern);
#endif /* _MPSTATS */
OFFSET(L_KSE_PTE, struct plocal, kse_pte);
OFFSET(L_SPECIAL_LWP, struct plocal, special_lwp);
OFFSET(L_FPE_KSTATE, struct plocal, fpe_kstate);
#ifdef MERGE386
OFFSET(L_VM86_IDTP, struct plocal, vm86_idtp);
#endif /* MERGE386 */

/*
 * FP emul struct offsets
 */

OFFSET(FPE_STATE, struct fpemul_kstate, fpe_state);

/*
 * lock_t member offsets
 */

OFFSET(SP_LOCK, lock_t, sp_lock);
#if (defined DEBUG || defined SPINDEBUG)
OFFSET(SP_FLAGS, lock_t, sp_flags);
OFFSET(SP_MINIPL, lock_t, sp_minipl);
OFFSET(SP_HIER, lock_t, sp_hier);
OFFSET(SP_VALUE, lock_t, sp_value);
OFFSET(SP_LKSTATP, lock_t, sp_lkstatp);
OFFSET(SP_INFOP, lock_t, sp_lkinfop);
#endif /* (DEBUG || SPINDEBUG)  */

/*
 * rwlock_t member offsets
 */

OFFSET(RWS_FSPIN, rwlock_t, rws_fspin);
OFFSET(RWS_LOCK, rwlock_t, rws_lock);
OFFSET(RWS_RDCOUNT, rwlock_t, rws_rdcount);
OFFSET(RWS_STATE, rwlock_t, rws_state);
#if (defined DEBUG || defined SPINDEBUG) 
OFFSET(RWS_FLAGS, rwlock_t, rws_flags);
OFFSET(RWS_HIER, rwlock_t, rws_hier);
OFFSET(RWS_MINIPL, rwlock_t, rws_minipl);
OFFSET(RWS_VALUE, rwlock_t, rws_value);
OFFSET(RWS_LKSTATP, rwlock_t, rws_lkstatp);
OFFSET(RWS_INFOP, rwlock_t, rws_lkinfop);
#endif /* (DEBUG || SPINDEBUG)  */

/*
 * sleep and rwsleep member offsets
 */
OFFSET(SL_AVAIL, sleep_t, sl_avail);
OFFSET(RW_AVAIL, rwsleep_t, rw_avail);

/*
 * lkstat_t member offsets
 */

OFFSET(LKS_INFOP, lkstat_t, lks_infop);
OFFSET(LKS_WRCNT, lkstat_t, lks_wrcnt);
OFFSET(LKS_RDCNT, lkstat_t, lks_rdcnt);
OFFSET(LKS_SOLORDCNT, lkstat_t, lks_solordcnt);
OFFSET(LKS_FAIL, lkstat_t, lks_fail);
OFFSET(LKS_STIME, lkstat_t, lks_stime);
OFFSET(LKS_NEXT, lkstat_t, lks_next);
OFFSET(LKS_WTIME, lkstat_t, lks_wtime);
OFFSET(LKS_HTIME, lkstat_t, lks_htime);


/*
 * offsets within the lwp structure.
 */

OFFSET(LWP_MUTEX, lwp_t, l_mutex);
OFFSET(LWP_PROCP, lwp_t, l_procp);
OFFSET(LWP_TRAPEVF, lwp_t, l_trapevf);
OFFSET(LWP_TSSP, lwp_t, l_tssp);
OFFSET(LWP_UP, lwp_t, l_up);
OFFSET(LWP_BPT, lwp_t, l_beingpt);
OFFSET(LWP_SPECIAL, lwp_t, l_special);
OFFSET(LWP_START, lwp_t, l_start);

/*
 * TSS-related offsets.
 */

OFFSET(ST_TSS, struct stss, st_tss);
OFFSET(ST_TSSDESC, struct stss, st_tssdesc);
OFFSET(T_ESP0, struct tss386, t_esp0);

/*
 * offsets within the proc structure
 */

OFFSET(P_AS, proc_t, p_as);

/*
 * Defines for user structure innards.
 */

OFFSET(U_LWPP, struct user, u_lwpp);
OFFSET(U_PROCP, struct user, u_procp);
OFFSET(U_PRIVATEDATAP, struct user, u_privatedatap);
OFFSET(U_FP_USED, struct user, u_fp_used);
OFFSET(U_FAULT_CATCH, struct user, u_fault_catch);
OFFSET(U_KSE_PTEP, struct user, u_kse_ptep);
OFFSET(U_FPINTGATE, struct user, u_fpintgate);
OFFSET(U_GDT_INFOP, struct user, u_dt_infop[DT_GDT]);
OFFSET(U_GDT_DESC, struct user, u_gdt_desc);
OFFSET(U_LDT_INFOP, struct user, u_dt_infop[DT_LDT]);
OFFSET(U_LDT_DESC, struct user, u_ldt_desc);
OFFSET(U_AR0, struct user, u_ar0);
OFFSET(U_FPE_RESTART, struct user, u_fpe_restart);
DEFINE(SIZEOF_FPE_RESTART, sizeof u.u_fpe_restart);
#ifdef MERGE386
OFFSET(U_VM86P, struct user, u_vm86p);
#endif /* MERGE386 */
DEFINE(SIZEOF_USER, sizeof(struct user));

/*
 * Offsets within the uvwindow structure.
 */

OFFSET(UV_PRIVATEDATAP, struct uvwindow, uv_privatedatap);
OFFSET(UV_FP_USED, struct uvwindow, uv_fp_used);

/*
 * Defines for kcontext innards.
 */

OFFSET(U_KCONTEXT, struct user, u_kcontext);

OFFSET(KCTX_EBX, kcontext_t, kctx_ebx);
OFFSET(KCTX_EBP, kcontext_t, kctx_ebp);
OFFSET(KCTX_EDI, kcontext_t, kctx_edi);
OFFSET(KCTX_ESI, kcontext_t, kctx_esi);
OFFSET(KCTX_EIP, kcontext_t, kctx_eip);
OFFSET(KCTX_ESP, kcontext_t, kctx_esp);
OFFSET(KCTX_EAX, kcontext_t, kctx_eax);
OFFSET(KCTX_ECX, kcontext_t, kctx_ecx);
OFFSET(KCTX_EDX, kcontext_t, kctx_edx);
OFFSET(KCTX_FS, kcontext_t, kctx_fs);
OFFSET(KCTX_GS, kcontext_t, kctx_gs);
OFFSET(KCTX_FPREGS, kcontext_t, kctx_fpregs);
OFFSET(FPCHIP_STATE, fpregset_t, fp_reg_set.fpchip_state);
OFFSET(FP_EMUL_SPACE, fpregset_t, fp_reg_set.fp_emul_space);
OFFSET(KCTX_DEBUGON, kcontext_t, kctx_debugon);
OFFSET(KCTX_DBREGS, kcontext_t, kctx_dbregs);

DEFINE(SIZEOF_FP_EMUL, sizeof(struct fpemul_state));

/*
 * defines for fault_catch_t
 */

OFFSET(FC_FLAGS, fault_catch_t, fc_flags);
OFFSET(FC_FUNC, fault_catch_t, fc_func);
DEFINE(CATCH_ALL_FAULTS, CATCH_ALL_FAULTS);

/*
 * Offsets within desctab_info
 */

OFFSET(DI_TABLE, struct desctab_info, di_table);

/*
 * Masks used to initialized the FPU after an fninit.
 */

DEFINE(FPU_INIT_OLD_ANDMASK, ~(FPINV|FPZDIV|FPOVR|FPPC));
DEFINE(FPU_INIT_OLD_ORMASK, FPSIG53|FPA);
DEFINE(FPU_INIT_ORMASK, FPA);

/*
 * Selector definitions.
 */

DEFINE(KTSSSEL, KTSSSEL);
DEFINE(KPRIVLDTSEL, KPRIVLDTSEL);
DEFINE(KPRIVTSSSEL, KPRIVTSSSEL);
DEFINE(KCSSEL, KCSSEL);
DEFINE(KDSSEL, KDSSEL);
DEFINE(KLDTSEL, KLDTSEL);
DEFINE(FPESEL, FPESEL);
DEFINE(SEL_RPL, SEL_RPL);

DEFINE(TSS3_KACC1, TSS3_KACC1);

/*
 * vmmeter l.cnt fields
 */

OFFSET(V_INTR, struct vmmeter, v_intr);

/*
 * Register offsets in stack locore frames.
 */

DEFINE(INTR_SP_CS, INTR_SP_CS*sizeof(int));
DEFINE(INTR_SP_IP, INTR_SP_IP*sizeof(int));
DEFINE(SP_CS, SP_CS*sizeof(int));
DEFINE(SP_EIP, SP_EIP*sizeof(int));
DEFINE(SP_EFL, SP_EFL*sizeof(int));
DEFINE(T_CS, T_CS);

/*
 * Trap type mnemonics.
 */

DEFINE(DIVERR, DIVERR);
DEFINE(SGLSTP, SGLSTP);
DEFINE(NMIFLT, NMIFLT);
DEFINE(BPTFLT, BPTFLT);
DEFINE(INTOFLT, INTOFLT);
DEFINE(BOUNDFLT, BOUNDFLT);
DEFINE(INVOPFLT, INVOPFLT);
DEFINE(NOEXTFLT, NOEXTFLT);
DEFINE(DBLFLT, DBLFLT);
DEFINE(EXTOVRFLT, EXTOVRFLT);
DEFINE(INVTSSFLT, INVTSSFLT);
DEFINE(SEGNPFLT, SEGNPFLT);
DEFINE(STKFLT, STKFLT);
DEFINE(GPFLT, GPFLT);
DEFINE(PGFLT, PGFLT);
DEFINE(EXTERRFLT, EXTERRFLT);
DEFINE(ALIGNFLT, ALIGNFLT);
DEFINE(MCEFLT, MCEFLT);
DEFINE(TRP_PREEMPT, TRP_PREEMPT);
DEFINE(TRP_UNUSED, TRP_UNUSED);

/*
 * CPU/FPU type defines.
 */

DEFINE(CPU_386, CPU_386);
DEFINE(CPU_486, CPU_486);
DEFINE(CPU_P5, CPU_P5);
DEFINE(FP_NO, FP_NO);
DEFINE(FP_SW, FP_SW);
DEFINE(FP_287, FP_287);
DEFINE(FP_387, FP_387);
#ifdef WEITEK
DEFINE(WEITEK_HW, WEITEK_HW);
DEFINE(WEITEK_20MHz, WEITEK_20MHz);
#endif /* WEITEK */

/*
 * Misc defines.
 */

DEFINE(UVBASE, UVBASE);
DEFINE(CE_PANIC, CE_PANIC);
DEFINE(SP_LOCKED, SP_LOCKED);
DEFINE(SP_UNLOCKED, SP_UNLOCKED);
DEFINE(KS_LOCKTEST, KS_LOCKTEST);
DEFINE(KS_MPSTATS, KS_MPSTATS);
DEFINE(RWS_READ, RWS_READ);
DEFINE(RWS_WRITE, RWS_WRITE);
DEFINE(RWS_UNLOCKED, RWS_UNLOCKED);
DEFINE(B_TRUE, B_TRUE);
DEFINE(B_FALSE, B_FALSE);
DEFINE(TRAPEXIT_FLAGS, TRAPEXIT_FLAGS);
DEFINE(RWS_LOCKTYPE, RWS_LOCKTYPE);
DEFINE(SP_LOCKTYPE, SP_LOCKTYPE);
DEFINE(NBPW, NBPW);
DEFINE(PAGESIZE, PAGESIZE);
DEFINE(SPECF_PRIVGDT, SPECF_PRIVGDT);
DEFINE(SPECF_PRIVLDT, SPECF_PRIVLDT);
DEFINE(SPECF_PRIVTSS, SPECF_PRIVTSS);
#ifdef MERGE386
DEFINE(SPECF_VM86, SPECF_VM86);
#endif /* MERGE386 */
DEFINE(KSTACK_RESERVE, KSTACK_RESERVE);

#ifdef DEBUG_TRACE_LOCKS

DEFINE(NVLTT_rwLockReadWait, NVLTT_rwLockReadWait);
DEFINE(NVLTT_rwLockReadGet, NVLTT_rwLockReadGet);
DEFINE(NVLTT_rwLockWriteGet, NVLTT_rwLockWriteGet);
DEFINE(NVLTT_rwLockWriteWait, NVLTT_rwLockWriteWait);
DEFINE(NVLTT_rwTryReadGet, NVLTT_rwTryReadGet);
DEFINE(NVLTT_rwTryReadFail, NVLTT_rwTryReadFail);
DEFINE(NVLTT_rwTryWriteGet, NVLTT_rwTryWriteGet);
DEFINE(NVLTT_rwTryWriteFail, NVLTT_rwTryWriteFail);
DEFINE(NVLTT_rwLockFree, NVLTT_rwLockFree);

OFFSET(LK_NAME, lkinfo_t, lk_name);
DEFINE(KS_NVLTTRACE, KS_NVLTTRACE);

#endif /* DEBUG_TRACE_LOCKS */
