/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:psm/netframe/apic.c	1.2"
#ident	"$Header: $"

#include <sys/bootinfo.h>
#include <sys/eisa.h>
#include <svc/intr.h>
#include <psm/intr_p.h>
#include <sys/nf_apic.h>
#include <sys/nf_pic.h>
#include <sys/systm.h>
#include <sys/cmn_err.h>
#include <sys/inline.h>
#include <sys/param.h>
#include <sys/ipl.h>
#include <sys/plocal.h>
#include <sys/sysmacros.h>
#include <sys/types.h>

extern void psm_aintr();
extern int picreload(void);
extern nioapics, napicints;
extern struct apic_int apic_int[];
extern void (*ivect[])();
extern uchar_t intpri[];	/* priority levels for interrupts */
extern pl_t svcpri[];		/* interrupt service priority levels */
extern int apic_groups_per_spl[];
extern int io_apic[];
extern int intmp[];
extern int intcpu[];
extern int picdeferndx;
extern int picdeferred[];

int apic_primask[PLHI+1], splmask[PLHI+1]; /* Mapping of spl to priority reg */
pl_t apicfree[256]; 		/* apic vec to spl mapping */
int apic_to_pic[256];		/* apic vec to pic mapping */
volatile uint_t iv_cookie;
STATIC void holdcpus(void *);

volatile vaddr_t	local_apic_addr;	/* virtual address for mapping
						   the local APIC */
volatile vaddr_t	taskpri_reg_addr;	/* virtual address for mapping
						   the task priority reg. */
volatile vaddr_t	eoi_reg_addr;		/* virtual address for mapping
						   the end of interrupt reg. */

gem_detect()
{
	register i;

	for (i = 0 ; i < nioapics ; i++) {
		outl(io_apic[i] + AP_IO_REG, AIR_VERS); /* select I/O version */
		outl(0, 0);			/* clear junk on the bus */
        	if ((inl(io_apic[i] + AP_IO_REG) != AIR_VERS) 
            	   || ((inl(io_apic[i] + AP_IO_DATA) & 0xff0000) != 0x0f0000))
			return(0);
	}
	/* All IO APICS responded properly, must be a Gemstone */
	return(1);
}

void
apicinit()
{
	int i, j, vec, pri;
	register volatile long *lp;
	register long addr;

	if (myengnum == 0) {
		picdeferndx = 0;
		picdeferred[picdeferndx] = APICINTS;
		local_apic_addr = physmap(APICADR, MMU_PAGESIZE, KM_NOSLEEP);
		taskpri_reg_addr = local_apic_addr + 0x80;
		eoi_reg_addr = local_apic_addr + 0xB0;

		/* Initially set all masks to disable all interrupts */
		for (i = 0; i <= PLHI ; i++)
			apic_primask[i] = 0xF0;

		ivect[0xF0] = psm_aintr;
		intpri[0xF0] = 8;
		for (i = 0 ; i < 256 ; i++) {
			svcpri[i] = MIN(intpri[i], PLHI);
			apic_to_pic[i] = i;
		}

		pri = APICINTS;
		for (i = 1 ; i <= PLHI ; i++) {
			splmask[i] = pri;
			if (j = apic_groups_per_spl[i] * 0x10)
				splmask[i] += j - 0x10;
			while (j--)
				apicfree[pri++] = i;
		}

		if ((j = 0xF0 - pri) > 0) {
			while (j--)
				apicfree[pri++] = PLHI; /* leftovers to PLHI */
		} else if (j < 0) 
			cmn_err(CE_PANIC, "APIC vector overflowed\n");

		for (i = 0 ; i < nioapics ; i++) {
			addr = io_apic[i];
			outl(addr + AP_IO_REG, AIR_ID); 
			outl(addr + AP_IO_DATA, ((i + 1) * 2) << 24); 
			for (j = 0 ; j < APICVECS ; j++) {
				outl(addr + AP_IO_REG, AIR_RDT + 2 * j);
				outl(addr + AP_IO_DATA, AV_MASK);
			}
		}

		for (i = 0 ; i < napicints ; i++) {
			if (apic_int[i].a_type == IO_APIC) {
				vec = apic_int[i].a_pic;
				apic_add_int(IO_APIC, vec, intpri[vec],
					apic_int[i].a_addr, apic_int[i].a_irq,
					intcpu[vec], intmp[vec]);
			}
		}
	}

	lp = (volatile long *)local_apic_addr;
	lp[AP_ID] = (2 * myengnum + 1) << 24;  /* local unit ID */
	lp[AP_DESTFMT] = -1;
	lp[AP_LDEST] = 1<<myengnum;
	lp[AP_TASKPRI] = 0xF0;
	lp[AP_SPUR] = 0x1ff;     /* enable local unit, stray vector = ff */
	lp[AP_LVT_TIMER] = AV_MASK;	/* Mask off all local interrupts */
	lp[AP_LVT_I0] = AV_MASK;
	lp[AP_LVT_I1] = AV_MASK;

	for (i = 0 ; i < napicints ; i++) {
		if (apic_int[i].a_type == PROC_APIC) {
			vec = apic_int[i].a_pic;
			apic_add_int(PROC_APIC, vec, intpri[vec],
				&lp[apic_int[i].a_addr], apic_int[i].a_irq,
				intcpu[vec], intmp[vec]);
		}
	}
	apic_xmsg(AV_RESET|AV_LEVEL|AV_DEASSERT|AV_XTOALL);

	l.ipl = l.picipl = PLBASE;
	asm("sti");	/* ENABLE */
}

void
apicstart(void)
{
	register i;

	if (!upyet) {
		for (i = 0; i <= PLHI ; i++)
			apic_primask[i] = splmask[i];
	}
	l.ipl = l.picipl = PLHI; /* Force spl0 to change PIC masks */
	spl0();
}

/*
 * void ndisableint(int iv, pl_t level, int engnum, int itype)
 *	Disable interrupts of specified interrupt number.
 *
 * Calling/Exit State:
 *	Called and exit with mod_iv_lock held at PLHI.
 */
void
apic_ndisableint(int iv, pl_t level, int intcpu, int itype)
{
	register i;
	register long addr;
        struct emask iv_emask;

#ifndef	UNIPROC	
        if (nonline > 1)
                xcall_all(&iv_emask, B_FALSE, holdcpus,
                          (void *)iv_cookie);
#endif	/* UNIPROC */

	for (i = 0 ; i < 256 ; i++)
		if (apic_to_pic[i] == iv)
			break;

	if (i >= 256)
		cmn_err(CE_PANIC, "cannot find active apic slot for %d\n", iv);

	apicfree[i] = level;
	apic_to_pic[i] = i;

	addr = io_apic[iv / APICVECS];
	outl(addr + AP_IO_REG, AIR_RDT + 2 * (iv % APICVECS));
	outl(addr + AP_IO_DATA, AV_MASK);
	++iv_cookie;
}

/*
 * void nenableint(int iv, pl_t level, int intcpu, int intmp, int itype)
 *	Enable interrupts of specified interrupt number.
 *
 * Calling/Exit State:
 *	Called and exit with mod_iv_lock held at PLHI.
 */
void
apic_nenableint(int iv, pl_t level, int intcpu, int intmp, int itype)
{
	apic_add_int(IO_APIC, iv, level, io_apic[iv/APICVECS], 
			iv%APICVECS, intcpu, intmp);
}

apic_add_int(type, iv, level, addr, irq, intcpu, intmp)
{
	register i;
	long rdt, rdt2;
        struct emask iv_emask;

#ifndef	UNIPROC	
        if (nonline > 1)
                xcall_all(&iv_emask, B_FALSE, holdcpus,
                          (void *)iv_cookie);
#endif	/* UNIPROC */

        if (level >= 7)
                level = 8;

	for (i = 0 ; i < 256 ; i++)
		if (apicfree[i] == level)
			break;

	if (i >= 256)
		cmn_err(CE_PANIC, "cannot allocate apic slot for %d\n", iv);

        apicfree[i] = 0;
	apic_to_pic[i] = iv;

	if (type == PROC_APIC) {
		*(volatile long *)addr = *(volatile long *)addr & 0xffffff00|i;
	} else {
		rdt2 = AIR_RDT2 + 2 * (iv % APICVECS);
		rdt = AIR_RDT + 2 * (iv % APICVECS);
		if (intcpu == -1) {
			if (intmp == 1) {
				outl(addr + AP_IO_REG, rdt2);
				outl(addr + AP_IO_DATA, AV_PALL);
				outl(addr + AP_IO_REG, rdt);
				outl(addr + AP_IO_DATA, i | AV_LOPRI);
			} else {
				outl(addr + AP_IO_REG, rdt2);
				outl(addr + AP_IO_DATA, 1 << 24); /* cpu 0 */
				outl(addr + AP_IO_REG, rdt);
				outl(addr + AP_IO_DATA, i | AV_FIXED);
			}
		} else {
			outl(addr + AP_IO_REG, rdt2);
			outl(addr + AP_IO_DATA, (2 * intcpu + 1) << 24);
			outl(addr + AP_IO_REG, rdt);
			outl(addr + AP_IO_DATA, i | AV_FIXED);
		}
        }
	++iv_cookie;
}

/*
 * STATIC void holdcpus(void *arg)
 *	Hold the cpu executing this routine until the value
 *	of the iv_cookie changed.
 *
 * Calling/Exit State:
 *	Called through xcall, the cpu is already in PLHI.
 */
STATIC void
holdcpus(void *arg)
{
	while (iv_cookie == (uint_t)arg)
		;
	picreload();
}

void
apic_stray(vec)
{
	cmn_err(CE_WARN, "Stray interrupt at 0x%x\n", vec);
}

apic_xmsg(x)
{
	volatile long *lp;

	lp = (volatile long *)local_apic_addr;

	/* Disable interrupts before touching the APIC's ICR register */
	asm ("pushfl");
	asm ("cli");

	while (lp[AP_ICMD] & AV_PENDING);
	lp[AP_ICMD] = x;
	while (lp[AP_ICMD] & AV_PENDING);

	/* Now restore interupt flag before exiting */
	asm ("popfl");
}

apic_xmsg2(x, eng)
{
	volatile long *lp;
	int x2;

	lp = (volatile long *)local_apic_addr;

	/* Disable interrupts before touching the APIC's ICR register */
	asm ("pushfl");
	asm ("cli");

	x2 = (2 * eng + 1)<<24;
	lp[AP_ICMD2] = x2;
	while (lp[AP_ICMD] & AV_PENDING) ;
	lp[AP_ICMD] = x;
	while (lp[AP_ICMD] & AV_PENDING) ;

	/* Now restore interupt flag before exiting */
	asm ("popfl");
}
