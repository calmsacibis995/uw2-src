/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_SSM_SSM_VME_H	/* wrapper symbol for kernel use */
#define	_IO_SSM_SSM_VME_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/ssm/ssm_vme.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * ssm_vme.h
 *      Systems Services Module (SSM) VMEbus definitions.
 */

#ifdef _KERNEL_HEADERS

#include <io/ssm/ssm.h> /* REQUIRED */
#include <fs/buf.h>	/* REQUIRED */
#include <io/slic.h>	/* REQUIRED */
#include <io/SGSproc.h>	/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/ssm.h> /* REQUIRED */
#include <sys/buf.h>	/* REQUIRED */
#include <sys/slic.h>	/* REQUIRED */
#include <sys/SGSproc.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#if defined(_KERNEL)

/*
 * The SSM VME occupies a 32Megabyte window of the 
 * physical address space.  Its location is programable
 * via the PIC on the SSM and is configurable via the
 * ssmvme_phys_addr table in the SSM's Space.c file. 
 */
#define SSMVME_ADDR_SPACE       (32*1024*1024)  /* 32M physical address space */
#define PA_SSMVME(i)		(ssmvme_phys_addr[i])
#define SSMVME_WINDOW(x) 	((PA_SSMVME(x) - PHYS_IO_SPACE) \
				 / SSMVME_ADDR_SPACE)

extern ulong ssmvme_phys_addr[];	/* Configurable base addresses of VME */
extern int ssmvme_phys_addr_cnt;	/* # entries in ssmvme_phys_addr */

#define SSM_VME_EXISTS(ssmp)   ((ssmp)->ssm_cd->cd_ssm_biff_type == \
                                CFG_SSM_BTYPE_VBIF)

/* 
 * Defintions for programming the SSM's I/O PIC
 * and VME mapping rams, which provide a buffered 
 * data path during VME data transfers.
 */
#define SSM_VME_PIC_MASK	0xfe000000 /* Sequent bus bits PIC decodes */
#define PIC_IO_ADDR_SHIFT 24		/* Sequent bus bits PIC ignores */

/*
 * ssm_pic_set( slic, window )
 * sets the PIC of the ssm whose slic is given to respond to the 32 megabyte
 * window which starts at (window & 0xff000000).  If the most significant bit
 * of window is 0, the PIC is turned off.
 */
#define SSM_PIC_SET(slic,window)	\
        slic_wrSubslave((slic),		\
		   SSM_IO_PIC_ADDR,	\
		   PIC_IO_ADDR_REG,	\
        	   (unchar) (((window)&SSM_VME_PIC_MASK) >> PIC_IO_ADDR_SHIFT));

#define PIC_REGISTER(base,reg) (*(ulong *)((ulong)(base) | 0x7f<<13 | (reg)))
#define PIC_REGISTER_ADDR(base,reg) ((paddr_t)(((ulong)base) | 0x7f<<13 | (reg)))
#define PIC_BCR_NARROW 		0x4c	  /* use with PIC_REGISTER as reg */
#define PIC_BCR_WIDE		0x6c	  /* wide PIC address for BCR */
#define PIC_BCR_EMPTY		0x10	  /* PIC buffers don't need flushing */
#define PIC_BCR_FLUSH		2	  /* Flush buffers */
#define PIC_BCR_FLUSH_ACK	1	  /* Buffers just flushed */

#define PIC_FLUSH_LOOPS		1000	  /* cycles to wait before panicing */

#define SSM_VME_COMP_MASK	0x7ff00000 /* comparator bits */

#define SSM_MAP_HIT 		1	  /* All maps--hit (valid) bit */

#define SSM_S2V_1ST_MAP 	64	  /* First usable sequent to vme map */
#define SSM_S2V_NMAPS		2048	  /* number of sequent to vme maps */
#define SSM_S2V_USABLE_MAPS	(SSM_S2V_NMAPS - SSM_S2V_1ST_MAP)
#define SSM_S2V_MAP_SIZE	16384	  /* size covered by a s2v map */
#define SSM_S2V_MAP_MASK	(-(long)SSM_S2V_MAP_SIZE)

#define SSM_VME_NMAPS		512	   /* # of maps per map set */
#define SSM_VME_A16_MAPPINGS	32	  /* # of distinct mappings */
#define SSM_VME_MAP_SIZE 	2048
#define SSM_VME_BYTES_PER_MAP 	SSM_VME_MAP_SIZE
#define SSM_VME_MAP_ADDR_MASK 	(-(long)SSM_VME_MAP_SIZE)

/* 
 * The following data structure is used by the SSM VME
 * to track VMEbus adapter map allocations.  There is
 * one of these associated with each map allocation.
 * The address of it may be returned to drivers as a
 * handle, but they should be oblivious to its contents.
 */
struct ssm_vme_mdesc {
	struct ssm_desc *ssmp;		/* SSM on which the mapping belongs */
	ushort mapset;			/* The set of adapter map rams used */
	ushort addmod;			/* Address modifier used (for remap) */
	ulong_t firstmap;		/* Index of 1st allocated map ram */
	volatile ulong_t *map_ram;	/* Address of the first map ram */
	size_t nmaps;			/* # map rams in the allocation */
	ulong_t nbytes;			/* # bytes in the requested map */
	caddr_t va;			/* Virtual addr if a physmap was done */
	paddr_t pa;			/* Physical vme address mapped */
	paddr_t vme_base;		/* VMEbus base address corresponding
					 * to the first map ram (V2S only)*/
	struct ssm_vme_mdesc *next;	/* Linked list for overlap detection */
};

/* 
 * Values for mapset.  All transfers between the Sequent bus
 * and VME space must be mapped by a set of SSM VME adapter
 * mapping rams, regardless of which side initiates it.  Each
 * initiator has its own map rams, with a set for each space
 * of VME initiated transfers.
 */
#define SSM_VME_A16_MAPS	0	/* VtoS DMA map of A16 map rams */
#define SSM_VME_A24_MAPS	1	/* VtoS DMA map of A24 map rams */
#define SSM_VME_A32_LO_MAPS	2	/* VtoS DMA map of low A32 map rams */
#define SSM_VME_A32_HI_MAPS	3	/* VtoS DMA map of high A32 map rams */
#define SSM_VME_S2V_MAPS	4	/* StoV programmed I/O map rams */
#define SSM_VME_NO_MAPS		5	/* No mapset currently in use */

/* 
 * External function definitions from ssm_vme.c.
 */
extern void ssm_vme_init(struct ssm_desc *);
extern void ssm_vme_start(struct ssm_desc *);
extern int ssm_vme_probe(int, int (*)(), void *);
extern void *ssm_vme_mdesc_alloc(int, ushort, int);
extern void ssm_vme_mdesc_free(void *);
extern void ssm_vme_release_map(void *);
extern caddr_t ssm_s2v_map(void *, paddr_t, uint, int);
extern paddr_t ssm_v2s_map(void *, caddr_t, uint, int);
extern paddr_t ssm_v2s_map_bp(void *, struct buf *, int);
extern paddr_t ssm_v2s_remap(void *, caddr_t, uint);
extern paddr_t ssm_v2s_remap_bp(void *, struct buf *);
extern int ssm_v2s_reserve(void *, uint, uint, int);
extern void ssm_vme_dma_flush(int);
extern int ssm_vme_intr_setup(int, int, int, unchar, unchar, unchar); 
extern int ssm_vme_assign_vec(int, int, unchar, unchar);

#endif /* _KERNEL */

/*
 * The following definitions will be used for specifying
 * the modifier for setting up SSM/VME tranfer mappings:  
 *
 * 	Bits zero thru 5 contain a VME address modifier, as
 *	described in the standard VMEbus specification.  
 *
 * 	Bits 8 and 9 contain rotation specifiers for transforming 
 *	data during tranfers.  They are specific to the SSM VME
 *	and are described in the SSM firmware functional spec.   
 *	For example, a value of zero passes the data through
 *	in completely rotated byte order, which is desirable on
 *	most data transfers since VME maps data bytes backwards
 *	from the Intel architecture.  A value of one passes data 
 *	thru in 1-to-1 byte order, which (for the reason stated
 *	for the value zero) may be desirable for communicating 
 *	integer values to an architecture that is mapped big-ndian 
 *	instead of little n-dian.  The transform selected may
 *	impact performance, since it may be possible to avoid
 *	having software perform byte swapping to compensate for 
 *	VME-to-sequent byte ordering differences.
 */
#define SSMVME_MODIFIER(rot,amod)	((rot) << 8 | (amod))
	/* Generate the space config data from a value for rotation bits
	 * and an address modifier. */
#define SSMVME_AMOD(x)		((x) & 0x3f)	
	/* Extracts address modifier field from config "space" data */
#define SSMVME_ROT(x)		((x) >> 8 & 3)
	/* Extracts rotation bits from config "space" data */

#define SSM_VME_PARS_ROTATE 0x0         /* ABCD -> DCBA */
#define SSM_VME_PARS_NOROTATE 0x1       /* ABCD -> ABCD */
#define SSM_VME_PARS_ROTATE_WORDS 0x2   /* ABCD -> CDAB */
#define SSM_VME_PARS_ROTATE_BYTES 0x3   /* ABCD -> BADC */

#define VME_SPACE(x)	((x) & 0x30)	/* Extract addr space from addr mod */
#define VME_A16_SPACE	0x20		/* Configured for A16 space */
#define VME_A24_SPACE	0x30		/* Configured for A24 space */
#define VME_A32_SPACE	0x00		/* Configured for A32 space */
#define VME_USER_SPACE	0x10		/* User defined - not supported */

#define VME_INVALID_ADDR  ((paddr_t)(0xffffffff)) /* Designated invalid address
						  * return for V-to-S mapping */

/* Possible return values from ssm_vme_intr_setup(): */
#define VME_INTMAP_OK 	0
#define VME_INVALID_BUS 1		/* Invalid SSM specified */
#define VME_INTLVL_BAD  2   		/* Invalid VME interupt level, */
#define VME_INTVEC_BAD  3		/* Invalid VME interupt vector, */
#define VME_INTMAP_BAD  4  		/* unable to complete the mapping;
					 * memory resources depleted. */

#define MAX_VME_VEC	255		/* VME vectors of 0..255 are usable */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_SSM_SSM_VME_H */
