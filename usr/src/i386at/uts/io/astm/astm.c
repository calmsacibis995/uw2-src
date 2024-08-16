/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/astm/astm.c	1.4"

/*
 * astm.c -- routines for manipulating the AST Manhattan hardware.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/processor.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/kdb/xdebug.h>
#include <psm/ast/ebi.h>
#include <psm/ast/ast.h>
#include <io/ddi.h>

#else

#include <sys/types.h>
#include <sys/ksynch.h>
#include <sys/param.h>
#include <sys/plocal.h>
#include <sys/processor.h>
#include <sys/kdb/xdebug.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/ebi.h>
#include <sys/ast.h>

#include <sys/conf.h>
#include <io/ddi.h>

#endif

/* 
 * MACROS 
 */
#define MMIONEED(x)  ((x).length)
#define MMIOSIZE(x)  ((x).length)
#define MMIOADDR(x)  ((x).address.low)
#define MMIOALLOC(x) ((x).flags & IO_ALLOCATE)

/* convert 8086 far pointer to address */
/* (this is no longer defined in immu.h) */
#define	ftop(x)	((((x) & 0xffff0000) >> 12) + ((x) & 0xffff))

#define CALLOC(a)        kmem_zalloc((a), KM_NOSLEEP)
#define PHYSMAP(x, s)    physmap((x), (s), KM_NOSLEEP)

/* 
 * GLOBAL DATA 
 */
	int astm_cache_on = 1;	/* do not disable cache--leave it unchanged */
	int ast_shutdown_pwr = 0;
struct  dispinfo ast_display;

/*
 * set by SPI/NMI code.  To be used to communicate power fail 
 * signal, shutdown switch press and attention switch press to 
 * user level code.
 */
	int          ast_event_code;
	event_t      ast_event_sv;

STATIC volatile uint_t ast_eventflags[EBI_MAX_CPUS];

/*
 * XXX: ast_calltab is referenced in ast_setpicmasks() defined in spl.s
 * any changes to the definition of this structure in ebi.h need to 
 * resolved against that file.  AST currently sez that no changes in
 * current structure size or offsets are planned, so this should not
 * be a problem.
 */
	EBI_II 		ast_calltab;
	void   		**MMIOTable;

STATIC int          	ast_bios_jmp_table = 0;
STATIC unsigned int 	ast_bios_vbase = 0;

/*
 * FUNCTION PROTOTYPES
 */
	void 	astm_update_graph(void);
	void 	ast_intr_init(void);
STATIC	void 	build_call_table(unsigned int *offtab, unsigned int *calltab);
STATIC	void 	ast_init_ebi(void);
STATIC	void 	ast_configure(void);

/*
 * FUNCTION DEFINITIONS
 */

/*
 * STATIC void
 * build_call_table(unsigned int *offtab, unsigned int *calltab)
 *	Copy addresses of EBI functions from offset table to call table.
 *
 * Calling/Exit States:
 *	Called by: 	ast_init_ebi()
 *	Parameters:
 *		offtab		int pointer to beginning of offset table
 *		calltab 	int pointer to beginning of call table
 *	Locking: 	none
 *	Globals read:	ast_bios_vbase
 *	Globs. written: none
 *	Returns:	none
 *
 * Description:
 *	Adds virtual base address of EBI to each offset to calculate 
 *	virtual address of each EBI entry point, store in call table.
 */
STATIC void
build_call_table(unsigned int *offtab, unsigned int *calltab)
{
	int i;
	
	for (i = 0; i < sizeof(offsetTable) / sizeof(unsigned int); i++)
	{
		calltab[i] = offtab[i] + ast_bios_vbase;
	}
}

/*
 * STATIC void
 * ast_init_ebi(void)
 *	Set up call table (calltab), IO info table (IOinfotab), and
 *	IO table (MMIOTable) needed to make EBI access possible.
 *
 * Calling/Exit States:
 *	Called by: 	ast_configure()
 *	Parameters:	none
 *	Locking: 	none
 *	Globals read:	ast_calltab, ast_cache_on, ebi_initted, ast_bios_vbase,
 *				ast_bios_jmp_tbl, ast_cache_on
 *	Globs. written: ebi_initted, calltable
 *	Returns:	none
 *
 * Description:
 *
 * Remarks:
 *	Does nothing if called more than once.
 */
STATIC void
ast_init_ebi(void)
{
	static int ebi_initted = 0;
	unsigned int nslots;
	int i;
	unsigned int calltable;
	unsigned int *funcaddr = (unsigned int *)&ast_calltab;
	IOInfoTable *itab;

	revisionCode revcode;
	
#ifdef AST_DEBUG
	cmn_err(CE_NOTE, "In ast_init_ebi()");
#endif

	if (ebi_initted) 
		return;

	ebi_initted++;

	/*
	 * calculate the offset of the jump table contained within EBI 
	 * segment.
	 */
	calltable = ast_bios_vbase + (ast_bios_jmp_table - ftop(EBI_BIOS_SEG));
	
	/*
	 * Build the callable addresses for ast_calltab.
	 */
	build_call_table((unsigned int *)calltable, funcaddr);
	
	
	/*
	 * find out how many device slots we have.
	 */
	if (((ast_calltab.GetNumSlots)(&nslots)) != OK) 
	{
		cmn_err(CE_PANIC, "ast_init_ebi: Unable to get number of bus slots");
	}
	

	/*
	 * allocate a little space for the io info table.  
	 */
	itab = (IOInfoTable *)CALLOC(sizeof(IOInfoTable) * nslots);
	if (itab == NULL) 
	{
		cmn_err(CE_PANIC, "ast_init_ebi: Unable to allocate IOInfoTable");
	}

	/*
	 * although the spec says that we only need to allocate
	 * enough space for "nslots", the example code supplied by
	 * AST shows an allocation for 32 slots worth.  We will
	 * do the same thing here just in case this is a bug in
	 * the EBI.
	 */
	MMIOTable = (void *)CALLOC(sizeof(void *) * 32);
	if (MMIOTable == NULL) 
	{
		cmn_err(CE_PANIC, "ast_init_ebi: Unable to allocate MMIOTable");
	}

	/*
	 * get the information necessary to build the MMIOTable
	 * which we will use from here on out.
	 */
	if ((ast_calltab.GetMMIOTable(itab)) != OK) 
	{
		cmn_err(CE_PANIC, "ast_init_ebi: Unable to get MMIOTable");
	}

	for (i = 0; i < nslots; i++) 
	{
		if (MMIONEED(itab[i])) 
		{
			if (MMIOALLOC(itab[i]))
			{
				MMIOTable[i] = (void *)CALLOC(MMIOSIZE(itab[i]));
			} else {
				MMIOTable[i] = (void *)PHYSMAP(MMIOADDR(itab[i]), 
												MMIOSIZE(itab[i]));
			}
			
			if (MMIOTable[i] == NULL)
			{
				cmn_err(CE_PANIC, "ast_init_ebi: no memory for MMIOTable");
			}
		} 
	}
	
	if (((ast_calltab.InitEBI)(MMIOTable)) != OK)
	{
		cmn_err(CE_PANIC, "ast_init_ebi: Cannot initialize EBI II");
	}

	if (((ast_calltab.GetRevision)(MMIOTable, &revcode)) != OK)
	{
		cmn_err(CE_PANIC, "ast_init_ebi: Cannot get revision level");
	}

	if (revcode.major <= MIN_MAJOR && revcode.minor < MIN_MINOR) 
	{
		cmn_err(CE_PANIC, "ast_init_ebi: EBI rev %d.%d not supported", 
				revcode.major, revcode.minor);
	}

	if (!astm_cache_on) {
		(ast_calltab.DisableRAMCache)(MMIOTable);
	}	
}

/*
 * psm support routines.
 */

/*
 * STATIC void
 * ast_configure(void)
 *	Map in the AST Extended Bios Interface (EBI).
 *
 * Calling/Exit State:
 *	Called by:	psm_intr_init()
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:	ast_bios_vbase
 *	Globs. written: ast_bios_jmp_table
 *	Returns:	none
 *
 * Description:
 *	1. Map in the EBI segment. 
 *	2. Verify version signature.  Panic if not EBI II.
 * 	3. Clear FPU busy latch.
 *	4. Call ast_init_ebi() to initialize the EBI.
 */
STATIC void
ast_configure(void)
{
	ebi_iiSig *sigp;
	
#ifdef AST_DEBUG
	cmn_err(CE_NOTE, "In ast_configure()");
#endif

	/*
	 * map in the AST EBI BIOS segment.
	 */
	ast_bios_vbase = (unsigned int)PHYSMAP(ftop(EBI_BIOS_SEG), EBI_BIOS_SIZ + 1);
	if (ast_bios_vbase == (unsigned int)NULL)
	{
		cmn_err(CE_PANIC, "Cannot map EBI BIOS!!!!!!!");
	}

    /*
	 * calculate the virtual address of the EBI signature.
     */
    sigp = (ebi_iiSig *)(ast_bios_vbase + (ftop(EBI_SIG_LOC) - ftop(EBI_BIOS_SEG)));

    /*
     * Let's see if it supports EBI2
     */
    if (sigp->sig[0] != 'E') return;
    if (sigp->sig[1] != 'B') return;    
    if (sigp->sig[2] != 'I') return;  
    if (sigp->sig[3] != '2') return;    

#ifdef AST_DEBUG
	cmn_err(CE_NOTE, "Found EBI");
#endif

	/*
	 * now get the physical address of the EBI II call table offsets.
	 */
	ast_bios_jmp_table = ftop((unsigned int)(((sigp->seg) << 16) | (sigp->off)));


	/*
	 * now we configure the EBI...
	 */
	ast_init_ebi();

	return;
}

/*
 * void 
 * ast_intr_init(void)
 * 	Initialize the AST EBI BIOS.  No PIC initialization need in UP.
 *
 * Calling/Exit State:
 *	Called by:	astm_init() in astioctl.c
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:   ast_calltab, MMIOTable
 *	Globs. written: none
 *	Returns:	none
 *
 * Description:
 *		The purpose of this routine is to init ast's ebi.
 *	This is almost identical to psm_intr_init() here (below). However,
 *	it's unique to the UP version. we can't use psm_intr_init() here,
 *	because if one builds a UP kernel, psm.c:psm_intr_init() is called.
 *	So the purpose of this routine is to separate out the functionality
 *	for the UP port.
 *		This routine also maps in the EBI and initializes the
 *	front panel parameters, because this is a convenient time to do so
 *	(in the UP kernel).
 */
void
ast_intr_init(void)
{
		/*
		 * Map in the EBI bios -- do it now so that
		 * we can set up the front panel. 
		 */
		ast_configure();

		(ast_calltab.GetPanelAlphaNumInfo)(MMIOTable, 
			&ast_display.type, &ast_display.size);

		/*
		 * Set front panel mode -- we will support reboot via
		 * the off switch and will put the machine into single
		 * user mode the attention switch.
		 *
		 * set the UPS LED to green; we will change it to red 
		 * if we get a power fail interrupt.
		 */
		(ast_calltab.SetPanelUPS)(MMIOTable, UPS_LED_GREEN);
		(ast_calltab.SetPanelSwitchVisibility)(MMIOTable, PANEL_SWITCH_VISIBLE);
		(ast_calltab.SetPanelOffSwitchMode)(MMIOTable, OFF_SWITCH_SOFT);

		EVENT_INIT(&ast_event_sv);
}

/*
 * void
 * astm_update_graph(void)
 * 	Update the panel graph display to show only which processors
 *	are online (available).  Called in USG on-line mode only.
 *
 * Calling/Exit State:
 *	Called by:	astm_init() and astmioctl() in astioctl.c
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:   ast_calltab, MMIOTable
 *	Globs. written: none
 *	Returns:	none
 */
void 
astm_update_graph(void)
{
	unsigned int panmode;

	(ast_calltab.GetPanelProcGraphMode)(MMIOTable, &panmode);

	if (panmode != PANEL_MODE_OVERRIDE )
		return;

	if ((ast_calltab.SetPanelProcGraphValue)(MMIOTable, 0x01))
		cmn_err(CE_WARN, "Unable to set panel graph value\n");
}

