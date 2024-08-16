/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)xrestore:dumprestor.h	1.3"

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft 	*/
/*	Corporation and should be treated as Confidential.	*/

/*
 *	dumprestor.h
 */

#define	NTREC		(20 * 512 / BSIZE)
#define MLEN    	16
#define MSIZ    	4096

#define TS_TAPE 	1
#define TS_INODE	2
#define TS_BITS 	3
#define TS_ADDR 	4
#define TS_END  	5
#define TS_CLRI 	6
#define MAGIC   	(short) 60011
#define CHECKSUM	(short) 84446

#pragma	pack(2)

struct	spcl
{
	short	c_type;
	time_t	c_date;
	time_t	c_ddate;
	short	c_volume;
	daddr_t	c_tapea;
	ino_t	c_inumber;
	short	c_magic;
	short	c_checksum;
	struct	dinode	c_dinode;
	short	c_count;
	char	c_addr[BSIZE];
};

struct	idates
{
	char	id_name[16];
	char	id_incno;
	time_t	id_ddate;
};

#pragma	pack()
