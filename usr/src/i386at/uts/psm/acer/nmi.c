/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/acer/nmi.c	1.2"
#ident	"$Header: $"

/*
 * Platform-dependent Non-Maskable Interrupt (NMI) handler.
 */

#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <proc/disp.h>
#include <proc/regset.h>
#include <proc/user.h>
#include <svc/bootinfo.h>
#include <svc/errno.h>
#include <svc/pit.h>
#include <svc/reg.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/plocal.h>
#include <util/param_p.h>
#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>

#include <io/f_ddi.h>
#include <io/ddi_i386at.h>
#include <io/ddi.h>


#define ECC_ECCFIXED    0xC10	/* latch reg. Recovered 1 bit ecc errs*/
#define ECC_MEMFATAL    0xC14	/* latch reg. fatal N bit ecc errs    */
#define ECC_MEMINFO     0xC18	/* memory config/event register       */

/* bit definition of ECC_MEMINFO */
#define	ECC_SBIT_ERROR	1	/* single bit error */
#define	ECC_MBIT_ERROR	2	/* mutiple bit error */

/*
 * 0xc38 - Reenable single error service by reading this port
 * 0xc39 - Reenable multiple error service by reading this port
 */
#define ECC_1ENAB()     inb(0xc38)      /* enable single-bit err service*/
#define ECC_2ENAB()     inb(0xc39)      /* enable multi-bit err service */

/* new vvv */
#define ECC_EDAC()      inb(0xc02)      /* detect EDAC hardware presence*/

char *ecc_merr1msg = \
"Single bit memory error. Address = 0x%x.\n\tMemory # %d. Access = %s.\n\t0xc18= 0x%x. 0x%x = 0x%x";

char *ecc_merr2msg = \
"Uncorrectable memory error. Address = 0x%x.\n\tMemory # %d. Access = %s.\n\t0xc18 = 0x%x. 0x%x = 0x%x";

int	ecc_merrhandler(int *r0ptr);

/*
 * An argument to ecc_merrhandler to distinguish between a real
 * NMI interrupt and the periodic ECC memory check.
 */
int	ecc_nmi_arg;


/*
 * void
 * ecc_init(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
ecc_init(void)
{
	uchar_t	edac_pres = ECC_EDAC();


	/*
	 * Check if EDAC hardware is present or if have parity memory.
	 *
	 * inb(c02) and look at bit 7; "low" means that edc memory
	 * board is present on system instead of parity memory board
	 */
#ifdef DEBUG
	if (!(edac_pres & 0x80)) {
		cmn_err(CE_NOTE, "EDAC support active.");
	} else {
		cmn_err(CE_NOTE, "Parity memory support active.");
        }
#endif /* DEBUG */

	drv_callback(NMI_ATTACH, ecc_merrhandler, &ecc_nmi_arg);
}


/*
 * int
 * ecc_merrhandler(int *r0ptr)
 *	Single-bit, double-bit, and parity error handler. 
 *	The ECC check is done every 5 minutes.
 *
 * Calling/Exit State:
 *	- Called from NMI handler and callout.
 *	- Return non-zero value if an ecc parity error occurred, otherwise
 *	  return zero. We would panic if a fatal double-bit parity occurred.
 *
 * Hardware Info:
 *	0xc10 - 0xc13	Read Only
 *			Single Error, error address on frame bus.
 *
 *		D7  D6  D5  D4  D3  D2  D1  D0
 *	0xc10	A7  A6  A5  A4  A3  A2  P#1 P#2
 *	0xc11	A15 A14 A13 A12 A11 A10 A9  A8
 *	0xc12	A23 A22 A21 A20 A19 A18 A17 A16    
 *	0xc13	P#3 P#4 A29 A28 A27 A26 A25 A24
 *
 *	0xc14 - 0xc17	Read Only
 *			Multiple Error, error address on local bus.
 *
 *		D7  D6  D5  D4  D3  D2  D1  D0
 *	0xc14	A7  A6  A5  A4  A3  A2  P#1 P#2
 *	0xc15	A15 A14 A13 A12 A11 A10 A9  A8
 *	0xc16	A23 A22 A21 A20 A19 A18 A17 A16    
 *	0xc17	P#3 P#4 A29 A28 A27 A26 A25 A24
 */
int
ecc_merrhandler(int *r0ptr)
{
	uint_t	errport;
	int	cmntype;
        int	i;
	paddr_t	memadr = 0;
	paddr_t	omemadr;
	vaddr_t	adr;			/* virtual address of memadr */
	uint_t	board0;
	uint_t	membrd;
	char	info = inb(ECC_MEMINFO);
	char	*errmsg, *access;
	union { 
		uint_t	ad;		/* address */
		char	c[4];
        } portval;


	/*
	 * Check parity/single/double bit errors, exit if none.
	 */

        if (!(info & ECC_MBIT_ERROR)) {		/* 0xc14 = parity/N bit err */
		errport = ECC_MEMFATAL;
		errmsg = ecc_merr2msg;		/* Uncorrectable */
		cmntype = CE_PANIC;
	} else if (!(info & ECC_SBIT_ERROR)) {	/* 0xc10 = single bit err */
		errport = ECC_ECCFIXED;
		errmsg = ecc_merr1msg;		/* Single bit */
		cmntype = CE_WARN;
        } else {
                goto    no_merr;
        }

	/*
	 * If errors read latched info from port.
	 * 
	 * For single bit error, read latched info from 0xc10 - 0xc13.
	 *
	 * For multiple bit error, read latched info from 0xc14 - 0xc17.
	 */
	for (i = 4; (i--); )
		portval.c[i] = inb(errport + i);

	/*
	 * Address is stored in bits 2-29 (See commentary section above)
	 */
        memadr = portval.ad & 0x3ffffffc;

	/*
	 * Check for rev D or older. Must be D-1 or newer to work ok
	 */
	if (memadr == errport)
		return(0);

	/*
	 * Find memory board from address + configuration info.
	 */

	switch(info & 0xc) {
	case 0x0:
		board0 = 0x2000000;	/* bits 3,2 = 00 = 32mb */
		break;
	case 0x8:
		board0 = 0x1800000;	/* bits 3,2 = 10 = 24mb */
		break;
	case 0xc:
		board0 = 0x3800000;	/* bits 3,2 = 11 = 56mb */
		break;
	case 0x4:
		board0 = 0x4000000;	/* bits 3,2 = 01 = 64mb */
		break;
        }

        if (memadr < board0) {
                membrd = 1;
        } else {                        /* all boards except 0 have 64 mb. */
                membrd = 2 + (memadr - board0) / 0x4000000;
        }

	/*
         * Find out whose access caused error.
	 */

        switch(portval.ad & (~memadr)) {
	case 0xc0000003:
		access = "EISA";
		break;
	case 0xc0000001:
		access = "CPU1";
		break;
	case 0xc0000002:
		access = "CPU2";
		break;
	case 0x40000003:
		access = "CPU3";
		break;
	case 0x80000003:
		access = "CPU4";
		break;
	default:
		access = "unknown";
		break;
        }

	/*
	 *+ Single bit/Uncorrectable memory error. Address = 0x%x.
	 *+      Memory # %d. Access = CPUn/EISA. 
	 *+      0xc18 = ????. 0xc10/0xc14 = 0x%x"
	 */
        cmn_err(cmntype, errmsg, memadr,
                membrd, access, info, errport, portval.ad);

	/*
	 * If we got here, non-fatal error. Clear single bit error.
	 */

	ECC_1ENAB();	/* enable single bit error latch */
	ECC_2ENAB();	/* enable multi bit error latch */

	/*
	 * Clear single-bit error.
	 */
	adr = physmap1(memadr, &omemadr);
	atomic_or((uint_t *)adr, 0);
	physmap1(omemadr, NULL);

	/* recheck 30 seconds later */
	itimeout((void (*)())ecc_merrhandler, 0, HZ * 30, plhi);
	
	return 1;

no_merr:
	/*
	 * If not coming from a trap routine, then reschedule the
	 * ECC memory error handler to be called every 5 min.
	 */
	if (!r0ptr) {
		itimeout((void (*)())ecc_merrhandler, 0, HZ * 300, plhi);
	}

	return 0;
}
