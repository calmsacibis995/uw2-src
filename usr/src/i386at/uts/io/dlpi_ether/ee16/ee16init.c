/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*			Copyright (c) 1991  Intel Corporation		*/
/*				All Rights Reserved			*/

/*			INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

static char prog_copyright[] = "Copyright 1991 Intel Corp. 468802-010";

#ident	"@(#)kern-i386at:io/dlpi_ether/ee16/ee16init.c	1.12"
#ident	"$Header: $"

/* hardware dependent code for ee16 */

#ifdef	_KERNEL_HEADERS

/* autoconfig start */
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/ca/eisa/nvm.h>
#include <io/autoconf/ca/ca.h>
/* autoconfig end */

#include <io/dlpi_ether/ee16/dlpi_ee16.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <io/dlpi_ether/ee16/ee16.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <net/dlpi.h>
#ifdef ESMP
#include <net/inet/if.h>
#else
#include <net/tcpip/if.h>
#endif
#include <svc/errno.h>
#ifndef lint
#ifdef ESMP
#include <net/inet/byteorder.h>
#else
#include <net/tcpip/byteorder.h>
#endif
#endif
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <util/inline.h>
#include <io/ddi_i386at.h>
#include <util/mod/moddefs.h>
#include <io/conf.h>
#include <io/ddi.h>

#elif defined(_KERNEL)

#include <sys/dlpi_ee16.h>
#include <sys/dlpi_ether.h>
#include <sys/ee16.h>
#include <sys/immu.h>
#include <sys/kmem.h>
#include <sys/dlpi.h>
#include <net/if.h>
#ifndef lint
#include <sys/byteorder.h>
#endif
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/debug.h>
#include <sys/inline.h>
#include <sys/ddi_i386at.h>
#include <sys/moddefs.h>
#include <sys/conf.h>
#include <sys/ddi.h>

#endif /* _KERNEL_HEADERS */

/* autoconfig start */
DL_bdconfig_t		*ee16config;
DL_sap_t		*ee16saps;
int			*ee16cable_type;
int			ee16boards;
static void		**cookie;

extern	int		ee16nboards;
extern	int		ee16nsaps;
extern	int		ee16cmajor;
extern	void		ee16intr();
/* autoconfig end */

#ifdef ESMP
#define EE16_HIER	2	/* lock */
STATIC LKINFO_DECL( ee16_lockinfo, "ID:ee16:ee16_lock", 0);
#endif

extern	struct  ifstats *ifstats;
extern	char            *ee16_ifname;
extern	int		ee16strlog;

/* Check for validity of interrupts */
DL_bdconfig_t *ee16intr_to_index[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int	ee16timer_id;
int	ee16devflag = 0;
char	ee16id_string[] = "ee16 (Intel EtherExpress 16)";

/*
 * board type:  old ee16, ee16TP = 0
 *		2nd generation ee16 and ee16TP, ee16Combo = 1
 *		ee16/MCA = 2
 */
int		ee16board_type = 0;
int		ee16slot_num[8];
ulong_t		ee16bus_p;

STATIC int	mem_ofst;
STATIC void     ee16tx_done(), ee16init_rx(), ee16init_tx();
STATIC void	ee16uninit();
STATIC int      ee16chk();
static void     raise_clock(), lower_clock(), eeprom_cleanup();
static void	shift_bits_out();
static ushort_t shift_bits_in(), write_eeprom(), read_eeprom();
static int	init_isa(), init_mca();

int		ee16init();
int		ee16start();
int		ee16ack_586();
int		ee16ia_setup_586();
int		ee16config_586();
int		ee16wait_scb();
int		ee16init_586();

extern void	ee16watch_dog();

/*
 * Header, wrapper, and function declarations and definitions for loadability
 */
#define DRVNAME	"ee16 - Loadable ee16 ethernet driver"

STATIC	int	ee16_load(), ee16_unload();

MOD_DRV_WRAPPER(ee16, ee16_load, ee16_unload, NULL, DRVNAME);

/*
 * Wrapper functions.
 */

STATIC int
ee16_load(void)
{
	int ret_code;

	cmn_err(CE_NOTE, "!MOD: in ee16_load()");

	if ((ret_code = ee16init()) != 0) {
		ee16uninit();
		return (ret_code);
	}
	ee16start();
	return(0);
}

STATIC int
ee16_unload(void)
{
	cmn_err(CE_NOTE, "!MOD: in ee16_unload()");

	ee16uninit();
	return(0);
}

/*************************************************************************
 * ee16init ()
 * Number of boards            	Configuration of boards
 *     	1              	from system configuration parameters
 *   	>1              assume pre-configuration of boards
 */

int
ee16init ()
{   
	DL_bdconfig_t   *bd;
	DL_sap_t        *sap;
	bdd_t		*bdd;
	ushort		base_io;
	ushort		cksum;
	ushort		x;
	int		i,j;	

	/* autoconfig start */
        cm_num_t		slot_num;	/* MCA slot number */
	cm_args_t		cm_args;	/* args to cm_getval() */
	struct  cm_addr_rng	range;		/* to pass address pairs */
	int			ret;		/* return code from resmgr */
	char			cable_str[64];
	/* autoconfig end */

	/* get bus type */
	if ((drv_gethardware ( IOBUS_TYPE, &ee16bus_p )) < 0 ) {
		cmn_err ( CE_WARN, "ee16: Can't Identify BUS!");
		return ( -1 );
	}

	/* autoconfig start */
	/*
	 *  Get number of boards from Resource Manager.
	 */
	DLboards = cm_getnbrd( DL_NAME );

	/*
	 *  If there are more than ee16_cmajors (from Space.c) cards of
	 *  this type in the resource manager, truncate the number to
	 *  the maximum allowed in the kernel (via the Master file).
	 */
	if (DLboards > ee16nboards) DLboards=ee16nboards;

	ASSERT(DLboards >= 0);

	if (DLboards == 0) {
		cmn_err (CE_WARN, "No %s board in DCU", DL_NAME);
		return(ENOENT);
	}

	/*
	 *  Allocate config structures based on number of boards found.
	 */
	DLsaps = (DL_sap_t *) kmem_zalloc 
		  ((sizeof(DL_sap_t) * DLboards * ee16nsaps), KM_NOSLEEP);
	DLconfig = (DL_bdconfig_t *) kmem_zalloc 
		  ((sizeof(DL_bdconfig_t) * DLboards), KM_NOSLEEP);
	ee16cable_type = (int *) kmem_zalloc 
		  ((sizeof(int) * DLboards), KM_NOSLEEP);
	cookie = (void **) kmem_zalloc 
		  ((sizeof(void **) * DLboards), KM_NOSLEEP);

	/*
	 *  For each board, get configuration parameters.
	 */
	for (i = 0; i < DLboards; i++) {
		/*
		 *  Get major number and max_saps.  Major number is the
		 *  major of board #0 + the instance number of the current
		 *  board# [range is 0 -> (ee16_cmajors - 1)].
		 */
		DLconfig[i].major = ee16cmajor + i;
		DLconfig[i].max_saps = ee16nsaps;

		/*
		 *  Get key for this board
		 */
		cm_args.cm_key = cm_getbrdkey(DL_NAME, i);
		cm_args.cm_n = 0;

		if (ee16bus_p == BUS_MCA) {
			/*
			 * Get MCA slot number
			 */
			cm_args.cm_param = CM_SLOT;
                	cm_args.cm_val = &(slot_num);
                	cm_args.cm_vallen = sizeof(cm_num_t);
                	ret = cm_getval(&cm_args);
                	if (ret) {
				cmn_err (CE_WARN,
			"%s MCA board %d does not have slot number in DCU",
					DL_NAME, i);
				return (ENOENT);
			}
			ee16slot_num[i] = slot_num;

			if (init_mca(bd, i))	return(-1);

			if (cm_AT_putconf(cm_args.cm_key,
                		bd->irq_level,
                		CM_ITYPE_EDGE,
                		bd->io_start, bd->io_end, 0, 0, -1,
                		CM_SET_IRQ|CM_SET_ITYPE|CM_SET_IOADDR, 0)) {
                		cmn_err(CE_WARN, "ee16 cm_AT_putconf failed");
				return(ENXIO);
        		}
		}
		else {
			/*
			 *  Get interrupt vector
			 */
			cm_args.cm_param = CM_IRQ;
			cm_args.cm_val = &(DLconfig[i].irq_level);
			cm_args.cm_vallen = sizeof(int);
			if (cm_getval(&cm_args)) {
				DLconfig[i].flags = 0;
				cmn_err (CE_WARN,
				"%s board %d has no irq in DCU", DL_NAME, i);
				return (ENOENT);
			}

			if (DLconfig[i].irq_level == 0) {
				DLconfig[i].flags = 0;
				cmn_err (CE_WARN,
				"%s board %d has irq 0 in DCU", DL_NAME, i);
				return (ENOENT);
			}

			/*
			 *  Get I/O Addresses
			 */
			cm_args.cm_param = CM_IOADDR;
			cm_args.cm_val = &range;
			cm_args.cm_vallen = sizeof(struct cm_addr_rng);
			ret = cm_getval(&cm_args);
			if (ret) {
				/*
				 *  ENOENT is permissible.
				 */
				if (ret == ENOENT) {
					bd->io_start	= 0;
					bd->io_end	= 0;
				}
				else {
					cmn_err (CE_WARN,
				"%s board %d has no I/O address in DCU",
						DL_NAME, i);
					return (ENOENT);
				}
			} else {
				DLconfig[i].io_start = range.startaddr;
				DLconfig[i].io_end = range.endaddr;
			}

			/*
			 *  Get cable type vector, for ISA only.
			 */
			cm_args.cm_param = "CABLE_TYPE";
			cm_args.cm_val = cable_str;
			cm_args.cm_vallen = sizeof(cable_str);
			if (cm_getval(&cm_args)) {
				/*
				 * This is just a tmp workaround. ODISTR1 is
				 * put in by pkg.nics to indicate cable_type.
				 */
				cm_args.cm_param = "ODISTR1";
				cm_args.cm_val = cable_str;
				cm_args.cm_vallen = sizeof(cable_str);
				if (cm_getval(&cm_args)) {
					cmn_err (CE_WARN,
			"%s board %d does not have cable_type in DCU",
								DL_NAME, i);
					return(EINVAL);
				}
			}

			switch(cable_str[0]) {
			case '0':
				ee16cable_type[i] = BNC;
				break;
			case '1':
				ee16cable_type[i] = AUI;
				break;
			case '2':
				ee16cable_type[i] = TP;
				break;
			default:
				cmn_err (CE_WARN,
				"ee16 %d has unknown cable_type %s in DCU",
								i, cable_str);
				return(EINVAL);
			}
		}
	}
	/* autoconfig end */

	/* Initialize the configured boards */
	for (i=0, bd=ee16config; i < ee16boards; bd++, i++) {

		if (ee16bus_p != BUS_MCA)
			if (init_isa(bd, i))	return(-1);

		base_io = bd->io_start;

		/* make sure we turn off memory mapping */
		outw( base_io+SMB_PTR, 0);
		outb( base_io+MEMDEC,  0x00 );
		outb( base_io+MEMECTRL,0xf0 );		/* segment E */
		outb( base_io+MEMCTRL, 0xf2 );		/* segment F */
		outb( base_io+MEMPC,   0xff );		/* segment C, D */

		/* Initialize DLconfig */
		bd->flags	  = 0;
		bd->bd_number     = i;
		bd->sap_ptr       = &ee16saps[ i * bd->max_saps ];
		bd->tx_next       = 0;
		bd->timer_val     = -1;
		bd->promisc_cnt   = 0;
		bd->multicast_cnt = 0;
		bzero((caddr_t)&bd->mib, sizeof(DL_mib_t));

		ee16intr_to_index[bd->irq_level] = bd;

		bd->bd_dependent1 =(caddr_t)kmem_alloc(sizeof(bdd_t), KM_SLEEP);
		if ((bdd = (bdd_t *)bd->bd_dependent1) == (bdd_t *)NULL) {
			cmn_err( CE_WARN, "can't alloc board structure" );
			return(-1);
		}

		bdd->ring_buff = (ring_t *)NULL;
		bdd->next_sap = (DL_sap_t *)NULL;

		for (j = 0; j < MULTI_ADDR_CNT; j++)
			bdd->ee16_multiaddr[j].status = 0;

		bd->bd_dependent2 = 0;
		bd->bd_dependent3 = 0;
		bd->bd_dependent4 = 0;
		bd->bd_dependent5 = 0;

		if (!(bd->bd_lock = LOCK_ALLOC( EE16_HIER, plstr,
			&ee16_lockinfo, KM_NOSLEEP))) {
			/*
			 *+ can't get lock, stop configuring the driver
			 */
			cmn_err(CE_WARN, "ee16init no memory for ee16_lock");
			return(ENOLCK);
		}

		/* Initialize SAP structure info */
		for (sap = bd->sap_ptr, j = 0; j < bd->max_saps; j++, sap++) {
			sap->state          = DL_UNBOUND;
			sap->sap_addr       = 0;
			sap->read_q         = NULL;
			sap->write_q        = NULL;
			sap->flags          = 0;
			sap->max_spdu       = USER_MAX_SIZE;
			sap->min_spdu       = USER_MIN_SIZE;
			sap->mac_type       = DL_ETHER;
			sap->service_mode   = DL_CLDLS;
			sap->provider_style = DL_STYLE1;;
			sap->bd		    = bd;

			if ((sap->sap_sv = SV_ALLOC(KM_NOSLEEP)) == NULL) {
				/*
				 *+ can't alloc sap_sv
				 */
				cmn_err(CE_WARN,
					"ee16_init no memory for sap_sv");
				return(ENOLCK);
                        }
		}

		/* Allocate internet stats structure */
		if ((bd->ifstats = kmem_zalloc(sizeof(struct ifstats),
			KM_SLEEP)) == NULL) {
			cmn_err(CE_WARN, "ee16init no memory fo ifstats");
			return(ENOMEM);
		}

		/* Initalize internet stat statucture */
		bd->ifstats->ifs_name   = ee16_ifname;
		bd->ifstats->ifs_unit   = (short)i;
		bd->ifstats->ifs_mtu    = USER_MAX_SIZE;
		bd->ifstats->ifs_active = 1;
		bd->ifstats->ifs_ipackets = 0;
		bd->ifstats->ifs_opackets = 0;
		bd->ifstats->ifs_ierrors  = 0;
		bd->ifstats->ifs_oerrors  = 0;
		bd->ifstats->ifs_collisions = 0;
		ifstats_attach(bd->ifstats);

		if ( ee16init_586 (bd)) {
			cmn_err(CE_WARN, "ee16init: 82586 initi failed");
			bd->flags = BOARD_DISABLED;
			return -1;
		}
		else {
			bd->mib.ifOperStatus = DL_UP;	/* SNMP */
			bd->mib.ifAdminStatus = DL_UP;	/* SNMP */
		}

		cmn_err (CE_CONT,
				"EtherExpress16 board %d is configured as", i);

		cmn_err (CE_CONT,
				" network device: ee16_%d\nnetwork media: ", i);

		switch (ee16cable_type[i]) {
		case BNC:
			cmn_err (CE_CONT, "BNC,");
			break;
		case AUI:
			cmn_err (CE_CONT, "AUI,");
			break;
		case TP:
			cmn_err (CE_CONT, "TP,");
			break;
		}

		cmn_err (CE_CONT, " irq:%d, io addr:0x%d%b, ",
		bd->irq_level, (ushort)(base_io) >> 8, (base_io & 0x00ff));

		ee16print_eaddr(bd->eaddr.bytes);
	}

	return (0);
}

int
ee16start()
{
	rm_key_t	key;		/* key in resource manager */
	int i;


	/* timeout every 5 seconds */
	ee16timer_id = itimeout (ee16watch_dog, 0, EE16_TIMEOUT, plstr);

	/* autoconfig start */
	for (i = 0; i < DLboards; i++) {
		key = cm_getbrdkey(DL_NAME, i);
		(void)cm_intr_attach(key, ee16intr, &DLdevflag, &(cookie[i]));
		ee16config[i].flags = BOARD_PRESENT;
	}
	/* autoconfig end */
}

static int
init_isa( bd, board )
DL_bdconfig_t   *bd;
int		board;
{
	ushort		base_io;
	ushort		ext_io;
	ushort		cksum;
	ushort		irq;
	ushort		x, z;
	uchar_t		y;
	int		j;


	base_io = bd->io_start;

	/* reset ASIC on board */
	outb(base_io + EE_CTRL, RESET_586);
	drv_usecwait(300);
	outb(base_io + EE_CTRL, RESET_586 | GA_RESET);
	drv_usecwait(300);
	outb(base_io + EE_CTRL, RESET_586);
	drv_usecwait(300);

	if (!ee16chk (base_io)) {
		cmn_err(CE_WARN,
		"ee16: board with io address %x not found", base_io);
		return 6;
	}
 
	/* ethernet cable type: 0=>BNC 2=>TP 1=>AUI */
	ext_io = (EE16_ECR | base_io );

	if (ee16board_type != EE16OLD) {
		switch (ee16cable_type[board]) {
		case BNC:
			y = inb(ext_io) & ~0x20;
			outb( ext_io, y | 0xc0);
			break;
		case AUI:
			y = inb(ext_io) & ~0x80;
			outb( ext_io, y | 0x40);
			break;
		case TP:
			y = inb(ext_io) | 0x02;
			outb( ext_io, y | 0xc0);
			break;
		}
	}

	if (ee16bus_p == BUS_ISA) {
		/* enable the test, and set to 16-bit mode */
		y = inb( base_io + CONFIG ) | 0x08;

		/* test IOCHRDY bit to see if C&T chip set presents */
		outb( base_io + CONFIG, y | 0x20 );

		/* perform an IO */
		outw( base_io + RDPTR, 0 );
    		z = inw(base_io + DXREG);
		outw(base_io + DXREG, z);

		/* check IO result */
		y = inb( base_io + CONFIG );
		if ( y & 0x40 )	{ (y & 0x10) ? (y &= ~0x10) : (y |= 0x10); };
		outb( base_io + CONFIG, y & ~0x20 );	/* turn off test bit */
	}

	/* This is needed to avoid a race condition in HW */
	x = inb(base_io + AUTOID);

	/* warm up dram */
	outw(base_io + RDPTR, 0);
	for (j=0; j<16; j++)
    		x = inb(base_io + DXREG);

	/* read the ethernet address off eeprom */
	for (j=0; j<3; j++) {
		x = read_eeprom (base_io, INTEL_EADDR_L+j);

		/* change the byte ordering */
		bd->eaddr.words[2-j] = htons(x);
	}

	/* release the RESET_586 bit */
	outb(base_io + EE_CTRL, 0); 
	return (0);
}


/*************************************************************************
 *  ee16chk()
 *
 *  Checks the validity of board signature
 */

STATIC int
ee16chk (io)
ushort_t io;
{
	ushort_t	j;
	int 		i;

	/* get it into a known state: lower nibble = 0 
	 * In the worst case this should happen in 15 attempts */

	for (i=0; i<20; i++) {
		j = inb(io + AUTOID);
		if (!(j & 0x0f))	break;
	}

	if (i == 20)			return 0;
	if (j & 0xf0 != 0xa0)		return 0;
	if (inb (io+AUTOID) != 0xb1)	return 0;
	if (inb (io+AUTOID) != 0xa2)	return 0;
	if (inb (io+AUTOID) != 0xb3)	return 0;

	/* done with auto id, try to see is shadow auto id supported */

	io += SHADOW_ID_OFFSET;

	for (i=0; i<20; i++) {
		j = inb(io + AUTOID);
		if (!(j & 0x0f)) break;
	}

	if (i == 20)  {
		ee16board_type = 0;
		return 1; /* old card, shadow auto id not supported */
	}

	/*
	 * ee16, ee16TP = 0,
	 * 2nd generation ee16 and ee16TP, ee16COMBO = 1,
	 * ee16/MCA = 2
	 */
	switch (j & 0xf0) {
	case 0xa0:
		ee16board_type = 0;
		break;
	case 0xb0:
		ee16board_type = 1;
		break;
	case 0xc0:
		ee16board_type = 2;
		break;
	}
	return (1);
}

/*************************************************************************/
find_mem_size(base_io)
ushort	base_io;
{
	ushort i;

	i = inb(base_io + AUTOID);
	outw(base_io + WRPTR, 0);
	outb(base_io + DXREG, 0);
	i = inb(base_io + AUTOID);
	outw(base_io + WRPTR, 0x8000);	/* 32K */
	outb(base_io + DXREG, 0);

	i = inb(base_io + AUTOID);
	outw(base_io + WRPTR, 0);
	outb(base_io + DXREG, 0xaa);
	i = inb(base_io + AUTOID);
	outw(base_io + RDPTR, 0x8000);	/* 32K */
	if (inb(base_io + DXREG) == 0xaa) 
		mem_ofst = 0x8000;
	else
		mem_ofst = 0;		/* 16K ? 64K ? */
}
/*************************************************************************
 * ee16init_586 ()
 *
 * initialize the 82586's scp, iscp, and scb; reset the 82586;
 * do IA setup command to add the ethernet address to the 82586 IA table;
 * and enable the Receive Unit.
 */

int
ee16init_586 (bd)
DL_bdconfig_t *bd;
{
	/* High addresses of Buffer RAM structure:
	 *      CB : for general (non xmit) commands
	 *      SCB
	 * 	ISCP
	 * 	SCP
	 *	--- End of MAX_RAM_SIZE (64K)
	 */

	bdd_t	*bdd      = (bdd_t *) bd->bd_dependent1;
	ushort	ofst_scp  = MAX_RAM_SIZE - sizeof (scp_t);	
	ushort	ofst_iscp = ofst_scp - sizeof (iscp_t);	
	ushort	ofst_scb  = ofst_iscp - sizeof (scb_t);
	ushort	base_io   = bd->io_start;
	ushort	i, rb;
	iscp_t	iscp;
	scb_t	scb;
	uchar_t	reg, irq;


	/* disable  interrupt */
	outb (base_io+SEL_IRQ, 0);  

	/* ISA: set to 16-bit mode. EISA: set to 8-bit */
	if (ee16bus_p != BUS_MCA) {
		reg = inb(base_io + CONFIG);
		if (ee16bus_p == BUS_ISA)
			outb (base_io, reg | 0x08);
		else
			outb (base_io, reg | 0x08);
			/* outb (base_io, reg & ~0x08); */
	}

	/* fill in scp : 16 bit accesses */
	write_byte(ofst_scp + 0, 0, base_io);		/* sysbus */
	write_word(ofst_scp + 6, ofst_iscp, base_io);	/* iscp */	
	outb(base_io + DXREG, 0); 		/* WRPTR autoincremented */

	/* fill in iscp */
	iscp.iscp_busy 		= 1;
	iscp.iscp_scb_ofst 	= ofst_scb;
	iscp.iscp_scb_base 	= 0;
	bcopy_to_buffer((char *)&iscp, ofst_iscp, sizeof(iscp_t), base_io);

	/* fill in scb */
	scb.scb_status 		= 0;
	scb.scb_cmd 		= 0;
	scb.scb_cbl_ofst 	= 0xffff;
	scb.scb_rfa_ofst 	= 0xffff;
	scb.scb_crc_err		= 0;
	scb.scb_aln_err		= 0;
	scb.scb_rsc_err		= 0;
	scb.scb_ovrn_err	= 0;
	bcopy_to_buffer((char *)&scb, ofst_scb, sizeof(scb_t), base_io);

	bdd->ofst_scb = ofst_scb;
	bdd->gen_cmd = ofst_scb - sizeof (cmd_t);	/* CB offset */

	ee16init_tx (bdd, base_io);
	ee16init_rx (bdd, base_io);
	
	/* start the 82586, by resetting the 586 & issuing a CA */
	outb (base_io + EE_CTRL, RESET_586);	/* reset the 82586 */
	outb (base_io + EE_CTRL, 0);		/* release from reset */

	outb (base_io + CA_CTRL, 1);		/* channel attention */

	/* wait for iscp busy to be cleared */
	for (i=1000; i; i--) {					
		read_byte(ofst_iscp + 0, base_io, rb);	/* iscp_busy */
		if (!(rb & 1))		break;
		drv_usecwait (10);
	}
	if (i == 0) {			/* if bit isn't cleared */
		cmn_err(CE_WARN, "ISCP busy not cleared\n");
		return (1);		/* return error */
	} 

	for (i=1000; i; i--) {		/* wait for scb status */
#if defined(lint) || defined(C_PIO)
                read_word(ofst_scb + 0, base_io, rb);
#else
                rb = read_word(ofst_scb+0, base_io);
#endif
		if (rb == (SCB_INT_CX | SCB_INT_CNA))
			break;
		drv_usecwait (10);
	}
	if (i == 0)			/* if CX & CNA aren't set */
		return (2);		/* return error */

	if (ee16ack_586 (ofst_scb, base_io))
		return (2);

	/* configure 586 with default parameters */
	if (ee16config_586 (bd, PRO_OFF))
		return (2);

	/* do IA Setup command and enable 586 Receive Unit */
	if (ee16ia_setup_586 (bd, bd->eaddr.bytes))
		return (2);

	/* enable 586 Receive Unit */
	write_word(ofst_scb + 0, 0, base_io);	/* status */
	outw(base_io + DXREG, SCB_RUC_STRT);	/* command: autoincrement */
	write_word(ofst_scb + 6, bdd->begin_fd, base_io);  /* rfa */

	outb (base_io + CA_CTRL, 1);

	if (ee16wait_scb (ofst_scb, 1000, base_io))
		return (2);

	if (ee16ack_586 (ofst_scb, base_io))
		return (2);

	/* enable interrupt */
	switch (irq = (uchar_t) bd->irq_level) {
	case 3: case 4: case 5: 
		irq--;
		break;

	case 9: case 2:
		irq = 1;
		break;

	case 10: case 11: 
		irq -= 5;
		break;

	case 12:
		irq = 0;
		break;
	case 15:
		irq = 7;
		break;
	}
	outb (base_io+SEL_IRQ, irq | 0x08);  
	return (0);
}

/*************************************************************************/
STATIC void
ee16init_tx (bdd, base_io)
bdd_t    *bdd;
ushort_t base_io;
{
	ushort	ofst_txb = 0;
	ushort	ofst_tbd = 0;
	ushort	ofst_cmd = 0;
	ring_t	*ring;
	ushort	i, j;
	tbd_t	tbd;
	cmd_t	cmd;

	/* the number of tbds and cmds are set to be the same since
	 * each cmd usually requires one tbd */
	bdd->n_tbd = bdd->n_cmd = 6;

	find_mem_size(base_io);			/* find mem_ofst */
	ofst_cmd = bdd->ofst_cmd = mem_ofst;
	ofst_tbd = bdd->ofst_tbd = ofst_cmd + (bdd->n_cmd * sizeof(cmd_t));
	ofst_txb = bdd->ofst_txb = ofst_tbd + (bdd->n_tbd * sizeof(tbd_t));

	/* allocate ring buffer as an array of ring_t */

	if ( ! bdd->ring_buff )
		bdd->ring_buff = 
		(ring_t *) kmem_alloc (bdd->n_cmd*sizeof(ring_t), KM_NOSLEEP);

	if ( (ring = bdd->ring_buff) == (ring_t *)NULL )
		cmn_err(CE_PANIC, "ee16_init: no memory for ring_buff");

	/* initialize cmd, tbd, and ring structures */
	for (i=0; i<bdd->n_cmd; i++, ring++) {

		/* initialize cmd */
		cmd.cmd_status		= 0;
		cmd.cmd_cmd		= CS_EL;
		cmd.cmd_nxt_ofst	= 0xffff;
		cmd.prmtr.prm_xmit.xmt_tbd_ofst	= ofst_tbd;
		bcopy_to_buffer((char *)&cmd, ofst_cmd, sizeof(cmd_t), base_io);

		/* initialize tbd */
		tbd.tbd_count		= 0;
		tbd.tbd_nxt_ofst	= 0xffff;
		tbd.tbd_buff		= ofst_txb;
		tbd.tbd_buff_base	= 0;
		bcopy_to_buffer((char *)&tbd, ofst_tbd, sizeof(tbd_t), base_io);
		
		ring->ofst_cmd = ofst_cmd;
		ring->next = ring + 1;

		ofst_cmd += sizeof(cmd_t);
		ofst_tbd += sizeof(tbd_t);
		ofst_txb += TBD_BUF_SIZ;
	}

	/* complete ring buffer by making the last next points to the first */
	(--ring)->next = bdd->ring_buff;
	bdd->head_cmd = 0;
	bdd->tail_cmd = 0;
}

/*************************************************************************
 * ee16init_rx (bdd, base_io)
 *   retain ee16 buffer configuration for receive side
 */

STATIC void
ee16init_rx (bdd, base_io)
bdd_t     *bdd;
ushort_t  base_io;
{
	ushort	ofst_rxb = 0;
	ushort	ofst_rbd = 0;
	ushort	ofst_fd  = 0;
	int	i, mem_left;
	fd_t	fd;
	rbd_t	rbd;

	/* receive buffer starts at the end of transmit data area */
	ofst_rxb = bdd->ofst_rxb = bdd->ofst_txb + (bdd->n_cmd * TBD_BUF_SIZ);
	mem_left = bdd->gen_cmd - bdd->ofst_rxb; 

	/* calculate number of rbd and fd */
	bdd->n_rbd = bdd->n_fd =
			mem_left/(sizeof(fd_t)+sizeof(rbd_t)+RBD_BUF_SIZ); 

	/* rbd's follow rxb's and fd's follow rbd's */
	ofst_rbd = bdd->ofst_rbd = ofst_rxb + (bdd->n_rbd * RBD_BUF_SIZ);
	ofst_fd  = bdd->ofst_fd  = ofst_rbd + (bdd->n_rbd * sizeof(rbd_t));

	bdd->begin_fd	= ofst_fd;
	bdd->begin_rbd	= ofst_rbd;

	/* initialize fds */
	for (i=0; i < (int)(bdd->n_fd - 1); i++) {
		fd.fd_status	= 0;
		fd.fd_cmd	= 0;
		fd.fd_nxt_ofst	= ofst_fd + sizeof(fd_t);
		fd.fd_rbd_ofst	= 0xffff;
		bcopy_to_buffer((char *)&fd, ofst_fd, sizeof(fd_t), base_io);
		ofst_fd += sizeof(fd_t);
	}
	bdd->end_fd = ofst_fd;

	/* initialize last fd */
	fd.fd_status	= 0;
	fd.fd_cmd	= CS_EL;
	fd.fd_nxt_ofst	= bdd->begin_fd;
	fd.fd_rbd_ofst	= 0xffff;
	bcopy_to_buffer((char *)&fd, ofst_fd, sizeof(fd_t), base_io);

	/* init first fd's rbd */
	write_word(bdd->begin_fd+6, bdd->begin_rbd, base_io);	

	/* initialize all rbds */
	for (i=0; i < (int)(bdd->n_rbd - 1); i++) {
		rbd.rbd_status		= 0;
		rbd.rbd_nxt_ofst	= ofst_rbd + sizeof(rbd_t);
		rbd.rbd_buff		= ofst_rxb;
		rbd.rbd_buff_base	= 0;
		rbd.rbd_size		= RBD_BUF_SIZ;
		bcopy_to_buffer((char *)&rbd, ofst_rbd, sizeof(rbd_t), base_io);
		ofst_rbd += sizeof(rbd_t);
		ofst_rxb += RBD_BUF_SIZ;
	}

	bdd->end_rbd = ofst_rbd;

	/* initialize last rbd */
	rbd.rbd_status		= 0;
	rbd.rbd_nxt_ofst	= bdd->begin_rbd;
	rbd.rbd_buff		= ofst_rxb;
	rbd.rbd_buff_base	= 0;
	rbd.rbd_size		= CS_EL | RBD_BUF_SIZ;
	bcopy_to_buffer((char *)&rbd, ofst_rbd, sizeof(rbd_t), base_io);
}

/*************************************************************************/
int
ee16config_586 (bd, prm_flag)
DL_bdconfig_t *bd;
ushort_t prm_flag;
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	ushort	base_io = bd->io_start;

	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io))
		return (1);

	write_word(bdd->ofst_scb + 0, 0, base_io);	 /* status */
	outw(base_io + DXREG, SCB_CUC_STRT);		/* cmd : auto-incr */
	outw(base_io + DXREG, bdd->gen_cmd);		/* cbl : auto-incr */

	write_word(bdd->gen_cmd+0, 0, base_io);	  /* status */
	outw(base_io + DXREG, CS_CMD_CONF | CS_EL);	/* cmd  : auto-incr */
	outw(base_io + DXREG, 0xffff);			/* link : auto-incr */

	/* default config page 2-28 586 book */
	/* fifo byte */
	write_word(bdd->gen_cmd+6, 0x080c, base_io);
	outw(base_io + DXREG, 0x2600);		      /* add mode : auto-incr */
	outw(base_io + DXREG, 0x6000);		      /* pri data : auto-incr */
	outw(base_io + DXREG, 0xf200);		      /* slot     : auto-incr */
	outw(base_io + DXREG, 0x0008 | prm_flag);     /* hrdwr    : auto-incr */
	outw(base_io + DXREG, 0x0040);		      /* min len  : auto-incr */

	outb (base_io + CA_CTRL, 1);

	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io))
		return (1);

	if (ee16ack_586 (bdd->ofst_scb, base_io))
		return (1);

	return (0);
}

/*************************************************************************/
int
ee16ia_setup_586 (bd, eaddr)
DL_bdconfig_t *bd;
uchar_t eaddr[];
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	ushort	base_io = bd->io_start;
	ushort		i;
	
	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io))
		return (1);

	write_word(bdd->ofst_scb + 0, 0, base_io); 	 /* status */
	outw(base_io + DXREG, SCB_CUC_STRT);		/* cmd : auto-incr */
	outw(base_io + DXREG, bdd->gen_cmd);		/* cbl : auto-incr */
	outw(base_io + DXREG, bdd->ofst_fd);		/* rfa : auto-incr */
	
	write_word(bdd->gen_cmd + 0, 0, base_io);	  /* status */
	outw(base_io + DXREG, CS_CMD_IASET | CS_EL);	/* cmd  : auto-incr */
	outw(base_io + DXREG, 0xffff);			/* link : auto-incr */

	/* add sizes of xmit_t and conf_t to the current offset */
	write_byte(bdd->gen_cmd + 6, eaddr[0], base_io);
	for (i=1; i<6; i++)
	    outb(base_io + DXREG, eaddr[i]);

	outb (base_io + CA_CTRL, 1);

	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io))
		return (1);

	if (ee16ack_586 (bdd->ofst_scb, base_io))
		return (1);

	return (0);
}

/*************************************************************************
 * ee16wait_scb (scb, how_long, base_io)
 *
 * Acceptance of a Control Command is indicated by the 82586 clearing
 * the SCB command field page 2-16 of the intel microcom handbook.
 */
int
ee16wait_scb (scb, n, base_io)
ushort_t 	scb; 	/* scb offset in the buffer */
int		n;
ushort_t 	base_io;
{
	int  i = 0;
	ushort	rb;

	n <<= 10;
	while (i++ != n) {
#if defined(lint) || defined(C_PIO)
                read_word(scb+2, base_io, rb);          /* cmd */
#else
                rb = read_word(scb+2, base_io);         /* cmd */
#endif
		if (rb == 0)	return (0);
	}
	cmn_err(CE_CONT, "rb = %x\n", rb);
	return (1);
}

/*************************************************************************/
int
ee16ack_586 (scb, base_io)
ushort_t scb;	/* ofst of scb */
ushort   base_io;
{
	ushort_t i;
	
#if defined(lint) || defined(C_PIO)
        read_word(scb, base_io, i);             /* status */
#else
        i = read_word(scb, base_io);            /* status */
#endif
	if (i &= SCB_INT_MSK) {
		write_word(scb + 2, i, base_io);	/* cmd */
		outb (base_io + CA_CTRL, 1);		/* channel attention */
		return (ee16wait_scb (scb, 10000, base_io));
	}
	return (0);
}

/*************************************************************************
 * read_eeprom(): read data from reg in eeprom
 *    Refer to the assembly routines provided with EE16
 *    for enlightenment!
 */

static ushort
read_eeprom(base_io, reg)
ushort base_io;
ushort reg;
{
	short x;
	ushort data;

	/* mask off 586 access */
	x = inb(base_io + EE_CTRL);
	x |= RESET_586;
	x &= ~GA_RESET;
	outb(base_io + EE_CTRL, x);

	/* select EEPROM, mask off ASIC and reset bits, set EECS */
	x = inb(base_io + EE_CTRL);
	x &= ~(GA_RESET | EEDI | EEDO | EESK);
	x |= EECS; 
	outb(base_io + EE_CTRL,x);

	/* write the read opcode and register number in that order 
 	 * The opcode is 3bits in length; reg is 6 bits long */
	shift_bits_out(EEPROM_READ_OPCODE, 3, base_io);
	shift_bits_out(reg, 6, base_io);
	data = shift_bits_in(base_io);

	eeprom_cleanup(base_io);
	return data;
}

/*************************************************************************
 * write_eeprom(): write data to reg in eeprom
 */

static ushort_t
write_eeprom(base_io, reg, data)
ushort base_io;
ushort reg;
ushort data;
{
	ushort_t x,i;

	/* mask off 586 access */
	x = inb(base_io + EE_CTRL);
	x |= RESET_586;
	x &= ~GA_RESET;
	outb(base_io + EE_CTRL, x);

	/* select EEPROM, mask off ASIC and reset bits, set EECS */
	x = inb(base_io + EE_CTRL);
	x &= ~(GA_RESET | EEDI | EEDO | EESK);
	x |= EECS;
	outb(base_io + EE_CTRL, x);

	/* write the erase/write enable opcode */
	shift_bits_out(EEPROM_EWEN_OPCODE, 5, base_io);
	shift_bits_out(EEPROM_EWEN_OPCODE, 4, base_io);	/* 4 dont cares */

	/* lower chip select for 1 usecond */
	x = inb(base_io + EE_CTRL) & ~(GA_RESET | EESK | EECS);
	outb(base_io + EE_CTRL, x);
	drv_usecwait(10);
	outb(base_io + EE_CTRL, x | EECS);

	shift_bits_out(EEPROM_ERASE_OPCODE, 3, base_io);
	shift_bits_out(reg, 6, base_io);    /* send eeprom location */

	/* lower chip select for 1 usecond */
	x = inb(base_io + EE_CTRL) & ~(GA_RESET | EESK | EECS);
	outb(base_io + EE_CTRL, x);
	drv_usecwait(10);
	outb(base_io + EE_CTRL, x | EECS);

	/* wait for > 10 ms for eedo to go high: command done */
	drv_usecwait(20000); /* wait for 20 ms */
	if (!(inb(base_io + EE_CTRL) | EEDO)) {
		cmn_err(CE_WARN,"write to eeprom failed\n");
		return 0;
	}
	/* lower chip select for 1 usecond */
	x = inb(base_io + EE_CTRL) & ~(GA_RESET | EESK | EECS);
	outb(base_io + EE_CTRL, x);
	drv_usecwait(10);
	outb(base_io + EE_CTRL, x | EECS);

	shift_bits_out(EEPROM_WRITE_OPCODE, 3, base_io);
	shift_bits_out(reg, 6, base_io);    /* send eeprom location */
	shift_bits_out(data, 16, base_io);	/* write data */

	/* lower chip select for 1 usecond */
	x = inb(base_io + EE_CTRL) & ~(GA_RESET | EESK | EECS);
	outb(base_io + EE_CTRL, x);
	drv_usecwait(10);
	outb(base_io + EE_CTRL, x | EECS);

	/* wait for >= 10 ms for eedo to go high : command done */
	drv_usecwait(20000); /* wait for 20 ms */
	if (!(inb(base_io + EE_CTRL) | EEDO)) {
		cmn_err(CE_WARN,"write to eeprom failed\n");
		return 0;
	}
	/* lower chip select for 1 usecond */
	x = inb(base_io + EE_CTRL) & ~(GA_RESET | EESK | EECS);
	outb(base_io + EE_CTRL, x);
	drv_usecwait(10);
	outb(base_io + EE_CTRL, x | EECS);

	shift_bits_out(EEPROM_EWDS_OPCODE, 5, base_io);
	shift_bits_out(EEPROM_EWDS_OPCODE, 4, base_io);	/* dont cares */
	eeprom_cleanup(base_io);
	return 1;
}

/*************************************************************************
 * shift count bits of data to eeprom 
 */
static void
shift_bits_out(data, count, base_io)
ushort data;
ushort count;
ushort base_io;
{
	ushort_t x, mask;

	mask = 0x01 << (count - 1);
	x = inb(base_io + EE_CTRL) & ~(GA_RESET | EEDO | EEDI); 
	do {
	  	x &= ~EEDI;
	  	if (data & mask)	x |= EEDI;

	  	outb(base_io + EE_CTRL, x);
 	  	drv_usecwait(10);
	  	raise_clock(base_io, &x);
  		lower_clock(base_io, &x); 
	  	mask = mask >> 1;
	} while (mask);

	x &= ~EEDI;
	outb(base_io + EE_CTRL, x);
}

/*************************************************************************
 * raise eeprom clock 
 */
static void
raise_clock(base_io, x)
ushort base_io;
ushort *x;
{
	*x = *x | EESK;
	outb(base_io + EE_CTRL, *x);
 	drv_usecwait(10);
}

/*************************************************************************
 * lower eeprom clock 
 */
static void
lower_clock(base_io, x)
ushort base_io;
ushort *x;
{
	*x = *x & ~EESK;
	outb(base_io + EE_CTRL, *x);
 	drv_usecwait(10);
}

/*************************************************************************
 * shift count bits of data in from eeprom 
 */
static ushort_t
shift_bits_in(base_io)
ushort base_io;
{
	ushort x,d,i;

	x = inb(base_io + EE_CTRL) & ~(GA_RESET | EEDO | EEDI); 
	d = 0;
	for (i=0; i<16; i++) {
		d = d << 1;
		raise_clock(base_io, &x); 
		x = inb(base_io + EE_CTRL) & ~(GA_RESET | EEDI); 
		if (x & EEDO)	d |= 1;
		lower_clock(base_io, &x);
	}
	return d;
}

/*************************************************************************/
static void
eeprom_cleanup(base_io)
ushort base_io;
{
	ushort x;
	x = inb(base_io + EE_CTRL) & ~GA_RESET;
	x &= ~(EECS | EEDI);
	outb(base_io + EE_CTRL, x);
	raise_clock(base_io, &x);
	lower_clock(base_io, &x);
}

STATIC void
ee16uninit()
{
	DL_bdconfig_t	*bd;
	bdd_t		*bdd;
	DL_sap_t	*sap;
	int		i, j;
	int		opri;

	untimeout (ee16timer_id);

	if (!cookie || !DLconfig)	return;

	for (i=0; i < ee16boards; i++) {

		bd = &ee16config[i];

		if (bd->bd_lock)
			opri = DLPI_LOCK (bd->bd_lock, plstr);

		if (bd->bd_dependent1) 
			kmem_free ((int *)bd->bd_dependent1, sizeof(bdd_t));

		cm_intr_detach(cookie[i]);

		if (sap = bd->sap_ptr) {
			for (j = 0; j < bd->max_saps; j++, sap++) {
				if (sap->sap_sv) {
					SV_DEALLOC (sap->sap_sv);
				}
			}
		}

		if (DLsaps)
			kmem_free (DLsaps, sizeof(DL_sap_t)*DLboards*ee16nsaps);

		if (ee16cable_type)
			 kmem_free (ee16cable_type, (sizeof(int)*DLboards));

		if (bd->ifstats) {
			(void)ifstats_detach(bd->ifstats);
			kmem_free (bd->ifstats, sizeof(struct ifstats));
		}

		if (bd->bd_lock) {
			DLPI_UNLOCK (bd->bd_lock, opri);
			LOCK_DEALLOC (bd->bd_lock);
		}
	}
	kmem_free (cookie, (sizeof(void **) * ee16boards));
	kmem_free (ee16config, (sizeof(DL_bdconfig_t) * ee16boards));
}

static int mca_io[16] = {
			0x270, 0x260, 0x250, 0x240, 0x230, 0x220, 0x210, 0x200,
			0x370, 0x360, 0x350, 0x340, 0x330, 0x320, 0x310, 0x300};

static int mca_irq[8] = {12, 9, 3, 4, 5, 10, 11, 15};

static int
init_mca( bd, board )
DL_bdconfig_t   *bd;
int		board;
{
	uchar_t 	slot;
	uchar_t		pos_reg0;
	uchar_t		pos_reg1;
	ushort		x;
	int		j;


	slot = ee16slot_num[board];

	/* Select adapter by slot */
	outb (ADAP_ENAB, (slot | 0x08));

	if (((inb(POS_MSB)) == 0x62) && ((inb(POS_LSB)) == 0x8B)) {

    		/* adapter found, get the POS info and enable the card */
		if (((pos_reg0 = inb(POS_0)) & 0x01) == 0)
			pos_reg0 |= 0x01;

		/* disable flash/boot rom */
		outb (POS_0, (pos_reg0 | 0x08));

		pos_reg1 = inb(POS_1);

		/* get cable type */
		if (pos_reg0 & 0x02) {
			cmn_err(CE_CONT,
			"Please use ADF disk to select a specific cable ");
			cmn_err(CE_CONT,
			"type for your\nMCA EtherExpress16 card. auto-detect ");
			cmn_err(CE_CONT,
			"mode is not supported.\n");
			outb (ADAP_ENAB,0);	/* de_select adapter */
			return (-1);
		}
		else if (pos_reg0 & 0x04) {
			if (pos_reg1 & 0x80)
				ee16cable_type[board] = TP;
			else
				ee16cable_type[board] = BNC;
		}
		else {
			ee16cable_type[board] = AUI;
		}

		/* figure out I/O address */
		bd->io_start = mca_io[pos_reg1 & 0x0f];
		bd->io_end = bd->io_start + 0x0f;

		/* figure out irq */
		bd->irq_level = mca_irq[(pos_reg1 & 0x70) >> 4];

		outb (POS_2, 0x00);
		outb (POS_3, 0x03);

		outb (ADAP_ENAB,0); 	/* de_select adapter */

		/* do not release the RESET_586 while accessing eeprom */
		outb(bd->io_start + EE_CTRL, RESET_586);

		/* read the ethernet address off eeprom */
		for (j=0; j<3; j++) {
			x = read_eeprom (bd->io_start, INTEL_EADDR_L+j);

			/* change the byte ordering */
			bd->eaddr.words[2-j] = htons(x);
		}

		/* This is needed to avoid a race condition in HW */
		x = inb(bd->io_start + AUTOID);

		/* warm up dram */
		outw(bd->io_start + RDPTR, 0);
		for (j=0; j<16; j++)
    			x = inb(bd->io_start + DXREG);

		outb(bd->io_start + EE_CTRL, 0);
		return (0);
	}
	outb (ADAP_ENAB,0);	/* de_select adapter */
	return (-1);		/* No adapter found */
}
