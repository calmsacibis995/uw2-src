/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/autoconf/ca/pci/pci_ca.c	1.15"
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
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/autoconf/ca/ca.h>
#include <io/autoconf/ca/pci/pci.h>


extern pci_read_8(uchar_t bus, uchar_t devfun, ushort_t offset, uchar_t *ret);
extern pci_read_16(uchar_t bus, uchar_t devfun, ushort_t offset, ushort_t *ret);
extern pci_read_32(uchar_t bus, uchar_t devfun, ushort_t offset, uint_t *ret);
extern pci_write_8(uchar_t bus, uchar_t devfun, ushort_t offset, uchar_t val);
extern pci_write_16(uchar_t bus, uchar_t devfun, ushort_t offset, ushort_t val);
extern pci_write_32(uchar_t bus, uchar_t devfun, ushort_t offset, uint_t val);
extern register_pci_device(uchar_t bus, uchar_t dev, uchar_t fun);
extern pci_read_exp_rom_signature(struct pci_rom_signature_data *);
extern lock_t *pci_lock;
extern char * Pci_bios_entry;
static find_exp_rom_data(ushort_t vendor_id, ushort_t device_, uint_t *addr, uint_t *length);

#if defined(DEBUG) || defined(DEBUG_TOOLS)
STATIC int ca_pci_debug = 0;
#define DBG1(s)	if(ca_pci_debug == 1) printf(s)
#define DBG2(s)	if(ca_pci_debug == 2) printf(s)
#else
#define	DBG1(s)
#define DBG2(s)
#endif

/*
 * Autoconfig -- PCI CA access
 */

/*
 * The functions provided within read the devices on the variosu PCI
 * busses and create entries for them in the configuration manager-maintained
 * database (via cm_register_devconfig.)
 * This relies heavily on 'pci_bios.c'
 *
 * major routines:
 * ca_pci_init: Find the # of PCI buses, then all the PCI devices
 * on the bus.
 *
 *
 */

static struct pci_rom_signature_data *pci_rom_signature_buf;
int pci_num_signatures;
int
ca_pci_init(void)
{
	regs	reg;
	size_t rom_sig_size;
	uchar_t	curbus, maxbus, devnum, funcnum, header;

	if (pci_verify() != 0)
		return -1; /* already been called once */
	if (find_pci_id(&maxbus) != 0){
		cmn_err(CE_WARN,"!PCI Bios present but no bus devices.\n");
		return -1;
	}
	rom_sig_size = sizeof(struct pci_rom_signature_data) * PCI_EXP_ROM_SIZE/PCI_EXP_ROM_HDR_CHUNK;
	pci_rom_signature_buf = kmem_zalloc(rom_sig_size, KM_NOSLEEP);

	if (pci_rom_signature_buf == NULL){
		cmn_err(CE_WARN, "!kmem_zalloc failed for rom signature data.\n");
		return -1;
	}
	pci_num_signatures = pci_read_exp_rom_signature(pci_rom_signature_buf);
	if (pci_num_signatures < 0) { /* error */
		cmn_err(CE_WARN, "!failure when reading exp rom area.\n");
		kmem_free(pci_rom_signature_buf, rom_sig_size);
		return -1;
	}

	for (curbus = 0; curbus <= maxbus; curbus++)
	   for (devnum = 0; devnum < MAX_PCI_DEVICES; devnum++){
		if(pci_read_8(curbus, (devnum<<3)|0,
			     PCI_HEADER_OFFSET, &header) == PCI_FAILURE)
			     continue;
		if (header & 0x80) {/* multi-function device */
		   for(funcnum = 0; funcnum < MAX_PCI_FUNCTIONS; funcnum++)
			if (!register_pci_device(curbus, devnum, funcnum))
				break;
		}
		else register_pci_device(curbus, devnum, 0);
	}

	kmem_free(pci_rom_signature_buf, rom_sig_size);
	return 0;
}

extern char * Pci_bios_entry;
int
register_pci_device(uchar_t bus, uchar_t dev, uchar_t fun)
{
	int i, nio, nmem; 
	ushort_t command, dis_command, vendor_id, device_id;
	uchar_t  iline, ipin, base_class;
	uchar_t devfun;
	struct config_info *cip;
	static struct config_irq_info iattrib = {
		1,	/* level sensitive */
		0,	/* active low */
		1,	/* sharable */
		0};	/* reserved */	
	regs reg;

	static struct config_irq_info *irq_attrib = &iattrib;
	uint_t base_regs[MAX_BASE_REGISTERS], reg_lengths[MAX_BASE_REGISTERS];

	devfun = (dev << 3) | fun;
	pci_read_16(bus, devfun, PCI_VENDOR_ID_OFFSET, &vendor_id);
	if (vendor_id == PCI_INVALID_VENDOR_ID){
		return 0;
	}
	pci_read_16(bus, devfun, PCI_DEVICE_ID_OFFSET, &device_id);
	pci_read_8(bus, devfun, PCI_INTERPT_LINE_OFFSET, &iline);
	
	pci_read_8(bus, devfun, PCI_INTERPT_PIN_OFFSET, &ipin);
	pci_read_8(bus, devfun, PCI_BASE_CLASS_OFFSET, &base_class);
	

	/*
	* allocate structure, fill in, and continue
	*/

	if (base_class != PCI_CLASS_TYPE_BRIDGE
		&& (base_class == PCI_CLASS_TYPE_PCC ||
		    base_class == PCI_CLASS_TYPE_MASS_STORAGE ||
		    base_class == PCI_CLASS_TYPE_NETWORK ||
		    base_class == PCI_CLASS_TYPE_DISPLAY ||
		    base_class == PCI_CLASS_TYPE_MULTIMEDIA ||
		    base_class == PCI_CLASS_TYPE_MEMORY)) {
		
		for (i = 0; i < MAX_PCI_FUNCTIONS; i++){
			bzero(&reg, sizeof(reg));
			reg.eax.byte.ah = PCI_FUNCTION_ID;
			reg.eax.byte.al = PCI_FIND_PCI_DEVICE;
			reg.ecx.word.cx = device_id;
			reg.edx.word.dx = vendor_id;
			reg.esi.word.si = (short) i;
			_pci_bios_call(&reg, Pci_bios_entry);

			if (reg.eax.byte.ah == 0 &&
			    reg.ebx.byte.bh == bus) /* found_it */{
				break;
			}
			else if (reg.eax.byte.ah ==PCI_BAD_VENDOR_ID)
				return -1;
			else if (reg.eax.byte.ah == PCI_DEVICE_NOT_FOUND)
				return -1;
		}
		pci_read_16(bus, devfun, PCI_COMMAND_OFFSET, &command);
		if ((command & 0x3) == 0){ /* not enabled for memory or i/o */
			cmn_err(CE_NOTE,"!device 0x%x%x NOT ENABLED!\n",
				vendor_id, device_id);
			return -1;
		}
		CONFIG_INFO_KMEM_ZALLOC(cip);
		cip->ci_busid = CM_BUS_PCI;
		cip->ci_pci_busnumber = bus;
		cip->ci_pci_devfuncnumber = devfun;
		cip->ci_pcivendorid = vendor_id;
		cip->ci_pcidevid = device_id;

		if (ipin != 0){
			cip->ci_numirqs = 1;
			cip->ci_irqline[0] = iline;
			bcopy((uchar_t *) irq_attrib, &cip->ci_irqattrib[0], 1);
		}
		for (i = 0; i < MAX_BASE_REGISTERS; i++)
			pci_read_32(bus, devfun,
				   PCI_BASE_REGISTER_OFFSET + (i * 4),
				   &base_regs[i]);
		dis_command = command & PCI_COMMAND_DISABLE;

		/*
		* If a register is not 0, read the length it specifies.
		* disable the device (via writing 0's to low-order 2 bits)
		* Get the length associated with the register
		* If I/O, store where it is mapped to, along with its length
		* If it's memory, if it is a 64-bit range, punt (break from loop
		* and put out a warning message.) If it's not 64 bit it
		* works a lot like I/O (though you disable memory accesses
		* to the device in this case.)
		*/

		bzero(reg_lengths, sizeof(reg_lengths));
		pci_write_16(bus, devfun, PCI_COMMAND_OFFSET,
	  		    dis_command);
		nio = nmem = 0;
		for (i = 0; i < MAX_BASE_REGISTERS; i++){
			if (base_regs[i] != 0){
			   pci_write_32(bus, devfun, 
				   PCI_BASE_REGISTER_OFFSET + (i * 4),
				   0xFFFFFFFF);
			   pci_read_32(bus, devfun, 
				   PCI_BASE_REGISTER_OFFSET + (i * 4),
				   &reg_lengths[i]);
			   pci_write_32(bus, devfun, 
				   PCI_BASE_REGISTER_OFFSET + (i * 4),
				   base_regs[i]);
			}
		}
		pci_write_16(bus, devfun, PCI_COMMAND_OFFSET, command);
		if (find_exp_rom_data(vendor_id, device_id,
				     (uint_t *)&cip->ci_membase[nmem],
			             (uint_t *)&cip->ci_memlength[nmem]) == 0)
				nmem++;
		for (i = 0; i < MAX_BASE_REGISTERS; i++){
			if (base_regs[i] == 0)
				continue;
			if (base_regs[i] & 0x01){ /* I/O base addr */
				base_regs[i] &= 0xFFFFFFFC; /* 0 low 2 bits*/
				cip->ci_ioport_base[nio] = base_regs[i];
				reg_lengths[i] ^= 0xFFFFFFFF;
				reg_lengths[i] |= 1;
				reg_lengths[i]++;
				reg_lengths[i] &= 0xFFFFFFFF;
				cip->ci_ioport_length[nio] = reg_lengths[i];
				nio++;
			}
			else switch(base_regs[i] & 0x07){/* memory base addr */
			   case 0x04: /*64 bit address space */
				cmn_err(CE_NOTE,
				"!PCI board 0x%x,0x%x: no support for 64 bit memory space.\n",
				vendor_id, device_id);
				break;
			   case 0x06: /*reserved*/
				cmn_err(CE_NOTE,
				"!PCI board 0x%x,0x%x: Unexepcted memory map requste.\n",
				vendor_id, device_id);
				break;
			   default: /*presumed already mapped */
				base_regs[i] &= 0xFFFFFFF0; /* 0 low 4 bits */
				cip->ci_membase[nmem] = base_regs[i];
				reg_lengths[i] ^= 0xFFFFFFFF;
				reg_lengths[i] |= 0xF;
				reg_lengths[i]++;
				cip->ci_memlength[nmem] = reg_lengths[i];
				nmem++;
				}
			}

		cip->ci_numioports = (uchar_t) nio;
		cip->ci_nummemwindows = (uchar_t) nmem;
	}	
	return 0;
}
int
find_exp_rom_data(ushort_t vid, ushort_t did, uint_t *base, uint_t *len)
{
	int i;

	for (i = 0; i < pci_num_signatures; i++){
		if (pci_rom_signature_buf[i].vendor_id == vid &&
		    pci_rom_signature_buf[i].device_id == did &&
		    pci_rom_signature_buf[i].used == 0){
			*base = pci_rom_signature_buf[i].addr;
			*len = pci_rom_signature_buf[i].length;
			pci_rom_signature_buf[i].used = 1;
			return 0;
		}
	}
	return -1; /* did not match or none free */
}
int
pci_write_devconfig(uchar_t bus, uchar_t devfun, uchar_t *buffer, int offset, int length){
	int i, ret;
	pl_t oldprio;

	if (pci_verify() != 0) /* can't set up BIOS access */
		return PCI_FAILURE;

	if (offset + length > 256) {
		cmn_err(CE_WARN,
		       "!Tried to write past the end of PCI config space.\n");	
		return -1;
	}

	oldprio = LOCK(pci_lock, plbase);
	for (i = offset; i < offset + length; i++) {
		if ((ret = pci_write_8(bus, devfun, i, *buffer))
			!= PCI_SUCCESS){
				UNLOCK(pci_lock, oldprio);
				return ret;
			}
			buffer++;
	}
	UNLOCK(pci_lock, oldprio);

	return length;
}
