/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/autoconf/ca/ca.c	1.19"
#ident	"$Header: $"

/*
 * Autoconfig -- Configuration Access (CA)
 *
 * The Configuration Access (CA) provide the interfaces to access 
 * the bus dependent device specific information.
 */

#include <io/conf.h>
#include <proc/cred.h>
#include <svc/eisa.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>

#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/autoconf/ca/ca.h>
#include <io/autoconf/ca/eisa/nvm.h>


#if defined(DEBUG) || defined(DEBUG_TOOLS)
STATIC int ca_debug = 0;
#define DEBUG1(a)	if (ca_debug == 1) printf a
#define DEBUG2(a)	if (ca_debug == 2) printf a
#else
#define DEBUG1(a)
#define DEBUG2(a)
#endif /* DEBUG || defined(DEBUG_TOOLS) */

#define	CAHIER		1
#define	CAPL		plbase
#define	CA_OPEN		1

/*
 * Configuration Manager interfaces
 */
extern int cm_register_devconfig(struct config_info *);

extern int ca_mca_init(void);
extern int ca_eisa_init(void);
extern int ca_pci_init(void);

struct config_info_list *config_info_list_head;
struct config_info_list *config_info_list_tail;

int ca_flags;
int ca_devflag = D_MP;
lock_t *ca_mutex;
STATIC LKINFO_DECL(ca_mutex_lkinfo, "IO:ACFG:CA mutex lock", 0);


/*
 * int
 * ca_init(void)
 *	Initialize and read configuration space for each device.
 *
 * Calling/Exit State:
 *	Return 0 on success and 1 on failure.
 */
int
ca_init(void)
{
	struct config_info *cip;
	struct config_info_list *cil;
	int	eerr = 0;		/* ca_eisa_init error code */
	int	merr = 0;		/* ca_mca_init error code */
	int	perr = 0;		/* ca_pci_init error code */

	if (!(ca_mutex = LOCK_ALLOC(CAHIER, CAPL,
			&ca_mutex_lkinfo, KM_NOSLEEP))) {
		/*
		 *+ Failed to allocate lock at system init.
		 *+ This is happening at a time where there should
		 *+ be a great deal of free memory on the system.
		 *+ Corrective action:  Check the kernel configuration
		 *+ for excessive static data space allocation or
		 *+ increase the amount of memory on the system.
		 */
		cmn_err(CE_PANIC, "ca_init: LOCK_ALLOC failed");
	}

	if (eisa_verify() != -1) {
		/* We are on an EISA machine. */
		DEBUG1(("EISA bus detected.\n"));
		eerr = ca_eisa_init();
	}

	if (mca_verify() != -1) {
		/* We are on a MCA machine. */
		DEBUG1(("MCA bus detected.\n"));
		merr = ca_mca_init();
	}

	if (pci_verify() != -1) {
		/* We are on a PCI machine. */
		DEBUG1(("PCI bus detected."));
		perr = ca_pci_init();
	}

	/*
	 * Now register the config_info with the configuration manager.
	 */
	for (cil = config_info_list_head; cil; cil = cil->ci_next) {
		if (cil->ci_ptr)
			cm_register_devconfig(cil->ci_ptr);
	}

	if (eerr || merr || perr)
		return (1);
	else
		return (0);
}


/*
 * size_t 
 * ca_devconfig_size(ulong_t bustype, ulong_t devconf)
 *	Get the size of device configuration space information. 
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
size_t
ca_devconfig_size(ulong_t bustype, ulong_t devconf)
{
	struct bus_access *bap;

	bap = (struct bus_access *)&devconf;

	switch (bustype) {
	case CM_BUS_EISA:
		return (ca_eisa_devconfig_size(bap->ba_eisa_slotnumber));

	case CM_BUS_MCA:
		return (ca_mca_devconfig_size(bap->ba_mca_slotnumber));

	case CM_BUS_PCI:
		return (pci_devconfig_size(bap->ba_pci_busnumber,
			bap->ba_pci_devfuncnumber));

	case CM_BUS_PNPISA:
	default:
		break;
	}; /* end case */

	return (0);
}


/*
 * int
 * ca_read_devconfig(ulong_t bustype, ulong_t devconf, void *buf, off_t off, size_t len)
 *	Get device configuration space information. 
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
ca_read_devconfig(ulong_t bustype, ulong_t devconf, 
			void *buf, off_t off, size_t len)
{
	struct bus_access *bap;

	bap = (struct bus_access *)&devconf;

	switch (bustype) {
	case CM_BUS_EISA:
		return (ca_eisa_read_devconfig(bap->ba_eisa_slotnumber,
			bap->ba_eisa_functionnumber, buf, off, len));

	case CM_BUS_MCA:
		return (mca_read(bap->ba_mca_slotnumber, buf, off, len));

	case CM_BUS_PCI:
		return (pci_read_devconfig(bap->ba_pci_busnumber,
			bap->ba_pci_devfuncnumber, buf, off, len));

	case CM_BUS_PNPISA:
	default:
		break;
	}; /* end case */

	return (-1);
}


/*
 * int
 * ca_write_devconfig(ulong_t devconf, ulong_t devconf, void *buf, off_t off, size_t len)
 *	Put device configuration space information. 
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
ca_write_devconfig(ulong_t bustype, ulong_t devconf, 
			void *buf, off_t off, size_t len)
{
	struct bus_access *bap;

	bap = (struct bus_access *)&devconf;

	switch (bustype) {
	case CM_BUS_EISA:
		return (ca_eisa_write_devconfig(bap->ba_eisa_slotnumber, 
			bap->ba_eisa_functionnumber, buf, off, len));

	case CM_BUS_MCA:
		return (mca_write(bap->ba_mca_slotnumber, buf, off, len));

	case CM_BUS_PCI:
		return (pci_write_devconfig(bap->ba_pci_busnumber,
			bap->ba_pci_devfuncnumber, buf, off, len));

	case CM_BUS_PNPISA:
	default:
		break;
	}; /* end case */

	return (-1);
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * ca_print_config_info(struct config_info *cip)
 *	Print a formatted <config_info> data structure. 
 *
 * Calling/Exit State:
 *	None.
 */
void
ca_print_config_info(struct config_info *cip)
{
	int	i;

	switch (cip->ci_busid) {
	case CM_BUS_EISA:
		debug_printf("\tBUS TYPE: ci_busid=%d, " 
			"ci_eisa_slotnumber=%d, ci_eisa_funcnumber=%d\n",
			cip->ci_busid, cip->ci_eisa_slotnumber, 
			cip->ci_eisa_funcnumber);
		switch (cip->ci_eisa_reserved) {
		case EISA_NVM_EMB_SLOT:
			debug_printf("\t%s slot\n", "EMBEDDED"); 
			break;
		case EISA_NVM_EXP_SLOT:
			debug_printf("\t%s slot\n", "EXPANSION"); 
			break;
		case EISA_NVM_VIR_SLOT:
			debug_printf("\t%s slot\n", "VIRTUAL"); 
			break;
		default:
			debug_printf("\t%s slot\n", "UNKNOWN"); 
			break;
		};
		break;
	case CM_BUS_MCA:
		debug_printf("\tBUS TYPE: ci_busid=%d, " 
			"ci_mcabrdid=%d, ci_mca_slotnumber=%d\n",
			cip->ci_busid, cip->ci_mcabrdid, 
			cip->ci_mca_slotnumber);
		break;
	case CM_BUS_PCI:
		debug_printf("\tBUS TYPE: ci_busid=%d, ci_pcivendorid=%d, " 
			"ci_pcidevid=%d, ci_pci_devfuncnumber\n",
			cip->ci_busid, cip->ci_pcivendorid, 
			cip->ci_pcidevid, cip->ci_pci_devfuncnumber);
		break;
	default:
		break;
	};

	debug_printf("\tTYPE: ci_type=%s\n", cip->ci_type);

	debug_printf("\tMEM: ci_nummemwindwos=%d\n", cip->ci_nummemwindows);
	for (i = 0; i < (int)cip->ci_nummemwindows; i++) {
		debug_printf("\tci_membase[%d]=0x%x, \tci_memlength[%d]=0x%x\n",
			i, cip->ci_membase[i], i, cip->ci_memlength[i]);
		debug_printf("\tci_memattr[%d]=0x%x\n",
			i, cip->ci_memattr[i]);
	}

	debug_printf("\tDMA: ci_numdmas=%d\n", cip->ci_numdmas);
	for (i = 0; i < (int)cip->ci_numdmas; i++) {
		struct config_dma_info *cdmaip;

		cdmaip = (struct config_dma_info *)&cip->ci_dmaattrib[i];

		debug_printf("\tci_dmachan[%d]=0x%x\n",
			i, cip->ci_dmachan[i]);
		debug_printf("\tci_dmaattrib[%d]=0x%x\n",
			i, cip->ci_dmaattrib[i]);
		debug_printf("\t\ttransfersize=0x%x\n",
			cdmaip->cdmai_transfersize);
		debug_printf("\t\ttype=0x%x\n",
			cdmaip->cdmai_type);
		debug_printf("\t\ttiming1=0x%x\n",
			cdmaip->cdmai_timing1);
	}

	debug_printf("\tPORT: ci_numioports=%d\n", cip->ci_numioports);
	for (i = 0; i < (int)cip->ci_numioports; i++) {
		debug_printf("\tci_ioport_base[%d]=0x%x, ci_ioport_length[%d]= 0x%x\n",
			i, cip->ci_ioport_base[i], i, cip->ci_ioport_length[i]);
	}

	debug_printf("\tIRQ: ci_numirqs=%d\n", cip->ci_numirqs);
	for (i = 0; i < (int)cip->ci_numirqs; i++) {
		struct config_irq_info	*cirqip;

		cirqip = (struct config_irq_info *)&cip->ci_irqattrib[i];

		debug_printf("\tci_irqline[%d]=0x%x\n",
			i, cip->ci_irqline[i]);
		debug_printf("\tci_irqattrib[%d]=0x%x\n",
			i, cip->ci_irqattrib[i]);
		debug_printf("\t\ttype=%s\n",
			cirqip->cirqi_type ? "SHARED" : "NON-SHARABLE");
		debug_printf("\t\ttrigger=%s\n",
			cirqip->cirqi_trigger ? "LEVEL" : "EDGE");
		debug_printf("\t\tlevel=%s\n",
			cirqip->cirqi_level ? "HIGH" : "LOW");
	}

	debug_printf("\n\n");
}


/*
 * void
 * ca_print_config_info_list(void)
 *	Print all the <config_info> members in <config_info_list> list.
 *
 * Calling/Exit State:
 *	None.
 */
void
ca_print_config_info_list(void)
{
	int	i;
	struct config_info_list *cilp;

	for (cilp = config_info_list_head; cilp->ci_ptr; cilp = cilp->ci_next) {
		ca_print_config_info(cilp->ci_ptr);
		if (cilp->ci_next == NULL)
			break;
	}
}

#endif /* DEBUG || DEBUG_TOOLS */


/*
 * int
 * ca_open(dev_t *devp, int flag, int otyp, cred_t *credp)
 *	Open CA driver.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
ca_open(dev_t *devp, int flag, int otyp, cred_t *credp)
{
	int	ret = 0;
	pl_t	opl;

	opl = LOCK(ca_mutex, CAPL);

	if (!(ca_flags & CA_OPEN)) {
		ca_flags |= CA_OPEN;
	} else {
		ret = EBUSY;
	}

	UNLOCK(ca_mutex, opl);

	return (ret);
}


/*
 * int
 * ca_close(dev_t dev, int flag, int otyp, cred_t *credp)
 *	Close CA driver.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
ca_close(dev_t dev, int flag, int otyp, cred_t *credp)
{
	pl_t	opl;

	opl = LOCK(ca_mutex, CAPL);

	if (ca_flags & CA_OPEN) {
		ca_flags &= ~CA_OPEN;
	}

	UNLOCK(ca_mutex, opl);

	return (0);
}


/*
 * int
 * ca_ioctl(dev_t dev, int cmd, int arg, int mode,
 *				cred_t *credp, int *rvalp)
 *	CA ioctl function.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
ca_ioctl(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *credp, int *rvalp)
{
	int	rv = 0;

	switch (cmd) {
	case CA_EISA_READ_NVM: {
		char	*buffer;
		int	nfuncs;
		int	error = 0;
		struct cadata nvm;
		extern int ca_eisa_read_nvm(int, uchar_t *, int *);

		if (copyin(arg, (caddr_t) &nvm, sizeof(struct cadata)) == -1) {
			rv = EFAULT;
			break;
		}
	   
		/* Allocate buffer to store the EISA non-volatile memory data.*/
		if ((buffer = (char *) kmem_zalloc(
				EISA_BUFFER_SIZE, KM_NOSLEEP)) == 0) {
			/*
			 *+ Not enough memory.
			 */
			cmn_err(CE_WARN, 
				"!ca: kmem_zalloc failed, could not allocate"
				" memory for EISA buffer.");
			return (ENOMEM);
		}

		nfuncs = ca_eisa_read_nvm(
			(int)nvm.ca_busaccess, (uchar_t *)buffer, &error);

		if (nfuncs == 0)
			nvm.ca_size = 0;
		else
			nvm.ca_size = EISA_NVM_SLOTINFO_SIZE + 
					(nfuncs * EISA_NVM_FUNCINFO_SIZE);

		if (copyout(buffer, nvm.ca_buffer, nvm.ca_size) == -1)
			rv = EFAULT;
		else if (copyout((caddr_t)&nvm, arg, sizeof(struct cadata)) == -1)
			rv = EFAULT;

		kmem_free(buffer, EISA_BUFFER_SIZE);

		break;
	}
	case CA_MCA_READ_POS:
	case CA_PCI_READ_CFG:
	default:
		break;

	}; /* end switch */

	return rv;
}
