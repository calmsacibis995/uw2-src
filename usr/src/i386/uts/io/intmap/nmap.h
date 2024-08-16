/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_INTMAP_NMAP_H	/* wrapper symbol for kernel use */
#define _IO_INTMAP_NMAP_H	/* subject to change without notice */

#ident	"@(#)kern-i386:io/intmap/nmap.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif
/*
 *	Copyright (C) The Santa Cruz Operation, 1988, 1989.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
							/* BEGIN SCO_INTL */
/* Channel mapping ioctl's */

#define	NMIOC  ('n'<< 8)	/* So not to conflict with LDIOCs */
#define NMSMAP (NMIOC|1)	/* set no-map */
#define NMGMAP (NMIOC|2)	/* get no-map */
#define NMNMAP (NMIOC|3)	/* clear no-map */

typedef	struct nmtab	*nmp_t;
typedef	struct nmseq	*nmsp_t;
typedef	unsigned char	*nmcp_t;

/* Nmap control structure */

struct nxmap {
	unsigned char	n_count;	/* Usage count of this map */
	nmp_t		n_p;		/* Ptr. to sequence table */
	struct buf	*n_bp;		/* Ptr. to table buffer header */
};

/* Nmap sequence table structs */

struct nmtab {
	unsigned char	n_iseqs;	/* Number of input sequences */
	unsigned char	n_aseqs;	/* Number of input + output sequences */
	short		n_seqidx[1];	/* Array of sequence offsets */
};

struct nmseq {
	unsigned char	n_nmcnt;	/* Sequence trailer length */
	char		n_nmseq[1];	/* Sequence lead-in(null terminated) */
};

extern struct nxmap nxmap[];
							/* END SCO_INTL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_INTMAP_NMAP_H */
