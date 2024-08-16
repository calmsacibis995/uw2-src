/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)stand:i386at/standalone/boot/bootlib/sbfs.c	1.5"
#ident  "$Header: $"

#if defined(SFBOOT)
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/fs/bfs.h>
#include <sys/sysmacros.h>
#include <sys/bootinfo.h>
#include <boothdr/libfm.h>
#include <boothdr/boot.h>
#include <boothdr/bfsconf.h>


int	sbfs_rdblk_cnt;		/* runtime determined total block read count */
off_t 	fd;			/* global bfs file descriptor - file offset  */
off_t 	eof;			/* global end of file offset of fd           */
size_t 	fsize;			/* global size of file of fd                 */
typedef struct sbfs_d{
	char	fname[B_PATHSIZ];
	daddr_t	s_addr;
	daddr_t	e_addr;
}	sbfsdir_t;

sbfsdir_t	sbfs_dir[BOOT+1];

/*
 * SBFS - read routine
 *       convert file offset into logical block
 *	 taking care of file offset that may begin or end in
 *	 non-block boundary
 */
sbfsread(foffset,mem,rdbytes)
off_t foffset;
char *mem;
int rdbytes;
{
	off_t	off_srt;
	off_t	off_end;
	register ulong blk_srt;
	ulong	blk_end;
	register ulong blkcnt;
	int	blk_xfer;
	int	buf_off;
	int 	nbytes;
	register int byte_xfer;

	off_srt = fd + foffset;

	if ( off_srt >= eof )
		return 0;

	if ( off_srt + rdbytes > eof )
		rdbytes = eof - off_srt;

	nbytes = rdbytes;

/*	setup all the block count					*/
	off_end = off_srt + nbytes - 1;
	blk_srt = OFF2BLK(off_srt);
	blk_end = OFF2BLK(off_end);
	blkcnt  = blk_end - blk_srt + 1;

#ifdef SBFS_DEBUG
printf("sbfsreadd:\n");
printf("off_srt= 0x%x off_end= 0x%x blk_srt= 0x%x blk_end= 0x%x blkcnt= 0x%x\n", off_srt, off_end, blk_srt, blk_end, blkcnt);
#endif

/*	
 *	read in terms of block number
 *	taken care of non-block boundary file offset 
 *	before copying to the destination memory
 */
	buf_off = (int) BUFOFF(off_srt);
	for (;blkcnt>0;) {
/*	set block transfer to allocated buffer size			*/
		blk_xfer=sbfs_rdblk_cnt;
		if (blkcnt < blk_xfer)
			blk_xfer = blkcnt;
/*	dread returns the actual number of blocks transferred 
 *	taking care of track boundary condition
 */
		blk_xfer = dread(blk_srt,blk_xfer);
		byte_xfer = blk_xfer * BFS_BSIZE;
		byte_xfer -= buf_off;
		if (nbytes <= byte_xfer) 
			byte_xfer = nbytes;
		memcpy(physaddr(mem),physaddr(&gbuf[buf_off]),byte_xfer);
		mem += byte_xfer;
		blk_srt += blk_xfer;
		buf_off = 0;
		nbytes -= byte_xfer;
		blkcnt -=blk_xfer;
	}

#ifdef BOOT_DEBUG
	if (BOOTENV->db_flag & LOADDBG)
		if (nbytes != 0)
			printf("bfsread: Incomplete read\n");
#endif
	return(rdbytes);
}

/*
 * SBFS - initialization routine
 *	 cache the directory index 
 */
sbfsinit()
{
/*	set fd to zero for index block read		*/
	fd = 0;
	eof = sizeof(sbfs_dir) + 1;

	get_sbffs();	/* prep diskette support for subsequent access */
	sbfsread(0, &sbfs_dir, sizeof(sbfs_dir)); 


}

/*
 * SBFS - open routine
 */
sbfsopen(fname)
char *fname;
{
	sbfsdir_t	*dirp;

	extern sbfsdir_t *sbfs_search();

/*	scan through the file list cache first				*/
	if ((dirp = sbfs_search(fname)) != 0){

	/* set beginning file offset from the file system */
		fd = dirp->s_addr;
		eof = dirp->e_addr + 1;
		fsize = eof - fd;

		return(fd);
	}else
		return(-1);
}

/*
 *	search routine - locate filename in the sbfs file list structure
 *	return -1 if no filename match
 */
sbfsdir_t *
sbfs_search(fcomp )
char	*fcomp;
{
	sbfsdir_t	*sbfsdirp;

	for( sbfsdirp = sbfs_dir; sbfsdirp <= &sbfs_dir[SBF_DIRSIZ]; sbfsdirp++)
		if( strncmp( fcomp, &sbfsdirp->fname, B_PATHSIZ) == 0)
			return(sbfsdirp);

	return (0);
}

/*
 * SBFS - stat size routine
 */
int
sbfsstat_size()
{
	return( fsize);
}
#endif /* SFBOOT */
