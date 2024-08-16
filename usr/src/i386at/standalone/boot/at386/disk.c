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
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)stand:i386at/standalone/boot/at386/disk.c	1.2.2.9"
#ident	"$Header: $"
#ident "@(#) (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990"

/*
 *	disk.c - disk driver for the boot program.
 */
#include <sys/types.h>
#include <sys/bootinfo.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/vtoc.h>
#include <sys/alttbl.h>
#include <sys/fdisk.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/fs/bfs.h>
#include <boothdr/boot.h>
#include <boothdr/bootlink.h>
#include <boothdr/vxvm_boot.h>


#include <boothdr/bfsconf.h>
#include <boothdr/libfm.h>

ushort	bootdriv;	/* boot drive indicator		*/

struct	gcache gcache;		/* disk cache header		*/
short	fd_adapt_lev;
unsigned char	*gbuf=0;		/* global disk buffer		*/

extern	int	bfs_rdblk_cnt;
extern 	daddr_t	part_start;	/* start of active partition 	*/
extern	short	bps;		/* bytes per sector 		*/
extern	short	spt;		/* disk sectors per track 	*/
extern	short	spc;		/* disk sectors per cylinder 	*/


/*
 * the following must be set up for use by the high level filesystem 
 * routines
 */

off_t	boot_delta = 0;		/* sector offset to filesystem 	*/
off_t	root_delta;		/* sector offset to filesystem 	*/
int 	vxvm_priv_slice;
int 	vxvm_priv_delta;

bfstyp_t boot_fs_type = UNKNOWN;/* boot file system type	*/
bfstyp_t root_fs_type = UNKNOWN;/* root file system type	*/

/* file containing boot (and kernel) parameters */
char	standboot[] = "boot";

#if !defined(SFBOOT)

/*
 * get_fs: 	initialize the driver; open the disk, find
 *		the root slice and fill in the global 
 *		variables and tables.
 */

#ifdef WINI

get_hdfs()
{
	extern	bd_getalts();
	struct pdinfo	*pip;
	struct vtoc	*vtp;
	struct mboot 	*mb;
	struct ipart	*ip;
	int		i;
	int		partid = 0;
	int		bootfs;

#ifdef HDTST
	struct ipart	*actpart_ptr;	/* pointer to active partition	*/
	struct		int_pb ic;
#endif

	if ( gbuf == 0 )
		gbuf = (unsigned char *)bt_malloc(GBUFSZ+4096);

	bootdriv = hdisk_id;	/* hard disk drive indicator */

#ifdef HDTST
/*	The HDTST is used to allow the system to boot up from
 *	the hard disk without installing the boot program onto
 * 	the hard disk. Instead, the boot program is a combination 
 *	of the floppy primary boot and the hard disk boot programs.
 *	This speeds up the testing of hard disk boot by installing
 *	this mixed boot program in the floppy, and starting the
 *	machine up from floppy.
 */
	ic.ax = 0x0800;
	ic.dx = bootdriv;
	ic.ds = 0;
	ic.intval = 0x13;
	if (doint(&ic)) {
		bootabort("get_hdfs: Can't get hard disk drive parameters");
	}
	spt = ic.cx & 0x3f;
	spc = (((ushort)(ic.dx & 0xff00) >> 8) + 1) * spt;

#ifdef BOOT_DEBUG
	printf("hd:get_hdfs: bps= 0x%x spt=0x%x spc=0x%x\n", bps, spt,spc);
#endif /* BOOT_DEBUG */

	disk( 0, physaddr(gbuf), (short)1 );
	actpart_ptr = (struct ipart *)&(gbuf[BOOTSZ]);
	for (i=0; i<FD_NUMPART; i++, actpart_ptr++) {
		if (actpart_ptr->bootid == B_ACTIVE)
			break;
	}
	if (i< FD_NUMPART) {
		part_start = actpart_ptr->relsect;
	} else {
		bootabort("disk: unable to locate active boot partition\n");
	}
#ifdef BOOT_DEBUG
	printf("fdisk info: bootid= 0x%x systid=0x%x part_start= 0x%x begcyl= 0x%x\n",
	actpart_ptr->bootid, actpart_ptr->systid, actpart_ptr->relsect,
	actpart_ptr->begcyl);
#endif /* BOOT_DEBUG */
#endif /* HDTST */

/* 	read in pdinfo 							*/

#ifdef BOOT_DEBUG
	if (BOOTENV->db_flag & BOOTDBG)
		printf("\nReading disk info (pdinfo) from sector %ld\n",
			part_start + HDPDLOC);
#endif /* BOOT_DEBUG */
	disk( part_start + HDPDLOC, physaddr(gbuf), (short)1 );
	pip = (struct pdinfo *)gbuf;

/*
 * 	look in vtoc to find start of stand|root partition
 */
	bootfs=-1;
	boot_delta = 0;
	vxvm_priv_slice = -1;	
	vtp = (struct vtoc *)&gbuf[pip->vtoc_ptr % bps];
	for (i = 0; i < (int)vtp->v_nparts; i++) {
		switch (vtp->v_part[i].p_tag){
		case V_STAND: 
			bootfs = i;	
			boot_fs_type = BFS;
			boot_delta = (off_t) vtp->v_part[i].p_start;
			break;
#ifdef HDTST
		case V_MANAGED_2:
			vxvm_priv_slice = i;	
			vxvm_priv_delta = (off_t) vtp->v_part[i].p_start;
			break;
#endif
		}
	}

	if (bootfs == -1) {
		bootabort("\nboot: No file system to boot from.");
	}


	/*
	 * read in alternate sector table
	 */
	bd_getalts(vtp, pip, part_start + (pip->alt_ptr >> 9));

	BTE_INFO.bootflags &= ~BF_FLOPPY;

	if (boot_fs_type == BFS) {
		bfs_rdblk_cnt = BLK_GBUF;
		bfsinit();
	}

#ifdef BOOT_DEBUG
	if (BOOTENV->db_flag & BOOTDBG)
		printf("get_hdfs: boot filesystem start sector %ld\n",boot_delta);
#endif 
	return(root_delta);
}

#endif

#if defined(HDTST)
get_fpfs()
{
#ifdef HDTST
	struct ipart	*actpart_ptr;	/* pointer to active partition	*/
	struct		int_pb ic;
#endif
	if ( gbuf == 0 )
		gbuf = (unsigned char *)bt_malloc(GBUFSZ+4096);


	bootdriv = BOOTDRV_FP;	/* floppy disk drive indicator */
	BTE_INFO.bootflags |= BF_FLOPPY;

#ifdef HDTST
	ic.ax = 0x0800;
	ic.dx = bootdriv;
	ic.ds = 0;
	ic.intval = 0x13;
	if (doint(&ic)) {
		bootabort("get_fpfs: Can't get diskette drive parameters");
	}
	spt = ic.cx & 0x3f;
	spc = (((ushort)(ic.dx & 0xff00) >> 8) + 1) * spt;
#endif

#ifdef BOOT_DEBUG
	printf("get_fpfs: bps= 0x%x spt=0x%x spc=0x%x\n", bps, spt,spc);
#endif 
	switch (spt) {
	case 9:
		/*
		 * for 3.5", 720K ds/ds floppy drive		
		 * set start of filesystem at the third cylinder		
		 */
		boot_delta = (off_t) (2 * spc);
		break;

	default:
		/*
		 * For all other types set start of filesystem at the
		 * start of the second cylinder
		 */
		boot_delta = (off_t) spc;
		break;
	}

		boot_fs_type = BFS;

/*		setup bfs logical block read count to be 1 track long	
 *		check for maximum blocks available in global disk buf
 */
		bfs_rdblk_cnt = (spt * bps / BFS_BSIZE);
		if (bfs_rdblk_cnt > BLK_GBUF)
			bfs_rdblk_cnt = BLK_GBUF;
		bfsinit();

#ifdef BOOT_DEBUG
	if (BOOTENV->db_flag & BOOTDBG)
		printf("get_fpfs: boot filesystem start sector %ld\n",boot_delta);
#endif 

	return(boot_delta);
}


#endif /* HDTST */
#else
get_sbffs()
{
	if ( gbuf == 0 )
		gbuf = (unsigned char *)bt_malloc(GBUFSZ+4096);

}
#endif /* SFBOOT */

/*
 * dread(): 	convert block number to sector number.
 *		read the given number of blocks into the global buffer.
 *		A guaranteed full-track read algorithm is applied for
 *		floppy read to increase the performance.
 */

dread(bno, bnocnt)
daddr_t	bno;
ulong	bnocnt;
{
	register int	offset;
	daddr_t		secno;
	int		tcount;
	ulong		totcnt;
	ulong		secpbno;
	int		xfer_bnocnt = bnocnt;

/*	return if the begin block number and the amount of cached data
 *	covers the requested amount
 */
	if ((bno == gcache.gc_bno) && (bnocnt <= gcache.gc_cnt))
		return(xfer_bnocnt);

/*	xlate block # to sector number				*/
	if (boot_fs_type == BFS)
		secpbno = BFS_BSIZE / NBPSCTR;

	totcnt = bnocnt * secpbno;
	secno  = bno * secpbno + (ulong) boot_delta;
#ifdef BOOT_DEBUG2
	if (BOOTENV->db_flag & LOADDBG)
		printf(" dread: bno= 0x%x bnocnt= 0x%x secno= 0x%x\n",
			bno, bnocnt, secno);
		goany();
#endif


#ifdef WINI
/*	read what has been requested 
 *	neglect any cached data since the hard disk is a faster device
 */
	bd_rd_sec(secno,totcnt);
#else /* WINI */

/*	deduct the total blocks that have been cached			*/
	if (bno == gcache.gc_bno) {
		tcount  = gcache.gc_cnt * secpbno;
		secno  = secno + tcount;
		offset = tcount * NBPSCTR;
		totcnt = (bnocnt - gcache.gc_cnt) * secpbno;
	} else 
		offset = 0;

/*	limit read count if not starting at track boundary
 *	ensure subsequent reads to fall at track boundary
 *	-- apply only to filesystem with buffer size that can adjust
 *	   to end at the track boundary
 */
	tcount = spt - (secno % spt);
	if (!(spt % secpbno) && !(tcount % secpbno)) {
/*		if NOT start at track boundary and spanning cross tracks*/
		if ((tcount != spt) && (tcount < totcnt)) {
			totcnt = tcount;	
/*			modify the requested transferred block count	*/
			xfer_bnocnt = totcnt / secpbno;
			if (bno == gcache.gc_bno)
				xfer_bnocnt += gcache.gc_cnt;
		}
	}

/*	sector transfer count - tcount has been calculated		*/
	for (;;) {
		if (tcount > totcnt)
			tcount = totcnt;
		disk(secno, physaddr(&gbuf[offset]), (short) tcount);
		if ((totcnt -= tcount) == 0)
			break;
		secno += tcount;
		offset += tcount * NBPSCTR;
		tcount = spt - (secno % spt);
	}

#endif /* WINI */

/*	update cache record					*/
	gcache.gc_bno = bno;
	gcache.gc_cnt = xfer_bnocnt;
	return(xfer_bnocnt);	
}



