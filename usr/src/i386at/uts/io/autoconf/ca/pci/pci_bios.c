/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/autoconf/ca/pci/pci_bios.c	1.12"
#ident	"$Header: $"

#include <proc/regset.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>
#include <io/ddi_i386at.h>
#include <io/autoconf/ca/ca.h>
#include <io/autoconf/ca/pci/pci.h>


#if defined(DEBUG) || defined(DEBUG_TOOLS)
STATIC int ca_pci_bios_debug = 0;
STATIC int ca_pci_bypass = 0;
#define DBG1(s)	if(ca_pci_bios_debug == 1) printf(s)
#define DBG2(s)	if(ca_pci_bios_debug == 2) printf(s)

#else
#define	DBG1(s)
#define DBG2(s)
#endif

/*
 * Autoconfig -- PCI bios setup (pci_verify) and bios calls
 */

/*
 * The functions provided within allow the pcica 'pci_init' function to
 * probe the PCI bios for various data elements. 
 *
 *
 * major routines:
 * pci_verify: finds the 32-bit BIOS entry point for the PCI bios and
 * saves it in a global (Pci_bios_addr).  Checks to be sure there really
 * is a PCI bios as well.

 * pci_read_8,_16, _32	read from a PCI card's config space
 * pci_write_8,_16,_32  write to a particular card's config space
 *
 * Uses: _pci_bios_call, the generic interface to do a protect mode BIOS call
 * given a protected-mode entry point
 *
 *
 */

extern pci_read_8(uchar_t bus, uchar_t devfun, ushort_t offset, uchar_t *ret);
extern pci_read_16(uchar_t bus, uchar_t devfun, ushort_t offset, ushort_t *ret);
extern pci_read_32(uchar_t bus, uchar_t devfun, ushort_t offset, uint_t *ret);
extern pci_write_8(uchar_t bus, uchar_t devfun, ushort_t offset, uchar_t val);
extern pci_write_16(uchar_t bus, uchar_t devfun, ushort_t offset, ushort_t val);
extern pci_write_32(uchar_t bus, uchar_t devfun, ushort_t offset, uint_t val);

char 	*Pci_bios_entry = NULL;
char	*pci_scan	= "YES";
LKINFO_DECL(pci_lockinfo, "AUTO-CONF:pci:pci_lock", 0);
lock_t *pci_lock;

/*
 * int
 * pci_verify(void)
 * 
 * Calling/Exit State:
 *	Find if there is a PCI BIOS on the system
 *	return 0 if so, -1 if not
 */
int
pci_verify(void)
{
	char * bios_32_service;
	caddr_t	pci_bios_paddr;
	char	*bios_32_directory;
	int	i, offset, bios_size;
	int	pci_bios_length;
	int	pci_bios_offset;
	int	csum(char *);
	regs	reg;
	uchar_t ignore;

	/*
	* Algorithm:
	*
	*	According to the PCI Bios spec 2.0
	*	32-bit protected mode BIOS access to the
	*	PCI devices is guaranteed. To find the
	* 	bios itself, you have to
	*	1) see if there is a 'BIOS 32 directory'
	*	2) see if there's a PCI bios in that directory
	*	3) see if that PCI bios indeed 'id's' to being a PCI bios
	*
	*	In cases (1) and (2), this involves mapping in some real
	*	memory, and hunting for strings.
	*	In case 3, a bios call (using _pci_bios_call) is actually
	*	made and a return value checked.
	*/
#ifdef DEBUG
	if (ca_pci_bypass == 1){
		DBG1(("Bypassing PCI bus check!\n"));
		return -1;
	}
#endif
	if (strcmp(pci_scan,"YES") != 0){
		cmn_err(CE_NOTE,"Bypassing PCI bus scan.\n");
		return -1;
	}
	if (Pci_bios_entry != 0)
		return 0; /* already been called once */

	bios_size = BIOS_32_SIZE;
	bios_32_service = (caddr_t) 0;
	bios_32_directory = (char *)physmap((paddr_t)BIOS_32_START,
			bios_size,
			KM_NOSLEEP);

	if (bios_32_directory == NULL){
		return -1;
	}
	/*
	*
	* search for BIOS 32 service id ("_32_")
	*/

	for (i = 0; bios_32_directory + i < bios_32_directory + BIOS_32_SIZE; i += 16)

		if (!bcmp(bios_32_directory + i, BIOS_32_ID, 4) &&
			csum(bios_32_directory + i) == 0){
			bcopy((caddr_t) bios_32_directory + i + 4, &bios_32_service,4);
			break;
		}

	if (bios_32_service == 0){
		return -1;
	}
DBG1(("bios_32_service phys addr = 0x%x\n", bios_32_service));
	/* Find location of  PCI bios within bios dir space */
	/*
	* assumption: if the BIOS 32 directory is there, the BIOS
	* support _usually_ is within the range already mapped.
	* If not, i.e. the value returned is less than
	* BIOS_32_START, then remap what is there. If it is beyond
	* what is already mapped, that is,
	* the value is greater than BIOS_32_START + BIOS_32_SIZE,
	* then assume the BIOS lives in the 64K
	* bytes given in the value (note: the spec says it should be
	* contained within that page and the following page, so
	* if pages ever get as big as 64k, this may stop working)
	*
	* The normal case is for the BIOS 32 directory memory to be
	* mapped already.
	*/
	if ((uint_t) bios_32_service < BIOS_32_START){
		physmap_free(bios_32_directory,BIOS_32_SIZE, 0);
		bios_size += BIOS_32_START - *(int *)bios_32_service;
		bios_32_directory = (char *)physmap((paddr_t)bios_32_service,
			bios_size,
			KM_NOSLEEP);
		if (bios_32_directory == NULL) return -1;
		bios_32_service = bios_32_directory;
	}
	else if ((uint_t) bios_32_service > (BIOS_32_START + BIOS_32_SIZE)){
		physmap_free(bios_32_directory,BIOS_32_SIZE, 0);
		bios_size = 0x20000; /* guess here */
		bios_32_directory = (char *)physmap((paddr_t)bios_32_service,
			bios_size,
			KM_NOSLEEP);

		if (bios_32_directory == NULL) return -1;
		bios_32_service = bios_32_directory;
	}
	else {
		offset = (int) bios_32_service - BIOS_32_START;
		bios_32_service = bios_32_directory + offset;
	}
DBG1(("bios_32_service virt addr = 0x%x\n", bios_32_service));
	/* find the PCI bios base and length, using the bios_32 entry address*/

	/* fill in a regs structure, with service ID in EAX */

	bzero(&reg, sizeof(reg));
	bcopy(PCI_BIOS_ID, (caddr_t) &reg.eax.eax, 4);
	reg.ebx.ebx = 0;

	_pci_bios_call(&reg, bios_32_service);

	switch(reg.eax.byte.al){
		case 0:
			break; /* found PCI entry point */
		case 0x80:
		case 0x81:
		default:
			cmn_err(CE_WARN, "!PCI Bios service not supported.\n");
			physmap_free(bios_32_directory,bios_size, 0);
			return -1;
	}
	/* have the address now of the PCI BIOS entry point. Make
	* a BIOS call to find out how many PCI Busses there are (and
	* hence ensure there may be some PCI devices to worry about)
	*/

	pci_bios_paddr = (caddr_t) reg.ebx.ebx;
	if (pci_bios_paddr < (caddr_t) 0xA0000){
		cmn_err(CE_NOTE,"!Unsupported PCI BIOS physaddr:0x%x",
		pci_bios_paddr);
		return -1;
	}
	pci_bios_paddr -= (paddr_t) MMU_PAGESIZE;
	pci_bios_length = reg.ecx.ecx + MMU_PAGESIZE*2;
	pci_bios_offset = reg.edx.edx;
	/* don't need BIOS 32 mapped anymore */
	physmap_free(bios_32_directory, bios_size, 0);
	
	Pci_bios_entry = (char *)physmap((paddr_t)
					(pci_bios_paddr + pci_bios_offset),
			pci_bios_length,
			KM_NOSLEEP);
	if (Pci_bios_entry == NULL){
		cmn_err(CE_NOTE, "!Detected PCI bios but unable to map.\n");
		return -1;
	}
	/* found BIOS, now, is it *really* a PCI BIOS */
	Pci_bios_entry += (paddr_t) MMU_PAGESIZE; 
	if (find_pci_id(&ignore)!= 0){
		cmn_err(CE_NOTE, "!PCI has unknown id.\n");
		physmap_free(Pci_bios_entry,pci_bios_length, 0);
		return -1;
	}
	if (!(pci_lock = LOCK_ALLOC(1, plbase, &pci_lockinfo, KM_NOSLEEP)))
		return ENOMEM;
	return 0; /* finally got PCI bios entry address!*/
}

/*
 * int
 * csum(char *)
 * 
 * Calling/Exit State:
 *	Find if the 11 bytes pointed at by p sum to 0. Necessary
 *	for determining presence of BIOS 32 service dir. Return
 *	the sum.
 */
static int
csum(char *p) {
	/* inelegant way to compute checksum of BIOS 32 service directory */
	uchar_t sum;
	if (p == 0) return -1;
	sum = p[0] + p[1] + p[2] + p[3] + p[4] + p[5] + p[6] + p[7] + p[8] +
		p[9] + p[10];	
	return (int) sum;
}


/*
 * int
 * find_pci_id(uchar_t *bus)
 * 
 * Calling/Exit State:
 *	Find the number of the last PCI bus on the system. Has the
 *	side effect of 'exercising' the PCI BIOS
 *
 * Arguments:
 *	uchar_t *bus:	pointer to uchar_t to contain bus
 *
 * Return values: 0 if successful, -1 if BIOS call failed somehow
 */
int
find_pci_id(uchar_t *bus){
	regs reg;

	bzero(&reg, sizeof(reg));
	reg.eax.byte.ah = PCI_FUNCTION_ID;
	reg.eax.byte.al = PCI_BIOS_PRESENT;	

	_pci_bios_call(&reg, Pci_bios_entry);

	if (bcmp(&reg.edx.edx, PCI_ID, 4) != 0){
		return -1;
	}
	*bus = reg.ecx.byte.cl;
	return 0;
}


/*
 * int
 * pci_<read|write>_[8|16|32](uchar_t bus, uchar_t devfun, ushort_t offset,
 *			      value)
 * 
 * Calling/Exit State:
 *	read or write the # of bits at the given PCI device/function, bus
 * 	config space location.
 *
 *
 * Arguments:
 *	uchar_t bus:	bus # to write at
 *	uchar_t devfun:	PCI device/function byte(see PCI BIOS spec for details)
 *	ushort_t offset:Which of the 256 registers to write in
 *	value:	properly sized value to read from/write to
 *
 * Algorithm: read/write (based on name) the value given, using appropriate
 * PCI BIOS call. Uses _pci_bios_call entry point as to make call.
 *
 * Return values: 0 if successful, PCI_FAILURE if BIOS call failed somehow
 */
int
pci_read_8(uchar_t bus, uchar_t devfun, ushort_t offset, uchar_t *ret)
{
	regs reg;

	bzero(&reg, sizeof(reg));
	reg.eax.byte.ah = PCI_FUNCTION_ID;
	reg.eax.byte.al = PCI_READ_CONFIG_BYTE;
	reg.ebx.byte.bh = bus;
	reg.ebx.byte.bl = devfun;
	reg.edi.word.di = offset;

	if (_pci_bios_call(&reg, Pci_bios_entry) != PCI_SUCCESS)
		return  PCI_FAILURE;

	*ret = reg.ecx.byte.cl;
	return 0;
}

int
pci_read_16(uchar_t bus, uchar_t devfun, ushort_t offset, ushort_t *ret)
{
	regs reg;

	bzero(&reg, sizeof(reg));
	reg.eax.byte.ah = PCI_FUNCTION_ID;
	reg.eax.byte.al = PCI_READ_CONFIG_WORD;
	reg.ebx.byte.bh = bus;
	reg.ebx.byte.bl = devfun;
	reg.edi.word.di = offset;

	if (_pci_bios_call(&reg, Pci_bios_entry) != PCI_SUCCESS)
		return  PCI_FAILURE;

	*ret = reg.ecx.word.cx;
	return 0;
}	

int
pci_read_32(uchar_t bus, uchar_t devfun, ushort_t offset, uint_t *ret)
{
	regs reg;

	bzero(&reg, sizeof(reg));
	reg.eax.byte.ah = PCI_FUNCTION_ID;
	reg.eax.byte.al = PCI_READ_CONFIG_DWORD;
	reg.ebx.byte.bh = bus;
	reg.ebx.byte.bl = devfun;
	reg.edi.word.di = offset;

	if (_pci_bios_call(&reg, Pci_bios_entry) != PCI_SUCCESS)
		return  PCI_FAILURE;

	*ret = reg.ecx.ecx;
	return 0;
}

/* write routines */

int
pci_write_8(uchar_t bus, uchar_t devfun, ushort_t offset, uchar_t val)
{
	regs reg;

	bzero(&reg, sizeof(reg));
	reg.eax.byte.ah = PCI_FUNCTION_ID;
	reg.eax.byte.al = PCI_WRITE_CONFIG_BYTE;
	reg.ebx.byte.bh = bus;
	reg.ebx.byte.bl = devfun;
	reg.edi.word.di = offset;
	reg.ecx.byte.cl = val;

	_pci_bios_call(&reg, Pci_bios_entry);
	return  (int) reg.eax.byte.ah;
}

int
pci_write_16(uchar_t bus, uchar_t devfun, ushort_t offset, ushort_t val)
{
	regs reg;

	bzero(&reg, sizeof(reg));
	reg.eax.byte.ah = PCI_FUNCTION_ID;
	reg.eax.byte.al = PCI_WRITE_CONFIG_WORD;
	reg.ebx.byte.bh = bus;
	reg.ebx.byte.bl = devfun;
	reg.edi.word.di = offset;
	reg.ecx.word.cx = val;

	_pci_bios_call(&reg, Pci_bios_entry);
	return  (int) reg.eax.byte.ah;
}

int
pci_write_32(uchar_t bus, uchar_t devfun, ushort_t offset, uint_t val)
{
	regs reg;

	bzero(&reg, sizeof(reg));
	reg.eax.byte.ah = PCI_FUNCTION_ID;
	reg.eax.byte.al = PCI_WRITE_CONFIG_DWORD;;
	reg.ebx.byte.bh = bus;
	reg.ebx.byte.bl = devfun;
	reg.edi.word.di = offset;
	reg.ecx.ecx = val;

	_pci_bios_call(&reg, Pci_bios_entry);
	return  (int) reg.eax.byte.ah;
}

extern lock_t *pci_lock;

int
pci_read_devconfig(uchar_t bus, uchar_t devfun, uchar_t *buffer, int offset, int length){
	int i, ret;
	pl_t oldprio;

	if (pci_verify() != 0) /* can't set up BIOS access */
		return PCI_FAILURE;

	if (offset + length > 256){
		cmn_err(CE_WARN,
		       "!Tried to read past the end of PCI config space.\n");	
		return -1;
	}

	oldprio = LOCK(pci_lock, plbase);
	for(i = offset; i < offset + length; i++){
		if ((ret = pci_read_8(bus, devfun, i, buffer))
			!= PCI_SUCCESS){
				UNLOCK(pci_lock, oldprio);
				return ret;
		}
		buffer++;
	}
	UNLOCK(pci_lock, oldprio);
	return length;
}

/*
 * size_t
 * pci_devconfig_size(uchar_t bus, uchar_t devfun)
 *
 * Calling/Exit State:
 *	Return the size of device configuration space.
 */
/* ARGSUSED */
size_t
pci_devconfig_size(uchar_t bus, uchar_t devfun)
{
	if (pci_verify() != 0) /* can't set up BIOS access */
		return (0);

	return (MAX_PCI_REGISTERS);
}

/*
 * int
 * pci_read_exp_rom_signature(struct pci_rom_signature_data *)
 * 
 * Calling/Exit State:
 *	Read the expansion ROM area on a PCI system for all rom signatures
 *	and return the count;
 *
 * If fail, return -1, otherwise return the number of expansion rom
 * signatures found. Compute the addr and offset of the exp rom as well
 * and store in the data structure.
 *
 * Note: The PCI bios spec gives details on finding the expansion rom
 * and getting the lengths and so on. 
 */
int
pci_read_exp_rom_signature(struct pci_rom_signature_data *buf){

	char *e_rom_start = NULL; 
	struct pci_rom_header *p = NULL;
	struct pci_rom_data *q = NULL;
	int	i, count;

	e_rom_start = (char *)physmap((paddr_t)PCI_EXP_ROM_START_ADDR,
			PCI_EXP_ROM_SIZE,
			KM_NOSLEEP);
	if (e_rom_start == NULL){
		cmn_err(CE_WARN,"!Could not map PCI expansion rom space.\n");
		return -1;
	}
	count = 0;
	for (i = 0; i < (PCI_EXP_ROM_SIZE / PCI_EXP_ROM_HDR_CHUNK); i++){
		q = NULL;
		p = (struct pci_rom_header *) ((char *) e_rom_start + i * PCI_EXP_ROM_HDR_CHUNK);
		if (p->sig == PCI_EXP_ROM_HDR_SIG){

			q = (struct pci_rom_data *)( (char *) p + p->offset);

			if ((char *)q >= (e_rom_start + PCI_EXP_ROM_SIZE) ||
			    q->v_id == PCI_INVALID_VENDOR_ID) continue;
			if (bcmp(q->signature, PCI_EXP_ROM_DATA_SIG, 4) != 0)
				continue;

			if (q->base_class[2] == PCI_CLASS_TYPE_DISPLAY) continue;
			buf[count].vendor_id = q->v_id;
			buf[count].device_id = q->d_id;
			buf[count].addr = PCI_EXP_ROM_START_ADDR + i*PCI_EXP_ROM_HDR_CHUNK;
			buf[count].length = (p->run_length * 512);
			buf[count].used = 0;
			count++;
		}
	}
	physmap_free(e_rom_start, PCI_EXP_ROM_SIZE, 0);
	return count;
}
