/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_DOSFS_BPB_H       /* wrapper symbol for kernel use */
#define _FS_DOSFS_BPB_H       /* subject to change without notice */

#ident	"@(#)kern-i386:fs/dosfs/bpb.h	1.1"
#ident  "$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *  BIOS Parameter Block (BPB) for DOS 3.3
 */
struct bpb33 {
	u_short bpbBytesPerSec;	/* bytes per sector			*/
	u_char bpbSecPerClust;	/* sectors per cluster			*/
	u_short bpbResSectors;	/* number of reserved sectors		*/
	u_char bpbFATs;		/* number of FATs			*/
	u_short bpbRootDirEnts;	/* number of root directory entries	*/
	u_short bpbSectors;	/* total number of sectors		*/
	u_char bpbMedia;	/* media descriptor			*/
	u_short bpbFATsecs;	/* number of sectors per FAT		*/
	u_short bpbSecPerTrack;	/* sectors per track			*/
	u_short bpbHeads;	/* number of heads			*/
	u_short bpbHiddenSecs;	/* number of hidden sectors		*/
};

/*
 *  BPB for DOS 5.0
 *  The difference is bpbHiddenSecs is a short for DOS 3.3,
 *  and bpbHugeSectors is not in the 3.3 bpb.
 */
struct bpb50 {
	u_short bpbBytesPerSec;	/* bytes per sector			*/
	u_char bpbSecPerClust;	/* sectors per cluster			*/
	u_short bpbResSectors;	/* number of reserved sectors		*/
	u_char bpbFATs;		/* number of FATs			*/
	u_short bpbRootDirEnts;	/* number of root directory entries	*/
	u_short bpbSectors;	/* total number of sectors		*/
	u_char bpbMedia;	/* media descriptor			*/
	u_short bpbFATsecs;	/* number of sectors per FAT		*/
	u_short bpbSecPerTrack;	/* sectors per track			*/
	u_short bpbHeads;	/* number of heads			*/
	u_long bpbHiddenSecs;	/* number of hidden sectors		*/
	u_long bpbHugeSectors;	/* number of sectrs if bpbSectors == 0	*/
};

/*
 *  The following structures represent how the bpb's look
 *  on disk.  shorts and longs are just character arrays
 *  of the appropriate length.  This is because the compiler
 *  forces shorts and longs to align on word or halfword
 *  boundaries.
 */
#define	getushort(x)	*((unsigned short *)(x))
#define	getulong(x)	*((unsigned long *)(x))

/*
 *  BIOS Parameter Block (BPB) for DOS 3.3
 */
struct byte_bpb33 {
	char bpbBytesPerSec[2];	/* bytes per sector			*/
	char bpbSecPerClust;	/* sectors per cluster			*/
	char bpbResSectors[2];	/* number of reserved sectors		*/
	char bpbFATs;		/* number of FATs			*/
	char bpbRootDirEnts[2];	/* number of root directory entries	*/
	char bpbSectors[2];	/* total number of sectors		*/
	char bpbMedia;		/* media descriptor			*/
	char bpbFATsecs[2];	/* number of sectors per FAT		*/
	char bpbSecPerTrack[2];	/* sectors per track			*/
	char bpbHeads[2];	/* number of heads			*/
	char bpbHiddenSecs[2];	/* number of hidden sectors		*/
};

/*
 *  BPB for DOS 5.0
 *  The difference is bpbHiddenSecs is a short for DOS 3.3,
 *  and bpbHugeSectors is not in the 3.3 bpb.
 */
struct byte_bpb50 {
	char bpbBytesPerSec[2];	/* bytes per sector			*/
	char bpbSecPerClust;	/* sectors per cluster			*/
	char bpbResSectors[2];	/* number of reserved sectors		*/
	char bpbFATs;		/* number of FATs			*/
	char bpbRootDirEnts[2];	/* number of root directory entries	*/
	char bpbSectors[2];	/* total number of sectors		*/
	char bpbMedia;		/* media descriptor			*/
	char bpbFATsecs[2];	/* number of sectors per FAT		*/
	char bpbSecPerTrack[2];	/* sectors per track			*/
	char bpbHeads[2];	/* number of heads			*/
	char bpbHiddenSecs[4];	/* number of hidden sectors		*/
	char bpbHugeSectors[4];	/* number of sectrs if bpbSectors == 0	*/
};

#if defined(__cplusplus)
        }
#endif

#endif /* _FS_DOSFS_BPB_H */

