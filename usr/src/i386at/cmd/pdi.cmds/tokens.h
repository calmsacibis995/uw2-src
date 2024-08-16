/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_TARGET_TOKENS_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_TOKENS_H	/* subject to change without notice */

#ident	"@(#)pdi.cmds:tokens.h	1.2"
#ident	"$Header: $"

#define	MAX_LINE	128

#define	UNKNOWN		0
#define	BLOCK		1
#define	BYTES		2
#define	DISK		3
#define	DISKINFO	4
#define	FORMAT		5
#define	FORMAT_DEFECTS	6
#define	MDSELECT	7
#define	MDSENSE		8
#define	PHYSICAL	9
#define	RDDEFECT	10
#define	READ		11
#define	READCAP		12
#define	REASSIGN	13
#define	VERIFY		14
#define	WRITE		15
#define	TCINQ		16
#define	MKDEV		17
#define	COMMENT		18
#define	GENERIC		19
#define	TCLEN		20
#define	NUMTOKENS	21	/* Should be one beyond the largest token number */

#define	UNTOKEN		-2


#define	DISK_INFO_SZ	16

typedef struct disk_info {
	unsigned long di_size;	/* Number of blocks after formatting	*/
	unsigned long di_gapsz;	/* Gap Size for PD Sector		*/
	int di_tracks	: 16;	/* Number of tracks per cylinder	*/
	int di_sectors	: 16;	/* Number of sectors per track		*/
	int di_asec_t	: 16;	/* Alternate sectors per track		*/
	int di_bytes	: 16;	/* Number of bytes per sector		*/
} DISK_INFO_T;

typedef struct tokens {
	char string[32];
	int token;
} TOKENS_T;

extern int	get_data(),
		get_string(),
		get_token();
extern void	put_data(),
		put_token(),
		put_string();
extern FILE *	scriptopen();

#endif /* _IO_TARGET_TOKENS_H */
