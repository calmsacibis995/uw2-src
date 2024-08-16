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

#ident	"@(#)stand:i386sym/standalone/sys/ssm.c	1.1"

/*
 * ssm.c
 *	Suport routines for SSM standalone device drivers.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/boot.h>
#include <sys/cfg.h>
#include <sys/slic.h>
#include <sys/slicreg.h>
#include <sys/vtoc.h>
#include <sys/ssm.h>
#include <sys/saio.h>
#include <sys/scsi.h>
#include <sys/scsidisk.h>
#include <sys/ssm_scsi.h>
#include <sys/ssm_cons.h>
#include <sys/ssm_misc.h>

extern void 	bcopy(void *, void *, size_t);
extern void     bzero(void *, size_t);
extern char 	*calloc(int);
extern void 	callocrnd(int);
extern int	mIntr(unchar, unchar, unchar);
extern int      printf(const char *, ...);

struct ssm_dip {
	struct	ssm_dip *s_next;
	int	s_unit;
	struct	drive_info *s_dip;
};

static struct	ssm_sinfo common_sinfo;
static struct ssm_misc SSM_misc; 
static struct ssm_dip *ssm_dip = (struct ssm_dip *)0;

struct	drive_info *ssm_get_dip();
static void ssm_scsi_printcb(struct scsi_cb *);
static void ssm_scsi_error(struct scsi_cb *, struct iob *, char *);
static char *ssm_alloc(unsigned, unsigned, unsigned);
static int ssm_scsi_cmd(struct iob *, struct scsiioctl *);
static unchar ssm_scsi_send(struct ssm_sinfo *, 
			struct scsiioctl *, struct scsi_cb *);

/* Fumctions/data structures for console i/o using the SSM.  */
#define CONS_BUFF_SIZE 32

static char input_buff[CONS_BUFF_SIZE];
static ushort buff_index;
static int _cbinit = 1;
static int cb_sent = 0;
static struct cons_cb cb[NCONSDEV*NCBPERCONS];

extern int _slscsi;			/* Slic addr for putchar's 


/*
 * ssm_get_devinfo(int, int, struct ssm_info *, unchar)
 * 	Validates the description for the SCSI device and fills in 
 *	an info structure for it.  
 *
 * Calling/Exit State:
 *	Allocates a set of SCSI CBs and request/sense buffer 
 *	which are shared by all SSM/SCSI devices since device 
 *	operations are atomic (call this iterface prior to each
 *	operation), uses less memory, and the SSM has a limited 
 *	number of device identifiers.  
 *
 *	The SSM device identifier is recycled when the SSM 
 *	notes the SLIC vector its given before the address
 *	of the initialization CB, which must occur during
 *	each successful invocation of this function.  
 *
 *	Returns 1 for success, 0 for failure.
 *
 * Description:
 *	Validate the range of the device and board numbers.
 *	Then locate the SSM it corresponds to and verify
 *	that it is usable.  If the static CBs and sense
 *	buffer have not yet been allocated, do so next.
 *	Then communicate this temporary device configuration
 *	to the SSM via an "initialization CB".
 */
int
ssm_get_devinfo(int boardno, int devno, 
	struct ssm_sinfo *info, unchar control_byte) 
{
	struct scsi_init_cb *icb;
	unsigned char *addr_bytes = (unsigned char *)&icb;
	struct ctlr_desc *cp;
	static struct scsi_cb *scsi_cbs; 	/* Recycled SCSI CBs */
	static char *scsi_sensebuf;		/* Recycled SCSI rsense buf */
        struct config_desc *cd = CD_LOC;
        int count, ssm_cnt, ssm2_cnt;

        /* get cnts of SSM and SSM2 and total
        */
        ssm_cnt = cd->c_toc[SLB_SSMBOARD].ct_count;
        ssm2_cnt = cd->c_toc[SLB_SSM2BOARD].ct_count;
        count = ssm_cnt + ssm2_cnt;

	/* 
	 * Validate range of devno and boardno.
	 */
	 if (devno != (devno & SCSI_MAXDEVNO)) {
		printf("ssm: Bad SCSI address number %d\n", devno);
		return (0);
	} else if (boardno >= count) {
		printf("ssm: Bad board number %d\n", boardno);
		return (0);
	} else {
                /* scale boardno into the proper ptr to SSM or SSM2 descriptor
                */
                if( boardno < ssm_cnt ) {
                        cp = cd->c_ctlrs + (cd->c_toc[SLB_SSMBOARD].ct_start +
                                              boardno);
                } else {
                        cp = cd->c_ctlrs + (cd->c_toc[SLB_SSM2BOARD].ct_start +
                                              (boardno-ssm_cnt));
                }

		if (cp->cd_diag_flag & (CFG_FAIL | CFG_DECONF)) {
			printf("ssm: Board failed diagnostics or ");
			printf("was deconfigured %d\n", boardno);
			return(0);
		}
	}

	/*
	 * If the one-time allocation of SCSI CBs and
	 * request/sense buffer has not occurred
	 * then allocate them.  The buffer address
	 * must be stored in the first CB later
	 * since the initialization CB would clobber
	 * its value.  Only one buffer is allocated
 	 * which is later assigned to the first CB,
 	 * since it is the only one that should
	 * be used in the standalone environment.
	 */
	if (!scsi_cbs) {
		scsi_cbs = (struct scsi_cb *) (void *)
		       ssm_alloc((sizeof(struct scsi_cb) << NCBSCSISHFT),
				SSM_ALIGN_XFER, SSM_BAD_BOUND);
		if (!scsi_cbs) {
			printf("ssm: Out of memeory for SCSI CBs.\n");
			return (0);
		}
		scsi_sensebuf = (char *)
			ssm_alloc(SCB_RSENSE_SZ, SSM_ALIGN_XFER, SSM_BAD_BOUND);
		if (!scsi_sensebuf) {
			printf("ssm: Out of memeory for SCSI sense buffer.\n");
			return (0);
		}
	} 

	/* 
	 * Fill in the initialization CB.
	 */
	icb = (struct scsi_init_cb *)scsi_cbs;
	bzero((char *)icb, sizeof(struct scsi_cb));
	icb->icb_pagesize = PAGESIZE;
	icb->icb_scsi = 0;			/* Could change in the future */
	icb->icb_target = SCSI_TARGET(devno);
	icb->icb_lunit = SCSI_UNIT(devno);
	icb->icb_control = control_byte;
	icb->icb_cmd = SCB_INITDEV;
	icb->icb_compcode = SCB_BUSY;

	/* 
	 * Notify the SSM of the initialization CB 
	 * and device's CB table address.
	 */
	(void)mIntr(cp->cd_slic, SCSI_BIN, (unsigned char) SCB_PROBEVEC);
	(void)mIntr(cp->cd_slic, SCSI_BIN, addr_bytes[0]); /* low byte first */
	(void)mIntr(cp->cd_slic, SCSI_BIN, addr_bytes[1]);
	(void)mIntr(cp->cd_slic, SCSI_BIN, addr_bytes[2]);
	(void)mIntr(cp->cd_slic, SCSI_BIN, addr_bytes[3]); /* high byte last */
	
	while (icb->icb_compcode == SCB_BUSY)	
		continue;		/* Poll for command completion */

	if (icb->icb_id < 0) {
		printf("ssm: Invalid device i.d. returned by SSM;");
		printf(" an initialization error occurred.\n");
		return (0);
	}

	/*
	 * Now save request sense information
	 * since that would have been clobbered
	 * while filling out the initialization CB.
	 */
	scsi_cbs->sh.cb_sense = (ulong) scsi_sensebuf;
	scsi_cbs->sh.cb_slen = SCB_RSENSE_SZ;

	/* 
	 * Fill in the info structure passed to this function.
	 */
	info->si_cb = scsi_cbs;		/* Device CB address to return */
	info->si_id = icb->icb_id;	/* SSM device i.d. to return */
	info->si_unit = (unchar)devno;
	info->si_slic = cp->cd_slic;	/* SLIC address of its SSM */
	info->si_version = ((ulong)cp->cd_ssm_version[0]) << 16;
	info->si_version |= ((ulong)cp->cd_ssm_version[1]) << 8;
	info->si_version |= (ulong)cp->cd_ssm_version[2];
	return (1);
}

/*
 * void
 * ssm_print_sense(struct scsi_cb *)
 *	Display request sense information from the CB.
 *
 * Calling/Exit State:
 *	The specified SCSI CB has a buffer attached to it
 *	containing SCSI request-sense data.
 *
 *	No return value.
 *
 * Description: 
 * 	For extended error codes print as much data as is 
 *	available.  Otherwise, print only the standard data.
 */
void
ssm_print_sense(struct scsi_cb *cb)
{
	int i;
	unchar *cp = (unchar *)cb->sh.cb_sense;
	struct scrsense *rs = (struct scrsense *) cb->sh.cb_sense;

	i = ((rs->rs_error & RS_CLASS_EXTEND) == RS_CLASS_EXTEND) 
		? cb->sh.cb_slen : 8; 

	printf("Request sense bytes:");
	while (i--) 
		printf(" %x", *cp++);
	printf("\n");
}

/*
 * static char *
 * ssm_alloc(unsigned, unsigned, unsigned) 
 *	Allocate a properly-aligned chunk of memory that does 
 *	not cross the specified boundary, for use with the SSM.
 *
 * Calling/Exit State:
 * 	'nbytes' is the number of bytes to allocate.
 *
 * 	'align' is the byte multiple at which the memory is to 
 *	be aligned (e.g. 2 means align to two-byte boundary).  
 *
 * 	'badbound' is a boundary which cannot be crossed (usually one 
 *	megabyte for the SSM); it must be a power of two and a 
 *	multiple of 'align'.
 *
 *	Returns the address of the allocated buffer.
 *
 * Descriptions:
 *	Use callocrnd() to round the next allocation up so it
 *	starts on an aligned boundary.  Then confirm that the
 *	it won't cross a bad boundary.  If so, then callocrnd()
 *	the next allocation again so its past the bad boundary.
 *	Finally, allocate the buffer using calloc() and return
 *	its return value to the caller.
 */
static char *
ssm_alloc(unsigned nbytes, unsigned align, unsigned badbound)
{
	long addr;

	callocrnd((int)align);
	addr = (long)calloc(0);
	if ((addr & ~(badbound - 1)) != (addr + nbytes & ~(badbound - 1))) {
		/*
		 * It would have crossed a 'badbound' boundary,
		 * so bump past this boundary.
		 */
		callocrnd((int)badbound);
	}
	return (calloc((int)nbytes));
}

/*
 * static void
 * ssm_send_misc_addr(void) 
 *	Send the address of an ssm_misc structure to the SSM firmware.
 *
 * Calling/Exit State:
 *	Use the static "misc_addr_sent" so this action is only 
 *	performed once.
 *
 *	The structure whose address is sent is the variable SSM_misc.
 *
 *	The firmware config table contains the SLIC i.d. of the system
 *	front-panel controller, to which this is targetted.
 *
 * Description:
 *	To establish message passing with this SSM, send the 
 *	physical address of the core-memory message block to
 *	the SSM.  This is sent over the SLIC, one byte at a
 *	time, preceeded by a SLIC message containing the 
 *	value SM_ADDRVEC.  The SLIC messages are directed to a
 *	predetermined SLIC bin, reserved for this type of operation.
 */
static void
ssm_send_misc_addr(void)
{
	struct ssm_misc *addr;
	unchar dest;
	unchar *addr_bytes;
	static int misc_addr_sent = 0;

	if (misc_addr_sent)
		return;		/* Short circuit multiple calls */

	addr = &SSM_misc;
	dest = CD_LOC->c_cons->cd_slic;
	addr_bytes = (unchar *)&addr;

	(void)mIntr(dest, SM_BIN, SM_ADDRVEC);
	(void)mIntr(dest, SM_BIN, addr_bytes[0]);	/* low byte first */
	(void)mIntr(dest, SM_BIN, addr_bytes[1]);
	(void)mIntr(dest, SM_BIN, addr_bytes[2]);
	(void)mIntr(dest, SM_BIN, addr_bytes[3]);	/* high byte last */
	misc_addr_sent = 1;
}

/*
 * static void
 * ssm_gen_cmd(struct ssm_misc *, unchar, int, int)
 *	Common interface for sending misc. commands to the SSM.
 *
 * Calling/Exit State:
 *	'ssm_mc' is the address of the misc-cb for communicating with
 *	the SSM whose SLIC id is 'dest'.  ssm_send_misc_addr() must 
 *	already have establish its location with the SSM.
 *
 *	'who' is who (which basic functional area) on the SSM gets the message.
 *
 * 	'cmd' is the a command/message code to send to that functional area.
 *
 * 	Assume the caller has initialized 'ssm_mc' with relevant values
 *	and data to execute the designated command.
 *
 *	No return value.
 *
 * Description:
 *	Simply load the misc-CB with the command args to this function,
 *	then send a SLIC message to the specified SSM telling it to
 *	read and execute that CB.  Finally, poll the CB's status field
 *	until the SSM marks it as having been executed.
 */
static void
ssm_gen_cmd(struct ssm_misc *ssm_mc, unchar dest, int who, int cmd)
{
	/* build command to ssm */
	ssm_mc->sm_who = (unchar)who;
	ssm_mc->sm_cmd = (unchar)cmd;
	ssm_mc->sm_stat = SM_BUSY;

	/* send it and wait for completion */
	(void)mIntr(dest, SM_BIN, (unchar)who);
	while (ssm_mc->sm_stat == SM_BUSY)
		;
}

/*
 * static ulong
 * ssm_get_fpst(void)
 *	Returns a bit-vector indicating the current front-panel state.
 *
 * Calling/Exit State:
 *	Assumes the address of SSM_misc has already have communicated
 *	to the SSM and that this function now owns it for requesting
 *	the front-panel status.	
 *
 *	Returns the front-panel state value returned by the SSM in the
 *	completed misc-CB.
 */
static ulong
ssm_get_fpst(void)
{	
	struct ssm_misc *ssm_mc = &SSM_misc;

	ssm_send_misc_addr();
	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_gen_cmd(ssm_mc, CD_LOC->c_cons->cd_slic, SM_FP, SM_GET);
	return(ssm_mc->sm_fpst);
}

/*
 * static void
 * ssm_reboot(uint, ushort, char *)
 *	Request the SSM to halt this program and reboot using
 *	the specified boot flags and boot string.
 *
 * Calling/Exit State:
 *	Reboot with these flags and string.
 *
 * 	'flags' is the boot flags to reboot with.
 * 	'size' is the number of bytes in 'str'.
 * 	'str' is a character buffer with the boot string.
 *
 *	No return value.
 *
 * Description:
 *	Simply copy the reboot ags into the misc.-CB, then
 *	invoke ssm_gen_cmd() to send it the REBOOT command.
 */
static void
ssm_reboot(uint flags, ushort size, char *str)
{
	struct ssm_misc *ssm_mc = &SSM_misc;

	ssm_send_misc_addr();
	bzero((char *)&ssm_mc->sm_un, sizeof ssm_mc->sm_un);
	ssm_mc->sm_boot.boot_flags = (ushort)flags;
	if (size)
		bcopy(str, ssm_mc->sm_boot.boot_str,
	      		(size > BNAMESIZ)? BNAMESIZ: size);
	ssm_gen_cmd(ssm_mc, CD_LOC->c_cons->cd_slic, SM_BOOT, SM_REBOOT);
}

/*
 * void
 * ssm_rtofw(void)
 * 	SSM dependent "program halt and return to firmware monitor".
 *
 * Calling/Exit State:
 *	Never returns to the caller after invoking
 *	ssm_reboot to halt the machine; it just
 *	spins until shut down by the firmware.  
 */
void
ssm_rtofw(void)
{
	ssm_reboot(RB_HALT, 0, (char *)NULL);	
	for (;;)
		/* Spin until halted by the firmware */ ;
}

/*
 * static void
 * init_ssm_cons(void) 
 *	Send the address of the console CB's to the SSM firmware.
 *
 * Calling/Exit State:
 *	Executed once during bootup to establish message block
 *	communication of console line commands.
 *
 *	The structure whose address is sent is the variable 'cb'.
 *
 *	The firmware config table contains the SLIC i.d. of the system
 *	front-panel controller, to which this is targetted.
 *
 * 	Return the base address of the consoles CBs to the caller.
 *
 * Description:
 *	To establish message passing with the SSM console, send 
 *	the physical address of its core-memory message blocks to
 *	the SSM.  This is sent over the SLIC, one byte at a
 *	time, preceeded by a SLIC message containing the 
 *	value CONS_ADDRVEC.  The SLIC messages are directed to a
 *	predetermined SLIC bin, reserved for console operations.
 */
static void
init_ssm_cons(void)
{
	struct config_desc *cd = CD_LOC;
	unchar slic = cd->c_cons->cd_slic;
	uint addr = (uint)cb;

 	/* Notify the SSM of the CB'a location. */
	(void)mIntr(slic, CONS_BIN, CONS_ADDRVEC);
	(void)mIntr(slic, CONS_BIN, (unchar)(addr >>0));/* low byte first */
	(void)mIntr(slic, CONS_BIN, (unchar)(addr >>8));
	(void)mIntr(slic, CONS_BIN, (unchar)(addr >>16));
	(void)mIntr(slic, CONS_BIN, (unchar)(addr >>24));/* high byte last */
}

/*
 * int
 * ssm_getchar(int)
 *      Read 1 character from the system console.
 *
 * Calling/Exit State:
 *	The SLIC i.d. of the console-SSM is in the firmware
 *	configuration table and the console-CB's have been
 *	allocated.
 *
 *	The state of the front-panel indicates if the console
 *	is currently the local vs. remote port of the SSM.  This
 *	is required to determine which port's read-CB to use.
 *
 *	Returns -1 if input is not yet available and "wait"
 *	is non-zero.  Otherwise, returns the byte next available
 *	byte of input.
 *
 * Description:
 *	Invoke init_ssm_cons() to establish message passing
 *	with the console-SSM if it has not already been done.
 *	If a read-message is not currently active, and there
 *	is no remaining unconsumed input from a previously read,
 *	then start the read-cb for the console.  When the CB
 *	completes, return the first returned data byte in its
 *	buffer, if any.  Subsequent calls siphon the remaining
 *	data out of the buffer; once exhausted, repeat the cycle.
 *
 *	If input is not immediately available and "wait" is zero,
 *	then return -1 without any delay.  Otherwise, continue
 *	soliciting input until a byte is available.
 */
int
ssm_getchar(int wait)
{
	struct config_desc *cd = CD_LOC;
	unchar slic = cd->c_cons->cd_slic;
	int unit;
	static struct cons_rcb *rcb;

	if (_cbinit) {
		_cbinit = 0;
		init_ssm_cons();
	}

    	do {
		if (!cb_sent) {
			/* Determine which port is currently the console */
			unit = (ssm_get_fpst() & FPST_LOCAL) ? 
				CCB_LOCAL : CCB_REMOTE;
			rcb = (struct cons_rcb *)
				(CONS_BASE_CB(cb, unit) + CCB_RECV_CB);

			/* build and send console a request for data */
			rcb->rcb_addr = (ulong)input_buff;
			rcb->rcb_timeo = 30;	/* wait 30 msec */
			rcb->rcb_count = CONS_BUFF_SIZE;
			rcb->rcb_cmd = CCB_RECV;
			rcb->rcb_status = CCB_BUSY;	/* Transfer status */
			(void)mIntr(slic, CONS_BIN, COVEC(unit,CCB_RECV_CB));
			cb_sent++;
			buff_index = 0;
		}
	
		if (rcb->rcb_status != CCB_BUSY) {
			if (&input_buff[buff_index] < (char *)rcb->rcb_addr)
				return((int)input_buff[buff_index++] & 0xff);
			else
				cb_sent = 0;	/* Need to fetch more data */
		}
    	} while (wait);
	return(-1);			/* Can't wait */
}

/*
 * int
 * ssm_putchar(int)
 *      Write a character to the system console.
 *
 * Calling/Exit State:
 *	Assumes the SSM is the system console controller and
 *	that its SLIC address has been stored into "_slscsi".
 *
 *      No return value.
 *
 * Remarks:
 *	Console output is passed to the SSM over the SLIC
 *	a byte at a time with a SLIC-bin 1 maskable interrupt.
 *	"Newline" characters result in a "carriage-return" being
 *	sent first.
 */
void
ssm_putchar(int c)
{
	if (c == '\n') ssm_putchar('\r');
	(void)mIntr(_slscsi, 1, c);
}

/*
 * int
 * ssm_disk_init(struct iob *)
 *	Common SCSI disk-driver entry point for initializing required
 *	SSM data.
 *
 * Calling/Exit State:
 *	Invoked by the common scsi Open() routine.
 *
 *	Returns zero if initialization succeeds, -1 otherwise.
 *
 * Description:
 *	Invoke ssm_get_devinfo() to do most of the SSM specific work, 
 *	including the CB allocation.  Then allocate and initialize
 *	a drive-info structure used by the common SCSI interface for 
 *	the device.  Save its address in the iob argument to reference
 *	the SSM data within other common SCSI entry points.
 */
int
ssm_scsi_diskinit(struct iob *io)
{
	int	boardno;
	int	devno;
	struct	drive_info *dip;
	struct	ssm_sinfo *sip;

	boardno = SCSI_BOARD(io->i_unit);
	devno = io->i_unit & SCSI_MAXDEVNO;
	sip = &common_sinfo;

	if (!ssm_get_devinfo(boardno, devno, sip, 0)) {
		printf("wd(%d,0): No such ssm board\n", io->i_unit);
		io->i_error = ENXIO;
		return (-1);
	}

	/*
	 * allocate the drive info structure and initialize it
	 */
	dip = ssm_get_dip(io->i_unit);
	dip->di_cmd = ssm_scsi_cmd;
	dip->di_prefix = "wd";
	dip->di_ptr = (char *)sip;
	io->i_dptr = (char *)dip;

	/*
	 * use the local flag to indicate when ssm_get_devinfo is
	 * called: this call only needs to be made once for as long
	 * as the execution thread remains in the driver
	 */
	SAIOSCSI_FLAG(io) = 1;

	return (0);
}

/*
 * static int
 * ssm_scsi_cmd(struct iob *, struct scsiioctl *)
 * 	Execute a SCSI driver ioctl request.
 *
 * Calling/Exit State:
 * 	If ssm_get_devinfo() has already been called from 
 *	this driver thread, then the iob already has a drive-info
 *	struct and SSM/SCSI-CB allocated for it, the address of
 *	which is in the iob.
 *
 *	Return zero if the request succeeds, -1 otherwise.
 *
 * Description:
 *	Get the drives SSM/SCSI CB, format it for this command, 
 *	send the command, then process the results upon completion.
 *	Retry SCSI failures up to 4 times, except format operations.
 *	Error out adapter failures without any retries.
 *	Display a error message on the console for all failures
 *	except the retryable TEST-UNIT_READY commands failing due
 *	to the UNIT-ATTENTION condition.
 */
static int
ssm_scsi_cmd(struct iob *io, struct scsiioctl *sioctl)
{
	int	boardno;
	int	devno;
	struct	ssm_sinfo *sip;
	struct	scsi_cb *cb;
	struct	scrsense *rsense;
	unchar	compcode, opcode;

	boardno = SCSI_BOARD(io->i_unit);
	devno = io->i_unit & SCSI_MAXDEVNO;
	sip = (struct ssm_sinfo *)(void *)SAIOSCSI_PTR(io);

	/*
	 * if ssm_get_devinfo has already been called from this driver thread,
	 * then this drive already has a CB currently allocated for it
	 */
	if (!SAIOSCSI_FLAG(io)) {
		if (!ssm_get_devinfo(boardno, devno, sip, 0)) {
			io->i_error = EIO;
			return (-1);
		}
		SAIOSCSI_FLAG(io) = 1;
	}

	/*
	 * set the lun and record the opcode for later use
	 */
	sioctl->sio_cmd6.cmd_lun &= ~SCSI_UNIT(sioctl->sio_cmd6.cmd_lun);
	sioctl->sio_cmd6.cmd_lun |= SCSI_UNIT(io->i_unit);
	opcode = sioctl->sio_cmd6.cmd_opcode;
	io->i_error = 0;
	io->i_errcnt = 0;
	cb = sip->si_cb;
	for (;;) {
		/*
		 * send the command, then act according to the opcode and
		 * completion code
		 */
		compcode = ssm_scsi_send(sip, sioctl, cb);
		switch (compcode) {
		case SCB_BAD_CB:
		case SCB_NO_TARGET:
		case SCB_SCSI_ERR:
			ssm_scsi_error(cb, io, "Hard Error: ");
			return (-1);
		
		case SCB_OK:
			if ((opcode == SCSI_FORMAT) ||
			    (!SCSI_CHECK_CONDITION(cb->sh.cb_status)))
				return (0);
			rsense = (struct scrsense *) cb->sh.cb_sense;
			if (rsense->rs_error != RS_DEFERR
			&& ((rsense->rs_error & RS_ERRCLASS) != RS_CLASS_EXTEND 
			|| (rsense->rs_error & RS_ERRCODE) != RS_CODE_EXTEND)) {
				ssm_scsi_error(cb, io,
					"Hard Error, bad sense data: ");
				return (-1);
			} 
         	 	switch (rsense->rs_key & RS_ERRCODE) {
			case RS_RECERR:
				ssm_scsi_error(cb, io, "Soft Error: ");
				return (0);

			case RS_UNITATTN:
				/*
				 * If this occurs on while testing
				 * the unit, don't print a message
				 * about it here - let the caller.
				 */
				if (io->i_errcnt >= 4) {
					if (opcode != SCSI_TEST) 
					ssm_scsi_error(cb, io, "Hard Error: ");
				    return (-1);
				} 

				if (opcode != SCSI_TEST) 
					ssm_scsi_error(cb, io, "Soft Error: ");
				io->i_errcnt += 1;
				break;

			default:
				if (io->i_errcnt >= 4) {
					ssm_scsi_error(cb, io, "Hard Error: ");
					return (-1);
				}

				ssm_scsi_error(cb, io, "Soft Error: ");
				io->i_errcnt += 1;
				break;
			}
		}
	}
}

/*
 * static unchar
 * ssm_scsi_send(struct ssm_sinfo *, struct scsiioctl *, struct scsi_cb *)
 * 	Send a SCSI command to an SSM/SCSI device and await its completion.
 *
 * Calling/Exit State:
 *	sioctl contains the SCSI command to execute and the address 
 *	of its associated data buffer, if needed.
 *
 *	sip contains SLIC addresses and vectors used to notify the SSM 
 *	to begin execution of the specified SSM SCSI CB.
 *
 *	The CB must have been allocated for this device by ssm_init_scsi().
 *
 *	Return the SSM's completion status upon command termination.
 *
 * Description:
 *	Format the CB with the given SCSI command, send it to the 
 *	SSM, then wait for the completion code.  When done, return
 *	the resulting command completion status from the SSM.
 */
static unchar
ssm_scsi_send(struct ssm_sinfo *sip, 
		struct scsiioctl *sioctl, struct scsi_cb *cb)
{
	bzero((caddr_t)SWBZERO(cb), SWBZERO_SIZE);
	bzero((caddr_t)cb->sh.cb_sense, SCB_RSENSE_SZ);
	bcopy((caddr_t)&sioctl->sio_cmd, (caddr_t)cb->sh.cb_scmd,
		sioctl->sio_cmdlen);
	cb->sh.cb_count = sioctl->sio_datalength;
	cb->sh.cb_addr = sioctl->sio_addr;
	cb->sh.cb_iovec = 0;
	cb->sh.cb_cmd = (sioctl->sio_dir == SDIR_DTOH) ? SCB_READ : SCB_WRITE;
	cb->sh.cb_clen  = sioctl->sio_cmdlen;
	cb->sh.cb_compcode  = SCB_BUSY;

	/*
	 * start the ssm scsi command by interrupting the ssm
 	 * with the scsi lun || cb num
	 */
	(void)mIntr(sip->si_slic, SCSI_BIN, SCVEC(sip->si_id, 0));

	while (cb->sh.cb_compcode == SCB_BUSY)
		continue;

	return (cb->sh.cb_compcode);
}

/*
 * static void
 * ssm_scsi_printcb(struct scsi_cb *)
 * 	Display a hex dump of the SCSI command portion of the 
 *	specified SSM/SCSI cb.
 *
 * Calling/Exit State:
 *	The CB must have been allocated for this device by ssm_init_scsi().
 *
 *	No return value.
 *
 * Remarks:
 *	Interpretation of the output requires an understanding of
 *	the SCSI standard.
 */
static void
ssm_scsi_printcb(struct scsi_cb *cb)
{
	int x;

	printf("command: ");
	for (x = 0; x < (int)cb->sh.cb_clen; x++) 
		printf("0x%x ", cb->sh.cb_scmd[x]);
	printf("\n");
}

/*
 * ssm_scsi_printsense - prints sense information returned from a
 * 	scsi request sense command
 */
/*
 * static void
 * ssm_scsi_printsense(struct scrsense *)
 * 	Display the contents of the SCSI request/sense data buffer.
 *
 * Calling/Exit State:
 *	No return value.
 *
 * Remarks:
 *	This only works for valid sense data in EXTENDED format.
 *	The standard portions of the data are displayed proceeded
 *	by their field names.  The remainder is dumped in hex.
 *
 *	Interpretation of the output requires an understanding of
 *	the SCSI standard.
 */
static void
ssm_scsi_printsense(struct scrsense *rsense)
{
	int more, x;
	unchar *moredat;

	printf("sense: ");
	if ((rsense->rs_error & RS_ERRCLASS) != RS_CLASS_EXTEND) 
		printf("Don't understand request sense data\n"); 
	
	printf("Error 0x%x, Seg Num 0x%x, Key 0x%x, Info 0x%x, Additional 0x%x, ", 
		rsense->rs_error,
		rsense->rs_seg,
		rsense->rs_key, 
		*(ulong *)((void *)&rsense->rs_info[0]),
		rsense->rs_addlen);
	
	more = (int) rsense->rs_addlen;
	if (more > SCB_RSENSE_SZ - sizeof (struct scrsense))
		more = SCB_RSENSE_SZ - sizeof (struct scrsense);
	moredat = (unchar *) ((int) (&rsense->rs_addlen) + 1);
	for (x = 0; x < more; x++) 
		printf("0x%x ", moredat[x]);
	printf("\n\n");
}

/*
 * static void
 * ssm_scsi_error(struct scsi_cb *, struct iob *, char *)
 *	SCSI driver error reporting.
 *
 * Calling/Exit State:
 *	"cb" must have been allocated previously by ssm_scsi_init,
 *	when the specified iob was opened.
 *
 *	No return value.
 *
 * Description:
 *	Locate and display the contents a failed SCSI commmand and
 *	its resulting request/sense data.  Preceed it with a header
 *	identifying the failing device and message the caller provided.
 */
static void
ssm_scsi_error(struct scsi_cb *cb, struct iob *io, char *str)
{
	struct scrsense *rsense;

	rsense = (struct scrsense *) cb->sh.cb_sense;	

	if (rsense->rs_key & RS_ERRCODE) {
		printf("wd(%d,0): %s command=0x%x, sensekey=0x%x, ",
			io->i_unit, str, cb->sh.cb_scmd[0], rsense->rs_key);
		printf("sensecode=0x%x, lba=0x%x, compcode=0x%x\n",
			rsense[12], io->i_bn, cb->sh.cb_compcode);

	} else {
		printf("wd(%d,0): %s command=0x%x, lba=0x%x, compcode=0x%x\n",
			io->i_unit, str, cb->sh.cb_scmd[0], io->i_bn, 
			cb->sh.cb_compcode);
	}
		
	printf("wd(%d,0): ", io->i_unit);
	ssm_scsi_printcb(cb);

        if (rsense->rs_key & RS_ERRCODE) {
		printf("wd(%d,0): ", io->i_unit);
		ssm_scsi_printsense(rsense);
	}
}

/*
 * struct drive_info *
 * ssm_get_dip(int)
 *	Allocate a drive_info structure for the specified unit.
 *
 * Calling/Exit State:
 *	Once allocated, the drive_info struct is always saved for
 *	future references to its device.  If its already available,
 *	just reassign it for this request instead of allocating a 
 *	new one.  
 *
 *	After allocating the structure, link it onto a list of
 *	allocated drive_info structures for future reference.
 *
 *	Returns the address of the drive_info struct to use.
 */
struct drive_info *
ssm_get_dip(int unit)
{
	struct	ssm_dip *dip;

	for (dip = ssm_dip; dip != (struct ssm_dip *)0; dip = dip->s_next)
		if (dip->s_unit == unit)
			return (dip->s_dip);

	callocrnd(sizeof(int));
	dip = (struct ssm_dip *)(void *)calloc(sizeof (struct ssm_dip) +
		sizeof (struct drive_info));

	dip->s_next = ssm_dip;
	dip->s_unit = unit;
	dip->s_dip = (struct drive_info *)
		((void *)((char *)dip + sizeof (struct ssm_dip)));
	ssm_dip = dip;

	return (dip->s_dip);
}
