/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:svc/pbios.c	1.7"
#ident	"$Header: $"

/*
 * Physical mode BIOS calls. 
 *
 *	- EISA BIOS call interface to access NVRAM space.
 *
 *	- BIOS call to determine the monitor type.
 */

#include <io/gvid/vdc.h>
#include <io/kd/kd.h>
#include <svc/eisa.h>
#include <svc/bootinfo.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>

extern void psm_phys_puts(const char *);

extern paddr_t bootinfo_loc;
extern paddr_t phys_palloc2(uint_t, uint_t);

/*
 * Macros to get Bytes from longs.
 */
#define BYTE0(x)	((x) & 0x0FF)
#define BYTE1(x)	(((x) >> 8) & 0x0FF)
#define BYTE2(x)	(((x) >> 16) & 0x0FF)
#define BYTE3(x)	(((x) >> 24) & 0x0FF)

#define	DEBUG0(s) { \
		int	i; \
		psm_phys_puts((s)); \
		for (i = 0; i < 1000000; i++); \
}

#define	DEBUG1(s)

#define	PHYS_EISA_BCOPY(src, dst, size) { \
		int	i; \
		for (i = 0; i < (size); i++) \
			(*(dst)++) = (*(src)++); \
}

/*
 * We initialize these values to force them into static data (rather than
 * BSS) so that they are located in low physical memory.
 */

char *phys_nvm_data = { 0 };
eisa_info_t phys_eisa_info[EISA_MAX_SLOTS] = { 0 };
struct vdc_info phys_vdc_info = { 0 };

struct intregs {
	ushort_t	intval;
	ushort_t	ax;
	ushort_t	bx;
	ushort_t	cx;
	ushort_t	dx;
	ushort_t	bp;
	ushort_t	es;
	ushort_t	si;
	ushort_t	ds;
	ushort_t	di;
#ifdef NOTYET
	ushort_t	ss;
	ushort_t	sp;
	ulong_t		oesp;
#endif /* NOTYET */
};

typedef struct intregs	intregs_t;

/*
 * int
 * phys_eisa_read_slot(int slot, char *buffer)
 *
 * Calling/Exit State:
 *	Return 1 if carry flag is set, otherwise return 0.
 */
int
phys_eisa_read_slot(int slot, char *buffer)
{
	intregs_t	regs;
	int		status;

	regs.ax = 0xD800;
	regs.cx = (ushort_t)slot;
	regs.ds = 0;
	regs.intval = 0x15;
	regs.bx = 0;
	regs.bp = 0;
	regs.si = 0;
	regs.es = 0;
	regs.dx = 0;
	regs.di = 0;

	status = doint(&regs);

        /* Arranges data to match "eisa_slotinfo_t" structure. See "eisa.h". */

	*((short *)buffer)++ = regs.di;		/* board id bytes 0, 1 */
        *((short *)buffer)++ = regs.si;		/* board id bytes 2, 3 */
        *((short *)buffer)++ = regs.bx;		/* major and minor revision */
        *buffer++ = BYTE1(regs.dx);		/* no. of functions */
        *buffer++ = BYTE0(regs.dx);		/* function info byte */
        *((short *)buffer)++ = regs.cx;		/* checksum */
        *buffer++ = BYTE0(regs.ax);		/* dupid byte 0 */
        *buffer++ = BYTE1(regs.ax);		/* dupid byte 1 */ 

	return (status);
}


/*
 * int
 * phys_eisa_read_func(int slot, int func, char *buffer)
 *	Read function data from EISA cmos memory/configuration space.
 *
 * Calling/Exit State:
 *	Return 1 if carry flag is set, otherwise return 0.
 */
int
phys_eisa_read_func(int slot, int func, char *buffer)
{
	intregs_t	regs;
	int		status;

	regs.ax = 0xD801;
	regs.cx = (ushort_t) ((func << 8) | slot);
	regs.ds = (ushort_t) buffer >> 4; 
        regs.si = (ushort_t) buffer & 0xf;
	regs.intval = 0x15;
	regs.bx = 0;
	regs.bp = 0;
	regs.es = 0;
	regs.dx = 0;
	regs.di = 0;

	status = doint(&regs);

	return (status);
}


#define	PHYS_EISA_NPAGES	8
#define	PHYS_EISA_DATA_SIZE	(PHYS_EISA_NPAGES * MMU_PAGESIZE)
/*
 * flags for phys_palloc2
 */
#define	P_PREFER_DMA		0
#define	P_PREFER_NDMA		1

/*
 * Return value from phys_palloc2()
 */
#define PALLOC_FAIL     ((paddr_t)0xFFFFFFFF)


/*
 * void
 * _eisa_read_nvram(void)
 *	Read all slot data from eisa cmos memory.
 *
 * Calling/Exit Status:
 *	None.
 *
 * Remarks:
 *	There is an assumption that PHYS_EISA_NPAGES pages chunk of
 *	contiguous physical memory is available.
 */
void
_eisa_read_nvram(void)
{
	int	s;		/* slot number */
	int	f;		/* function number */
	eisa_funcinfo_t	funcinfo;
	char	*data;
	char	*biosdata;
	int	nfuncs;
	size_t	sz;
	ushort_t *dupid;
	uint_t	npages = PHYS_EISA_NPAGES;
#define	pbootinfo	(*((struct bootinfo *)bootinfo_loc))

	if ((pbootinfo.machflags & EISA_IO_BUS) == 0)
		return;

	phys_nvm_data = (char *)phys_palloc2(npages, P_PREFER_NDMA);
	data = (char *)phys_nvm_data;
	if ((paddr_t)phys_nvm_data == PALLOC_FAIL)
		return;

	for (s = 0; s < EISA_MAX_SLOTS; s++) {
		DEBUG1("phys_eisa_read_slot\r\n");
		biosdata = (char *)&phys_eisa_info[s].eslotinfo;
		if (phys_eisa_read_slot(s, biosdata)) {
			dupid = (ushort_t *)&phys_eisa_info[s].eslotinfo.dupid;
			phys_eisa_info[s].estatus = BYTE1(*dupid);
			continue;
		}
		nfuncs = (int)phys_eisa_info[s].eslotinfo.functions;
		sz = (EISA_SLOTINFO_SIZE + (nfuncs * EISA_FUNCINFO_SIZE));
		if ((data + sz) > ((char *)phys_nvm_data + PHYS_EISA_DATA_SIZE)) {
			npages = ((sz > PHYS_EISA_DATA_SIZE) ?
				  btopr(sz) :
				  PHYS_EISA_NPAGES);
			phys_nvm_data = (char *)phys_palloc2(npages, P_PREFER_NDMA);
			data = (char *)phys_nvm_data;
			if ((paddr_t)phys_nvm_data == PALLOC_FAIL)
				return;
		}

		phys_eisa_info[s].eaddr = (paddr_t)data;
		phys_eisa_info[s].esize = sz;
			
		PHYS_EISA_BCOPY(biosdata, data, EISA_SLOTINFO_SIZE);

 		for (f = 0; f < nfuncs; f++) {
			DEBUG1("phys_eisa_read_func\r\n");
 			if (phys_eisa_read_func(s, f, (char *)&funcinfo)) {
				dupid = (ushort_t *)&phys_eisa_info[s].eslotinfo.dupid;
				phys_eisa_info[s].estatus = BYTE1(*dupid);
				continue;
			}
			biosdata = (char *)&funcinfo;
			PHYS_EISA_BCOPY(biosdata, data, EISA_FUNCINFO_SIZE);
			phys_eisa_info[s].efuncs++;
		}
	}
}


/*
 * int
 * phys_vdc_mon_type(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	AH = 0x12 and BL = 0x10 return the information about the adapter's
 *	current setting -- color or monochrome mode, amount of memory and
 *	switch settings for EGA cards.
 */
int
phys_vdc_mon_type(void)
{
	intregs_t	regs;
	int		status;

	regs.ax = 0x1200;
	regs.cx = 0;
	regs.ds = 0;
	regs.intval = 0x10;
	regs.bx = 0x0010;
	regs.bp = 0;
	regs.si = 0;
	regs.es = 0;
	regs.dx = 0;
	regs.di = 0;

	status = doint(&regs);

	phys_vdc_info.v_info.dsply = (BYTE1(regs.bx) ? KD_STAND_M : KD_STAND_C);
	phys_vdc_info.v_info.rsrvd = BYTE0(regs.bx);	/* memory */
	phys_vdc_info.v_switch = BYTE0(regs.cx);	/* EGA switch setting */

	return (status);
}

