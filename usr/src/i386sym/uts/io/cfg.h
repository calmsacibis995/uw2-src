/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_CFG_H	/* wrapper symbol for kernel use */
#define	_IO_CFG_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/cfg.h	1.11"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Hardware configuration data structures for Symmetry
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Description of the hardware system we are booting
 */
struct sys_desc {
	unchar	sd_type;		/* B8/B21/S27/S81/etc */
	unchar	sd_nslots;		/* number of SB8000 slots */
	unchar	sd_spareb[2];		/* spare bytes */
	unchar	*sd_slotpri;		/* priority of slots */
	ulong	sd_spare[2];		/* spare for growth */
};

/* sd_type values */
#define	SYSTYP_B8	0		/* BALANCE 8000 */
#define	SYSTYP_B21	1		/* BALANCE 21000 */
#define	SYSTYP_S27	2		/* SYMMETRY S27 */
#define	SYSTYP_S81	3		/* SYMMETRY S81 */
#define SYSTYP_S16	4		/* SYMMETRY S16 */
#define SYSTYP_S1	5		/* SYMMETRY S1 */

/* sd_slotpri values */
#define	SYSPRI_NONE	0		/* not a slot in this backplane */
#define	SYSPRI_LOW	1		/* low-priority slot */
#define	SYSPRI_HIGH	2		/* high-priority slot */
#define	SYSPRI_BOTH	3		/* can be either high or low */

/*
 * Description of the system bus mode
 * the system has been booted in.
 */
struct sys_mode {
	unchar	sm_bus_mode;		/* bus mode, see below */
	unchar	sm_tsize;		/* basic bus transfer size: 8/16/32 */
	unchar	sm_bsize;		/* basic block ownership size: 16/32 */
	unchar	sm_opt_low;		/* low priority in optional slots */
	unchar	sm_cache;		/* cache mode for selectable cache */
	unchar	sm_spare[3];		/* spare bytes for sys_mode */
};

/* sm_bus_mode values */
#define	CD_BUS_DISABLE		0		/* used internally */
#define	CD_BUS_NO_CHANGE	CD_BUS_DISABLE	/* used externally */
#define	CD_BUS_COMPAT		1		/* bus compatibility mode */
#define	CD_BUS_EXTENDED_NARR	2		/* full SGS, 32-bits wide */
#define	CD_BUS_EXTENDED_WIDE	3		/* full SGS, 64-bits wide */

/* sm_cache values */
#define	SGSC_OFF	0x00		/* leave cache off */
#define	SGSC_WRITE_THRU	0x01		/* write-thru mode */
#define	SGSC_COPY_BACK	0x02		/* copy-back mode */
#ifdef CUSTOM_CACHE_MODE
#define	SGSC_ASK_ME	0x03		/* ask the operator */
#define	SGSC_LEAVE_AS_IS	0x04	/* leave based upon bus mode */
#endif	/* CUSTOM_CACHE_MODE */

#define	CD_VERSION	1		/* version of the following table */
#define	BNAMESIZE	80		/* Size of boot name */

/* memory configuration definitions */
#define	MC_CLICK	0x80000		/* memory controller click */
#define	MC_MAXMEM	0x1c00000	/* (FGS) max configurable memory */
#define	MC_BPI		32		/* bits per int */
#define	MC_MAPSIZE	(MC_MAXMEM/MC_CLICK)	/* size of memory map */
/* MC_MMAP is true iff bit-map for memory click 'i' is on. */
#define	MC_MMAP(i,cfgp)	(cfgp->c_mmap[(i)/MC_BPI] & (1 << ((i)%MC_BPI)))

struct config_desc {
	unchar		c_version;	/* version of this file */
	unchar		c_flags;	/* CFG_PORT[01]_CRT flags */
	unchar		c_spareb[2];	/* spare bytes up front */
	struct sys_desc	c_sys;		/* description of this system */
	struct sys_mode	c_sys_mode;	/* the mode the system is inited for */
	uint		c_boot_flag;	/* boot flags */
	char 		*c_boot_name;	/* boot image name */
	short		c_bname_size;	/* # bytes in c_boot_name */
	unchar		c_clock_rate;	/* system clock rate in MHz */
	char		c_kill;		/* kill character */
	char		c_erase;	/* erase character */
	char		c_interrupt;	/* interrupt character */
	int		c_memsize;	/* total KB of configured mem */
	int		c_maxmem;	/* max location configured */
	int		c_bottom;	/* max loc used by SCED f/w */
	int		c_mmap_size;	/* number of bits in c_mmap */
	int		*c_pagetable;	/* page table for procs that need it */
	ulong		*c_mmap;	/* memory bitmap */
	struct ctlr_toc	*c_toc;		/* indices into c_ctlrs[] */
	int		c_toc_size;	/* number of entries in c_toc */
	struct ctlr_desc *c_ctlrs;	/* ctlr descriptions */
	struct ctlr_desc *c_end_ctlrs;	/* just past end of c_ctlrs */
	struct ctlr_desc *c_cons;	/* console SCED's description */
	long		c_spare[20];	/* room to grow */
};

/*
 * Config description is placed at physical address
 * KVCD_LOC, which is mapped to CD_LOC in the
 * kernel's permanent virtual address space.
 */
#define CD_LOC	((struct config_desc *)0x2000)
#define CD_SIZE	(8 * 1024)		/* size of config description */
#ifdef SCED_FW
#define	SCED_CD_LOC	((struct config_desc *)WINDOW_ADDR(CD_LOC))
#endif

/*
 * processor tests and bootstraps are loaded at CD_LOAD_ADDR
 */
#define	CD_LOAD_ADDR	0		/* bootstraps loaded at 0 */
#ifdef SCED_FW
#define	SCED_CD_LOAD_ADDR	WINDOW_ADDR(CD_LOAD_ADDR)
#endif

/*
 * standalone programs are loaded at CD_STAND_ADDR
 */
#define	CD_STAND_ADDR	0x4000		/* standalone progs at 16K */

/*
 * Table of Contents, giving location and number
 * of each controller-type's entries in all_ctlrs[].
 */
struct ctlr_toc {
	unchar	ct_start;		/* index into ctlrs[] of first */
	unchar	ct_count;		/* number of controllers of this type */
};


/*
 * Description of a single controller found in the system.
 */

#define	CTLR_DESC_SIZ	64		/* size of ctlr_desc entry */
#define	CTLR_DESC_USIZ	40		/* max size of ctlr_desc union entry */

struct ctlr_desc {
	uint		cd_diag_flag;	/* results of powerup tests */
	unchar		cd_slic;	/* SLIC address of controller */
	unchar		cd_type;	/* SL_G_BOARDTYPE value */
	unchar		cd_var;		/* SL_G_VARIATION */
	unchar		cd_hrev;	/* SL_G_HGENERATION */
	unchar		cd_srev;	/* SL_G_SGENERATION */
	unchar		cd_i;		/* The i'th ctlr of this type */
	unchar		cd_spareb[2];	/* spare bytes */
	struct ctlr_type_desc	*cd_ct;	/* pointer to powerup info */
	long		cd_spare[2];	/* for more common stuff later */
	union cd_un {
		struct cdun_proc {
			unchar	cdup_speed;	/* 16 or 20 */
			unchar	cdup_fp;	/* floating-pt type
						 * (e.g. from SL_P2_FP_TYPE) */
			unchar	cdup_width;	/* bus width: 32 or 64 */
			unchar	cdup_nsets;	/* number of cache sets */
			unchar	cdup_setsize;	/* kb in each cache set */
			unchar	cdup_custom;	/* flags from SL_P2_CUSTOM */
			unchar	cdup_custom2;	/* flags from SL_P2_CUSTOM2 */
			unchar	cdup_bsize;	/* cache-block size */
#ifdef SCED_FW
						/* SCED_FW-only fields below */
			int	(*cdup_start)();	/* start a processor */
			int	(*cdup_build_pt)();	/* build page tables */
#endif	/* SCED_FW */
		} cdun_proc;
		struct cdun_mem {
			int	cdum_psize;	/* size of actual mem */
			int	cdum_lsize;	/* size of good mem */
			uint	cdum_base;	/* base address */
			ushort	cdum_pbanks;	/* mask of physical banks */
			ushort	cdum_lbanks;	/* mask of addrs responding */
			unchar	cdum_ileave;	/* interleaved? */
			unchar	cdum_type;	/* type from SL_M_BSIZE (FGS)
						 * or SL_SM_TYPE (SGS) */
			unchar	cdum_width;	/* bus width: 32 or 64 */
			unchar	cdum_spare;
			int	cdum_round;	/* boundary for cdum_base */
			unchar	cdum_perm;	/* SGS bank permutation desc. */
			unchar	cdum_expid;	/* expansion id register */
			unchar	cdum_expid2;	/* 2nd expansion id register */
#ifdef SCED_FW
						/* SCED_FW-only fields below */
			struct mem_info *cdum_info;	/* ptr to mem info */
#endif	/* SCED_FW */
		} cdun_mem;
		struct cdun_mbad {
			unchar	cdumb_version;	/* firmware version */
		} cdun_mbad;
		struct cdun_sced {
			char	*cdus_init_queue;
			unchar	cdus_version;	/* firmware version */
			unchar	cdus_host_num;	/* host adapter number */
			unchar	cdus_enet_addr[6];	/* ether address */
			unchar	cdus_cons;	/* console SCED? */
		} cdun_sced;
		struct cdun_ssm {
			ulong	cduss_sysid;	/* system id */
			unchar	cduss_spare;	/* spare; old f/w version no */
			unchar	cduss_num_scsi;	/* number of SCSI channels */
			unchar	cduss_cons;	/* console SSM? */
			unchar	cduss_host_num;	/* host target adapter  */
			unchar	cduss_version[3];	/* f/w version no */
                        unchar  cduss_biff_type;  /* The type of biff on none */
			unchar	cduss_Aversion[3];	/* lvl A fw vrs. */
                        unchar  cduss_biff_flags; /* Flags for with biff */
		} cdun_ssm;
		struct cdun_dcc {
			unchar	cdud_version;	/* firmware version */
		} cdun_dcc;
		struct cdun_cadm {
			ulong	cduc_sysid;	/* system id */
			unchar	cduc_fptype;	/* front panel type */
		} cdun_cadm;
#ifndef SCED_FW
		char	cdun_pad[CTLR_DESC_USIZ];	/* CTLR_DESC_USIZ pad */
#endif	/* SCED_FW */
	} cd_un;
};


/* diagnostics flags (first 4 bits are general across boards) */
#define	CFG_FAIL	0x00000002	/* board failed power-up diagnostics */
#define	CFG_DECONF	0x00000004	/* board is deconfigured */
#define	CFG_BDUSE	0x00000008	/* board in use for diagnostics */

/*
 * collect diag flags
 */
#ifdef SCED_FW
#define	DIAG_FLAGS(cdp)	((cdp)->cd_diag_flag | pp.pp_diag_flags[(cdp)->cd_slic])
#else
#define	DIAG_FLAGS(cdp)	((cdp)->cd_diag_flag)
#endif	/* SCED_FW */

/*
 * Is the controller deconfigured?
 */
#define	DECONFIGURED(cdp)	(DIAG_FLAGS(cdp) & CFG_DECONF)

/*
 * Is the controller deconfigured, or did it fail powerup tests?
 */
#define	FAILED(cdp)	(DIAG_FLAGS(cdp) & (CFG_FAIL|CFG_DECONF))

/*
 * Is there a soft failure on this controller?
 */
#define	SOFT_FAILURE(cdp)	(DIAG_FLAGS(cdp) & ~(CFG_FAIL|CFG_DECONF))


/*
 * Type fields that should be directly accessible through ctlr_desc,
 * save for a space optimization that we do.
 */
#define	cd_name		cd_ct->ct_name
#define	cd_ttype	cd_ct->ct_type
#define	cd_print_order	cd_ct->ct_print_order
#define	cd_pr_config	cd_ct->ct_pr_config
#define	cd_test		cd_ct->ct_test
#define	cd_init		cd_ct->ct_init
#define	cd_bld_config	cd_ct->ct_bld_config
#define	cd_disable	cd_ct->ct_disable
#define	cd_read_buserr	cd_ct->ct_read_buserr
#define	cd_clr_buserr	cd_ct->ct_clr_buserr
#define	cd_read_accerr	cd_ct->ct_read_accerr
#define	cd_clr_accerr	cd_ct->ct_clr_accerr
#define	cd_cacheop	cd_ct->ct_cacheop
#define	cd_diagnose	cd_ct->ct_diagnose
#define	cd_nbics	cd_ct->ct_nbics
#define	cd_bics		cd_ct->ct_bics
#define	cd_nbdps	cd_ct->ct_nbdps
#define	cd_bdps		cd_ct->ct_bdps
#define	cd_ncmcs	cd_ct->ct_ncmcs
#define	cd_cmcs		cd_ct->ct_cmcs
#define	cd_flags	cd_ct->ct_flags


/*
 * PROCESSOR BOARDS
 */
#define	cd_p_speed	cd_un.cdun_proc.cdup_speed
#define	cd_p_fp		cd_un.cdun_proc.cdup_fp
#define	cd_p_width	cd_un.cdun_proc.cdup_width
#define	cd_p_nsets	cd_un.cdun_proc.cdup_nsets
#define	cd_p_setsize	cd_un.cdun_proc.cdup_setsize
#define	cd_p_custom	cd_un.cdun_proc.cdup_custom
#define	cd_p_custom2	cd_un.cdun_proc.cdup_custom2
#define cd_p_bsize	cd_un.cdun_proc.cdup_bsize
#ifdef SCED_FW
#define	cd_p_start	cd_un.cdun_proc.cdup_start
#define	cd_p_build_pt	cd_un.cdun_proc.cdup_build_pt
#define	cd_p_flush	cd_un.cdun_proc.cdup_flush
#endif	/* SCED_FW */

/*
 * cd_p_setsize settings
 */
#define	CDP_NO_CACHE	0		/* no cache on processor */
#define	CDP_CACHE_1K	1		/* 1 Kb cache set */
#define	CDP_CACHE_2K	2		/* 2 Kb cache set */
#define	CDP_CACHE_4K	3		/* 4 Kb cache set */
#define	CDP_CACHE_8K	4		/* 8 Kb cache set */
#define	CDP_CACHE_16K	5		/* 16 Kb cache set */
#define	CDP_CACHE_32K	6		/* 32 Kb cache set */
#define	CDP_CACHE_64K	7		/* 64 Kb cache set */
#define	CDP_CACHE_128K	8		/* 128 Kb cache set */
#define	CDP_CACHE_256K	9		/* 256 Kb cache set */
#define	CDP_CACHE_512K	10		/* 512 Kb cache set */
#define	CDP_CACHE_1M	11		/* 1 Mb cache set */

/* Compute cache set size in bytes */
#define	CDP_SETSIZE(x)	((x)? (1 << ((x) + 9)): 0)
/*
 * NS32x32 processor board cd_diag_flag settings
 */
#define	CFG_RUN		0x00000010	/* processor board is running */
#define	CFG_CACHE0	0x00000100	/* first cache set bad */
#define	CFG_CACHE1	0x00000200	/* second cache set bad */
#define	CFG_WBUF	0x00000400	/* write buffer bad */

/*
 * i80386 processor board cd_diag_flag settings
 */
#define	CFG_SP_386_BIT	0x00000010	/* 386 Internal test failed */
#define	CFG_SP_386_TST	0x00000020	/* 386 test failed */
#define	CFG_SP_LNG_JMP	0x00000040	/* Jump to main memory test failed */
#define	CFG_SP_SLIC_TST	0x00000080	/* SLIC test failed */
#define	CFG_SP_386_MMU	0x00000100	/* 386 MMU test failed */
#define	CFG_SP_BUS_TST	0x00000200	/* Bus data test failed */
#define	CFG_SP_BDP_LO	0x00000400	/* Lower BDP test failed */
#define	CFG_SP_BDP_HI	0x00000800	/* Upper BDP test failed */
#define	CFG_SP_BIC	0x00001000	/* BIC test failed */
#define	CFG_SP_FPU	0x00002000	/* FPU test failed */
#define	CFG_SP_CMC0	0x00004000	/* Set 0 CMC array test failed */
#define	CFG_SP_DRAM_0	0x00008000	/* Cache Data RAM Set 0 test failed */
#define	CFG_SP_TRAM_0	0x00010000	/* Cache Tag RAM Set 0 test failed */
#define	CFG_SP_SRAM_0	0x00020000	/* Cache State RAM Set 0 test failed */
#define	CFG_SP_CMC1	0x00040000	/* Set 1 CMC array test failed */
#define	CFG_SP_DRAM_1	0x00080000	/* Cache Data RAM Set 1 test failed */
#define	CFG_SP_TRAM_1	0x00100000	/* Cache Tag RAM Set 1 test failed */
#define	CFG_SP_SRAM_1	0x00200000	/* Cache State RAM Set 1 test failed */
#define	CFG_SP_FPA	0x00400000	/* FPA 1163/1164/1165 test failed */
#define	CFG_SP_UXINRPT	0x02000000	/* Unexpected interrupt occurred */
#define	CFG_SP_TIMEOUT	0x04000000	/* 386 Test timeout */

#define	CFG_SP_FATAL_ERR	0x6003FF0	/* error bits that are fatal */

/* error flags for each cache set */
#define	CFG_SP_CACHE0_ERR \
	(CFG_SP_CMC0| CFG_SP_DRAM_0| CFG_SP_TRAM_0| CFG_SP_SRAM_0)
#define	CFG_SP_CACHE1_ERR \
	(CFG_SP_CMC1| CFG_SP_DRAM_1| CFG_SP_TRAM_1| CFG_SP_SRAM_1)

/*
 * MEMORY BOARDS
 */
#define	cd_m_psize	cd_un.cdun_mem.cdum_psize
#define	cd_m_lsize	cd_un.cdun_mem.cdum_lsize
#define	cd_m_round	cd_un.cdun_mem.cdum_round
#define	cd_m_base	cd_un.cdun_mem.cdum_base
#define	cd_m_pbanks	cd_un.cdun_mem.cdum_pbanks
#define	cd_m_lbanks	cd_un.cdun_mem.cdum_lbanks
#define	cd_m_ileave	cd_un.cdun_mem.cdum_ileave
#define	cd_m_type	cd_un.cdun_mem.cdum_type
#define	cd_m_width	cd_un.cdun_mem.cdum_width
#define	cd_m_perm	cd_un.cdun_mem.cdum_perm
#define	cd_m_expid	cd_un.cdun_mem.cdum_expid
#ifdef SCED_FW
#define	cd_m_info	cd_un.cdun_mem.cdum_info
#endif	/* SCED_FW */

/*
 * Interleave flags: cd_m_ileave values
 */
#define	NO_ILEAVE	0		/* not interleaved */
#define	ILEAVE_LO	1		/* interleaved; low block */
#define	ILEAVE_HI	2		/* interleaved; high block */

#ifdef SCED_FW
/*
 * Description of operations useful for all memory controllers.
 */
struct mem_info {
	int	(*mi_enable)();		/* func to enable a controller */
	int	(*mi_clear)();		/* func to zero the memory */
	int	(*mi_setup)();		/* func to enable/disable refresh/ECC */
	int	(*mi_can_ileave)();	/* func to ask if ctlrs can ileave */
	int	(*mi_lcompute)();	/* func to compute lmask, lsize */
	int	(*mi_fill_mmap)();	/* fill in mmap bitmask */
	int	(*mi_has_ecc)();	/* func to return if ecc error */
	int	(*mi_pr_ecc)();		/* func to print info from ECC errors */
};
#endif	/* SCED_FW */

/*
 * cd_m_perm describes how banks on Symmetry Series
 * memories are permuted.
 */
#define	CDMB_DONT_USE	0x01		/* board is unusable */
#define	CDMB_SWAP	0x02		/* bank 0 is bad; swap banks 0 and 1 */
#define	CDMB_SHIFT	0x04		/* shift banks 2...7 downto 1...6 */
#define	CDMB_FOURBANKS	0x08		/* use four-bank addressing */

/*
 * Is this memory controller unusable
 * because of the bad banks?  (That is,
 * could we not determine a reasonable
 * permutation of the banks?)
 */
#define	MEM_PERM_UNUSABLE(x)	((x) & CDMB_DONT_USE)

/*
 * Balance Series Memory cd_diag_flag settings
 */
#define	CFG_M_SIZE	0x00000010	/* memory board size inconsistency */
#define	CFG_M_TYPE	0x00000020	/* memory board other inconsistency */

/*
 * Symmetry Series Memory cd_diag_flag settings
 */
#define	CFG_SM_BDP_LO	0x00000010	/* lower BDP is bad */
#define	CFG_SM_BDP_HI	0x00000020	/* upper BDP is bad */
#define	CFG_SM_BIC	0x00000040	/* BIC is bad */
#define	CFG_SM_EDC_LO	0x00000080	/* lower EDC is bad */
#define	CFG_SM_EDC_HI	0x00000100	/* upper EDC is bad */
#define	CFG_SM_MEM_5	0x00000200	/* memory 5's test failed */
#define	CFG_SM_MEM_A	0x00000400	/* memory A's test failed */
#define	CFG_SM_A_TST	0x00000800	/* bank address test failed */
#define	CFG_SM_D_TST	0x00001000	/* bank data test failed */
#define	CFG_SM_EXP_INC	0x00002000	/* controller:expansion inconsistancy */
#define	CFG_SM_CLR	0x00004000	/* clear function failed */
#define	CFG_SM_FILL_TO	0x00008000	/* memory fill timeout */
#define	CFG_SM_CHK_TO	0x00010000	/* check-for-EDC-errors timeout */
#define	CFG_SM_BANKS	0x3ff00000	/* the failed or disabled banks */
#define	CFG_SM_FATAL	0x000ffff0	/* mask of fatal errors */

#define	CFG_SM_BANKS_SHIFT	20
#define	CFG_SM_BANKS_BIT(x)	(1<<((x)+CFG_SM_BANKS_SHIFT))

/*
 * MULTIBUS ADAPTER BOARDS
 */
#define	cd_mb_version	cd_un.cdun_mbad.cdumb_version

/*
 * MBAd cd_diag_flag settings
 */
#define	CFG_A_CKS	0x00000010	/* bad checksum in firmware */
#define	CFG_A_VERS	0x00000020	/* mbad, can't obtain f/w version# */
#define	CFG_A_SLIC	0x00000040	/* SLIC timeout on wr/rdslave() */
#define	CFG_A_TOUT	0x00000080	/* mbad bus timeouts not working */
#define	CFG_A_TEST_LPBK	0x00000100	/* do test of mbad<->mbif connection */
#define	CFG_A_LOOPBK	0x00000200	/* mbad<->mbif connection not there */
#define	CFG_A_ARB	0x00000400	/* multibus arbitration not working */
#define	CFG_A_BTO	0x00000800	/* mbad unexp. bus timeout occured */
#define	CFG_A_ACC	0x00001000	/* mbad unexpected access error */ 
#define	CFG_A_ALM_0	0x00002000	/* ALM memory won't set to 0 */
#define	CFG_A_ALM_1	0x00004000	/* ALM memory won't set to 1 */
#define	CFG_A_GEN	0x00008000	/* generation is old non-alm mbad */

#define	cd_dc_version	cd_un.cdun_dcc.cdud_version

/*
 * SCED CONTROLLERS
 */
#define	cd_sc_init_queue	cd_un.cdun_sced.cdus_init_queue
#define	cd_sc_version	cd_un.cdun_sced.cdus_version
#define	cd_sc_host_num	cd_un.cdun_sced.cdus_host_num
#define	cd_sc_enet_addr	cd_un.cdun_sced.cdus_enet_addr
#define	cd_sc_cons	cd_un.cdun_sced.cdus_cons

/*
 * SSM CONTROLLERS
 */
#define	cd_ssm_sysid	cd_un.cdun_ssm.cduss_sysid
#define	cd_ssm_version	cd_un.cdun_ssm.cduss_version
#define	cd_ssm_num_scsi	cd_un.cdun_ssm.cduss_num_scsi
#define	cd_ssm_host_num cd_un.cdun_ssm.cduss_host_num
#define	cd_ssm_cons	cd_un.cdun_ssm.cduss_cons
#define cd_ssm_biff_type        cd_un.cdun_ssm.cduss_biff_type
#define cd_ssm_biff_flags       cd_un.cdun_ssm.cduss_biff_flags

/*
 * SSM cd_diag_flag settings
 */
#define CFG_SSM_PROC	0x00000010	/* processor failure */
#define CFG_SSM_PPIC	0x00000020	/* processor peripheral interface 
					 * controller failure */
#define CFG_SSM_8254	0x00000040	/* interval timer failure */
#define CFG_SSM_8259	0x00000080	/* interrupt controller failure */
#define CFG_SSM_ARC	0x00000100	/* arbitration/refresh failure */
#define CFG_SSM_BIC	0x00000200	/* bus interface controller failure */
#define CFG_SSM_EPROM	0x00000400	/* boot eprom failure */
#define CFG_SSM_DRAM	0x00000800	/* local memory failure */
#define CFG_SSM_NVRAM	0x00001000	/* non-volatile memory failure */
#define CFG_SSM_SLIC	0x00002000	/* slic diagnostic failure */
#define CFG_SSM_FP	0x00004000	/* front panel failure */
#define CFG_SSM_LOCAL	0x00008000	/* local console port failure */
#define CFG_SSM_REMOTE	0x00010000	/* remote console port failure */
#define CFG_SSM_PRINT	0x00020000	/* printer port failure */
#define CFG_SSM_WDT	0x00040000	/* watchdog timer failure */
#define CFG_SSM_TOD	0x00080000	/* time-of-day clock failure */
#define CFG_SSM_SCSI0	0x00100000	/* scsi port 0 diagnostic failure */
#define CFG_SSM_SCSI1	0x00200000	/* scsi port 1 diagnostic failure */
#define CFG_SSM_SCSI2	0x00400000	/* scsi port 2 diagnostic failure */
#define CFG_SSM_SCSI3	0x00800000	/* scsi port 3 diagnostic failure */
#define CFG_SSM_VME	0x01000000	/* VME interface failure */
#define CFG_SSM_VTOS	0x02000000	/* VME to Sequent map ram failure */
#define CFG_SSM_STOV	0x04000000	/* Sequent to VME map ram failure */
#define CFG_SSM_VPIC	0x08000000	/* VME peripheral interface 
					 * controller failure */

/* 
 * Definitions for failed subsections 
 * of the SSM.
 */
#define CFG_SSM_PROC_FAIL	(CFG_SSM_SLIC | CFG_SSM_BIC | CFG_SSM_PROC \
				 | CFG_SSM_PPIC | CFG_SSM_EPROM | CFG_SSM_DRAM \
				 | CFG_SSM_NVRAM | CFG_SSM_8254 | CFG_SSM_8259)

#define CFG_SSM_BOOT_FAIL	(CFG_SSM_PROC_FAIL | CFG_SSM_ARC | CFG_SSM_FP \
				 | CFG_SSM_WDT | CFG_SSM_LOCAL | CFG_SSM_SCSI0)

#define CFG_SSM_LOCAL_FAIL	(CFG_SSM_LOCAL | CFG_SSM_PROC_FAIL)
#define CFG_SSM_REMOTE_FAIL	(CFG_SSM_REMOTE | CFG_SSM_PROC_FAIL)
#define CFG_SSM_SCSI0_FAIL	(CFG_SSM_SCSI0 | CFG_SSM_PROC_FAIL)
#define CFG_SSM_SCSI1_FAIL	(CFG_SSM_SCSI1 | CFG_SSM_PROC_FAIL)
#define CFG_SSM_SCSI2_FAIL	(CFG_SSM_SCSI2 | CFG_SSM_PROC_FAIL)
#define CFG_SSM_SCSI3_FAIL	(CFG_SSM_SCSI3 | CFG_SSM_PROC_FAIL)
#define CFG_SSM_PRINT_FAIL	(CFG_SSM_PRINT | CFG_SSM_PROC_FAIL)

/*
 * Note that the VME could be used in a polled manner
 * without any processor functions except the BIC.
 */
#define CFG_SSM_VME_FAIL	(CFG_SSM_VME | CFG_SSM_VTOS | CFG_SSM_STOV \
				 | CFG_SSM_VPIC | CFG_SSM_PROC_FAIL)

/*
 * SSM cdun_ssm biff type and flags defines.
 */
#define CFG_SSM_BTYPE_NONE     0x00
#define CFG_SSM_BTYPE_VBIF     0x01

                                        /* controller failure */
#define CFG_SSM_SIC     0x10000000      /* SIC failure */
#define CFG_SSM_DL      0x20000000      /* Datalink failure */
#define CFG_SSM_BIFF    0x40000000      /* No Biff present */

/*
 * Non fatal diag bits.
 */
#define CFG_SSM_NON_FATAL (CFG_SSM_PRINT | CFG_SSM_BIFF)
#define CFG_SSM_FATAL (~CFG_SSM_NON_FATAL)


/*
 * SCED cd_diag_flag settings
 */
#define	CFG_S_SCSI	0x00000010	/* scsi diagnostic failure */
#define	CFG_S_ETHER	0x00000020	/* ether diagnostic failure */
#define	CFG_S_SLIC	0x00000040	/* slic diagnostic failure */
#define	CFG_S_TOD	0x00000080	/* tod diagnostic failure */
#define	CFG_PORT0	0x00000100	/* port 0 is failed */
#define	CFG_PORT1	0x00000200	/* port 1 is failed */
#define	CFG_SINGLE	0x00000400	/* single ended interface is bad */
#define	CFG_DIFF	0x00000800	/* differential interface is bad */

/*
 * Console designation: cd_sc_cons values
 */
#define	CDSC_NOT_CONS	0		/* not attached to frontpanel */
#define	CDSC_LOCAL	1		/* local console */
#define	CDSC_REMOTE	2		/* remote console */
#define	CDSC_AUX	3		/* AUX console (unused) */

/*
 * CLOCK-ARBITRATION/DATA-MOVER
 */
#define	cd_ca_sysid	cd_un.cdun_cadm.cduc_sysid
#define	cd_ca_fptype	cd_un.cdun_cadm.cduc_fptype

/*
 * CADM cd_diag_flag settings
 */
#define	CFG_C_SLIC	0x00000010	/* SLIC subsystem failure */
#define	CFG_C_ARB	0x00000020	/* arbitration logic failure */
#define	CFG_C_DEAD	0x00000040	/* CLKARB failures prevent booting */
#define	CFG_C_SYSID	0x00000080	/* System id# looks bad */
#define	CFG_C_BAD_DM	0x00000100	/* DM Data mover has a problem */
#define	CFG_C_BAD_DM_0	0x00000200	/* DM Table 0 was in use */
#define	CFG_C_BAD_DM_1	0x00000400	/* DM Table 1 was in use */
#define	CFG_C_NO_STOP	0x00000800	/* DM Data Mover won't stop */
#define	CFG_C_BAD_DM_TO	0x00001000	/* DM Data Mover Timeout Occurred */
#define	CFG_C_ACCERR_S	0x00002000	/* DM Access error detected in SLIC */
#define	CFG_C_OPT_PRI	0x00008000	/* use optional arbitration priority */
#define	CFG_C_BAD_DATA	0x00010000	/* DM Data miscompare occurred */
#define	CFG_C_TBL_CORPT	0x00020000	/* DM Table addresses corrupted */
#define	CFG_C_ACCERR_T	0x00040000	/* DM Status byte access err detected */
#define	CFG_C_NO_STATOK	0x00080000	/* DM Status byte corrupted */

/*
 * Struct ctlr_type_desc provides per-controller-type information
 */
struct ctlr_type_desc {
	char	*ct_name;		/* controller name */
	unchar	ct_type;		/* controller type */
	unchar	ct_print_order;		/* order to print this */ 
	unchar	ct_nbics;
	struct bic_desc	*ct_bics;	/* BICs for this controller */
	unchar	ct_nbdps;
	struct bdp_desc	*ct_bdps;	/* BDPs for this controller */
	unchar	ct_ncmcs;
	struct cmc_desc	*ct_cmcs;	/* CMCs for this controller */
	int	ct_flags;		/* flags for this device */
	int	(*ct_pr_config)();	/* print group's config info */
	int	(*ct_test)();		/* test function */
	int	(*ct_init)();		/* boot-time init function */
	int	(*ct_bld_config)();	/* boot-time config builder */
	int	(*ct_disable)();	/* post-boot disabler */
	int	(*ct_read_buserr)();	/* bus error register reader */
	int	(*ct_clr_buserr)();	/* bus error clearer */
	int	(*ct_read_accerr)();	/* access error register reader */
	int	(*ct_clr_accerr)();	/* access error clearer */
	int	(*ct_cacheop)();	/* cache operation (flush, inval) */
	int	(*ct_diagnose)();	/* diagnose failure at gross level */
	int	ct_spare[4];		/* spare words */
};

/* ct_flags values */
#define	CTF_ISPROC	0x00000001	/* is a processor-like thing */
#define	CTF_ISMEM	0x00000002	/* is a memory */
#define	CTF_ISOEM	0x00000004	/* is an OEM controller */
#define	CTF_BEXT	0x00000010	/* understands extended protocols */
#define	CTF_WIDE	0x00000020	/* can handle 64-bit transactions */
#define	CTF_BINIT	0x00000040	/* is a bus initiator */
#define	CTF_BRESP	0x00000080	/* is a bus responder */
#define	CTF_ODDONLY	0x00000100	/* BIC/buserr logic on odd SLIC only */
#define	CTF_B32		0x00000200	/* requires 32-byte ownership blocks */
#define	CTF_HIPRI	0x00000400	/* belongs in a high-priority slot */
#define	CTF_LOWPRI	0x00000800	/* belongs in a low-priority slot */
#define	CTF_NEEDMEM	0x00001000	/* powerup test requires memory */
#define	CTF_STD_HEADER	0x00002000	/* print with standard format header */
#define	CTF_COMPAT	0x00004000	/* requires compatibility mode */
#define	CTF_EVENONLY	0x00008000	/* BIC/buserr logic on even SLIC only */

/* ct_print_order values */
#define	CTP_FIRST	1		/* print these first */
#define	CTP_MIDDLE	2		/* print these in the middle */
#define	CTP_LAST	3		/* print these last */

/* ct_cacheops commands */
#define	CTCO_FLUSH	0		/* flush the cache (copy-back) */
#define	CTCO_INVAL	1		/* invalidate cache contents */
#define	CTCO_SLOW_INVAL	2		/* slow-inval cache contents */

#define	BIC_SLIC(slicid, flags) \
	(((flags) & CTF_ODDONLY)? ((slicid) | 1) : \
		((flags) & CTF_EVENONLY)? ((slicid) & ~1): slicid)

#define	CT_NTYPES	256		/* 256 potential controller types */

/*
 * Describe a BIC for a controller.
 *
 * Since the decision to use compatibility mode is made
 * dynamically by the SCED firmware, some of the bits in
 * some of the BIC and BDP registers are never specified below.
 */
struct bic_desc {
	unchar	bic_i;			/* The i'th BIC on this controller */
	unchar	bic_slave;		/* reg number of BIC, 0 => !there */
	unchar	bic_chanctl;		/* setting of channel control reg */
	unchar	bic_throttle;		/* setting of throttle register */
	unchar	bic_policy;		/* setting of policy register */
	unchar	bic_errctl;		/* setting of error control register */
	unchar	bic_flags;		/* options regarding this BIC */
	unchar	bic_spareb;
	char	*bic_name;		/* name; e.g. "BIC" or "BIC 0" */
	long	bic_spare[2];
};

#define	BICF_TIMEOUT	0x01		/* This BIC can do timeouts */

/* Loop through all the BICs in a bic_desc table.  */
#define	BIC_LOOP(bicp,bic_desc) \
	for ((bicp) = (bic_desc); (bicp)->bic_slave; ++(bicp))

/*
 * Describe a CMC attached to a BIC.
 */
struct cmc_desc {
	unchar	cmc_i;			/* The i'th CMC on this BIC */
	unchar	cmc_slave;		/* SLIC slave reg of CMC */
	unchar	cmc_mode;		/* mode control bits to use */
	unchar	cmc_parms;		/* CMC cache parameters settings */
	char	*cmc_name;		/* name of this CMC */
	unchar	cmc_iface;		/* interface control bits to use */
	char	cmc_spareb[3];
	long	cmc_spare[1];
};

/* Loop though all the CMCs in a cmc_desc table.  */
#define	CMC_LOOP(cmcp, cmcs) for ((cmcp) = (cmcs); (cmcp)->cmc_slave; ++(cmcp))

/*
 * Describe a BDP attached to a BIC.
 */
struct bdp_desc {
	unchar	bdp_i;			/* The i'th BDP on this BIC */
	unchar	bdp_slave;		/* SLIC slave reg of BDP */
	unchar	bdp_throttle;		/* throttle count to use */
	unchar	bdp_conf;		/* BDP config register setting */
	char	*bdp_name;		/* name of this BDP; e.g. "low BDP" */
	long	bdp_spare[2];
};

/* Loop though all the BDPs in a bdp_desc table.  */
#define	BDP_LOOP(bdpp, bdps) for ((bdpp) = (bdps); (bdpp)->bdp_slave; ++(bdpp))



struct cfg_com {
	char	     *c_next;		/* pointer to next entry */
	uint	      c_diag_flag;	/* bit-mapped diagnostic failures */
	unchar	      c_type;		/* Basic Type of entry */
};

/* Configuration Structure Types */
#define	CFG_BOOT	1		/* BOOT */
#define	CFG_PROC	2		/* PROCESSOR */
#define	CFG_MMEM	3		/* MEMORY */
#define	CFG_SCSI	4		/* SCSI/ETHER */
#define	CFG_MBAD	5		/* MULTIBUS ADAPTER */
#define	CFG_CLKARB	6		/* CLOCK/ARBITER */
#define	CFG_ZDC		7		/* ZDC */
#define	CFG_UNKN	100		/* CUSTOMER'S BOARD */

struct cfg_proc {			/* Processor Board */
	struct cfg_com cfg_com;			/* common stuff */
	unchar	       pr_slic_addr;		/* 6 bit SLIC address */
	unchar	       pr_srev;			/* software rev level */
	unchar	       pr_hrev;			/* hardware rev level */
};

struct cfg_mmem {			/* Memory Board */
	struct cfg_com cfg_com;			/* common stuff */
	unchar	       me_slic_addr;		/* 6 bit SLIC address */
	uint   	       me_start_addr;		/* Memory starting address */
	uint   	       me_size;			/* Memory board size */
	unchar	       me_interleave;		/* non-zero if interleaved */
	unchar	       me_type;			/* chip type */
	unchar	       me_srev;			/* software rev level */
	unchar	       me_hrev;			/* hardware rev level */
};

struct cfg_scsi {			/* SCSI/ETHER board */
	struct cfg_com cfg_com;			/* common stuff */
	unchar         sc_slic_addr;		/* 6 bit SLIC address */
	unchar         sc_enet_addr[6];		/* 48 bit ETHER address */
	unchar         sc_host_num;		/* SCSI host number */
	unchar         sc_is_console;		/* True (non zero) if console */
	char 	      *sc_init_queue;		/* Pointer to initial queue */
	ushort         sc_version;		/* Firmware version */
	unchar         sc_srev;			/* software rev level */
	unchar         sc_hrev;			/* hardware rev level */
};

struct cfg_mbad {			/* Multibus Adapter */
	struct cfg_com cfg_com;			/* common stuff */
	unchar         mb_slic_addr;		/* 6 bit SLIC address */
	ushort         mb_version;		/* Firmware version */
	unchar         mb_srev;			/* software rev level */
	unchar         mb_hrev;			/* hardware rev level */
};

struct cfg_unkn {			/* Customer Board */
	struct cfg_com cfg_com;			/* common stuff */
	unchar         un_slic_addr;		/* 6 bit SLIC address */
	unchar         un_type;			/* board type */
	unchar         un_srev;			/* software rev level */
	unchar         un_hrev;			/* hardware rev level */
};

struct cfg_clkarb {			/* Clock/Arbiter Board */
	struct cfg_com	cfg_com;		/* common stuff */
	unchar	       clk_slic_addr;		/* 6 bit SLIC address */
	uint	       clk_sysid;		/* system ID number */
	unchar	       clk_fptype;		/* front panel type */
	unchar	       clk_srev;		/* software rev level */
	unchar	       clk_hrev;		/* hardware rev level */
};

struct cfg_zdc {			/* ZDC Controller Board */
	struct cfg_com cfg_com;			/* common stuff */
	unchar         zdc_slic_addr;		/* 6 bit SLIC address */
	ushort         zdc_version;		/* Firmware version */
	unchar         zdc_srev;		/* software rev level */
	unchar         zdc_hrev;		/* hardware rev level */
};

struct cfg_ssm {			/* System Services Module */
	struct cfg_com cfg_com;			/* common stuff */
	unchar         sm_slic_addr;		/* 6 bit SLIC address */
	unchar         sm_host_num;		/* SCSI host number */
	unchar         sm_is_console;		/* True (non zero) if console */
	ushort         sm_version;		/* Firmware version */
	unchar         sm_srev;			/* software rev level */
	unchar         sm_hrev;			/* hardware rev level */
};

struct cfg_boot {			/* BOOT */
	struct cfg_com	cfg_com;		/* common stuff */
	uint     b_boot_flag;		/* Boot flags */
	char 	         b_boot_name[BNAMESIZE]; /* Boot image name */
	int	        *b_pagetables;		/* page tables for 4M+LR */
	char		*b_bottom;		/* bottom of all used info */
	unchar	         b_clock_rate;		/* system clock rate in MHz */
	unchar	         b_console;		/* power-up console port */
	char		 b_kill;		/* kill character */
	char		 b_erase;		/* erase character */
	char		 b_interrupt;		/* interrupt character */
	unchar	         b_cfg_version;		/* version of this file */
	int		 b_memsize;		/* total of configured mem */
	int		 b_mmap[(MC_MAPSIZE+MC_BPI-1)/MC_BPI]; /* mem bitmap */
	struct cfg_proc *b_cfg_proc;		/* pointer to the procs */
	int		 b_num_proc;		/* number of processors */
	struct cfg_mmem *b_cfg_mmem;		/* pointer to the main mems */
	int		 b_num_mmem;		/* number of main memorys */
	struct cfg_scsi *b_cfg_scsi;		/* pointer to the scsis */
	int		 b_num_scsi;		/* number of scsis */
	struct cfg_mbad *b_cfg_mbad;		/* pointer to the mbads */
	int		 b_num_mbad;		/* number of mbads */
	struct cfg_unkn *b_cfg_unkn;		/* pointer to the unkns */
	int		 b_num_unkn;		/* number of unkns */
	struct cfg_clkarb *b_cfg_clkarb;	/* pointer to the clkarbs */
	int		 b_num_clkarb;		/* number of clkarbs */
	struct cfg_zdc  *b_cfg_zdc;		/* pointer to the zdcs */
	int		 b_num_zdc;		/* number of zdcs */
};

/* for bootstrap flags */
#define	CFG_PORT0_CRT	0x00000010	/* port 0 is a CRT */
#define	CFG_PORT1_CRT	0x00000020	/* port 1 is a CRT */
#define	CFG_PORTE_CRT	0x00000040	/* port on ether is a CRT */
#define	CFG_NEW_CFG	0x00000080	/* new config tables were also built */

/* console port definitions */
#define	CON_LOCAL	0		/* serial port 0 */
#define	CON_REMOTE	1		/* serial port 1 */
#define	CON_ETHER	2		/* ethernet */

/*
 * Bin for console putchar.
 */
#define	PUTCHAR_BIN	1	/* for SCSI putchar()'s */

/*
 * Bin for cross-processor interrupts.
 */
#define XINTR_BIN	7

struct cfg_ptr {		/* Pointers to the Configuration List */
	char *bot_cfg;			/* pointer to lowest byte of data */
	struct cfg_boot *head_cfg;	/* pointer to head of list */
	char *top_cfg;			/* pointer to highest byte+1 */
};

/*
 * The reboot structure is used to define what is to
 * be loaded when a reboot occurs. It is used with commands to
 * the board device. It lives here because of its dependency on BNAMESIZE.
 */
struct reboot {
	unchar	 re_powerup;		/* reference powerup defaults */
	uint     re_boot_flag;		/* Boot flags */
	unchar	*re_cfg_addr[2];	/* address to build config */
	char 	         re_boot_name[2][BNAMESIZE];	/* Boot image name */
};

/*
 * Since ioctl has a 128 byte transfer limit, the ioctl commands
 * to get/set the reboot structure via the console memory driver need
 * to transfer partial reboot structures. The ioctl_reboot structure
 * is broken up to only get/set one re_boot_name.
 */ 
struct ioctl_reboot {
	unchar	 re_powerup;		/* reference powerup defaults */
	uint     re_boot_flag;		/* Boot flags */
	unchar	*re_cfg_addr;		/* address to build config */
	char		 re_boot_name[BNAMESIZE];	/* Boot image name */
};

/*
 * Defines for re_powerup in struct reboot and struct ioctl_reboot.
 */
#define RE_TEMP_FLAG	0		/* Use temporary copy of reboot info */
#define RE_PERM_FLAG	1		/* Use permanent copy of reboot info */

/*
 * Special Minor device number pertaining to the 
 * system controllers (SCED/SSM).
 */
#define CTLR_MINOR	64

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_CFG_H */
