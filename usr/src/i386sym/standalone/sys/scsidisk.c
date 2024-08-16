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

#ident	"@(#)stand:i386sym/standalone/sys/scsidisk.c	1.1"

/*
 * SCSI disk device driver (stand alone).
 *
 * This device driver assumes that it will run single user
 * single processor. There is *no* Mutual exclusion
 * done at all.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/boot.h>
#include <sys/cfg.h>
#include <sys/sysmacros.h>
#include <sys/vtoc.h>
#include <sys/saio.h>
#include <sys/ssm.h>
#include <sys/scsi.h>
#include <sys/scsidisk.h>
#include <sys/ccs.h>
#include <sys/ssm_scsi.h>

extern	struct drive_type drive_table[];

extern int 	ssm_scsi_diskinit(struct iob *);
extern void 	bzero(void *, size_t);
extern int     printf(const char *, ...);
extern int      strncmp(char *, char *, int);
extern int 	vtoc_setboff(struct iob *, int);

static void scsidiskopen(struct iob *);
static int scsidisk_cmd (struct iob *, struct scsiioctl *);


/*
 * Sequent host adapters need to be
 * told the "direction" of the transfer.  The following
 * table, indexed by opcode, provides this information.
 * Using a full table instead of a switch statement
 * allows us to do fully-general user specified "passthru"
 * commands.
 *
 * The value SDIR_DTOH (device to host) is the correct
 * default for commands that either read or don't transfer
 * data and so is used to initialize the table.
 */
static unchar sqnt_scsi_cmd_info [256] = {	     /* HEX OPCODE */
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,	/* 000 */
	SDIR_HTOD, SDIR_DTOH, SDIR_DTOH, SDIR_HTOD,
	SDIR_DTOH, SDIR_DTOH, SDIR_HTOD, SDIR_HTOD,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,

	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,	/* 010 */
	SDIR_DTOH, SDIR_HTOD, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_HTOD, SDIR_DTOH, SDIR_DTOH,

	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,	/* 020 */
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_HTOD, SDIR_HTOD,
	SDIR_DTOH, SDIR_DTOH, SDIR_HTOD, SDIR_HTOD,

	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,	/* 030 */
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_HTOD,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_HTOD,

	SDIR_DTOH, SDIR_HTOD, SDIR_DTOH, SDIR_DTOH,	/* 040 */
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_HTOD, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,

	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,	/* 050 */
	SDIR_DTOH, SDIR_HTOD, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,

	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,	/* 060 */
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,

	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,	/* 070 */
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,

	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,	/* 080 */
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,

	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,	/* 090 */
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,

	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,	/* 0A0 */
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,

	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,	/* 0B0 */
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,

	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,	/* 0C0 */
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,

	SDIR_DTOH, SDIR_HTOD, SDIR_DTOH, SDIR_DTOH,	/* 0D0 */
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,

	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,	/* 0E0 */
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_HTOD, SDIR_DTOH, SDIR_DTOH,

	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,	/* 0F0 */
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_DTOH, SDIR_DTOH, SDIR_DTOH,
	SDIR_DTOH, SDIR_HTOD, SDIR_HTOD, SDIR_DTOH,
};

/*
 * void
 * scsidisk_ssmopen(struct iob *)
 *	Open an SSM SCSI disk.
 *
 * Calling/Exit State:
 * 	Calls the SSM specific initialization routine,
 *	ssm_scsi_diskinit() to establish or locate previously
 *	established SSM/SCSI control structures for communicating
 *	with the specified drive.  
 *
 *	There is no return value, although iob->i_error may be
 *	set indicating a failure when control is passed to the
 *	generic SCSI disk open routine, scsidiskopen() to complete
 *	the non-SSM specific portions of the open.
 */
void
scsidisk_ssmopen(struct	iob *io)
{

	if (ssm_scsi_diskinit(io)) {
		SAIOSCSI_FLAG(io) = 0;
		return;
	}
	scsidiskopen(io);
	SAIOSCSI_FLAG(io) = 0;
}

/*
 * static void
 * scsidiskopen(struct iob *)
 *	 generic SCSI disk open procedure.
 *
 * Calling/Exit State:
 *	Assumes that the SCSI adapter has already spun
 *	the disk drive up, and that it is either ready 
 *	for use or not available at all.
 *
 *	Assumed that an adapter specific handle to adapter
 *	dependent resources for this device has been established
 *	and planted in the iob.
 *
 *	If the open succeeds, a pointer to the device types entry
 *	in driver_table is embedded in the iob for future reference.
 *
 *	Has no explicit return value, but will set iob->i_error
 *	to an errno if the device open fails.
 *
 * Description:
 * 	Issues a SCSI test-unit-ready command to determine if
 *	any device exists at the specified SCSI address.  If
 *	so, execute a SCSI inquiry command to retrieve data
 *	about its device type, vendor, etc.  Use this information
 *	to determine if the device is a supported disk drive,
 *	locating an entry for it in the drive_table.  Finally,
 *	invoke vtoc_setboff() to validate the specified sub-device
 *	to be either a valid partition on the device or refer to
 *	the entire, unpartitioned device.
 */
static void
scsidiskopen(struct iob *io)
{
	struct	drive_type *dp;
	struct	scsiioctl sioctl;
	struct	sdinq inq;

	/*
	 * Verify that the disk exists and is spun up.
	 */
	bzero(&sioctl, (size_t) sizeof(struct scsiioctl));
	sioctl.sio_cmdlen = SCSI_CMD6SZ;
	sioctl.sio_cmd6.cmd_opcode = SCSI_TEST;

	if (scsidisk_cmd(io, &sioctl)) {
		printf("%s(%d,0): TEST UNIT READY command failed\n",
			SAIOSCSI_PREFIX(io), io->i_unit);
		io->i_dptr = NULL;
		io->i_error = EIO;
		return;
	}

	/*
	 * Do an INQUIRY command to determine what kind of a disk
	 * we have.
	 */
	bzero((caddr_t) &sioctl, sizeof (struct scsiioctl));
	sioctl.sio_datalength = sizeof(struct sdinq);
	sioctl.sio_addr = (ulong)&inq;
	sioctl.sio_cmdlen = SCSI_CMD6SZ;
	sioctl.sio_cmd6.cmd_opcode = SCSI_INQUIRY;
	sioctl.sio_cmd6.cmd_length = sizeof (struct sdinq);

	if (scsidisk_cmd(io, &sioctl)) {
		printf("%s(%d,0): INQURIY command failed\n",
			SAIOSCSI_PREFIX(io), io->i_unit);
		io->i_dptr = NULL;
		io->i_error = EIO;
		return;
	}
	
	for (dp = drive_table; dp->dt_vendor; dp++) {
		if (strncmp(inq.sdq_vendor, dp->dt_vendor, SDQ_VEND) == 0 &&
		    strncmp(inq.sdq_product, dp->dt_product, SDQ_PROD) == 0)
			break;
	}

	/*
	 * If the drive type isn't known, complain and fail.
 	 * XXX - a future enhancement would be to create the
	 * information found in the driver table from data
 	 * returned by additional drive queries.
	 */
	if (!dp->dt_vendor) {
		int	i;
		char	*bp = inq.sdq_vendor;

		printf("%s(%d,0): vendor: ", SAIOSCSI_PREFIX(io), io->i_unit);
		for (i = 0; i < SDQ_VEND; i++)
			printf("%c", *bp++);
		printf("\n");

		bp = inq.sdq_product;
		printf("%s(%d,0): product: ", SAIOSCSI_PREFIX(io),
			io->i_unit);
		for (i = 0; i < SDQ_PROD; i++)
			printf("%c", *bp++);
		printf("\n");
	
		printf("%s(%d,0): could not find drive in drive_table\n", 
			SAIOSCSI_PREFIX(io), io->i_unit);
		io->i_dptr = NULL;
		io->i_error = EDEV;
		return;
	}

	SAIOSCSI_GEOM(io) = &dp->dt_st;
	
	/*
	 * Read the partition data.
	 */
	(void) vtoc_setboff(io, 0);
}

/*
 * void
 * scsidiskclose(struct iob *)
 * 	Close access to a previously opened SCSI disk.
 *
 * Calling/Exit State:
 *	Assumes the iob corresponds to a drive, previously
 *	opened via scsidiskopen(), in which case the iob
 *	contains a ptr to adapter specific info for managing
 *	the associated drive. 
 *
 *	No return value, but NULLs the iob private data 
 *	pointer to adapter specific info about the device.
 */
void
scsidiskclose(struct iob *io)
{
	
	io->i_dptr = NULL;
}

/*
 * int
 * scsidiskstrategy(struct iob *, int)
 *	Standard read/write routine for the SCSI disk driver.
 *
 * Calling/Exit State:
 *	"io" is a file descriptor; either an open one from
 *	the file table or a temporary one used when its unclear
 *	where the real one is.  If it doesn't already have an
 *	adapter specific data structure attached to its private
 *	field, invoke ssm_get_dip() to do so.  This iob also
 *	contains most of the information about the request, such
 *	as the location and size of the transfer buffer, and the
 *	disk address to start the transfer at.
 *
 *	"cmd" indicates that either a read or a write is to be executed.
 *
 *	Returns the number of bytes transferred if successful, otherwise
 *	returns -1 and io->i_error may be set to an applicable errno.
 *
 * Description:
 *	Reject the request if the transfer buffer is not aligned on 
 *	16 byte boundary or the request exceeds the device's maximum
 *	transfer size (128k).  Then assemble a SCSI command into a
 *	common SCSI command block and invoke scsidisk_cmd() to 
 *	execute it. 
 */
int
scsidiskstrategy(struct iob *io, int cmd)
{
	uint	nbytes;
	struct	scsiioctl sioctl;
	extern struct drive_info *ssm_get_dip();

	/*
	 * all I/O must have its in-core buffer aligned
	 */
	if (((ulong)io->i_ma & (SCSIDISK_ADDRALIGN - 1)) != 0) {
		io->i_error = EIO;
		return(-1);
	}

	/*
	 * don't transfer huge requests
	 */
	if (io->i_cc > (SCSIDISK_MAX_XFER * DEV_BSIZE)) {
		io->i_error = EIO;
		return(-1);
	}

	/*
	 * assemble the extended read or write SCSI command
	 */
	bzero((caddr_t) &sioctl, sizeof (struct scsiioctl));
	nbytes = howmany(io->i_cc, (uint)DEV_BSIZE);
	sioctl.sio_datalength = roundup(io->i_cc, (ulong)DEV_BSIZE);
	sioctl.sio_addr = (ulong)io->i_ma;
	sioctl.sio_cmdlen = SCSI_CMD10SZ;
	sioctl.sio_cmd10.cmd_opcode = (cmd == READ) ?
		SCSI_READ_EXTENDED : SCSI_WRITE_EXTENDED;
	sioctl.sio_cmd10.cmd_lba[0] = (unchar)((uint)io->i_bn >> 24);
	sioctl.sio_cmd10.cmd_lba[1] = (unchar)((uint)io->i_bn >> 16);
	sioctl.sio_cmd10.cmd_lba[2] = (unchar)((uint)io->i_bn >> 8);
	sioctl.sio_cmd10.cmd_lba[3] = (unchar)io->i_bn;
	sioctl.sio_cmd10.cmd_length[0] = (unchar)(nbytes >> 8);
	sioctl.sio_cmd10.cmd_length[1] = (unchar)nbytes;

	if (! SAIOSCSI_INFO(io)) {
		/* 
		 * This must have been a temporary iob, since
		 * it doesn't have the driver specific struct
		 * attached yet.  Locate it and fill it in so
		 * that calls future to the real driver don't 
		 * fault on it.
		 *
		 * This code is admittedly not the ideal way
		 * to handle this, but the alternative of never 
		 * allowing this to occur is difficult to implement,
		 * since it affects so many functions would be 
		 * affected.  This at least localizes it.
		 */
		io->i_dptr = (char *)ssm_get_dip(io->i_unit);
	}
	/*
	 * let the scsidisk_cmd function process the command: return -1
	 * on failure; otherwise return the request count. scsidisk_cmd
	 * sets i_error
	 */
	if (scsidisk_cmd(io, &sioctl)) {
		SAIOSCSI_FLAG(io) = 0;
		return (-1);
	}
	else {
		SAIOSCSI_FLAG(io) = 0;
		return (io->i_cc);
	}
}

/*
 * int
 * scsidiskioctl(struct iob *, int, caddr_t)
 * 	Execute a SCSI disk specific special control command.
 *
 * Calling/Exit State:
 *	"io" is a file descriptor with an adapter specific data 
 *	structure attached to its private field.
 *
 *	Returns zero if the ioctl succeeds and -1 if it fails.
 *
 * Description:
 *	Determine if the cmd value is valid for this driver.
 *	If so, perform the action requested by it using the
 *	structure addresses by arg accordingly.  What arg
 *	addresses is dependent upon the command value.
 *
 *	Currently the only supported cmd values are:
 *
 *		SAIOFIRSTSECT - return the block address of the
 *				drives first usable data sector.
 *
 *		SAIODEVDATA - returns drive geometry specifications
 *				to the caller.
 *
 *		SAIOSCSI_CMD - allows the caller to pass a SCSI
 *				command directly through the driver
 *				to the SCSI device.  Data and request
 *				sense buffers must be provided in addition
 *				to the SCSI command block.
 */
/*ARGSUSED*/
int
scsidiskioctl(struct iob *io, int cmd, caddr_t arg)
{

	io->i_cc = 0;
	io->i_error = 0;

	switch(cmd) {
	/*
	 * Return first sector of usable space.
	 */
	case SAIOFIRSTSECT:
		*(int *)((void *)arg) = 0;
		break;

	/*
	 * Get device data
	 */
	case SAIODEVDATA:
		*(struct st *)((void *)arg) = *SAIOSCSI_GEOM(io);
		break;

	/*
	 * SCSI commands: simply pass them on to scsidisk_cmd
	 */
	case SAIOSCSICMD:
		if (scsidisk_cmd(io, (struct scsiioctl *)((void *)arg))) {
			SAIOSCSI_FLAG(io) = 0;
			return (-1);
		}
		SAIOSCSI_FLAG(io) = 0;
		break;

	/*
	 * complain about unsupported commands and fail
	 */
	default:
		printf("%s(%d,0): bad ioctl (('%c' << 8) | 0x%x)\n",
			SAIOSCSI_PREFIX(io), io->i_unit, 
			(unchar)(cmd >> 8), (unchar)cmd);
		io->i_error = ECMD;
		return (-1);
	}
	return (0);
}

/*
 * static int
 * scsidisk_cmd (struct iob *, struct scsiioctl *)
 *	Execute the SCSI command specified in a common SCSI command descriptor.
 *
 * Calling/Exit State:
 *	"io" is a file descriptor with an adapter specific data 
 *	structure attached to its private field.  It also contains
 *	other information about the request, such as the location 
 *	and size of the transfer buffers.
 *
 *	"sioctl" is a common SCSI command descriptor containing the
 *	SCSI command to be executed, its size, and information
 *	about transfer and request sense buffers.
 *
 *	Returns the value returned by the adapter specific common
 *	SCSI command execution interface, which is expected to be
 *	-1 upon failure or zero if execution succeeds.
 *
 * Description:
 *	Simply sets the direction of transfer information in the
 *	common SCSI command descriptor, then invokes the adapter
 *	specific common SCSI command execution interface.  It
 *	is referenced indirectly throught the adapter specific
 *	handle which was stored in the iob's private field.
 */
static int
scsidisk_cmd (struct iob *io, struct scsiioctl *sioctl)
{
	unchar	opcode;

	opcode = sioctl->sio_cmd6.cmd_opcode;
	sioctl->sio_dir = sqnt_scsi_cmd_info[opcode];
	return (SAIOSCSI_CMD(io,sioctl));
}
