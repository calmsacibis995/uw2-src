/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/ddi386at.c	1.5"
#ident	"$Header: $"

/*
 *	i386at-specific Device Driver Interface functions          
 *
 * This file contains support for i386at extensions to the DDI/DKI.
 */ 

#include <io/conf.h>
#include <svc/bootinfo.h>
#include <svc/cpu.h>
#include <svc/memory.h>
#include <svc/systm.h>
#include <util/inline.h>
#include <util/plocal.h>
#include <util/types.h>

/* These must come last: */
#include <io/ddi.h>
#include <io/ddi_i386at.h>

/*
 * EISA hardware-related defines used by EISA_BRDID subfunction:
 */
#define MAX_EISA_SLOT		16
#define EISA_SLOT_IO_BASE	0x1000
#define EISA_BRDID_ADDRESS	0xC80
#define EISA_BRD_BUSY		0x70
#define EISA_BRD_RESERVED	0x80


/*
 * int
 * drv_gethardware(ulong_t parm, void *valuep)
 *	Get i386at hardware-specific information for drivers.
 *
 * Calling/Exit State:
 *	The information accessed here is all read-only after sysinit,
 *	so no locking is necessary.
 */
int
drv_gethardware(ulong_t parm, void *valuep)
{
	switch (parm) {

	case PROC_INFO: { /* Get processor-type info. */

		struct cpuparms *cpup = valuep;

		struct_zero(cpup, sizeof (struct cpuparms));

		/*
		 * Take advantage of the fact that cpu_id is
		 * set to 3 for 386, 4 for 486, 5 for 586...
		 */
		cpup->cpu_id = l.cpu_id - 2;
		cpup->cpu_step = l.cpu_stepping;
		break;
	}

	case IOBUS_TYPE: { /* Are we ISA (std AT), EISA or MCA? */

		if (bootinfo.machflags & MC_BUS)
			*(ulong_t *)valuep = BUS_MCA;
		else if (bootinfo.machflags & EISA_IO_BUS)
			*(ulong_t *)valuep = BUS_EISA;
		else
			*(ulong_t *)valuep = BUS_ISA;
		break;
	}

	case TOTAL_MEM: { /* Return the number of bytes of physical memory. */

		*(ulong_t *)valuep = totalmem;
		break;
	}

	case DMA_SIZE: { /* How many bits of DMA addressing have we? */
		extern int Dma_addr_28;

		if (Dma_addr_28)
			*(ulong_t *)valuep = 28;
		else
			/* 24-bit is standard for MCA, EISA and ISA */
			*(ulong_t *)valuep = 24;
		break;
	}


	case BOOT_DEV: { /* What device did we boot from? */

		struct bootdev *bdevp = valuep;

		/*
		 * The struct_zero will set bdevp->bdv_unit to 0.
		 * Since on i386at machines, one always boots
		 * off the first hard disk or first flop,
		 * the unit number is always 0.
		 */
		struct_zero(bdevp, sizeof(struct bootdev));
		bdevp->bdv_type = BOOT_DISK;
		if (bootinfo.bootflags & BF_FLOPPY)
			bdevp->bdv_type = BOOT_FLOPPY;
		break;
	}

	case HD_PARMS: { /* Obtain parameters on 2 primary disks.
			  * Note that this function is not extensible
			  * to additional disks, as information for them
			  * is not available from bootinfo.
			  */

		ulong_t drive_num;
		struct hdparms *hdp = valuep;

		/* only "unit" values of 0 or 1 are OK */
		if ((drive_num = hdp->hp_unit) > 1)
			return -1;
		struct_zero(hdp, sizeof(struct hdparms));
		hdp->hp_unit = drive_num;
		hdp->hp_ncyls = bootinfo.hdparams[drive_num].hdp_ncyl;
		hdp->hp_nheads = bootinfo.hdparams[drive_num].hdp_nhead;
		hdp->hp_nsects = bootinfo.hdparams[drive_num].hdp_nsect;
		hdp->hp_precomp = bootinfo.hdparams[drive_num].hdp_precomp;
		hdp->hp_lz = bootinfo.hdparams[drive_num].hdp_lz;
		break;
	}

	case EISA_BRDID: { /* Return EISA Board ID.
			    * The desired slot number is passed in via valuep.
			    */

		long slot, slotaddr;
		uchar_t pval;

		if (!(bootinfo.machflags & EISA_IO_BUS))
			return -1;

		if ((slot = *(long *)valuep) > MAX_EISA_SLOT || slot < 0)
			return -1;

		slotaddr = EISA_SLOT_IO_BASE * slot + EISA_BRDID_ADDRESS;

		/* PreCharge the ID port. */
		outb(slotaddr, 0xFF);

		/*
		 * Check if the board has a readable ID:
		 *	(pval == 0xFF): port non-existent
		 *	(pval & EISA_BRD_RESERVED): reserved bit, must be 0
		 *	(pval & EISA_BRD_BUSY) == EISA_BRD_BUSY:
		 *	    if all bits set, board is not ready
		 *	Other bits in pval are don't care.
		 */
		pval = inb(slotaddr);
		if (pval == 0xFF || (pval & EISA_BRD_RESERVED) ||
		    (pval & EISA_BRD_BUSY) == EISA_BRD_BUSY)
			*(long *)valuep == -1L;
		else
			*(long *)valuep = (long)inw(slotaddr);
		break;
	}

	default: /* bad parm value */
		return -1;
	}

	return 0; /* success */
}
