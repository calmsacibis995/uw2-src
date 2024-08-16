/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/ast/astpic.c	1.11"
#ident	"$Header: $"

/* 
 * astpic.c:  Low Level Routines to Program PIC
 *
 * Modifications:
 * M001:  7/11/94 mgr 	Added code to set up EISA ELCR for level triggered IRQs
 */
#include <svc/bootinfo.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <util/engine.h>
#include <util/ipl.h>
#include <util/plocal.h>
#include <svc/pic.h>
#include <svc/eisa.h>

#include <ebi.h>
#include <ast.h>

extern void          (*ivect[])();
extern int           nintr;
extern unsigned char intpri[];
extern uint_t        myengnum;
extern int           intcpu[];
extern void          picreload();
extern unsigned long level_intr_mask;	/* M001 */

/*
 * FUNCTION PROTOTYPES
 *
 * XXX -- should move to ast.h header file
 */
void astpicinit(int engnum);
void astpicstart(void);
#ifdef AST_DEBUG
void astpicprint(void);
#endif
void nenableint(int iv, pl_t level, int engnum, int itype);
void ndisableint(int iv, pl_t level, int engnum, int itype);
void intnull(int vect);

STATIC	void holdcpus(void *arg);

/*
 * GLOBAL DATA
 */
/*
 * NOTE: ast_picmask is defined with the ipl as the row
 * because it was possible to write much faster assembly code
 * for setpicmasks since a multiply by 9 did not have to be
 * done.
 */
unsigned int ast_picmask[PLHI+1][EBI_MAX_CPUS];
unsigned int ast_curmask[EBI_MAX_CPUS];
unsigned int ast_int_handle[EBI_MAX_CPUS];

struct irqtab irqtab[NINT];
pl_t          svcpri[NINT];

/*
 * for soft spl code.
 */
int picdeferndx[EBI_MAX_CPUS];
int picdeferred[EBI_MAX_CPUS][PLHI + 1];
int *picdeferredp[EBI_MAX_CPUS];
extern k_pl_t ipl;		/* per-processor ipl */
extern pl_t picipl;		/* ipl to which pic is currently programmed */

/* 
 * Private global data 
 */
static volatile uint_t iv_cookie;
static unsigned int isubtype;

/*
 * void
 * astpicinit(int engnum)
 *	Initialize Peripheral Interrupt Controller.
 *  
 * Calling/Exit States:
 *	Called by: 	psm_intr_init() in ast.c
 *	Parameters:
 *		engnum	-- engine number
 *	Locking: 	none
 *	Globals read:	ast_calltab, MMIOTable	
 *	Globs. written: none
 *	Returns:	none
 *
 * Description:
 *	Initialize PIC in either "advanced" interrupt (MPIC) mode or
 *	8259-compatible mode.
 *
 * Remarks:
 *	Leaves interrupts masked at this CPU on exit.
 */
void
astpicinit(int engnum)
{
	int intno;
	struct irqtab *ip;
	int i, pl;
	unsigned int handle;
	
	/*
	 * Mask interrupts on this CPU.  Leave masked until astpicstart() runs.
	 */
    asm(" cli");
	
	if (!IS_BOOT_ENG(engnum))
	{
		goto piccommon;		/*** GOTO LABLE piccommon ***/
	}
	
	if ((ast_calltab.GetIntSubsysType)(MMIOTable, &isubtype) != OK)
	{
		/*
		 *+
		 */
		cmn_err(CE_PANIC, "ast_picinit: cannot get subsystem type");
	}

	/*
	 * kick this machine into "advanced" interrupt mode
	 * and initialize the interrupt hardware.
	 */
	if (isubtype == INT_TYPE_MPIC || isubtype == INT_TYPE_ADI) 
	{
		if ((ast_calltab.SetAdvIntMode)(MMIOTable) != OK) 
		{
			/*
		 	 *+
		 	 */
			cmn_err(CE_PANIC, "ast_picinit: unable to set adv. interrupt mode.");
		}
	}

	if (isubtype != INT_TYPE_MPIC)
	{
		/*
		 * set the base interrupt vector for the first PIC...
		 */
		if ((ast_calltab.SetIRQVectorAssign)(MMIOTable, 0, PIC_VECTBASE) != OK) 
		{
			/*
		 	 *+
		 	 */
			cmn_err(CE_PANIC, "ast_picinit: cannot set pic vectbase for irq 0");
		}
		/*
		 * set the base interrupt vector for the second PIC...
		 */
		if ((ast_calltab.SetIRQVectorAssign)(MMIOTable, 8, PIC_VECTBASE + 8) != OK) 
		{
			/*
		 	 *+
		 	 */
			cmn_err(CE_PANIC, "ast_picinit: cannot set pic vectbase for irq 8");
		}
	} else {
		for (i = 0; i < NINT; i++) 
		{
			/*
			 * set up interrupt vectors for MPIC based system.
			 */
			if (i == LSI || i == SPI || i == IPI)
			{
				/*
				 * we set these later.
				 */
				continue;
			}
			
			if ((ast_calltab.SetIRQVectorAssign)(MMIOTable, i, 
												 PIC_VECTBASE + i) != OK) 
			{
				/*
		 	 	 *+
		 	 	 */
				cmn_err(CE_PANIC, "ast_picinit: cannot set vector for irq %d", i);
			}
		}
	}

	/*
	 * XXX: we need to change the handling of of level_intr_mask.
	 * Currently this is setup in conf.c and reflects the configuration
	 * contained in the sdevice files.  
	 */
	ip = irqtab;
	for (intno = 0; intno < NINT; intno++, ip++) 
	{
		ip->irq_cmdport = 0;
		ip->irq_flags = ((1 << intno) & CHKSPUR) ? IRQ_CHKSPUR : 0;

		/* M001:
		 * Set Level Mode for those IRQ's requiring it.
		 */
		if (level_intr_mask & (1 << intno)) 
			eisa_set_elt(intno, LEVEL_TRIG);
	}
	
piccommon:	/*** LABEL piccommon ***/
	/*
	 * get the logical ID of *this* cpu and stash it, we'll need it
	 * later. 
	 */
	if ((ast_calltab.GetProcID)(MMIOTable, &ast_cpuid[engnum]) != OK) 
	{
		/*
		 *+
		 */
		cmn_err(CE_PANIC, "ast_picinit: unable to get cpuid");
	}

	/*
	 * set up LSI SPI and IPI vectors. -1 indicates that operation should
	 * be performed on all processors at once.  
	 *
	 * XXX: actually, the code in the BIOS does not special case for -1
	 * and as a result the thing crashes.  We'll set these vectors as
	 * the processors come up.
	 */
	if ((ast_calltab.SetSPIVector)(MMIOTable, ast_cpuid[engnum], SPI_VECT) != OK)
	{
		/*
		 *+
		 */
		cmn_err(CE_PANIC, "ast_picinit: cannot set SPI vector");
	}

	if ((ast_calltab.SetLSIVector)(MMIOTable, ast_cpuid[engnum], LSI_VECT) != OK)
	{
		/*
		 *+
		 */
		cmn_err(CE_PANIC, "ast_picinit: cannot set LSI vector");
	}
		 
	if ((ast_calltab.SetIPIVector)(MMIOTable, ast_cpuid[engnum], IPI_VECT) != OK)
	{
		/*
		 *+
		 */
		cmn_err(CE_PANIC, "ast_picinit: cannot set IPI vector");
	}

	if ((ast_calltab.GetProcIntHandle)(MMIOTable, ast_cpuid[engnum], 
									   &handle) != OK)
	{
		/*
		 *+
		 */
		cmn_err(CE_PANIC, "ast_picinit: cannot get interrupt handle");
	}

	ast_int_handle[engnum] = handle;

	if ((ast_calltab.SetIPIID)(MMIOTable, ast_cpuid[engnum], IPI_ID(engnum)) != OK)
	{
		/*
		 *+
		 */
		cmn_err(CE_PANIC, "ast_picinit: cannot set IPI Id for %d", engnum);
	}
	
	/*
	 * set up the deferred interrupt stack for soft-spl code.
	 * the other PSM's set this up in a separate routine called 
	 * immediately before this routine is called.  This seems to 
	 * be a relic from earlier times, so I'm just doing it here.
	 */
	picdeferndx[engnum] = 0;
	picdeferredp[engnum] = (int *)&picdeferred[engnum];
	picdeferredp[engnum][0] = NINT;
	
	/*
	 * initially set up masks to mask all interrupts at all levels.
	 */
	for (pl = PLBASE; pl <= PLHI; pl++) 
	{
		ast_picmask[pl][engnum] = 0x07ffffff; 
	}

	ipl = picipl = PLBASE;
	
	(ast_calltab.FastSetLocalIntMask)(handle, ast_curmask[engnum] = 0x07ffffff); 
}

/*
 * void
 * astpicstart(void)
 *	Set PIC mask to allow IPI and slave interrupt. Unmask interrupts at CPU.
 *
 * Calling/Exit States:
 *	Called by: 	psm_intr_init() in ast.c
 *	Parameters:	none
 *	Locking: 	none
 *	Globals read:	upyet, isubtype, ast_picmask[]
 *	Globs. written: ast_picmask[]
 *	Returns:	none
 */
void
astpicstart(void)
{
	int i;
	int intno, level;

	/*
	 * If astconfigure() has been run (evidenced by "upyet"),
	 * and ordinary (8259-compatilble) interrupt mode is selected,
	 * then set up PIC masks for standard configuration.
	 */
	if (upyet && isubtype != INT_TYPE_MPIC)
	{
		for (i = PLBASE; i <= PLHI; i++)
		{
			/*
			 * enable reception of IPI.  We are not going to use LSI
			 * so we don't enable it. 
			 */
			ast_picmask[i][myengnum] &= ~(IRQBIT(IPI));
		}

		/*
		 * enable reception of the slave interrupt
		 */
		for (i = PLBASE; i < PLHI; i++)
		{
			ast_picmask[i][myengnum] &= ~(IRQBIT(ATSLAVE));
		}

		picreload();
		/*
		 * unmask interrupts at CPU and away we go!
 		 */
		asm(" sti");
		return;
	}
	
	/*
	 * If we get to here, we are setting up for MPIC mode.
	 *
	 * Initialize PIC-related data structures:
	 *  (1)	Set up ast_picmask[][] from ivect[] and intpri[].  If an
	 *	interrupt number is configured, set its bit in the masks
	 *	at its priority level and higher.  Otherwise, mask it out
 	 *	for all priority levels, including PLBASE.  (Note that
	 *	interrupts above PLHI are never masked, since the
	 *	system never runs above PLHI, and iplmask only includes
	 *	entries up to and including PLHI, but not beyond it.)
	 *
	 *  (2)	Initialize the svcpri array.  For each hardware interrupt,
	 *	svcpri gives the priority level at which the interrupt service
	 *	routine should run.  The service priority level is the same
	 *	as the interrupt priority level, specified in intpri, unless
	 * 	the interrupt pl is above PLHI; in such cases, the service
	 *	pl is PLHI.  This is part of enforcing the constraint that no
	 *	code runs above PLHI.
	 */
	for (intno = 0; intno < nintr; ++intno)
	{
		if (intno >= nintr || ivect[intno] == (void (*)())intnull)
		{
			level = 0;
		}
		else if ((level = intpri[intno]) == 0 || level > PLMAX) 
		{
			/*
			 *+ The intpri array contains an invalid pl value.  
			 *+ Corrective action: ensure that IPLs assigned
			 *+ in sdevice files are less than or equal to PLMAX, 
			 *+ then rebuild the kernel and reboot.
			 */
			cmn_err(CE_WARN, "bad interrupt priority in intpri[]: %d", level);
			level = 0;
		}

		/*
		 * handle interrupt handler binding. A -1 indicates that
		 * the interrupt is not bound.
		 */
		if (intcpu[intno] != -1 && intcpu[intno] != myengnum)
		{
			level = 0;
		}

		/*
		 * if we are not an MPIC based system, we need to skip IRQ 2
		 * since that is the line the slave interrupts the master on.
		 * MPIC does not appear to dedicate IRQ 2 to any purpose, so
		 * we will treat it as a normal IRQ.
		 */
		if (intno == ATSLAVE && isubtype != INT_TYPE_MPIC) 
		{
			for (i = PLBASE; i < PLHI; i++)
			{
				ast_picmask[i][myengnum] &= ~IRQBIT(intno);
			}
			svcpri[intno] = MIN(intpri[intno], PLHI);
			continue;
		}
		
		/*
		 * if we are on an ADI system, the code in picinit initially blocks
		 * out all interrupts at all levels.  We need to enable the interrupt
		 * at levels below intpri and enable them at levels above intpri.
		 *
		 * interrupts which need to be routed to other cpu's will be routed
		 * to those cpu's as they are brought online but the initial state 
		 * for cpus other than BOOTENG is to have all interrupts disabled.
		 */
		for (i = PLBASE; i < level; i++)
			ast_picmask[i][myengnum] &= ~IRQBIT(intno);
		
		for ( ; level <= PLHI; level++)
			ast_picmask[level][myengnum] |= IRQBIT(intno);

		/*
	 	 * clear bits documented as unused but reserved.
		 * these bits are actually used for diags and *must*
		 * be zero.
		 */
		for (i = PLBASE; i <= PLHI; i++)
			ast_picmask[i][myengnum] &= ~0xf8000000;

		if (IS_BOOT_ENG(myengnum))
		{
			svcpri[intno] = MIN(intpri[intno], PLHI);
		}
	}

#ifdef DEBUG
	/*
	 * enable SPI at PLHI so that debugger can be entered
	 * by pushing ATTENTION switch.
	 */
	if (IS_BOOT_ENG(myengnum))
	{
		ast_picmask[PLHI][myengnum] &= ~IRQBIT(SPI);
	}
#endif

	picreload();
	/*
	 * unmask interrupts at CPU and away we go!
 	 */
	asm(" sti");
}

#ifdef AST_DEBUG
/*
 * void
 * astpicprint(void)
 * 	Print values of SPI, ILI, and LSI vectors, and also the vector
 *	assignment from the EBI for each of the 16 IRQs.
 *
 * Calling/Exit States:
 *	Called by: 	can be called with kernel debugger
 *	Parameters:	none
 *	Locking: 	none
 *	Globals read:	ast_calltab, MMIOTable	
 *	Globs. written: none
 *	Returns:	none
 *
 * Remarks:
 *	Used for debugging purposes from kernel debugger only.
 */
void
astpicprint(void)
{
	unsigned int spivect;
	unsigned int ipivect;
	unsigned int lsivect;
	unsigned int irqvect;
	unsigned int i;
	
	(ast_calltab.GetSPIVector)(MMIOTable, ast_cpuid[myengnum], &spivect);
	(ast_calltab.GetIPIVector)(MMIOTable, ast_cpuid[myengnum], &ipivect);
	(ast_calltab.GetLSIVector)(MMIOTable, ast_cpuid[myengnum], &lsivect);
	cmn_err(CE_CONT, "IPI = %x SPI = %x, LSI = %x\n", ipivect, spivect, lsivect);

	for (i = 0; i < 16; i++)
	{
		(ast_calltab.GetIRQVectorAssign)(MMIOTable, i, &irqvect);
		cmn_err(CE_CONT, "IRQ %x = %x\n", i, irqvect);
	}
}
#endif	/* ifdef AST_DEBUG */

/*
 * STATIC void 
 * holdcpus(void *arg)
 *	Hold the cpu executing this routine until the value
 *	of the iv_cookie changes.
 *
 * Calling/Exit States:
 *	Called by: 	via xcall()
 *	Parameters:	none
 *	Locking: 	must be at PLHI at entry
 *	Globals read:	iv_cookie
 *	Globs. written: none
 *	Returns:	none
 *
 * Remarks:
 *	Called through xcall, the cpu is already in PLHI.
 */
STATIC void
holdcpus(void *arg)
{
	while (iv_cookie == (uint_t)arg)
		;
	picreload();
}

/*
 * void
 * nenableint(int iv, pl_t level, int engnum, int itype)
 *	Unmask the interrupt vector from the iplmask.
 *
 * Calling/Exit States:
 *	Called by: 	psm_intron() in ast.c
 *	Parameters:	none
 *		iv -- interrupt request no. that needs to be enabled.
 *		engnum  -- engine on which the interrupt must be unmasked.
 *		level -- interrupt priority level of the iv.
 *		itype -- interrupt type.
 *	Locking: 	must hold mod_iv_lock on entry
 *	Globals read:	myengnum
 *	Globs. written: iv_cookie
 *	Returns:	none
 *
 * Remarks:
 *	mod_iv_lock spin lock is held on entry/exit.
 */
/*ARGSUSED*/
void
nenableint(int iv, pl_t level, int engnum, int itype)
{
	unsigned int mask;
	int	i;
	struct emask iv_emask;


	if (nonline > 1)
	{
		xcall_all(&iv_emask, B_FALSE, holdcpus, (void *)iv_cookie);
	}

        if (bootinfo.machflags & EISA_IO_BUS) {
                if (itype == 4) {
                        eisa_set_elt(iv, LEVEL_TRIG);
                }
        }
	
	/* change PIC mask */

	mask = ~IRQBIT(iv);

	for (i = PLBASE; i < (int)level; i++)
	{
		ast_picmask[i][engnum] &= mask;
	}
	
	mask = ~mask;

	/*
	 * Mask/Set the interrupt's bit in iplmask[]
	 * for priorities greater than, or equal to its own.
	 */
	for (; i <= PLHI; i++) 
	{
		ast_picmask[i][engnum] |= mask;
	}

	/*
	 * Reload pic masks for the local so that mask modifications 
	 * take effect immediately. For the non-local engines the
	 * return from xcall will reload the new picmask.
	 */
	if (myengnum == engnum)
	{
		picreload();
	}

	/* Done with pic mask change, release all the cpus */

	++iv_cookie;
}


/*
 * void
 * ndisableint(int iv, pl_t level, int engnum, int itype)
 *	Mask the interrupt vector in the iplmask of the engine currently
 *	running on.
 *
 * Calling/Exit State:
 *	Called by: 	psm_introff() in ast.c	
 *	Parameters:
 *		iv -- interrupt that needs to be disabled.
 *		engnum -- engine on which the interrupt must be masked.
 *		level  -- interrupt priority level of the iv.
 *		itype  -- interrupt type.
 *	Locking: 	must hold mod_iv_lock on entry
 *	Globals read:	iv_cookie
 *	Globs. written: none
 *	Returns:	none
 *
 * Remarks:
 *	mod_iv_lock spin lock is held on entry/exit.
 */
/*ARGSUSED*/
void
ndisableint(int iv, pl_t level, int engnum, int itype)
{
	unsigned int mask;
	int	i;			/* interrupt priority level */
	struct emask iv_emask;


	if (nonline > 1)
	{
		xcall_all(&iv_emask, B_FALSE, holdcpus, (void *)iv_cookie);
	}
	
	mask = IRQBIT(iv);

	/*
 	 * Mask/Set the interrupt's bit in iplmask[]
 	 * for all priorities, we no longer have interrupts 
	 * on this vector.
 	 */
	for (i = PLBASE; i <= PLHI; i++) 
	{
		ast_picmask[i][engnum] |= mask;
	}

	/*
	 * Reload pic masks for the local so that mask modifications 
	 * take effect immediately. For the non-local engines the
	 * return from xcall will reload the new picmask.
 	 */
	if (myengnum == engnum)
		picreload();

	++iv_cookie;
}

/*
 * void
 * intnull(void)
 *	Dummy interrupt service routine for unconfigured interrupts.
 *
 * Calling/Exit State:
 *	Called by: 	base kernel (interrupt context)
 *	Parameters:	none
 *	Locking: 	none
 *	Globals read:	none
 *	Globs. written: none
 *	Returns:	none
 *
 * Description:
 *	Does nothing.  Just gives unused ivects a place to point.
 *	See conf.c for how ivect table is initialized.
 */
/*ARGSUSED*/
void
intnull(int vect)
{
}
