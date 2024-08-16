/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:svc/memory.c	1.8"
#ident	"$Header: $"

/*
 * Architecture dependent memory handling routines
 * to deal with configration, initialization, and
 * error polling.
 */

#include <io/SGSmem.h>
#include <io/bdp.h>
#include <io/cfg.h>
#include <io/slic.h>
#include <io/slicreg.h>
#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#include <mem/tuneable.h>
#endif

extern paddr_t pmemptr;	/* next available physical page */

/*
 * sgs_mem_decode[]
 *	Table indexed by syndrome value for single-bit error, returns
 *	which bit is in error.
 *
 * The value is described below:
 *
 *	0 - 31 indicates the failing data bit position. 0 is the lsb and
 *	       31 is the msb.
 *
 *	32- 38 indicates a check bit failure.  32 indicates checkbit cx
 *	                                       33 indicates checkbit c0
 *	                                       34 indicates checkbit c1
 *	                                       35 indicates checkbit c2
 *	                                       36 indicates checkbit c4
 *	                                       37 indicates checkbit c8
 *	                                       38 indicates checkbit c16
 */

#define	EDCCX	32			/* checkbit cx  is a 1-bit error */
#define	EDCC0	(EDCCX+1)		/* checkbit c0  is a 1-bit error */
#define	EDCC1	(EDCCX+2)		/* checkbit c1  is a 1-bit error */
#define	EDCC2	(EDCCX+3)		/* checkbit c2  is a 1-bit error */
#define	EDCC4	(EDCCX+4)		/* checkbit c4  is a 1-bit error */
#define	EDCC8	(EDCCX+5)		/* checkbit c8  is a 1-bit error */
#define	EDCC16	(EDCCX+6)		/* checkbit c16 is a 1-bit error */
#define	EDCTWO	(EDCC16+1)		/* 2-bit error */
#define	EDCMLT	(EDCTWO+1)		/* 3 or more bits in error */
#define	EDCOK	255			/* no error */

static	unchar	sgs_mem_decode[] = {
	EDCOK,	EDCCX,	EDCC0,	EDCTWO,	EDCC1,	EDCTWO,	EDCTWO,	EDCMLT,
	EDCC2,	EDCTWO,	EDCTWO,	17,	EDCTWO,	EDCMLT,	16,	EDCTWO,
	EDCC4,	EDCTWO,	EDCTWO,	18,	EDCTWO,	19,	20,	EDCTWO,
	EDCTWO,	21,	22,	EDCTWO,	23,	EDCTWO,	EDCTWO,	EDCMLT,
	EDCC8,	EDCTWO,	EDCTWO,	8,	EDCTWO,	9,	10,	EDCTWO,
	EDCTWO,	11,	12,	EDCTWO,	13,	EDCTWO,	EDCTWO,	EDCMLT,
	EDCTWO,	14,	EDCMLT,	EDCTWO,	15,	EDCTWO,	EDCTWO,	EDCMLT,
	EDCMLT,	EDCTWO,	EDCTWO,	EDCMLT,	EDCTWO,	EDCMLT,	EDCMLT,	EDCTWO,
	EDCC16,	EDCTWO,	EDCTWO,	EDCMLT,	EDCTWO,	EDCMLT,	EDCMLT,	EDCTWO,
	EDCTWO,	EDCMLT,	1,	EDCTWO,	EDCMLT,	EDCTWO,	EDCTWO,	0,
	EDCTWO,	EDCMLT,	2,	EDCTWO,	3,	EDCTWO,	EDCTWO,	4,
	5,	EDCTWO,	EDCTWO,	6,	EDCTWO,	7,	EDCMLT,	EDCTWO,
	EDCTWO,	EDCMLT,	24,	EDCTWO,	25,	EDCTWO,	EDCTWO,	26,
	27,	EDCTWO,	EDCTWO,	28,	EDCTWO,	29,	EDCMLT,	EDCTWO,
	30,	EDCTWO,	EDCTWO,	EDCMLT,	EDCTWO,	31,	EDCMLT,	EDCTWO,
	EDCTWO,	EDCMLT,	EDCMLT,	EDCTWO,	EDCMLT,	EDCTWO,	EDCTWO,	EDCMLT
};

#define LO_EDC_SE_ME	(MEM_EDC_LO_SE | MEM_EDC_LO_ME)
#define HI_EDC_SE_ME	(MEM_EDC_HI_SE | MEM_EDC_HI_ME)

struct	memory	{
	unchar	m_type;			/* board type */
	void	(*m_init)();		/* procedure to init m_type boards */
	void	(*m_poll)();		/* procedure to poll m_type boards */
};

STATIC void  fgs_mem_init(struct ctlr_desc *);
STATIC void  fgs_mem_poll(struct ctlr_desc *);
STATIC void  sgs_mem_init(struct ctlr_desc *);
STATIC void  sgs_mem_poll(struct ctlr_desc *);
STATIC ulong sgs_mem_err_addr(int, int);
STATIC uint  sgs_mem_bank_decode(struct ctlr_desc *, ulong);

STATIC struct memory memory_boards[] = {
	{ SLB_MEMBOARD,	fgs_mem_init, fgs_mem_poll },
	{ SLB_SGSMEMBOARD,sgs_mem_init, sgs_mem_poll },
	{ 0 }
};

paddr_t		topmem;			/* top of memory */
size_t		totalmem;		/* total memory (topmem-holes)*/

/*
 * void
 * conf_mem(void)
 *
 *	Configure memory.
 *
 * Calling/Exit State:
 *
 *	Figure top of memory.  Allow holes; meminit() handles this.
 */

void
conf_mem(void)
{
	register struct ctlr_toc *toc;
	register struct	ctlr_desc *cd;
	register struct memory *mem;
	register int	i;
	int	bad_mem = 0;

	/*
	 * Print about deconfigured/failed boards.  These are not in
	 * the bit-map (power-up arranges this).
	 */

	for (mem = memory_boards; mem->m_type != 0; mem++) {
		toc = &CD_LOC->c_toc[mem->m_type];
		cd = &CD_LOC->c_ctlrs[toc->ct_start];
		for (i = 0; i < (int)toc->ct_count; i++, cd++) {
			if (cd->cd_diag_flag & (CFG_FAIL|CFG_DECONF)) {
				if (++bad_mem == 1) {
					/*
					 *+ This is an informative
					 *+ message listing memory
					 *+ boards that are not 
					 *+ being used because they
					 *+ are deconfigured or
					 *+ have failed.
					 */
					cmn_err(CE_NOTE,
					    "Not using memory boards: slic");
				}
				cmn_err(CE_CONT," %d", cd->cd_slic);
			}
		}
	}
	if (bad_mem) {
		cmn_err(CE_CONT, ".\n");
	}

	/*
	 * Determine top of physical memory, including holes.
	 */

	topmem = CD_LOC->c_maxmem;
	totalmem = CD_LOC->c_memsize * 1024;
}

/*
 * int
 * hwpage_exists(int)
 *
 *	Return non-zero iff HW page "pg" exists in physical memory.
 *
 * Calling/Exit State:
 *
 *	Returns 0 if address space given by page number is not
 *	backed up by physical memory, otherwise returns 1.
 */

int
hwpage_exists(register int pg)
{
	pg /= btop(MC_CLICK);
	return((pg >= CD_LOC->c_mmap_size) ? 0 : MC_MMAP(pg, CD_LOC));
}

/*
 * paddr_t
 * getnextpaddr(uint_t size, uint_t flag)
 *
 *	Return (and consume) size bytes of pages aligned physical memory.
 *	Intended for use by palloc().
 *
 * Calling/Exit State:
 *
 *	Returns the physical address of an unused page of mainstore memory.
 *	Each time called will return a different page.
 */
paddr_t
getnextpaddr(uint_t size, uint_t flag)
{
	paddr_t ret_paddr;
	uint_t remaining;

	if (topmem != 0 && pmemptr >= topmem) {
		/*
		 * Kernel configuration parameters may artifically limit
		 * the amount of physical memory we can use to "topmem".
		 */
		/*
		 *+ The kernel tried to allocate memory and sufficient
		 *+ physical memory was not available.
		 *+ Corrective action:  Configure more physical memory.
		 */
		cmn_err(CE_PANIC, "palloc: ran out of physical memory (topmem)");
	}

again:	ret_paddr = pmemptr;
	remaining = size;

	if (pmemptr == (paddr_t) 0) {
		/*
		 * We ran out of physical memory (looped around to 0).
		 */
		/*
		 *+ During kernel initialization (boot), there
		 *+ was insufficient physical memory to allocate
		 *+ kernel data structures.  (Note that some
		 *+ physical memory may have been left unused in order
		 *+ to satisfy alignment and contiguity requirements.)
		 *+ Corrective action:  Add more physical memory.
		 */
		cmn_err(CE_PANIC, "palloc: ran out of physical memory");
	}

	do {
		if (!hwpage_exists(pfnum(pmemptr))) {
			pmemptr += MMU_PAGESIZE;
			goto again;
		}
#ifndef NO_RDMA
		if ((flag & PMEM_PHYSIO) && !DMA_BYTE(pmemptr)) {
			/*
			 * We ran out of DMA-able memory.
			 */
			/*
			 *+ During kernel initialization (boot), there
			 *+ was insufficient memory in the DMA-able range
			 *+ to provide for device requiring restricted DMA
			 *+ support.
			 */
			cmn_err(CE_PANIC, "palloc: ran out of DMA-able memory");
		}
#endif /* NO_RDMA */
		remaining -= MMU_PAGESIZE;
		pmemptr += MMU_PAGESIZE;
	} while (remaining != 0);

	ASSERT(hwpage_exists(pfnum(ret_paddr)));
	return ret_paddr;
}

/*
 * STATIC void
 * fgs_mem_init(struct ctlr_desc *)
 *
 *	Initialize error handline on a First Generation System memory board.
 *
 * Calling/Exit State:
 *
 *	None.
 */

STATIC void
fgs_mem_init(register struct ctlr_desc *cd)
{
	/*
	 * Enable Ecc correctable and
	 * uncorrectable error logging and
	 * uncorrectable error reporting.
	 */
	slic_wrslave((int)cd->cd_slic, SL_M_ECC, 
		(SLB_EN_UCE_LOG | SLB_REP_UCE | SLB_EN_CE_LOG));
}

/*
 * STATIC void
 * fgs_mem_poll(struct ctlr_desc *)
 *
 *	Poll a given First Generation System memory board for errors.
 *
 * Calling/Exit State:
 *
 *	Called from timeout().
 *	Log and clear if possible any existing errors.
 *	No return value.
 */

STATIC void
fgs_mem_poll(register struct ctlr_desc *cd)
{
	unchar	eccreg;
	unchar	ilv;
	unchar	val;
	unchar	slbsize;
	unchar	syndrome;
	unchar	row;
	int	addr;
	int	s;

	/*
	 * Read memory board ECC register.
	 */

	s = splhi();
	eccreg = slic_rdslave((int)cd->cd_slic, SL_M_ECC);
	splx(s);

	/*
	 * If no errors, return.  Else print about it and get more data.
	 */

	if ((eccreg & (SLB_UCE|SLB_UCE_OV|SLB_CE|SLB_CE_OV)) == 0)
		return;

	/*
	 *+ A correctible memory error has occurred.
	 *+ No corrective action is required.
	 */
	cmn_err(CE_NOTE,
	"%s %d: ECC %scorrectable error: Slic id = %d, Error Reg = 0x%x,\n",
			cd->cd_name, cd->cd_i,
			(eccreg & (SLB_UCE|SLB_UCE_OV)) ? "Un" : "",
			cd->cd_slic, eccreg);

	/*
	 * Get the base address of the board.
	 */

	s = splhi();
	ilv = slic_rdslave((int)cd->cd_slic, SL_M_ENABLES);
	val = slic_rdslave((int)cd->cd_slic, SL_M_ADDR);
	slbsize = slic_rdslave((int)cd->cd_slic, SL_M_BSIZE);
	splx(s);

	/*
	 * Determine interleaving.
	 */

	if ((ilv & SLB_INTLV) && (val&1))
		val &= ~1;

	/*
	 * Base address.
	 */

	if (slbsize & SLB_LTYPE)
		addr = val << 19;	/* 64K chips */
	else
		addr = ((val & 0xE0) << 19) | ((val & 0x07) << 21);

	if ((eccreg & (SLB_UCE|SLB_CE)) == 0) {
		/*
		 * Clear the error and report.
		 */
		s = splhi();
		slic_wrslave((int)cd->cd_slic, SL_M_ECC, eccreg);
		splx(s);
		/*
		 *+
		 */
		cmn_err(CE_CONT,
		"%s %d: Base Address = 0x%x, Operation = ?, Syndrome = ?\n",
				cd->cd_name, cd->cd_i, addr);
		return;
	}

	/*
	 * Read the ECC error address and determine the row.
	 */

	s = splhi();
	addr &= 0xff000000;	/* mask out lower 24 bits */
	val = slic_rdslave((int)cd->cd_slic, SL_M_EADD_H);
	addr += val << 16;
	val = slic_rdslave((int)cd->cd_slic, SL_M_EADD_M);
	addr += val << 8;
	val = slic_rdslave((int)cd->cd_slic, SL_M_EADD_L);
	addr += val & ~SLB_ROW;
	row = val & SLB_ROW;

	/*
	 * Get operation code and syndrome.
	 */

	val = slic_rdslave((int)cd->cd_slic, SL_M_ES);
	syndrome = slic_rdslave((int)cd->cd_slic, SL_M_SYNDR);

	/*
	 * Clear the error and report.
	 */

	slic_wrslave((int)cd->cd_slic, SL_M_ECC, eccreg);
	splx(s);
	cmn_err(CE_CONT,
	"%s %d: Address = 0x%x (%s Board), Operation = 0x%x, Syndrome = 0x%x\n",
		cd->cd_name, cd->cd_i,
		addr, (row == SLB_CNTRL) ? "Controller" : "Expansion",
		val, syndrome);
}

/*
 * STATIC void
 * sgs_mem_init(struct ctlr_desc *)
 *
 *	Initialize error handling on a First Generation System memory board.
 *
 * Calling/Exit State:
 *
 *	The power-up firmware leaves the board enabled and in "if loggable"
 *	mode (ie., scrubbing is done if it can log an error it finds).
 *	This is how the OS wants to run the memory board, so nothing to do.
 */
/*ARGSUSED*/
STATIC	void
sgs_mem_init(register struct ctlr_desc *cd)
{
	/* nothing to do */
}

/*
 * STATIC void
 * sgs_mem_poll(struct ctlr_desc *)
 *
 *	Poll a given Second Generation System memory board for errors.
 *
 * Calling/Exit State:
 *
 *	Called from timeout().
 *	Log and clear if possible any existing errors.
 *	No return value.
 *
 * Description:
 *
 *	Read the EDC Error Register and see if there are any errors.
 *	Single bit errors can be located to address and bit; double bit
 *	errors can only be isolated to the address.
 *
 *	Smarter polling/recording/etc could be done that might
 *	turn off scrubbing for boards with a bad bank (don't want to
 *	get reports all the time for known bad bits), or be more intelligent
 *	about how often and how to report.
 */

STATIC void
sgs_mem_poll(register struct	ctlr_desc *cd)
{
	register ulong	addr;
	uint	bank;
	unchar	edc;
	int	errbit;
	int	s;
	int	normal_cycle;
	int	xover_cycle;
	unchar	synd_lo;
	unchar	synd_hi;
	static	char Local[] = "local (refresh/scrub)";
	static	char Normal[] = "normal bus";
	static	char XMessage[] = "Low EDC status reflects cross over status\n";
	static	char Edc[] = "%s %d: %s %s error, %s cycle.\nbank=%d addr=0x%x error status=0x%x synd=0x%x\n";
	static	char Interpret_Edc[] = "%s %d: single bit error on %s bit %d\n";
	static	char Hard[] = "Non-correctable";
	static	char Soft[] = "Correctable";
	/*
	 * These messages are for both rev 0 and rev 1 (or later) of
	 * the symmetry memory board.  The definitions of the overflow bits
	 * changed for these revisions.
	 */
	static	char Hiovfl0[] =
		"HIGH overflow bit set, multiple errors have occured.\n";
	static	char LOovfl0[] =
		"LOW overflow bit set, multiple errors have occured.\n";
	static	char Hiovfl1[] = "multiple bit error overflow occured.\n";
	static	char LOovfl1[] = "single bit error overflow occured.\n";

	/*
	 * Read EDC Error Register.  If zero, no errors.
	 */

	s = splhi();

	edc = slic_rdslave((int)cd->cd_slic, MEM_EDC) & MEM_EDC_MASK;

	/*
	 * If any errors, read the syndrome registers
	 */

	if (edc) {
		synd_lo = slic_rdslave((int)cd->cd_slic, MEM_SYND_LO) &
								MEM_SYND_MASK;
		synd_hi = slic_rdslave((int)cd->cd_slic, MEM_SYND_HI) &
								MEM_SYND_MASK;
	}

	/*
	 * Check low part of EDC error register, then high part.
	 */

	if (edc & LO_EDC_SE_ME) {			/* low edc */
		addr = sgs_mem_err_addr(cd->cd_slic, MEM_BDP_LO);
		bank = sgs_mem_bank_decode(cd, addr);
		normal_cycle = synd_lo & MEM_NORMCY;
		xover_cycle = synd_hi & MEM_XOVER;
		/*
		 *+ A memory error has occurred.  If the error is
		 *+ soft, no corrective action has been performed.  If
		 *+ the error is hard, contact service.
		 */
		cmn_err(CE_WARN, Edc, cd->cd_name, cd->cd_i,
			(edc & MEM_EDC_LO_ME) ? Hard : Soft,
			"EDC LO", normal_cycle ? Normal : Local,
			bank, addr, edc, synd_lo);
		if (xover_cycle && (cd->cd_nbdps == 2)) {
			cmn_err(CE_CONT, XMessage);
		}
		/*
		 * Report failing bit if single-bit error and not a
		 * multi-bit error.
		 */
		if ((edc & MEM_EDC_LO_SE) && !(edc & MEM_EDC_LO_ME)) {
			errbit = sgs_mem_decode[synd_lo & MEM_SYND_BITS];
			cmn_err(CE_CONT, Interpret_Edc, cd->cd_name, cd->cd_i,
				(errbit <= 31) ? "data" : "check",
				(errbit <= 31) ? errbit : errbit - 32);
		}
	}

	if (edc & HI_EDC_SE_ME) {			/* high edc */
		/*
		 * Note: address stored only in the low BDP.
		 */
		addr = sgs_mem_err_addr(cd->cd_slic, MEM_BDP_LO);
		bank = sgs_mem_bank_decode(cd, addr);
		xover_cycle =  synd_hi & MEM_XOVER;
		normal_cycle = synd_lo & MEM_NORMCY;
		/*
		 *+ A memory error has occurred.  If the error is
		 *+ soft, no corrective action has been performed.  If
		 *+ the error is hard, contact service.
		 */
		cmn_err(CE_NOTE, Edc, cd->cd_name, cd->cd_i,
			(edc & MEM_EDC_HI_ME) ? Hard : Soft,
			"EDC HI", normal_cycle ? Normal : Local,
			bank, addr, edc, synd_hi);
		if (xover_cycle && (cd->cd_nbdps == 2)) {
			cmn_err(CE_CONT, XMessage);
		}
		/*
		 * Report failing bit if single-bit error and not a
		 * multi-bit error.
		 */
		if ((edc & MEM_EDC_HI_SE) && !(edc & MEM_EDC_HI_ME)) {
			errbit = sgs_mem_decode[synd_hi & MEM_SYND_BITS];
			cmn_err(CE_CONT, Interpret_Edc, cd->cd_name, cd->cd_i,
				(errbit <= 31) ? "data" : "check",
				(errbit <= 31) ? errbit : errbit - 32);
		}
	}
	if (cd->cd_hrev == 0) {
		if (edc & MEM_EDC_LO_OV) {
			cmn_err(CE_CONT, LOovfl0);
		}
		if (edc & MEM_EDC_HI_OV) {
			cmn_err(CE_CONT, Hiovfl0);
		}
	} else {
		if (edc & MEM_EDC_HI_OV) {
			cmn_err(CE_CONT, Hiovfl1);
		} else if (edc & MEM_EDC_LO_OV) {
			cmn_err(CE_CONT, LOovfl1);
		}
	}

	/*
	 * If there was an error, clear it.
	 * Writing anything to Clear EDC Error Information register
	 * clears all error bits.
	 */

	if (edc)
		slic_wrslave((int)cd->cd_slic, MEM_CLR_EDC, 0);
	splx(s);
}

/*
 * STATIC ulong
 * sgs_mem_err_addr(int, int)
 *
 *	Return the address of the last memory error from the bdp.
 *
 * Calling/Exit State:
 *	Returns:  the address of the last memory error.
 */
STATIC ulong
sgs_mem_err_addr(int slic, int bdp)
{
	int	i;
	ulong	temp;
	ulong	result = 0;

	for (i = 0; i < 4; i++) {
		temp = (ulong)slic_rdSubslave(slic, bdp, 
					BDP_WAR | (BDP_BYTE0 - i));
		result |= (temp << (i*8));
	}
	return result;
}

/*
 * STATIC uint
 * sgs_mem_bank_decode(struct ctlr_desc *, ulong)
 *
 *	Return indication of which bank the error was in.
 *
 * Calling/Exit State:
 *
 *	Returns the bank of the error.
 *
 * Description:
 *
 *	A painful decode of cases.
 */

STATIC uint
sgs_mem_bank_decode(struct ctlr_desc *cd, ulong err_addr)
{
	uint	bank_select;
	int	normal_cycle;
	int	four_banks;
	int	half_banks;
	int	four_meg_rams;
	int	interleaved;

	normal_cycle = slic_rdslave(cd->cd_slic, MEM_SYND_LO) & MEM_NORMCY;
	four_banks = slic_rdslave(cd->cd_slic, MEM_CFG) & MEM_CFG_4_BANKS;
	half_banks = (cd->cd_m_type & SL_M2_RAM_POP) == SL_M2_RAM_HALF;
	four_meg_rams = (cd->cd_m_type & SL_M2_RAM_DENS) == SL_M2_RAM_4MB
					|| ((cd->cd_m_expid & MEM_EXP_4MB) &&
					    (cd->cd_m_expid != MEM_EXP_NONE));
	interleaved = cd->cd_m_ileave;

#define BIT(i)	((err_addr) & (1 << (i)))

	if (half_banks) {
		if (!normal_cycle) {
				bank_select = (BIT(25)|BIT(26)|BIT(27)) >> 25;
		} else if (four_meg_rams) {
			if (interleaved)
				bank_select = (BIT(24) >> 24)
					    | ((BIT(26)|BIT(27)) >> 25);
			else
				bank_select = (BIT(24)|BIT(25)|BIT(26)) >> 24;
		} else if (interleaved) {
			if (four_banks)
				bank_select = (BIT(22) >> 22) | (BIT(24) >> 23);
			else
				bank_select = (BIT(22) >> 22)
					    | ((BIT(24)|BIT(25)) >> 23);
		} else if (four_banks)
			bank_select = (BIT(22)|BIT(23)) >> 22;
		else
			bank_select = (BIT(22)|BIT(23)|BIT(24)) >> 22;
	} else {
		if (!normal_cycle) {
				bank_select = (BIT(25)|BIT(26)|BIT(27)) >> 25;
		} else if (four_meg_rams) {
			if (interleaved)
				bank_select = (BIT(26)|BIT(27)|BIT(28)) >> 26;
			else
				bank_select = (BIT(25)|BIT(26)|BIT(27)) >> 25;
		} else if (interleaved) {
			if (four_banks)
				bank_select = (BIT(24)|BIT(25)) >> 24;
			else
				bank_select = (BIT(24)|BIT(25)|BIT(26)) >> 24;
		} else if (four_banks)
			bank_select = (BIT(23)|BIT(24)) >> 23;
		else
			bank_select = (BIT(23)|BIT(24)|BIT(25)) >> 23;
	} 
	return (bank_select);
#undef	BIT
}
