/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* 
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ifndef _STAND_SCSIDISK_H
#define _STAND_SCSIDISK_H

#ident	"@(#)stand:i386sym/standalone/sys/scsidisk.h	1.1"

/*
 * scsidisk.h
 *	SCSI disk definitions shared between the 
 * 	disk driver and formatter.
 */

struct drive_type {
	
	char	*dt_diskname;	/* corresponds with disktab */
	char	*dt_vendor;	/* parts of ID string from INQUIRY cmd */
	char	*dt_product;
	struct	st dt_st;	/* disk geometry */
	struct	partition dt_part;	/* VTOC partition info */
	unchar	dt_inqformat;	/* format of INQUIRY return data */
	unchar	dt_reasslen;	/* bytes of defect data for REASSIGN BLOCKS */
	unchar	dt_formcode;	/* how to default format, CDB byte 1 */
	unchar	dt_pagecode;	/* page code for error recovery page */
	unchar	dt_pf;		/* byte 1 of CDB for MODE SELECT cmd */
	unchar	dt_tinymodelen;	/* length of short MODE SELECT command */
	unchar	dt_modebits;	/* which error recovery bits are set */
};

#define SCSIDISK_ADDRALIGN 	8	/* align I/O to 8-byte boundaries */
#define SCSIDISK_MAX_XFER	0x10000	/* SCSI xfer size limit, in blocks */

/*
 * This section defines the scsi disk ioctl interface.
 */

 /* format of a 6 byte command descriptor block (CDB) */
struct cmd6 {
	unchar cmd_opcode;
	unchar cmd_lun;
	unchar cmd_lba[2];
	unchar cmd_length;
	unchar cmd_cb;
};

/* format of a 10 byte command descriptor block (CDB) */
struct cmd10 {
	unchar cmd_opcode;
	unchar cmd_lun;
	unchar cmd_lba[4];
	unchar cmd_pad;
	unchar cmd_length[2];
	unchar cmd_cb;
};

/*
 * arguments to sdioctl().  This interface requires a packet which
 * contains the CDB in the first bytes and the data for the command
 * in the remaining bytes.
 */

struct scsiioctl {
	ulong	sio_datalength;
	ulong	sio_addr;
	ushort	sio_cmdlen;
	unchar	sio_dir;
	unchar	sio_pad;
	union {
		struct	cmd6 cmd6;
		struct	cmd10 cmd10;
	} sio_cmd;
};

#define sio_cmd6        sio_cmd.cmd6
#define sio_cmd10       sio_cmd.cmd10

struct reassarg {		/* used for REASSIGN BLOCKS */
	short  pad;
	unchar length[2];
	unchar defect[4];
};

struct readcarg {		/* used for READ CAPACITY */
	unchar nblocks[4];		/* highest addressable block on disk */
	unchar bsize[4];		/* formatted size of disk blocks */
};

#endif /* _STAND_SCSIDISK_H */
