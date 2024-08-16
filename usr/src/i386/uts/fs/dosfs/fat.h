/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_DOSFS_FAT_H       /* wrapper symbol for kernel use */
#define _FS_DOSFS_FAT_H       /* subject to change without notice */



#ident	"@(#)kern-i386:fs/dosfs/fat.h	1.1"
#ident  "$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *  Some useful cluster numbers.
 */
#define	DOSFSROOT	0	/* cluster 0 means the root dir		*/
#define	CLUST_FREE	0	/* cluster 0 also means a free cluster	*/
#define	DOSFSFREE	CLUST_FREE
#define	CLUST_FIRST	2	/* first legal cluster number		*/
#define	CLUST_RSRVS	0xfff0	/* start of reserved cluster range	*/
#define	CLUST_RSRVE	0xfff6	/* end of reserved cluster range	*/
#define	CLUST_BAD	0xfff7	/* a cluster with a defect		*/
#define	CLUST_EOFS	0xfff8	/* start of eof cluster range		*/
#define	CLUST_EOFE	0xffff	/* end of eof cluster range		*/

#define	FAT12_MASK	0x0fff	/* mask for 12 bit cluster numbers	*/
#define	FAT16_MASK	0xffff	/* mask for 16 bit cluster numbers	*/

/*
 *  Return true if filesystem uses 12 bit fats.
 *  Microsoft Programmer's Reference says if the
 *  maximum cluster number in a filesystem is greater
 *  than 4086 then we've got a 16 bit fat filesystem.
 */
#define	FAT12(pmp)	(pmp->vfs_maxcluster <= 4086)
#define	FAT16(pmp)	(pmp->vfs_maxcluster >  4086)

#define	DOSFSEOF(cn)	(((cn) & 0xfff8) == 0xfff8)

/*
 *  These are the values for the function argument to
 *  the function fatentry().
 */
#define	FAT_GET		0x0001		/* get a fat entry		*/
#define	FAT_SET		0x0002		/* set a fat entry		*/
#define	FAT_GET_AND_SET	(FAT_GET | FAT_SET)

/*
 *  This union is useful for manipulating entries
 *  in 12 bit fats.
 */
union fattwiddle {
	unsigned short word;
	unsigned char  byte[2];
};

#if defined(_KERNEL)
int pcbmap (struct denode *dep, u_long findcn, u_long *bnp, u_long *cnp);
int clusterfree (struct dosfs_vfs *pvp, u_long cluster);
int clusteralloc (struct dosfs_vfs *pvp, u_long *retcluster, u_long fillwith);
int fatentry (int function, struct dosfs_vfs *pvp, u_long cluster, u_long *oldcontents, u_long newcontents);
int freeclusterchain (struct dosfs_vfs *pvp, u_long startcluster);

#endif /* defined(_KERNEL) */


#if defined(__cplusplus)
        }
#endif

#endif /* _FS_DOSFS_FAT_H */

