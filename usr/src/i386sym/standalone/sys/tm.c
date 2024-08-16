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

#ident	"@(#)stand:i386sym/standalone/sys/tm.c	1.1"

/*
 * SCSI 1/4" tape device driver for the SSM (stand alone).
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/boot.h>
#include <sys/cfg.h>
#include <sys/sysmacros.h>
#include <sys/ssm.h>
#include <sys/saio.h>
#include <sys/scsi.h>
#include <sys/ssm_scsi.h>

/*
 * Macros for the size of a flag byte array for bit 
 * manipulation, computing the index into it for a 
 * particular unit, and the bit mask within that byte.
 */
#define TM_NUM_FLAG_BYTES	32		/* 4 ssm's x 8 ta's */
#define TM_INDEX(unit)	(SCSI_BOARD((unit)) * 8 + SCSI_TARGET((unit)))
#define TM_FLAG(unit)	(1 << SCSI_UNIT((unit)))

#define TM_MODESEL_LEN		12	/* # data bytes for SCSI mode select */	
#define TM_CMD_FAIL		(-1) 	/* Returned when scsi commands fail  */
#define TM_CONTROL		0	/* SCSI request/sense control byte */
#define TM_IOCTL		(-1)	/* tells tm_docmd() to execute ioctl */

/*
 * tmopen() uses the following data for mode selection.  
 * An attempt is first made to have the drive use its 
 * default modes.  If that fails, modes are selected 
 * explicitly.
 */
static unchar	tm_default[TM_MODESEL_LEN] = { 
					0, 0, 0x10, 8, 0, 0, 0, 0, 0, 0, 2, 0 
}; 
static unchar	tm_5_25st[TM_MODESEL_LEN] = {  
					0, 0, 0x10, 8, 5, 0, 0, 0, 0, 0, 2, 0 
};

/*
 * One information structure is shared by all SSM/SCSI 
 * tape drives.  Each time a particular drive wants to
 * perform an operation it invokes ssm_get_devinfo() to
 * provide information about the device from the diagnostic 
 * tables.  Sharing works since standalone operations are 
 * synchronous and automic.
 */
static	struct ssm_sinfo tm_info;

/*
 * Flags to indicate when a space to the end of data 
 * is needed prior to writing, since the tape can only 
 * be written when at BOT or at end of valid data.
 */
static	unchar tm_write_space_eof[TM_NUM_FLAG_BYTES];

/*
 * Flags to indicate that the tape has been written
 * upon and needs a filemark appended.
 * Also inhibits read operations until closed.
 */
static	unchar tm_written[TM_NUM_FLAG_BYTES];

/* 
 * Flags to prevent operations past EOF/EOM and to
 * ensure that a filemark is written when closing
 * a file that has been written.
 */
static	unchar tm_eof[TM_NUM_FLAG_BYTES];

static int tm_docmd(struct iob *, int, uint, int, void *);

extern void     bzero(void *, size_t);
extern void 	bcopy(void *, void *, size_t);
extern int      mIntr(unchar, unchar, unchar);
extern int     printf(const char *, ...);

/*
 * void
 * tmopen(struct iob *)
 * 	Open the specified SCSI tape drive.
 *
 * Calling/Exit State:
 *	io->i_unit must already be set to the SCSI address
 *	of the drive to be opened.
 *
 *	io->i_boff must already be set to the number of
 *	the file to be opened, relative to the beginning
 *	of tape.
 *
 *	Alters the value of the structure tm_info, which
 *	is shared by all tm-devices, since their operations
 *	are synchronous and atomic.
 *
 *	No return value, although upon failure io->i_error
 *	is set to an applicable errno and is zero upon 
 *	successful completion.
 *
 * Description:
 *	Reject the request if the value in io->i_unit is
 *	an invalid standalone SCSI device specifier.  Then
 *	invoke ssm_get_devinfo() to allocate information
 *	necessary for the adapter to communicate with this
 *	device and notify the SSM of it.
 * 
 *	Next issue a SCSI test-unit-ready command to ensure the 
 *	device is ready for operation.  Afterwards rewind the 
 *	medium and select execution modes for this device.  During 
 *	mode selection attempt to use default settings.  If that 
 *	fails, issue explicit settings for a 5.25" streamer density.
 *
 *	Then erase the medium if access mode indicates it would 
 *	write to the front of the tape.  Finally, possition the
 *	tape to the beginning of the designated file.
 */
void
tmopen(struct iob *io)
{
	int board = SCSI_BOARD(io->i_unit);
	int devno = io->i_unit & SCSI_MAXDEVNO;
	int retry = 5;
	
	/*
 	 * Verify that its a SCSI type device.
	 */
	if ((int)SCSI_TYPE(io->i_unit) > 0) {
		printf("tm(%d): drive type not supported\n", io->i_unit);
		io->i_error = ENXIO;
		return;
	}

	/*
	 */
	if (!ssm_get_devinfo(board, devno, &tm_info, TM_CONTROL)) {
		printf("tm: no such SSM board or SCSI device.\n");
		io->i_error = ENXIO;
		return;
	} 

	/* Retry Test-unit-ready a few times to clear unit attention. */
	while (tm_docmd(io, SCSI_TEST, 0, 0, 0) == TM_CMD_FAIL) {
		if (!retry--) {
			printf("tm: tape unit not ready\n"); 
			io->i_error = EIO;
			return;
		}
	}

	if (tm_docmd(io, SCSI_REWIND, 0, 0, 0) == TM_CMD_FAIL) {
		printf("tm: rewind failed; media not ready\n"); 
		io->i_error = EIO;
		return;
	}
	
	if (tm_docmd(io, SCSI_MODES, 0, TM_MODESEL_LEN, tm_default) 
		   == TM_CMD_FAIL) {
		if (tm_docmd(io, SCSI_MODES, 0, TM_MODESEL_LEN, tm_5_25st) 
		   	== TM_CMD_FAIL) {
			printf("tm: mode selection failed\n"); 
			io->i_error = EIO;
			return;
		}
	}

	/*
	 * If writing to the front of the tape, 
	 * first erase entire tape and rewind.
	 */
	if (io->i_boff == 0 && io->i_howto != 0) {
		printf("Erasing tape, please wait (takes 2-3 minutes)...\n ");
		if (tm_docmd(io, SCSI_ERASE, SCSI_ERASE_LONG, 0, 0) 
		    == TM_CMD_FAIL 
		||  tm_docmd(io, SCSI_REWIND, 0, 0, 0) == TM_CMD_FAIL) {
			io->i_error = EIO;
			return;
		}
	}

	/*
	 * Locate the specified file.
	 */
	if ((io->i_cc = io->i_boff) != 0) {
		tm_write_space_eof[TM_INDEX(io->i_unit)] |= 
			TM_FLAG(io->i_unit);
		if (tm_docmd(io, SCSI_SPACE, SCSI_SPACE_FILEMARKS, 
			      io->i_boff, 0) == TM_CMD_FAIL) {
			io->i_error = EIO;
			return;
		}
	}
}

/*
 * void
 * tmclose(struct iob *)
 * 	Close the specified SCSI tape drive.
 *
 * Calling/Exit State:
 *	"io" must address a open file descriptor for
 *	the tape drive to be closed.
 *
 *	A flag in the bit-vector tm_written indicates
 *	if the medium has been written to since opening.
 *	If it has been written to, and a corresponding
 *	flag in the bit-vector tm_write_space_eof is not
 *	set, a filemark must be written on the medium
 *	prior to completing the close.
 *
 *	No return value, although upon failure io->i_error
 *	is set to an applicable errno and is zero upon 
 *	successful completion.
 *
 * Description:
 *	Write a file mark at the end of the tape if one has 
 *	not been written and data has been.  Then clear the
 *	driver's state information for the next open and
 *	rewind the tape.
 *
 * Remarks:
 *	If a write was followed by a read, the read should 
 *	error out.  This may also result in a bit for the 
 *	unit being set in both the tm_written and 
 *	the tm_write_space_eof bit-vectors.
 */
void
tmclose(struct iob *io)
{
	if ((tm_written[TM_INDEX(io->i_unit)] & TM_FLAG(io->i_unit))
	&&  !(tm_write_space_eof[TM_INDEX(io->i_unit)] & TM_FLAG(io->i_unit))
	&&  tm_docmd(io, SCSI_WFM, 0, 1, 0) == TM_CMD_FAIL) {
		io->i_error = EIO;
		printf("tm: could not write file mark.\n");
	} 

	tm_written[TM_INDEX(io->i_unit)] &= ~TM_FLAG(io->i_unit);
	tm_write_space_eof[TM_INDEX(io->i_unit)] &= ~TM_FLAG(io->i_unit);
	tm_eof[TM_INDEX(io->i_unit)] &= ~TM_FLAG(io->i_unit);

	if (tm_docmd(io, SCSI_REWIND, 0, 0, 0) == TM_CMD_FAIL) {
		io->i_error = EIO;
		printf("tm: Rewind of media failed.\n");
	}
}

/*
 * int
 * tmstrategy(struct iob *, int) 
 *      Standard read/write routine for the SCSI tape driver.
 *
 * Calling/Exit State:
 *      "io" is an open file descriptor, which contains
 *      most of the information about the request, such
 *      as the location and size of the transfer buffer. 
 *
 *      "cmd" indicates whether a read or a write is to 
 *	be executed.
 *
 *	The state information about this drive in the 
 *	tm_write_space_eof, tm_written, and tm_eof is
 *	updated during execution of the requested operation.
 *
 *      Returns the number of bytes transferred if successful, 
 *	otherwise returns -1 and io->i_error may be set to an 
 *	applicable errno.
 *
 * Description:
 *	For read requests, return zero indicating that end-of-file
 *	has been reached if either EOF or EOM had been encountered
 * 	but not yet reported by a previous request.  Otherwise,
 *	note in the state information that a read has occurred and
 *	that subsequent write's must first reposition the medium
 *	to the logical end of recorded data (EOD).  Then invoke tm_docmd()
 *	to perform the actual SCSI READ command.
 *
 *	For write requests, return zero if the tape is currently
 *	positioned at the end of tape mark, in which case no further
 *	data may be written.  Since writes are permitted only at the
 *	begining of tape (BOT) or EOD, reposition the tape to EOD 
 *	if its not at BOT, noting it new position in the state 
 *	bit-vectors.  Finally, note that the medium has been written
 *	to, so filemarks will be written upon closing, and invoke
 *	tm_docmd() to perform the actual SCSI WRITE command.
 */
tmstrategy(struct iob *io, int func)
{
	int xferbytes = roundup(io->i_cc, DEV_BSIZE);
	int n;

	if(tm_eof[TM_INDEX(io->i_unit)] & TM_FLAG(io->i_unit))
		return (0);		/* EOM or EOF encountered */

	switch (func) {			/* Base action on the function */
	case READ:
		tm_write_space_eof[TM_INDEX(io->i_unit)] |= 
			TM_FLAG(io->i_unit);
		n = (tm_docmd(io, SCSI_READ, 0, xferbytes, io->i_ma));
		break;
	case WRITE:
		if (tm_write_space_eof[TM_INDEX(io->i_unit)] 
	    	    & TM_FLAG(io->i_unit)) {
			n = tm_docmd(io, SCSI_SPACE, 
				      SCSI_SPACE_ENDOFDATA, 0, 0);
			if (n == TM_CMD_FAIL)
				return (TM_CMD_FAIL);

			tm_write_space_eof[TM_INDEX(io->i_unit)] &= 
				~TM_FLAG(io->i_unit);
		}
		tm_written[TM_INDEX(io->i_unit)] |= TM_FLAG(io->i_unit);
		n = (tm_docmd(io, SCSI_WRITE, 0, xferbytes, io->i_ma));
		break;
	default:
		io->i_error = ECMD;
		return (TM_CMD_FAIL);
	}
	return ((n == TM_CMD_FAIL) ? n : ((!n) ? io->i_cc : xferbytes - n));
}

/*
 * static int
 * tm_docmd(struct iob *, int, uint, int, void *)
 * 	Execute the specified SCSI command to the designated device.
 *
 * Calling/Exit State:
 *	Alters the value of the structure tm_info, which
 *	is shared by all tm-devices, since their operations
 *	are synchronous and atomic.
 *
 *	"cmd" determines the type of request to execute.  If
 *	set to TM_IOCTL, then SCSI command block and related 
 *	data are addressed by the data argument. The related 
 *	data follows immediately after the command block.  The
 *	caller is responsible for determining if the SCSI
 *	command being pased through is valid.
 *
 *	If cmd if a data transferring SCSI command such as
 *	read, write, or mode-select, count and data describe
 *	the data to be transferred.
 *
 *	For other SCSI commands requiring an operation count,
 *	'count' is applicable for it, and data is not.  Otherwise
 *	both count and data are not relevant.
 *
 *	Returns TM_CMD_FAILED if the requested command fails
 *	completely, zero for complete success, and the number
 *	of bytes transferred or operations completed for
 *	partially successful commands.
 *
 * Description:
 *	Invoke ssm_get_devinfo to ensure that SSM/SCSI resources 
 *	are allocated for the device and the SSM is aware of it.
 *	Then fill out the SSM SCSI message block (CB) provided by 
 *	it for passing commands to the SSM, as per the requested
 *	operation.  Afterwards, notify the SSM that the CB is
 *	ready for executution and poll its status until completed
 *	by the SSM.  Finally, analyze the termination status
 *	and determine the next course of action and return value.
 */
static int
tm_docmd(struct iob *io, int cmd, uint modifiers, int count, void *data)
{
	struct scsi_cb	*cb;
	int board = SCSI_BOARD(io->i_unit); 
	int devno = io->i_unit & SCSI_MAXDEVNO;
	int blocks;
	unchar *cp;
	int not_xferred;
	struct scrsense *rs;

	if (!ssm_get_devinfo(board, devno, &tm_info, TM_CONTROL)) {
		printf("tm_docmd: no such SSM board or device\n");
		io->i_error = ENXIO;
		return(TM_CMD_FAIL);
	}

	io->i_errcnt = 0;
	io->i_error = 0;

	/*
	 * Build an I/O CB for the command.
	 */
	cb = tm_info.si_cb; 			/* Command independent stuff */
	bzero (SWBZERO(cb), SWBZERO_SIZE); 
	cb->sh.cb_scmd[1] = (SCSI_UNIT(io->i_unit)) << 5;
	cb->sh.cb_clen = SCSI_CMD6SZ;

	cb->sh.cb_scmd[0] = (unchar)cmd;
	switch (cmd) {			/* Command specific stuff */
	case SCSI_READ:
	case SCSI_WRITE:
		/*
	 	 * For errors uncorrected after retries 
		 * rezero the unit once and retry once more.
		 */
		blocks = btodb(count);
		cb->sh.cb_cmd = (cmd == SCSI_READ) ? SCB_READ : SCB_WRITE; 
		cb->sh.cb_scmd[1] |= SCSI_FIXED_BLOCKS;
		cb->sh.cb_scmd[2] = (unchar)(blocks >> 16); 
		cb->sh.cb_scmd[3] = (unchar)(blocks >> 8); 
		cb->sh.cb_scmd[4] = (unchar)blocks;
		cb->sh.cb_addr = (ulong)data;
		cb->sh.cb_count = count;
		break;
	case SCSI_ERASE:
	case SCSI_WFM:
	case SCSI_SPACE:
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_scmd[1] |= (unchar)modifiers;
		cb->sh.cb_scmd[2] = (unchar)(count >> 16); 
		cb->sh.cb_scmd[3] = (unchar)(count >> 8); 
		cb->sh.cb_scmd[4] = (unchar)count;
		tm_written[TM_INDEX(io->i_unit)] &= ~TM_FLAG(io->i_unit);
		break;
	case SCSI_TEST:
	case SCSI_REWIND:
		cb->sh.cb_cmd = SCB_READ;
		break;
	case SCSI_MODES: 
		cb->sh.cb_cmd = SCB_WRITE;
		cb->sh.cb_scmd[4] = (unchar)count;
		cb->sh.cb_addr = (ulong)data;
		cb->sh.cb_count = (ulong)count;
		break;
	case TM_IOCTL:
		/*
		 * This is a special case where the
		 * caller provides the SCSI command 
 		 * block and any other data.  
		 *
		 * Supported functions are 6-byte SCSI
		 * commands, which are validated by
		 * tmioctl prior to comming here. The
		 * byte count of the data passed will
		 * be validated here, then the SCSI
		 * command portion is copied into the
		 * CB.  If any data remains its size
		 * and address are stored in the CB.
		 *
		 * The logical unit number will be 'or'ed
		 * into the SCSI command block here.
		 */
		if (count < SCSI_CMD6SZ) {
			printf("tm: Illegal operation rejected %x\n", cmd);
			io->i_error = ECMD;
			return(TM_CMD_FAIL);
		} else {
			bcopy((char *)data, &cb->sh.cb_scmd[1], SCSI_CMD6SZ);
			cb->sh.cb_scmd[1] |= (SCSI_UNIT(io->i_unit)) << 5;
			if (count -= SCSI_CMD6SZ) {
				cb->sh.cb_cmd = SCSI_WRITE;
				cb->sh.cb_addr = (ulong)data + SCSI_CMD6SZ;
				cb->sh.cb_count = count;
			} else
				cb->sh.cb_cmd = SCSI_READ;
		}
		break;
	default:
		printf("tm: Illegal operation rejected %x\n", cmd);
		io->i_error = ECMD;
		return(TM_CMD_FAIL);
	}

	/*
	 * Start the CB executing by notifying 
	 * the SSM of it, then await its completion.
	 */
	cb->sh.cb_compcode = SCB_BUSY;
	(void)mIntr(tm_info.si_slic, SCSI_BIN, SCVEC(tm_info.si_id, 0));
	while (cb->sh.cb_compcode == SCB_BUSY)
		continue;		/* Poll for completion */

	/*
	 * Take action based on termination status.
	 */
	 if (cb->sh.cb_compcode != SCB_OK) {
		/* Interface timeout or error */
	 	/*
	 	 * Print out the failed command.
	  	 */
		printf("tm(%d): interface error occurred (cc=%d,bn=%d)\n", 
			io->i_unit, io->i_cc, io->i_bn);
		printf("The SCSI cmd that created the error:");
		cp = cb->sh.cb_scmd;
		while (cb->sh.cb_clen--)
			printf(" %x", *cp++);
		printf("\n");
		io->i_error = EHER;
		return (TM_CMD_FAIL);
	 } else if (SCSI_CHECK_CONDITION(cb->sh.cb_status)) {
		/* Take action based on the request sense data. */
		rs = (struct scrsense *) cb->sh.cb_sense;

		if ((rs->rs_error & RS_VALID) == 0 
		||  (rs->rs_error & RS_ERRCLASS) != RS_CLASS_EXTEND) {
			return (TM_CMD_FAIL);	/* Lost the error info */
		} else {
			not_xferred = rs->rs_info[0] << 24;
			not_xferred |= rs->rs_info[1] << 16;
			not_xferred |= rs->rs_info[2] << 8;
			not_xferred |= rs->rs_info[3];
			switch (rs->rs_key & RS_SENSEKEYMASK) { 
			case RS_PROTECT:
				printf("Write protected, no transfer\n");
				io->i_error = EWCK;
				return (TM_CMD_FAIL);
			case RS_RECERR:
				printf("tm(%d): recovered from error 0x%x ",
					io->i_unit, rs->rs_key);
				printf("(cc=%d,bn=%d)\n", io->i_cc, io->i_bn);
				ssm_print_sense(cb);
				if (rs->rs_key & (RS_EOM | RS_FILEMARK))
					tm_eof[TM_INDEX(io->i_unit)] |= 
						TM_FLAG(io->i_unit);
				return (dbtob(not_xferred));
			case RS_NOSENSE:
			case RS_OVFLOW:
			case RS_UNITATTN:
				if (rs->rs_key & (RS_EOM | RS_FILEMARK))
					tm_eof[TM_INDEX(io->i_unit)] |= 
						TM_FLAG(io->i_unit);
				return (dbtob(not_xferred));
			case RS_BLANK:
				if (rs->rs_key & (RS_EOM | RS_FILEMARK)) {
					tm_eof[TM_INDEX(io->i_unit)] |= 
						TM_FLAG(io->i_unit);
					return (dbtob(not_xferred));
				}
				break;	/* go handle the harderror */
			case RS_MEDERR:
			case RS_CPABORT:
				if (rs->rs_key & RS_EOM)
					return (dbtob(not_xferred));
				break;	/* go handle the harderror */
			default:
				break;	/* go handle the harderror */
			}

			/* 
			 * If it came here a hard error 
			 * occurred (all other cases returned).
	 		 * Print out request sense information 
	 		 * and the command itself.
	 		 */
			printf("tm(%d): hard error 0x%x (cc=%d,bn=%d)\n", 
				io->i_unit, rs->rs_key, io->i_cc, io->i_bn);
			ssm_print_sense(cb);
			printf("The SCSI cmd that created the error:");
			cp = cb->sh.cb_scmd;
			while (cb->sh.cb_clen--)
				printf(" %x", *cp++);
			printf("\n");
			io->i_error = EHER;
			return (TM_CMD_FAIL);
		}
	 } else if (!SCSI_GOOD(cb->sh.cb_status)) {
		io->i_error = EHER;
		return (TM_CMD_FAIL);	/* Unknown program error */
	 } else
		return (0);		/* Successful */
}

/*
 * int
 * tmioctl(struct iob *, int, caddr_t)
 *      Execute a SCSI tape drive special control command.
 *
 * Calling/Exit State:
 *      "io" is an open file descriptor for the designated tape drive.
 *
 *	The only supported "cmd" value is SAIOSCSICMDOLD, for
 *	passing SCSI commands directly through the driver to the
 *	device.  In that case, "arg" addresses a SCSI command
 *	block followed by an applicable data buffer.  The size
 *	of both the command block is implied by the SCSI command
 *	specified.  The size of the data buffer may be implied
 *	by the SCSI command type, or by 'count' fields within
 *	the SCSI command block (again implied by the command type).
 *
 *	The drives state information maintained in the bit-vectors
 *	tm_eof, tm_written, and tm_write_space_eof are updated to
 *	reflect the impact of the executed SCSI command, which are
 *	executed by invoking tm_docmd() from within this function.
 *
 *      Returns TM_CMD_FAIL if the ioctl fails or passes back the
 *	return value from tm_docmd otherwise.  In most cases, io->i_cc
 *	indicates the number of requested operations or transfers that
 *	succeeded, if any.
 */
int
tmioctl(struct iob *io, int cmd, caddr_t arg)
{
	int n;

	io->i_cc = 0;
	/* 
	 * Determine the type of the IOCTL
	 * and execute its semantics.
	 */
	switch (cmd) {
	case SAIOSCSICMDOLD:
		/*
	 	 * SCSI commands. Verify the SCSI command 
		 * type and take action based upon it 
		 * (attempt the command and set/clear state 
		 * flags).  
		 *
		 * tm_docmd() expects the SCSI command type 
		 * to be validated here, and that it is a 
		 * 6-byte type commands block.  The block 
		 * address and associated data must be 
		 * sequential and described by the data 
		 * address and size arguments. 
	 	 */
		switch (*(unchar *)arg) {
		case SCSI_STARTOP:
		case SCSI_REWIND:
		case SCSI_ERASE:
			/* 
			 * Results in tape positioned to BOT.
			 */
			n = tm_docmd(io, TM_IOCTL, 0, SCSI_CMD6SZ, arg);
			if (n != TM_CMD_FAIL) {
				tm_eof[TM_INDEX(io->i_unit)] &= 
					~TM_FLAG(io->i_unit);
				tm_written[TM_INDEX(io->i_unit)] &= 
					~TM_FLAG(io->i_unit);
				tm_write_space_eof[TM_INDEX(io->i_unit)] &= 
					~TM_FLAG(io->i_unit);
			}
			break;
		case SCSI_SPACE:
			/* 
			 * Always positions tape away from BOT.
			 */
			n = tm_docmd(io, TM_IOCTL, 0, SCSI_CMD6SZ, arg);
			if (n != TM_CMD_FAIL) {
				tm_eof[TM_INDEX(io->i_unit)] &= 
					~TM_FLAG(io->i_unit);
				tm_written[TM_INDEX(io->i_unit)] &= 
					~TM_FLAG(io->i_unit);
				tm_write_space_eof[TM_INDEX(io->i_unit)] |= 
					TM_FLAG(io->i_unit);
			}
			break;
		case SCSI_TEST:
			n = tm_docmd(io, TM_IOCTL, 0, SCSI_CMD6SZ, arg);
			break;
		case SCSI_WFM:
			/* 
			 * Writes a filemark on the tape.
			 * Sets eof flag for close.
			 */
			n = tm_docmd(io, TM_IOCTL, 0, SCSI_CMD6SZ, arg);
			if (n != TM_CMD_FAIL) {
				tm_eof[TM_INDEX(io->i_unit)] |= 
					TM_FLAG(io->i_unit);
				tm_written[TM_INDEX(io->i_unit)] &= 
					~TM_FLAG(io->i_unit);
				tm_write_space_eof[TM_INDEX(io->i_unit)] &= 
					~TM_FLAG(io->i_unit);
			}
			break;
		case SCSI_MODES:
			/*
			 * Sets drive execution modes.
			 */
			n = tm_docmd(io, TM_IOCTL, 0, 
			       SCSI_CMD6SZ + arg[4], arg);
			break;
		default:
			printf("tm: SCSI ioctl command type (0x%x)\n",
				*(unchar *)arg);
			io->i_error = ECMD;
			n = TM_CMD_FAIL;
			break;
		}
	default:
		printf("tm: bad ioctl type (('%c'<<8)|%d)\n",
			(unchar)(cmd>>8),(unchar)cmd);
		io->i_error = ECMD;
		n = TM_CMD_FAIL;
		break;
	}
	return (n);
}
