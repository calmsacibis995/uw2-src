/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_DOSFS_DOSFS_LBUF_H       /* wrapper symbol for kernel use */
#define _FS_DOSFS_DOSFS_LBUF_H       /* subject to change without notice */

#ident	"@(#)kern-i386:fs/dosfs/dosfs_lbuf.h	1.1"
#ident  "$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *
 *  Written for System V Release 4	(ESIX 4.0.4)	
 *
 *  Gerard van Dorth	(gdorth@nl.oracle.com)
 *  Paul Bauwens	(paul@pphbau.atr.bso.nl)
 *
 *  May 1993
 *
 *  This software is provided "as is".
 *
 *  The author supplies this software to be publicly
 *  redistributed on the understanding that the author
 *  is not responsible for the correct functioning of
 *  this software in any circumstances and is not liable
 *  for any damages caused by this software.
 *
 */

#include <sys/kmem.h>

/*
 * logical block I/O layer 
 * saves mapping junk for the moment
 */

/*
 * reserve pointers to handle 16K clustersize
 */

#ifndef MAXCLUSTBLOCKS
#define MAXCLUSTBLOCKS	32
#endif

typedef struct	lbuf {
	union {
		caddr_t b_addr;	/* pointer to logical buffer */
	} b_un;
	size_t	b_bsize;	/* size of logical buffer    */
	buf_t	*bp[MAXCLUSTBLOCKS];	/* pointers to physical buffers */
} lbuf_t;


/* prototypes for dosfs low-level interface functions */

extern	lbuf_t *lbread(dev_t dev, daddr_t blkno, long int bsize);
extern	lbuf_t *lbreada(register dev_t dev, daddr_t blkno, daddr_t rablk, long int bsize);
extern	lbuf_t *lgetblk(register dev_t dev, daddr_t blkno, long int bsize);
extern	void	lbwrite(lbuf_t *lbp);
extern	void	lbawrite(lbuf_t *lbp);
extern	void	lbdwrite(lbuf_t *lbp);
extern	void	lbrelse(lbuf_t *lbp);
extern	void	lclrbuf(lbuf_t *lbp);
extern	int	lbiowait(lbuf_t *lbp);
extern	int	lbwritewait(lbuf_t *lbp);
extern	void	lbiodone(lbuf_t *lbp);
extern	int	lgeterror(lbuf_t *lbp);

#if defined(__cplusplus)
        }
#endif

#endif /* _FS_DOSFS_DOSFS_LBUF_H */

