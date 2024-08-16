/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */


#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/ufsdump/dumprestore.h	1.1.6.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fs.d/ufs/ufsdump/dumprestore.h,v 1.1 91/02/28 17:30:15 ccs Exp $"

/*
 * TP_BSIZE is the size of file blocks on the dump tapes.
 * Note that TP_BSIZE must be a multiple of DEV_BSIZE.
 *
 * NTREC is the number of TP_BSIZE blocks that are written
 * in each tape record. HIGHDENSITYTREC is the number of
 * TP_BSIZE blocks that are written in each tape record on
 * 6250 BPI or higher density tapes.  CARTRIDGETREC is the
 * number of TP_BSIZE blocks that are written in each tape
 * record on cartridge tapes.
 *
 * TP_NINDIR is the number of indirect pointers in a TS_INODE
 * or TS_ADDR record. Note that it must be a power of two.
 */

#define TP_BSIZE	1024
#define NTREC   	10
#define HIGHDENSITYTREC	32
#define CARTRIDGETREC	63
#define TP_NINDIR	(TP_BSIZE/2)

#define TS_TAPE 	1
#define TS_INODE	2
#define TS_BITS 	3
#define TS_ADDR 	4
#define TS_END  	5
#define TS_CLRI 	6
#define OFS_MAGIC   	(int)60011
#define NFS_MAGIC   	(int)60012
#define CHECKSUM	(int)84446

union u_spcl {
	char dummy[TP_BSIZE];
	struct	s_spcl {
		int	c_type;
		time_t	c_date;
		time_t	c_ddate;
		int	c_volume;
		daddr_t	c_tapea;
		ino_t	c_inumber;
		int	c_magic;
		int	c_checksum;
		struct	dinode	c_dinode;
		int	c_count;
		char	c_addr[TP_NINDIR];
	} s_spcl;
} u_spcl;

#define spcl u_spcl.s_spcl

#define	DUMPOUTFMT	"%-s %c %s"		/* for printf */
						/* name, incno, ctime(date) */
#define	DUMPINFMT	"%s %c %[^\n]\n"	/* inverse for scanf */
