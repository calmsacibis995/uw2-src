/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/autoconf/confmgr/confmgr_p.c	1.54"
#ident	"$Header: $"

/*
 * Autoconfig -- CM/CA Interface 
 *
 * Architecture-specific section of Configuration Manager.
 */

#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/ca/ca.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <svc/eisa.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/moddrv.h>
#include <util/param.h>
#include <util/types.h>

/* DO WE NEED/WANT ddi.h ?? */

/* For debugging only */
#undef STATIC
#define STATIC

#if defined(DEBUG) || defined(DEBUG_TOOLS)
int cm_debug = 0;
#define DEBUG1(a)	if (cm_debug == 1) printf a
#define DEBUG2(a)	if (cm_debug == 2) printf a
#else
#define DEBUG1(a)
#define DEBUG2(a)
#endif /* DEBUG  || DEBUG_TOOLS */

/*
 * MACRO
 * SET_RM_ARGS(struct rm_args *rma, rm_key_t key, 
 *				void *val, int vlen, int n)
 *	Set add data fields of <rma>.
 *
 * Calling/Exit State:
 *	None.
 */
#define	SET_RM_ARGS(rma, param, key, val, vlen, n) { \
	(void)strcpy((rma)->rm_param, (param)); \
	(rma)->rm_key = (key); \
	(rma)->rm_val = (val); \
	(rma)->rm_vallen = (vlen); \
	(rma)->rm_n = (n); \
} 

#define	CM_NUM_SIZE		(sizeof(cm_num_t))
#define	CM_ADDR_RNG_SIZE	(sizeof(struct cm_addr_rng))

#define EISA_SLOT(x)	((x) & 0x0FF)
#define MCA_SLOT(x)		((x) & 0x0FF)

/*
 * CM_MAXHEXDIGITS is max length of 64-bit ulong_t converted to a
 * HEX ASCII string with 0x prepended.
 */

#define CM_MAXHEXDIGITS		19
#define CM_HEXBASE		16

struct cm_key_list
{
	rm_key_t		key;
	char			brdid[CM_MAXHEXDIGITS];
	struct cm_key_list	*knext;
};

struct cm_cip_list
{
	struct config_info	*cip;
	char			brdid[CM_MAXHEXDIGITS];
	int			order;
	struct cm_cip_list	*cnext;
};

STATIC struct cm_cip_list	*_cm_new_cips = NULL;
STATIC struct cm_key_list	*_cm_inuse_keys = NULL;
STATIC struct cm_key_list	*_cm_purge_keys = NULL;
STATIC struct cm_key_list	*_cm_boot_hba = NULL;

extern int		cm_find_match(struct rm_args *, void *);
extern void		_cm_save_cip(struct config_info * );
extern void		_cm_match_em(struct cm_key_list **, struct cm_cip_list **);
extern void		_cm_add_vals(rm_key_t, struct config_info *, boolean_t);
extern void		_cm_add_entry(struct cm_cip_list *, rm_key_t);
extern void		_cm_update_key(struct cm_cip_list *, struct cm_key_list *);
extern void		_cm_sync_up(void);
extern void		_cm_itoh(ulong_t, char[], int);
extern boolean_t	_cm_chk_key(rm_key_t);
extern void		_cm_save_key(rm_key_t, struct cm_key_list **, boolean_t);
extern void		_cm_clean_klist(void);
extern int		_cm_strncmp(const char *, const char *, int);

char	*cm_bootarg[10];
uint_t	cm_bootarg_count;
uint_t	cm_bootarg_max = sizeof(cm_bootarg) / sizeof(cm_bootarg[0]);


/*
 * STATIC void
 * _cm_bootarg_parse(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Parse boot arguments and add/delete/replace the parameters.
 *	Currently, we only care about arguments that have the
 *	following form:
 *
 *		<modname>:[<instance>:]<param>=<value>
 */
STATIC void
_cm_bootarg_parse(void)
{
	int	i;			/* index */
	int	rv;			/* return value */
	char	*bas;			/* boot arg string */
	char	*baid;			/* boot arg id (driver modname) */
	char	*baparam;		/* boot arg parameter */
	char	*baval;			/* boot arg parameter value */
	char	val[RM_MAXPARAMLEN];	/* max storage for parameter value */
	char	*s;			/* temporary ptr to the boot string */
	struct rm_args rma;		/* packet to pass to resmgr */
	rm_key_t key;			/* modname, instance key */
	int	inst;			/* board instance */

	for (i = 0; i < cm_bootarg_count; i++) {

		inst = 0;
		baid = cm_bootarg[i];
		s = strpbrk(baid, ":=");
		if (s == NULL || *s != ':')
			continue;
		*s++ = '\0';
		bas = s;
		s = strpbrk(bas, ":=");
		if (s == NULL)
			continue;
		if (*s == ':') {
			*s++ = '\0';
			inst = stoi(&bas);
			bas = s;
			s = strpbrk(bas, ":=");
			if (s == NULL || *s != '=')
				continue;
		}
		baparam = bas;
		*s++ = '\0';
		baval = s;
	
		DEBUG1(("baparam=%s, baid=%s, inst=%d, baval=%s\n",
				baparam, baid, inst, baval));

		if ((key = cm_getbrdkey(baid, inst)) != RM_NULL_KEY) {
			SET_RM_ARGS(&rma, baparam, key, val, RM_MAXPARAMLEN, 0);
			switch ((rv = rm_getval(&rma, UIO_SYSSPACE))) { 
			case 0:
				if (strcmp(baval, val) == 0)
					continue;
				/* Replace (delete and add) existing value. */
				(void) rm_delval(&rma);
				/* FALLTHROUGH */
			case ENOENT:
				rma.rm_vallen = strlen(baval) + 1;
				rma.rm_val = baval;
				(void) rm_addval(&rma, UIO_SYSSPACE);
				break;
			default:
				ASSERT(rv != ENOSPC);
				break;
			};
		}
	}
}

/*
 * int
 * cm_init_p(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
cm_init_p(void)
{
	struct rm_args		rma;
	cm_num_t		dcu_mode = CM_DCU_NOCHG;
	cm_num_t		bustype;
	time_t			rm_time = 1;
	int			error;

	error = ca_init();

	rma.rm_key = RM_KEY;

	if (_cm_new_cips != NULL) {
		/* We found new entries, but haven't added them yet */
		dcu_mode = CM_DCU_SILENT;
	} else {
		/* Check to see if we've changed the resmgr database */

		(void)strcpy(rma.rm_param, RM_TIMESTAMP);
		rma.rm_val = &rm_time;
		rma.rm_vallen = sizeof(rm_time);
		rma.rm_n = 0;

		/*
		 * This should NEVER fail, but just in case it ever does,
		 * we'll assume the resmgr database has changed.
		 */
		(void)rm_getval(&rma, UIO_SYSSPACE);

		if (rm_time != 0)
			dcu_mode = CM_DCU_SILENT;
	}

	(void)strcpy(rma.rm_param, CM_DCU_MODE);
	rma.rm_val = &dcu_mode;
	rma.rm_vallen = sizeof(dcu_mode);
	(void)rm_addval(&rma, UIO_SYSSPACE);

	/*
	 * If there are NO errors, then walk through resmgr looking for
	 * obsolete entries.
	 */

	if (error == 0) {

		(void)strcpy(rma.rm_param, CM_BRDBUSTYPE);
		rma.rm_vallen = sizeof(bustype);
		rma.rm_val = &bustype;
		rma.rm_n = 0;

		rma.rm_key = RM_NULL_KEY;

		while (rm_nextkey(&rma) == 0) {

			/* If we have key in list, then DON'T purge entry */

			if (_cm_chk_key(rma.rm_key) == B_TRUE)
				continue;

			/* Can't purge entry if there's no BRDBUSTYPE */

			if (rm_getval(&rma, UIO_SYSSPACE) != 0)
				continue;

			switch (bustype) {
			/* We can only purge those with NVRAM */
			case CM_BUS_EISA:
			case CM_BUS_MCA:
			case CM_BUS_PCI:
				_cm_save_key(rma.rm_key, &_cm_purge_keys, B_TRUE);
				break;
			default:
				break;
			}
		}
	}

	_cm_sync_up();

	_cm_clean_klist();

	_cm_bootarg_parse();
}

/*
 * rm_key_t
 * cm_findkey(struct config_info *cip)
 *	Find an existing entry in the resmgr in-core database with a matching 
 *	<board id, bus type, bus access> tuple.
 *
 * Calling/Exit State: None
 */

STATIC rm_key_t
cm_findkey(struct config_info *cip)
{
	struct rm_args	id_rma;			/* brdid access rm_args */
	struct rm_args	drma;			/* dev/bus access rm_args */
	struct rm_args	brma;			/* bus type rm_args */
	cm_num_t	ba;			/* bus/device access */
	char		id[CM_MAXHEXDIGITS];	/* board id */
	cm_num_t	btype;			/* type of bus */
	cm_num_t	dba;			/* bus/device access */
	cm_num_t	bustype;		/* type of bus */
	char		bid[CM_MAXHEXDIGITS];	/* board id */
	char		*brdid = bid;

	extern boolean_t ca_eisa_clone_slot( ulong_t, ulong_t );

	bustype = cip->ci_busid;

	switch (bustype) {
	case CM_BUS_EISA:
		brdid = eisa_uncompress((char *)&cip->ci_eisabrdid);
		dba = (cm_num_t)cip->ci_eisaba;
		break;

	case CM_BUS_MCA:
		_cm_itoh(cip->ci_mcabrdid, brdid, 4);
		dba = (cm_num_t)cip->ci_mcaba;
		break;

	case CM_BUS_PCI:
		_cm_itoh(cip->ci_pcibrdid, brdid, 0);
		dba = (cm_num_t)cip->ci_pciba;
		break;

	default:
		return RM_NULL_KEY;
	}

	SET_RM_ARGS(&id_rma, CM_BRDID, RM_NULL_KEY, id, strlen(brdid) + 1, 0); 
	SET_RM_ARGS(&brma, CM_BRDBUSTYPE, RM_NULL_KEY, &btype, CM_NUM_SIZE, 0);
	SET_RM_ARGS(&drma, CM_CA_DEVCONFIG, RM_NULL_KEY, &ba, CM_NUM_SIZE, 0);

	/*
	 * Note: The consecutive cm_find_match begins the search from
	 * the last brdid match.
	 */

	while (cm_find_match(&id_rma, brdid) == 0) {
		/* Check for bustype match */

		brma.rm_key = id_rma.rm_key;

		if (rm_getval(&brma, UIO_SYSSPACE) != 0 || btype != bustype)
			continue;

		/* Check for bus access match */

		drma.rm_key = id_rma.rm_key;

		if (rm_getval(&drma, UIO_SYSSPACE) != 0)
			continue;

		if (ba == dba || (btype == CM_BUS_EISA  &&
		    ca_eisa_clone_slot((ulong_t)ba, (ulong_t)dba) == B_TRUE))
			return id_rma.rm_key;
	}

	return RM_NULL_KEY;
}

/*
 * STATIC int
 * _cm_check_irq(struct rm_args *rma, struct config_info *cip)
 *
 * Calling/Exit State:
 *	Return 0 on success, and 1 on failure.
 */
STATIC int
_cm_check_irq(struct rm_args *rma, struct config_info *cip)
{
	cm_num_t	irq;
	int		ret = 0;

	rma->rm_val = &irq;
	rma->rm_n = 0;

	while (rm_getval( rma, UIO_SYSSPACE) == 0) {
		if (irq != (cm_num_t)cip->ci_irqline[ rma->rm_n++]) {
			ret = 1;
			break;
		}
	}

	if (ret == 1 || rma->rm_n != (uint_t)cip->ci_numirqs) {
		(void)rm_delval(rma);
		return 1;
	}

	return ret;
}

/*
 * STATIC int
 * _cm_check_itype(struct rm_args *rma, cm_num_t itype[], int count)
 *
 * Calling/Exit State:
 *	Return 0 on success, and 1 on failure.
 */
STATIC int
_cm_check_itype(struct rm_args *rma, cm_num_t itype[], int count)
{
	cm_num_t	val;
	int		ret = 0;

	rma->rm_val = &val;
	rma->rm_n = 0;

	while (rm_getval( rma, UIO_SYSSPACE) == 0) {
		if (itype[rma->rm_n] != 0 && val != itype[rma->rm_n]) {
			ret = 1;
			break;
		}
		rma->rm_n++;
	}

	if (ret == 1 || rma->rm_n != count) {
		(void)rm_delval( rma );
		return 1;
	}

	return ret;
}

/*
 * STATIC int
 * _cm_check_dma(struct rm_args *rma, struct config_info *cip)
 *
 * Calling/Exit State:
 *	Return 0 on success, and 1 on failure.
 */
STATIC int
_cm_check_dma(struct rm_args *rma, struct config_info *cip)
{
	cm_num_t	dma;
	int		ret = 0;

	rma->rm_val = &dma;
	rma->rm_n = 0;

	while (rm_getval(rma, UIO_SYSSPACE) == 0) {
		if (dma != (cm_num_t)cip->ci_dmachan[rma->rm_n++]) {
			ret = 1;
			break;
		}
	}

	if (ret == 1 || rma->rm_n != (uint_t)cip->ci_numdmas) {
		(void)rm_delval( rma );
		return 1;
	}

	return ret;
}

/*
 * int
 * _cm_check_ioaddr(struct rm_args *rma, struct config_info *cip)
 *
 * Calling/Exit State:
 *	Return 0 on success, and 1 on failure.
 */
STATIC int
_cm_check_ioaddr(struct rm_args *rma, struct config_info *cip)
{
	struct cm_addr_rng	addr_rng;
	int			ret = 0;
	int			adj;

	rma->rm_val = &addr_rng;
	rma->rm_n = 0;

	while (rm_getval(rma, UIO_SYSSPACE) == 0) {

		adj = 0;

		if (cip->ci_ioport_length[rma->rm_n] != 0)
                        adj = 1;

		if (addr_rng.startaddr !=
				(long)cip->ci_ioport_base[rma->rm_n] ||
		     addr_rng.endaddr != addr_rng.startaddr +
				(long) cip->ci_ioport_length[rma->rm_n] - adj) {
			rma->rm_n++;
			ret = 1;
			break;
		}

		rma->rm_n++;
	}

	if (ret == 1 ||  rma->rm_n != (uint_t)cip->ci_numioports) {
		(void)rm_delval(rma);
		return 1;
	}

	return ret;
}

/*
 * int
 * _cm_check_memaddr(struct rm_args *rma, struct config_info *cip)
 *
 * Calling/Exit State:
 *	Return 0 on success, and 1 on failure.
 */
STATIC int
_cm_check_memaddr(struct rm_args *rma, struct config_info *cip)
{
	struct cm_addr_rng	addr_rng;
	int			ret = 0;

	rma->rm_val = &addr_rng;
	rma->rm_n = 0;

	while (rm_getval(rma, UIO_SYSSPACE) == 0) {
		if (addr_rng.startaddr !=
				(long)cip->ci_membase[rma->rm_n] ||
		     addr_rng.endaddr != addr_rng.startaddr +
				(long) cip->ci_memlength[rma->rm_n] - 1) {
			rma->rm_n++;
			ret = 1;
			break;
		}

		rma->rm_n++;
	}

	if (ret == 1 || rma->rm_n != (uint_t)cip->ci_nummemwindows) {
		(void)rm_delval(rma);
		return 1;
	}

	return ret;
}


/*
 * int
 * cm_register_devconfig(struct config_info *cip)
 *	Register the device configuration information with the resource mgr.
 *
 * Calling/Exit State:
 *	None.
 */
int
cm_register_devconfig(struct config_info *cip)
{
	rm_key_t	key;

	if ((key = cm_findkey( cip )) == RM_NULL_KEY) {
		/*
		 * This is a new entry in NVRAM, save the information
		 * for later consumption.
		 */
		_cm_save_cip( cip );
		return 0;
	}

	DEBUG1(("cm_register_devconfig: key=0x%x\n", key));

	/*
	 * Save the keys as we deal with them.  Then in cm_init_p()
	 * we'll use this list of keys when we walk through the resmgr
	 * database and purge those entries that are no longer needed.
	 */

	_cm_save_key(key, &_cm_inuse_keys, B_FALSE);

	/* Since we can't interpret NVRAM info on MCA systems--we're done */

	if (cip->ci_busid == CM_BUS_MCA)
		return 0;

	_cm_add_vals( key, cip, B_FALSE );
}

STATIC void
_cm_add_vals( rm_key_t key, struct config_info *cip, boolean_t newkey )
{
	struct config_irq_info	*cirqip;
	struct cm_addr_rng	ioportrng;
	struct cm_addr_rng	memrng;
	struct rm_args		rma;
	cm_num_t		itype_val;
	cm_num_t		itype[MAX_IRQS];
	cm_num_t		claim = 0;
	cm_num_t		irq;
	cm_num_t		dma;
	int			i;

	rma.rm_key = key;
	rma.rm_n = 0;

	if (newkey == B_FALSE) {
		(void)strcpy(rma.rm_param, CM_CLAIM);
		rma.rm_vallen = sizeof(claim);
		rma.rm_val = &claim;

		(void)rm_getval(&rma, UIO_SYSSPACE);
	}

	/*
	 * Register IRQ.
	 */ 

	rma.rm_vallen = sizeof(irq);
	(void)strcpy(rma.rm_param, CM_IRQ);

	if (newkey == B_TRUE || (claim & CM_SET_IRQ) == 0  &&
					_cm_check_irq(&rma, cip) != 0) {
		rma.rm_val = &irq;

		for (i = 0; i < (int)cip->ci_numirqs; i++) {
			irq = (cm_num_t)cip->ci_irqline[i];

			if (rm_addval(&rma, UIO_SYSSPACE) != 0) 
				cmn_err(CE_NOTE,
					"Could not add %s param", rma.rm_param);
		}
	}

	/*
	 * Register ITYPE (interrupt type -- edge/level shared/!shared.
	 */

	for (i = 0; i < (int)cip->ci_numirqs; i++) {
		cirqip = (struct config_irq_info *)&cip->ci_irqattrib[i];

		if (cirqip->cirqi_trigger) {
			/*
			 * This controller uses a level-sensitive
			 * interrupt vector, which can be shared with
			 * any controller for any driver.
			 */
			itype[i] = 4;
		} else if (!cirqip->cirqi_trigger && cirqip->cirqi_type) {
			/*
			 * This controller uses an interrupt vector which
			 * can be shared with any controller for any driver.
			 */
			itype[i] = 3;
		} else if (!cirqip->cirqi_trigger && !cirqip->cirqi_type) {
			/*
			 * This controller uses an interrupt vector which
			 * is not sharable, even with another controller
			 * for the same driver.
			 */
			itype[i] = 1;
		} else {
			/*
			 * No interrupt type information available.
			 */
			itype[i] = 0;
		}
	}

	rma.rm_vallen = sizeof(itype_val);
	(void)strcpy(rma.rm_param, CM_ITYPE);

	if (newkey == B_TRUE || (claim & CM_SET_ITYPE) == 0  &&
			_cm_check_itype(&rma, itype, cip->ci_numirqs) != 0) {
		rma.rm_val = &itype_val;

		for (i = 0; i < (int)cip->ci_numirqs; i++) {
			itype_val = itype[i];
			if (itype_val == 0)
				continue;

			if (rm_addval(&rma, UIO_SYSSPACE) != 0) 
				cmn_err(CE_NOTE,
					"Could not add %s param", rma.rm_param);
		}
	}

	/*
	 * Register DMA channel.
	 */

	rma.rm_vallen = sizeof(dma);
	(void)strcpy(rma.rm_param, CM_DMAC);

	if (newkey == B_TRUE || (claim & CM_SET_DMAC) == 0 &&
					_cm_check_dma(&rma, cip) != 0) {
		rma.rm_val = &dma;

		for (i = 0; i < (int)cip->ci_numdmas; i++) {
			dma = (cm_num_t)cip->ci_dmachan[i];

			if (rm_addval(&rma, UIO_SYSSPACE) != 0) 
				cmn_err(CE_NOTE,
					"Could not add %s param", rma.rm_param);
		}
	}

	/*
	 * Register I/O port.
	 */

	rma.rm_vallen = sizeof(ioportrng);
	(void)strcpy(rma.rm_param, CM_IOADDR);

	if (newkey == B_TRUE || (claim & CM_SET_IOADDR) == 0 &&
					_cm_check_ioaddr(&rma, cip) != 0) {
		rma.rm_val = &ioportrng;

		for (i = 0; i < (int)cip->ci_numioports; i++) {
			ioportrng.startaddr = (long) cip->ci_ioport_base[i];
			if (cip->ci_ioport_length[i] == 0)
				ioportrng.endaddr = ioportrng.startaddr;
			else if (cip->ci_ioport_length[i] > 0)
				ioportrng.endaddr = ioportrng.startaddr +
					(long) cip->ci_ioport_length[i] - 1;

			if (rm_addval(&rma, UIO_SYSSPACE) != 0) 
				cmn_err(CE_NOTE,
					"Could not add %s param", rma.rm_param);
		}
	}

	/*
	 * Register Memory Address Range.
	 */

	rma.rm_vallen = sizeof(memrng);
	(void)strcpy(rma.rm_param, CM_MEMADDR);

	if (newkey == B_TRUE || (claim & CM_SET_MEMADDR) == 0 &&
					_cm_check_memaddr(&rma, cip) != 0) {
		rma.rm_val = &memrng;

		for (i = 0; i < (int)cip->ci_nummemwindows; i++) {
			memrng.startaddr = (long) cip->ci_membase[i];
			memrng.endaddr = memrng.startaddr +
					(long) cip->ci_memlength[i] - 1;

			if (rm_addval(&rma, UIO_SYSSPACE) != 0) 
				cmn_err(CE_NOTE,
					"Could not add %s param", rma.rm_param);
		}
	}
}

STATIC boolean_t
_cm_chk_key( rm_key_t key )
{
	struct cm_key_list	*keyp = _cm_inuse_keys;

	while ( keyp != NULL  &&  keyp->key != key )
		keyp = keyp->knext;

	if ( keyp == NULL )
		return B_FALSE;

	return B_TRUE;

	/* return keyp != NULL; ?? */
}

STATIC void
_cm_save_key( rm_key_t key, struct cm_key_list **klist, boolean_t purgelist )
{
	struct cm_key_list	*keyp;
	struct cm_key_list	**addit = klist;
	struct rm_args		rma;
	cm_num_t		boothba;

	/*
	** This needs to deal with an arbitrary number of keys.
	**
	** Using a linked list, one node per key, seems kinda
	** wasteful, but considering they're aren't going to
	** be that many entries in NVRAM, it's the least
	** complicated way, and I don't think the costs are
	** that high in terms of time and space.
	*/

	keyp = (struct cm_key_list *)kmem_alloc( sizeof( *keyp ), KM_SLEEP );

	keyp->key = key;

	if ( purgelist == B_TRUE )
	{
		(void)strcpy( rma.rm_param, CM_BRDID );
		rma.rm_key = key;
		rma.rm_val = keyp->brdid;
		rma.rm_vallen = CM_MAXHEXDIGITS;
		rma.rm_n = 0;

		if ( rm_getval( &rma, UIO_SYSSPACE ) != 0 )
			keyp->brdid[ 0 ] = '\0';

		/* NOT needed if brdid[0] = '\0' */

		while ( *addit != NULL )
		{
			if ( _cm_strncmp( keyp->brdid, (*addit)->brdid, CM_MAXHEXDIGITS ) <= 0 )
				break;

			addit = &(*addit)->knext;
		}

		(void)strcpy( rma.rm_param, CM_BOOTHBA );
		rma.rm_val = &boothba;
		rma.rm_vallen = sizeof( boothba );
		rma.rm_n = 0;
		
		if ( rm_getval( &rma, UIO_SYSSPACE ) == 0 )
		{
			/* Value of param is irrelevant */

			_cm_boot_hba = keyp;
			return;
		}
	}

	keyp->knext = *addit;
	*addit = keyp;
}

STATIC void
_cm_clean_klist( void )
{
	struct cm_key_list	*keyp = _cm_inuse_keys;
	struct cm_key_list	*tmpkeyp;

	while (keyp != NULL) {
		tmpkeyp = keyp->knext;
		kmem_free(keyp, sizeof(*keyp));
		keyp = tmpkeyp;
	}
}

STATIC void
_cm_save_cip( struct config_info *cip )
{
	struct cm_cip_list	**addit = &_cm_new_cips;
	struct cm_cip_list	*newcip;
	char			bid[ CM_MAXHEXDIGITS ];
	char			*brdid = bid;
	static int		order = 1;

	switch (cip->ci_busid) {
	case CM_BUS_EISA:
		brdid = eisa_uncompress((char *)&cip->ci_eisabrdid);
		break;

	case CM_BUS_MCA:
		_cm_itoh(cip->ci_mcabrdid, bid, 4);
		break;

	case CM_BUS_PCI:
		_cm_itoh(cip->ci_pcibrdid, bid, 0);
		break;

	default:
		return;
	}

	/*
	** This needs to deal with an arbitrary number of entries.
	**
	** Using a linked list, one node per key, seems kinda
	** wasteful, but considering they're aren't going to
	** be that many entries in NVRAM, it's the least
	** complicated way, and I don't think the costs are
	** that high in terms of time and space.
	*/

	newcip = (struct cm_cip_list *)kmem_alloc(sizeof(*newcip), KM_SLEEP);

	(void)strncpy(newcip->brdid, brdid, CM_MAXHEXDIGITS);

	/*
	** In addition to keeping the list in sorted order, based on BRDID,
	** I'm going to keep track of the order I received them from CA.
	** Then when I'm done syncing up the list with the obsolete entries
	** from the resource manager, I'll add the new entries in the same
	** order I received them from CA.  This will preserve adding the
	** entries in "slot order."
	*/

	newcip->order = order++;

	/* Now insert in sorted order, based on brdid */

	while (*addit != NULL) {
		if (_cm_strncmp(brdid, (*addit)->brdid, CM_MAXHEXDIGITS) <= 0)
			break;

		addit = &(*addit)->cnext;
	}

	newcip->cip = cip;
	newcip->cnext = *addit;
	*addit = newcip;
}

STATIC int
_cm_strncmp( const char *str1, const char *str2, int max )
{
	while ( max-- > 0  &&  *str1 != '\0'  &&  *str1 == *str2 )
	{
		str1++;
		str2++;
	}

	if ( max < 0 )
		return 0;

	return *str1 - *str2;
}

STATIC void
_cm_add_entry( struct cm_cip_list *clist, rm_key_t key )
{
	struct config_info	*cip = clist->cip;
	struct rm_args		rma;
	cm_num_t		slot = -1;
	cm_num_t		ba;			/* bus/device access */
	cm_num_t		bustype;		/* type of bus */

	bustype = cip->ci_busid;

	switch (bustype) {
	case CM_BUS_EISA:
		ba = (cm_num_t)cip->ci_eisaba;
		slot = EISA_SLOT(ba);
		break;

	case CM_BUS_MCA:
		ba = (cm_num_t)cip->ci_mcaba;
		slot = MCA_SLOT(ba);
		break;

	case CM_BUS_PCI:
		ba = (cm_num_t)cip->ci_pciba;
		break;

	default:
		return;
	}

	/*
	 * Get a new key and register all the system resources.
	 */

	if (key != RM_NULL_KEY)
		rma.rm_key = key;
	else if (rm_newkey(&rma) != 0)
		return;

	/* Register bus type */

	(void)strcpy( rma.rm_param, CM_BRDBUSTYPE );
	rma.rm_vallen = sizeof(bustype);
	rma.rm_val = &bustype;

	if (rm_addval(&rma, UIO_SYSSPACE) != 0) {
		(void)rm_delkey(&rma);
		return;
	}

	/* Register device board id */

	(void)strcpy(rma.rm_param, CM_BRDID);
	rma.rm_vallen = strlen( clist->brdid ) + 1;
	rma.rm_val = clist->brdid;

	if (rm_addval(&rma, UIO_SYSSPACE) != 0) {
		(void)rm_delkey(&rma);
		return;
	}

	/*
	 * Register device specific information necessary to read/write
	 * the configuration space.
	 */

	(void)strcpy(rma.rm_param, CM_CA_DEVCONFIG);
	rma.rm_vallen = sizeof(ba);
	rma.rm_val = &ba;

	if (rm_addval(&rma, UIO_SYSSPACE) != 0) {
		(void)rm_delkey(&rma);
		return;
	}

	/* Register slot information */

	if (slot != -1) {
		(void)strcpy(rma.rm_param, CM_SLOT);
		rma.rm_vallen = sizeof(slot);
		rma.rm_val = &slot;

		if (rm_addval(&rma, UIO_SYSSPACE) != 0) {
			(void)rm_delkey(&rma);
			return;
		}
	}

	/* Since we can't interpret NVRAM info on MCA systems--we're done */

	if (cip->ci_busid == CM_BUS_MCA)
		return;

	_cm_add_vals( rma.rm_key, cip, key == RM_NULL_KEY ? B_TRUE : B_FALSE );
}

STATIC void
_cm_sync_up( void )
{
	struct cm_cip_list	*bestmatch;
	struct cm_cip_list	**clean;
	struct cm_cip_list	*tcips;
	struct cm_key_list	*tkeys;
	struct rm_args		rma;
	cm_range_t		memaddr;
	long			lowestaddr;
	long			tlow;
	int			lcv;
	int			cmp;
	int			i;

	/* cases

	simple: - I move a non-boot board for static driver
		- I move a non-boot board for loadable driver
		- I move my boot hba

		- swap positions of two boards
		- shift boards from 2/3 to 3/4
		- with brds in 2/3, move 2 to 4

		advantages of reusing entries--other params maintained
		disadvantages--other params maintained %^)

		We still have the 1540/1740 verify deliema here.  How
		about if I "mark" these using the CM_TYPE or CM_ENTRYTYPE
		params so dcu can double check my work.
	*/

	/* First deal with the case of the boot hba simply moved */
		/* Don't even put this entry into sorted list */

	if ( _cm_boot_hba != NULL )
	{
		bestmatch = NULL;
		lowestaddr = LONG_MAX;

		tcips = _cm_new_cips;

		while ( tcips != NULL )
		{
			cmp = _cm_strncmp( tcips->brdid, _cm_boot_hba->brdid,
								CM_MAXHEXDIGITS );

			if ( cmp == 0 )
			{

				/*
				** Since we can't interpret NVRAM info on
				** MCA systems--we're going to take the
				** first entry with matching brdid.
				*/

				if ( tcips->cip->ci_busid == CM_BUS_MCA )
				{
					bestmatch = tcips;
					break;
				}

				/* MUST deal with multiple memaddrs */

				lcv = tcips->cip->ci_nummemwindows;
				tlow = LONG_MAX;

				for ( i = 0; i < lcv; i++ )
				{
					if (tcips->cip->ci_membase[ i ] != 0  &&
						tcips->cip->ci_membase[ i ] < tlow )
						tlow = tcips->cip->ci_membase[ i ];
				}

				if ( tlow < lowestaddr )
				{
					lowestaddr = tlow;
					bestmatch = tcips;
				}
			}
			else if ( cmp > 0 )
			{
				break;
			}

			tcips = tcips->cnext;
		}

		if ( bestmatch != NULL )
		{
			_cm_update_key( bestmatch, _cm_boot_hba );

			kmem_free( _cm_boot_hba, sizeof( *_cm_boot_hba ));
			_cm_boot_hba = NULL;

			clean = &_cm_new_cips;

			while ( *clean != bestmatch )
				clean = &(*clean)->cnext;

			*clean = (*clean)->cnext;
			kmem_free( bestmatch, sizeof( *bestmatch ));
		}
	}

	/* Now, do I A) check for _cm_boot_hba != NULL again and
	   pick the NEXT best, or B) wait until after I'm done
	   processing the rest of the lists and then try one last
	   time to find a match from what's left ??

	   B for now !!
	*/

	tkeys = _cm_purge_keys;
	tcips = _cm_new_cips;

	while ( tkeys != NULL  &&  tcips != NULL )
	{
		cmp = _cm_strncmp(tkeys->brdid, tcips->brdid, CM_MAXHEXDIGITS);

		if ( cmp == 0 )
			_cm_match_em( &tkeys, &tcips );
		else if ( cmp < 0 )
			tkeys = tkeys->knext;
		else
			tcips = tcips->cnext;
	}

	if ( _cm_boot_hba != NULL  &&  _cm_new_cips != NULL )
	{
	 	 
	}

	/* Now delete remaining obsolete entries */

	while ( _cm_purge_keys != NULL )
	{
		rma.rm_key = _cm_purge_keys->key;
		tkeys = _cm_purge_keys;
		_cm_purge_keys = _cm_purge_keys->knext;
		kmem_free( tkeys, sizeof( *tkeys ));

		(void)rm_delkey( &rma );
	}

	/*
	** Now add new resmgr entries for the any left in list
	**
	** They will be added in the same relative order as I
	** originally received them from CA to preserve "slot order"
	*/

	for ( ;; )
	{
		tcips = _cm_new_cips;
		cmp = INT_MAX;

		while ( tcips != NULL )
		{
			if ( tcips->order < cmp )
			{
				bestmatch = tcips;
				cmp = tcips->order;
			}

			tcips = tcips->cnext;
		}

		if ( cmp == INT_MAX )
			break;

		_cm_add_entry( bestmatch, RM_NULL_KEY );

		bestmatch->order = INT_MAX;	/* Mark this entry "done" */
	}


	while ( _cm_new_cips != NULL )
	{
		tcips = _cm_new_cips->cnext;
		kmem_free( _cm_new_cips, sizeof( *_cm_new_cips ));
		_cm_new_cips = tcips;
	}
}

STATIC void
_cm_match_em( struct cm_key_list **keysp, struct cm_cip_list **cipsp )
{
	struct cm_key_list	*nextk = (*keysp)->knext;
	struct cm_cip_list	*nextc = (*cipsp)->cnext;
	struct cm_key_list	*tkeys;
	struct cm_cip_list	*tcips;
	boolean_t		lastk = B_FALSE;
	boolean_t		lastc = B_FALSE;
	char			brdid[ CM_MAXHEXDIGITS ];
	
	(void)strncpy( brdid, (*cipsp)->brdid, CM_MAXHEXDIGITS );

	if ( nextk == NULL  ||
		_cm_strncmp( brdid, nextk->brdid, CM_MAXHEXDIGITS ) != 0 )
	{
		lastk = B_TRUE;
	}

	if ( nextc == NULL  ||
		_cm_strncmp( brdid, nextc->brdid, CM_MAXHEXDIGITS ) != 0 )
	{
		lastc = B_TRUE;
	}

	if ( lastc == B_TRUE  &&  lastk == B_TRUE )
	{
		_cm_update_key( *cipsp, *keysp );

		tcips = *cipsp;
		*cipsp = (*cipsp)->cnext;

		cipsp = &_cm_new_cips;

		while ( *cipsp != tcips )
			cipsp = &(*cipsp)->cnext;

		*cipsp = (*cipsp)->cnext;
		kmem_free( tcips, sizeof( *tcips ));

		tkeys = *keysp;
		*keysp = (*keysp)->knext;

		keysp = &_cm_purge_keys;

		while ( *keysp != tkeys )
			keysp = &(*keysp)->knext;

		*keysp = (*keysp)->knext;
		kmem_free( tkeys, sizeof( *tkeys ));
	}
	else
	{
		/*
		** Too complicated to deal with, so bump pointers along
		*/

		*cipsp = nextc;

		while ( *cipsp != NULL  &&
			_cm_strncmp( brdid, (*cipsp)->brdid, CM_MAXHEXDIGITS ) == 0 )
			*cipsp = (*cipsp)->cnext;

		*keysp = nextk;

		while ( *keysp != NULL  &&
			_cm_strncmp( brdid, (*keysp)->brdid, CM_MAXHEXDIGITS ) == 0 )
			*keysp = (*keysp)->knext;
	}
}

STATIC void
_cm_update_key( struct cm_cip_list *cip, struct cm_key_list *key )
{
	struct rm_args	rma;

	rma.rm_key = key->key;

	(void)strcpy( rma.rm_param, CM_CLAIM );
	(void)rm_delval( &rma );

	(void)strcpy( rma.rm_param, CM_SLOT );
	(void)rm_delval( &rma );

	(void)strcpy( rma.rm_param, CM_CA_DEVCONFIG );
	(void)rm_delval( &rma );

	(void)strcpy( rma.rm_param, CM_BRDBUSTYPE );
	(void)rm_delval( &rma );

	(void)strcpy( rma.rm_param, CM_BRDID );
	(void)rm_delval( &rma );

	_cm_add_entry( cip, key->key );
}

/*
 * STATIC int
 * cm_get_intr_info(rm_key_t key, int *devflagp, struct intr_info *ip)
 *	Utility function to get interrupt info from a key.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
cm_get_intr_info(rm_key_t key, int *devflagp, struct intr_info *ip)
{
	struct rm_args rma;
	cm_num_t irq;
	cm_num_t ipl;
	cm_num_t itype;
	cm_num_t cpu;
	int stat;

	rma.rm_key = key;
	rma.rm_n = 0;

	(void)strcpy(rma.rm_param, CM_IRQ);
	rma.rm_val = &irq;
	rma.rm_vallen = sizeof irq;
	if (rm_getval(&rma, UIO_SYSSPACE) != 0 || rma.rm_vallen != sizeof irq)
		return -1;
	ip->ivect_no = irq;

	(void)strcpy(rma.rm_param, CM_IPL);
	rma.rm_val = &ipl;
	rma.rm_vallen = sizeof ipl;
	if (rm_getval(&rma, UIO_SYSSPACE) != 0 || rma.rm_vallen != sizeof ipl)
		return -1;
	ip->int_pri = ipl;

	(void)strcpy(rma.rm_param, CM_ITYPE);
	rma.rm_val = &itype;
	rma.rm_vallen = sizeof itype;
	if (rm_getval(&rma, UIO_SYSSPACE) != 0 || rma.rm_vallen != sizeof itype)
		return -1;
	ip->itype = itype;

	(void)strcpy(rma.rm_param, CM_BINDCPU);
	rma.rm_val = &cpu;
	rma.rm_vallen = sizeof cpu;
	if ((stat = rm_getval(&rma, UIO_SYSSPACE)) == ENOENT)
		cpu = -1;
	else if (stat != 0 || rma.rm_vallen != sizeof cpu)
		return -1;
	ip->int_cpu = cpu;

	ip->int_mp = (*devflagp & D_MP);

	return 0;
}

/*
 * Structure used to maintain state between cm_intr_attach and cm_intr_detach.
 */
struct cm_intr_cookie {
	struct intr_info ic_intr_info;
	void		 (*ic_handler)();
};

/*
 * int
 * cm_intr_attach( rm_key_t key, void (*handler)(), int *devflagp,
 *		   void **intr_cookiep )
 *	Attach device interrupts.
 *
 * Calling/Exit State:
 *	May block, so must not be called with basic locks held.
 *
 *	If intr_cookiep is non-NULL, it will be filled in with a cookie
 *	which can be passed to cm_intr_detach to detach the interrupts
 *	when the driver is done with them.  If intr_cookiep is NULL,
 *	the interrupts will never be detached.
 */
int
cm_intr_attach(rm_key_t key, void (*handler)(), int *devflagp,
		void **intr_cookiep)
{
	struct intr_info ii;

	if (cm_get_intr_info(key, devflagp, &ii) != 0 ||
	    mod_add_intr(&ii, handler) != 0) {
		if (intr_cookiep != NULL)
			*intr_cookiep = NULL;
		return 0;
	}

	if (intr_cookiep != NULL) {
		*intr_cookiep = kmem_alloc(sizeof(struct cm_intr_cookie),
					   KM_SLEEP);
		((struct cm_intr_cookie *)*intr_cookiep)->ic_intr_info = ii;
		((struct cm_intr_cookie *)*intr_cookiep)->ic_handler = handler;
	}

	return 1;
}

/*
 * void
 * cm_intr_detach(void *intr_cookie)
 *	Detach device interrupts.
 *
 * Calling/Exit State:
 *	May block, so must not be called with basic locks held.
 *
 *	The intr_cookie must be a value passed back from a previous call
 *	to cm_intr_attach.
 */
void
cm_intr_detach(void *intr_cookie)
{
	struct cm_intr_cookie *icp = intr_cookie;

	if (icp == NULL)
		return;

	(void)mod_remove_intr(&icp->ic_intr_info, icp->ic_handler);

	kmem_free(icp, sizeof *icp);
}


/*
 * STATIC int 
 * _cm_get_ca_devconfig(rm_key_t key, cm_num_t *val)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int 
_cm_get_ca_devconfig(rm_key_t key, cm_num_t *val)
{
	struct rm_args	rma;
	cm_num_t	devconfig;

	SET_RM_ARGS(&rma, CM_CA_DEVCONFIG, key, &devconfig, sizeof(devconfig), 0);
	if (rm_getval(&rma, UIO_SYSSPACE) != 0 || 
	    rma.rm_vallen != sizeof(devconfig))
		return -1;

	*val = devconfig;
	return 0;
}

/*
 * STATIC cm_num_t 
 * _cm_get_brdbustype(rm_key_t key)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC cm_num_t
_cm_get_brdbustype(rm_key_t key)
{
	struct rm_args	rma;
	cm_num_t bustype;

	SET_RM_ARGS(&rma, CM_BRDBUSTYPE, key, &bustype, CM_NUM_SIZE, 0);
	if (rm_getval(&rma, UIO_SYSSPACE) == 0) 
		return bustype;

	return CM_BUS_UNK;
}

/*
 * size_t
 * cm_devconfig_size(rm_key_t key)
 *	Get the size of device configuration space.
 *
 * Calling/Exit State:
 *	Returns the size of the device configuration space.
 */
size_t
cm_devconfig_size(rm_key_t key)
{
	cm_num_t devconfig;
	ulong_t bustype;

	switch((bustype = (ulong_t)_cm_get_brdbustype(key))) {
	case CM_BUS_EISA:
	case CM_BUS_MCA:
	case CM_BUS_PCI:
	case CM_BUS_UNK:
		break;
	default:
		bustype = CM_BUS_UNK;
		break;
	}

	if (_cm_get_ca_devconfig(key, &devconfig) == -1)
		return 0;

	return ca_devconfig_size(bustype, devconfig);
}

/*
 * int
 * cm_read_devconfig(rm_key_t key, off_t offset, void *buf, size_t nbytes)
 *	Read device configuration space.
 *
 * Calling/Exit State:
 *	None.
 */
int
cm_read_devconfig(rm_key_t key, off_t offset, void *buf, size_t nbytes)
{
	cm_num_t devconfig;
	ulong_t bustype;

	switch((bustype = (ulong_t)_cm_get_brdbustype(key))) {
	case CM_BUS_EISA:
	case CM_BUS_MCA:
	case CM_BUS_PCI:
	case CM_BUS_UNK:
		break;
	default:
		bustype = CM_BUS_UNK;
		break;
	}

	if (_cm_get_ca_devconfig(key, &devconfig) == -1)
		return -1;

	return ca_read_devconfig(bustype, devconfig, buf, offset, nbytes);
}

/*
 * int
 * cm_write_devconfig(rm_key_t key, off_t offset, void *buf, size_t nbytes)
 *	Write device configuration space.
 *
 * Calling/Exit State:
 *	None.
 */
int
cm_write_devconfig(rm_key_t key, off_t offset, void *buf, size_t nbytes)
{
	cm_num_t devconfig;
	ulong_t bustype;

	switch((bustype = (ulong_t)_cm_get_brdbustype(key))) {
	case CM_BUS_EISA:
	case CM_BUS_MCA:
	case CM_BUS_PCI:
	case CM_BUS_UNK:
		break;
	default:
		bustype = CM_BUS_UNK;
		break;
	}

	if (_cm_get_ca_devconfig(key, &devconfig) == -1)
		return -1;

	return ca_write_devconfig(bustype, devconfig, buf, offset, nbytes);
}


/*
 * STATIC void
 * _cm_itoh(ulong_t n, char s[], int pad)
 *	Internal routine to convert unsigned integer to HEX ASCII.
 *  pad with 0's to "pad" digits.  pad=0 for NO padding.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
_cm_itoh(ulong_t n, char s[], int pad)
{
	char	buf[CM_MAXHEXDIGITS];
	char	*bp;
	int	i;

	bp = buf;
	i = 0;

	do {
		*bp++ = "0123456789ABCDEF"[n % CM_HEXBASE];
		i++;

	} while (((n /= CM_HEXBASE) > 0) && i < CM_MAXHEXDIGITS);

	/* pad to "pad" digits with 0's */

	pad -= bp - buf;

	bp--;

	*s++ = '0';	/* Add standard HEX prefix '0x' */
	*s++ = 'x';

	while ( pad-- > 0 )
		*s++ = '0';

	while (bp >= buf)
		*s++ = *bp--;

	*s = '\0';
}


/*
 * int
 * cm_AT_putconf(rm_key_t rm_key, int vector, int itype,
 *		ulong sioa, ulong eioa,
 *		ulong scma, ulong ecma, int dmac,
 *		uint setmask, int claim)
 *	Populate the internal Resource Manager Database with
 * 	the PARAMETER=values that were interpreted from
 * 	PCU data by the driver.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
cm_AT_putconf(rm_key_t rm_key, int vector, int itype,
		ulong sioa, ulong eioa,
		ulong scma, ulong ecma, int dmac,
		uint setmask, int claim)
{
	cm_args_t		cma;
	cm_num_t		num;
	struct	cm_addr_rng	rng;
	int			rv;

	cma.cm_key = rm_key;
	cma.cm_n   = 0;

	if (setmask & CM_SET_IRQ)	{
cmn_err(CE_CONT, "!cm_AT_putconf(): setting vector = %d\n", vector);
		/* Delete and Set IRQ.  */
		cma.cm_param = CM_IRQ;
		cma.cm_vallen = sizeof(cm_num_t);
		(void) cm_delval(&cma);
		num = vector;
		cma.cm_val = &num;
		if ((rv = cm_addval(&cma)) != 0) {
			/*
			 *+ Warning message
			 */
			cmn_err(CE_WARN, "cm_AT_putconf() failed cm_addval()\n");
			return(rv);
		}
	}

	if (setmask & CM_SET_ITYPE)	{
cmn_err(CE_CONT, "!cm_AT_putconf(): setting itype = %d\n", itype);
		/* Delete and Set ITYPE.  */
		cma.cm_param = CM_ITYPE;
		cma.cm_vallen = sizeof(cm_num_t);
		(void) cm_delval(&cma);
		num = itype;
		cma.cm_val = &num;
		if ((rv = cm_addval(&cma)) != 0) {
			/*
			 *+ Warning message
			 */
			cmn_err(CE_WARN, "cm_AT_putconf() failed cm_addval()\n");
			return(rv);
		}
	}

	if (setmask & CM_SET_IOADDR)	{
cmn_err(CE_CONT, "!cm_AT_putconf(): setting ioaddr = %0x%x-0x%x\n", sioa, eioa);
		/* Delete and Set IO address range. */
		cma.cm_param = CM_IOADDR;
		cma.cm_vallen = sizeof(struct cm_addr_rng);
		(void) cm_delval(&cma);
		rng.startaddr = sioa;
		rng.endaddr = eioa;
		cma.cm_val = &rng;
		if ((rv = cm_addval(&cma)) != 0) {
			/*
			 *+ Warning message
			 */
			cmn_err(CE_WARN, "cm_AT_putconf() failed cm_addval()\n");
			return(rv);
		}
	}

	if (setmask & CM_SET_MEMADDR)	{
cmn_err(CE_CONT, "!cm_AT_putconf(): setting memaddr = %0x%x-0x%x\n", scma, ecma);
		/* Delete and Set MEM address range.  */
		cma.cm_param = CM_MEMADDR;
		cma.cm_vallen = sizeof(struct cm_addr_rng);
		(void) cm_delval(&cma);
		rng.startaddr = scma;
		rng.endaddr = ecma;
		cma.cm_val = &rng;
		if ((rv = cm_addval(&cma)) != 0) {
			/*
			 *+ Warning message
			 */
			cmn_err(CE_WARN, "cm_AT_putconf() failed cm_addval()\n");
			return(rv);
		}
	}

	if (setmask & CM_SET_DMAC)	{
cmn_err(CE_CONT, "!cm_AT_putconf(): setting dmac = %d\n", dmac);
		/* Delete and Set DMAC.  */
		cma.cm_param = CM_DMAC;
		cma.cm_vallen = sizeof(cm_num_t);
		(void) cm_delval(&cma);
		num = dmac;
		cma.cm_val = &num;
		if ((rv = cm_addval(&cma)) != 0) {
			/*
			 *+ Warning message
			 */
			cmn_err(CE_WARN, "cm_AT_putconf() failed cm_addval()\n");
			return(rv);
		}
	}

	if (claim != 0) {
		cma.cm_param = CM_CLAIM;
		cma.cm_vallen = sizeof(cm_num_t);
		cma.cm_val = &num;
		num = setmask;
		(void)cm_delval(&cma);
		/* If bitmask is 0, no need to have the CM_CLAIM param */
		if (setmask != 0  &&  (rv = cm_addval(&cma)) != 0) {
			cmn_err(CE_WARN, "cm_AT_putconf() failed CM_CLAIM\n");
			return rv;
		}
	}

	return (0);
}

/*
** _cm_ro_param()
**
** Calling/Exit State: None
**
** Description:
**	Returns B_TRUE if param is read-only, B_FALSE otherwise.
*/

boolean_t
_cm_ro_param( const char *param )
{
	static const char *ro_params[] = {
					    CM_MODNAME,
					    CM_BRDBUSTYPE,
					    CM_BRDID,
					    CM_CA_DEVCONFIG,
					    CM_SLOT,
					    CM_TYPE,
					    CM_BINDCPU,
					    NULL
				   	 };

	const char	**rop = ro_params;

	while ( *rop != NULL )
		if ( strcmp( *rop++, param ) == 0 )
			return B_TRUE;

	return B_FALSE;
}
