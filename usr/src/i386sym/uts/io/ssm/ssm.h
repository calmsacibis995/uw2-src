/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_SSM_SSM_H	/* wrapper symbol for kernel use */
#define _IO_SSM_SSM_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/ssm/ssm.h	1.11"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <svc/clock.h>  /*REQUIRED*/
#include <util/map.h>	/*REQUIRED*/
#include <io/slic.h>	/*REQUIRED*/

#elif defined(_KERNEL) 

#include <sys/clock.h>  /*REQUIRED*/
#include <sys/map.h>	/*REQUIRED*/
#include <sys/slic.h>	/*REQUIRED*/

#endif /* _KERNEL_HEADERS */

/*
 * ssm.h
 *      Systems Services Module (SSM) definitions
 */

/*
 * This structure contains a single interrupt 
 * level/vector pair for an SSM VME bus as well 
 * as a pointer to the next such structure.
 */
struct ssm_vme_int {
	unchar 			level;
	long 			vector;
	struct ssm_vme_int 	*next;
};

/*
 * The following structure is used to manage
 * VME map ram allocation.  Their is a structure
 * each type of map ram.  Currently there is one
 * type of StoV map rams and four sets of VtoS
 * map rams (a16, a24, a32hi, a32lo).
 */
struct ssm_vme_maps {
	int	nmap_rams;		/* # of vme map rams of this type */
	volatile ulong *map_ram;	/* Address of an array of
					 * corresponding VME map rams. */
	struct map *rmap;		/* Address of a private space management
					 * map for allocating map rams. */ 
};

/*
 * One of these exists per active SSM.
 */
struct ssm_desc {
	const struct ctlr_desc *ssm_cd; /* descriptor for this SSM */
	struct cons_cb	*ssm_cons_cbs;	/* Base console command block for SSM */
	struct print_cb *ssm_prnt_cbs;	/* Base printer command block for SSM */
	struct ssm_misc	*ssm_mc;	/* Low level message block for SSM */
	unchar          ssm_target_in_use; /* Bitmap of SCSIbus targets which
                                            * are currently in use by drivers */

	/* Members for managing the attached VME bus */
	int		ssm_vme_alive;	/* Is the attached and usable? */
	paddr_t		ssm_vme_paddr;	/* Phys addr base of the VME window */
	ulong		ssm_vme_compar;	/* Value of the VMEbus comparitor */
	ulong		*ssm_pic_flush_reg; /* Virtual addr of PIC flush reg */
	struct ssm_vme_maps ssm_vme_map[5]; /* VME adapter map ram management */
	struct ssm_vme_int *ssm_vints;	/* Linked list of VME interrupt 
					 * levels and vectors in use. */
};

#define ssm_diag_flags  ssm_cd->cd_diag_flag
#define ssm_target_no   ssm_cd->cd_ssm_host_num
#define ssm_is_cons     ssm_cd->cd_ssm_cons
#define ssm_version     ssm_cd->cd_sc_version
#define ssm_slicaddr    ssm_cd->cd_slic
#define ssm_biff_type   ssm_cd->cd_ssm_biff_type
#define ssm_biff_flags  ssm_cd->cd_ssm_biff_flags

#define TODFREQ		(1000/HZ)

#define SSM_EXISTS(index)	\
		((index) < ssm_cnt && !FAILED(SSM_desc[index].ssm_cd))

#define SSM_IO_PIC_ADDR 0x68            /* SLIC slave address for PIC */
#define PIC_IO_REV_REG	1               /* SLIC subslave address for PIC
					 * revision number register. */
#define PIC_IO_ADDR_REG 3               /* SLIC subslave address for PIC
                                         * responder address register. */

#define SSM_READ_PIC_REV(slic) \
		(slic_rdSubslave(slic, SSM_IO_PIC_ADDR, PIC_IO_REV_REG))
#define GOOD_PIC_REV	(0x12)
#define SSM_BAD_PIC(ssmp)  (((ssmp)->ssm_cd->cd_type == SLB_SSM2BOARD) && \
 	    (SSM_READ_PIC_REV((ssmp)->ssm_cd->cd_slic) < GOOD_PIC_REV))


#define SSM_WDT_TIMEOUT 4	/* SSM Watchdog timer timeout values in secs */
				/* Watchdog timer reset every 1/2 second */

/*
 * Memory allignment definitions for the SSM.
 * Data structures transferred by the SSM CPU 
 * cannot cross a megabyte boundary and should
 * be 16 byte aligned.  DMA transfers are most
 * efficient when located upon 16 byte boundaries.
 */
#define	SSM_ALIGN_BASE	16		/* Align host/SSM shared data 
					 * structures to 16-byte boundary */
#define	SSM_ALIGN_XFER	16		/* Align xfers to 16-byte boundary */
#define	SSM_BAD_BOUND	(1 << 20)	/* Xfer can't cross meg. boundary */

#if defined(_KERNEL)

/* 
 * Global definitions associated with the SSM.
 */
extern struct ssm_desc *SSM_desc;	/* Array of SSM descriptors. */
extern int ssm_cnt;			/* #elements in SSM_desc table */
extern struct ssm_desc *SSM_cons;	/* SSM descriptor of console board. */

/*
 * Function declarations
 */
void return_fw(void);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_SSM_SSM_H_ */
