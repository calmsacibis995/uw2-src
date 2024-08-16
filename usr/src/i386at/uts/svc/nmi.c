/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:svc/nmi.c	1.4"
#ident	"$Header: $"

/*
 * Machine-dependent Non-Maskable Interrupt (NMI) handler.
 */

#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <proc/disp.h>
#include <proc/regset.h>
#include <proc/user.h>
#include <svc/bootinfo.h>
#include <svc/eisa.h>
#include <svc/errno.h>
#include <svc/pit.h>
#include <svc/reg.h>
#include <svc/systm.h>
#include <svc/uadmin.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/plocal.h>
#include <util/param_p.h>
#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>

#include <io/f_ddi.h>
#include <io/ddi_i386at.h>

#define PORT_B		0x61	/* System Port B */
#define IOCHK_DISABLE	0x08	/* Disable I/O CH CK */
#define PCHK_DISABLE	0x04	/* Disable motherboard parity check */
#define IOCHK		0x40	/* I/O CH CK */
#define PCHK		0x80	/* Motherboard parity check */

#define NMI_EXTPORT	0x461	/* Extended NMI register */
#define	BUSTCHK		0x40	/* Bus timeout NMI CK */
#define BUSTCHK_ENABLE	0x08	/* Enable 32-bit bus timeout NMI interrupt */	

#define CMOS_PORT	0x70	/* CMOS Port */
#define NMI_ENABLE	0x0F	/* Enable NMI interrupt */
#define NMI_DISABLE	0x8F	/* Disable NMI interrupt */


/*
 * NMI handlers.
 */
int nmi_parity(void);

paddr_t	nmi_addr;		/* address at which parity error occurred */

extern void _nmi_hook(int *);


/*
 * void
 * nmi_init(void)
 *	Handle an NMI interrupt.
 * 
 * Calling/Exit State:
 *	None.
 */
nmi_init(void)
{
	if (nmi_handler == NULL)
		nmi_handler = _nmi_hook;

	drv_callback(NMI_ATTACH, nmi_parity, NULL);

        if (eisa_verify() == 0)
        	drv_callback(NMI_ATTACH, eisa_nmi_bus_timeout, NULL);

}


/*
 * void
 * nmi(...)
 *	Handle an NMI interrupt.
 * 
 * Calling/Exit State:
 *	The arguments are the saved registers which will be restored
 *	on return from this routine.
 *
 * Description:
 *      Currently, NMIs are presented to the processor in these situations:
 *
 *		- [Software NMI] 
 *		- [Access error from access to reserved processor
 *			LOCAL address]
 *		- Access error:
 *			- Bus Timeout
 *			- ECC Uncorrectable error
 *			- Parity error from System Memory
 *			- Assertion of IOCHK# (only expansion board assert this)
 *			- Fail-safe Timer Timeout
 *		- [Cache Parity error (these hold the processor & freeze
 *                                      the bus)]
 */
/* ARGSUSED */
void
nmi(volatile uint edi, volatile uint esi, volatile uint ebp,
    volatile uint unused, volatile uint ebx, volatile uint edx,
    volatile uint ecx, volatile uint eax, volatile uint es, volatile uint ds,
    volatile uint eip, volatile uint cs, volatile uint flags, volatile uint sp,
    volatile uint ss)
{
	if (nmi_handler == NULL)
		nmi_handler = _nmi_hook;
	(*nmi_handler)((int *)&eax);
}


/*
 * void 
 * _nmi_catch(int *r0ptr)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Latch the physical address at which the parity error occurred.
 */
void
_nmi_catch(int *r0ptr)
{
	nmi_addr = kvtophys(r0ptr[T_ESI]);
}


/*
 * int
 * nmi_parity(void)
 *	PARITY ERROR ON MOTHERBOARD OR EXPANSION BOARD.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	The function tries to determine whether the NMI was caused by a 
 *	memory parity error. If so, it tries to find the bad location and 
 *	react appropriately.
 */
int
nmi_parity(void)
{
	int	stat, motherboard;
	int	board_addr = -1; 
	paddr_t	addr;			/* physical address */
	paddr_t	oaddr;			/* old physical address */
	volatile caddr_t caddr;		/* virtual address of addr */
	int	i;


	/*
	 * If its not a parity error or expansion board error, restore
	 * the fault-catch flags and return immediately.
	 */
	if (!((motherboard = ((stat = inb(PORT_B)) & PCHK)) != 0 ||
	      (stat & IOCHK))) {

		return (NMI_UNKNOWN);
	}

	/*
	 * Disable NMI until we're ready for it.
	 */
	outb(CMOS_PORT, NMI_DISABLE);

	/*
	 * If it's not the motherboard try to identify the particular board.
	 */
	if (!motherboard) {
		for (i = 0; i < bootinfo.memavailcnt && board_addr == -1; i++) {
			if (bootinfo.memavail[i].extent == 0)
				break;
	
			for (addr = bootinfo.memavail[i].base;
					addr < bootinfo.memavail[i].base
					+ bootinfo.memavail[i].extent;
							addr += 0x10000) {
				caddr = (caddr_t)physmap1(addr, &oaddr);
				*caddr = *caddr;
				(void)physmap1(oaddr, NULL);

				if (!(inb(PORT_B) & IOCHK)) {
					board_addr = addr;
					break;
				}
			}
		}

		/*
		 * Clear the I/O CH CK flip/flop. 
		 *
		 * NMI Status and Control port (0x61) bit <6> is set
		 * (IOCHK# NMI) if an expansion board asserts IOCHK# 
		 * on the EISA/ISA bus to indicate a serious error. 
		 * This interrupt is enabled by setting port 0x61
		 * bit <3> to "0". To reset the interrupt, port
		 * 0x61 bit <2> is set to "1" (Disable IOCHK# NMI) 
		 * and then is set to "0" (Enable IOCHK# Interrupt). 
		 * 					-- EISA TRM
		 */
		outb(PORT_B, inb(PORT_B) | IOCHK_DISABLE);
		outb(PORT_B, inb(PORT_B) & ~IOCHK_DISABLE);
		drv_usecwait(10);		/* wait for ten microsecond */
		stat = (inb(PORT_B) & IOCHK);
	} else {
		/*
		 * Clear the motherboard parity check flip/flop.
		 *
		 * NMI Status and Control port (0x61) bit <7> is set
		 * (PARITY ERROR) if system memory detects a parity
		 * error. This interrupt is enabled by setting port
		 * 0x61 bit <2> to "0". To reset the parity error, 
		 * port 0x61 bit <2> is set to "1" (Disable Parity
		 * Interrupt) and then is set to "0" (Enable Parity
		 * Interrupt).				 -- EISA TRM
		 */
		outb(PORT_B, inb(PORT_B) | PCHK_DISABLE);
		outb(PORT_B, inb(PORT_B) & ~PCHK_DISABLE);
		drv_usecwait(10);		/* wait for ten microsecond */
		stat = (inb(PORT_B) & PCHK);
	}

	/*
	 * Try to reproduce the problem and identify the exact address.
	 */

	nmi_addr = (paddr_t)-1;

	if (stat == 0) {
		nmi_handler = _nmi_catch;
		outb(CMOS_PORT, NMI_ENABLE);	/* enable NMI interrupts */
		enable_nmi();			/* enable x86 internal NMI */

		for (i = 0; i < bootinfo.memavailcnt &&
			     nmi_addr == (paddr_t)-1; i++) {
			caddr_t caddr;
			paddr_t base = bootinfo.memavail[i].base;
			long	size = bootinfo.memavail[i].extent;

			if (size == 0)
				break;

			for (; base + MMU_PAGESIZE <= bootinfo.memavail[i].base + size; base += MMU_PAGESIZE) {
				caddr = (caddr_t)physmap1(base, &oaddr);
				bscan(caddr, MMU_PAGESIZE);
				physmap1(oaddr, NULL);
			}
		}

		outb(CMOS_PORT, NMI_DISABLE);	/* Disable NMI interrupts */
		nmi_handler = _nmi_hook;
	}

	/*
	 * Tell the user what we found out. 
	 */

	if (board_addr == -1) {
		cmn_err(CE_NOTE, "Memory parity error on %s.",
			motherboard ? "the motherboard" : "an add-on card");
	} else {
		cmn_err(CE_NOTE,
			"Memory parity error on an add-on card which starts at address 0x%X.",
			board_addr);
	}

	if (nmi_addr == (paddr_t)-1) {
		/*
		 *+ Parity error occured at an unknown address.
		 */
		cmn_err(CE_CONT, 
			"Memory parity error address unknown.\n");
	} else {
		/*
		 *+ Parity error occurred at a known address.
		 */
		cmn_err(CE_CONT, 
			"Memory parity error at address 0x%X.\n", nmi_addr);
	}

	return (NMI_FATAL);
}
