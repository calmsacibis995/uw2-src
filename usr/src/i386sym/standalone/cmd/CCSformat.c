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

#ident	"@(#)stand:i386sym/standalone/cmd/CCSformat.c	1.1"

/*
 * CCSformat 
 *	standalone formatter for Common Command Set (CCS) SCSI disks
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/vtoc.h>
#include <sys/saio.h>
#include <sys/scsi.h>
#include <sys/scsidisk.h>
#include <sys/ccs.h>

#define MANUALLY	0
#define FILE		1

/*
 * macros used in doing byte swapping.
 */

#define itob4(i, b) \
	b[3] = (unchar) i; \
	b[2] = (unchar) (i >> 8); \
	b[1] = (unchar) (i >> 16); \
	b[0] = (unchar) (i >> 24);

#define itob3(i, b) \
	b[2] = (unchar) i; \
	b[1] = (unchar) (i >> 8); \
	b[0] = (unchar) (i >> 16);

#define cmd_pgcode cmd_lba[0]		/* page code for MODE SENSE/SELECT */

/*
 * tasks that can be done by the formatter
 */

struct task_type {
	int tt_cmd;
	char *tt_desc;
};

/*
 * tasks performable by the formatter
 * (DISPLAY is not yet implemented -- make sure tasks that appear
 * on the menu are numbered in the order they appear!)
 */
#define EXIT	0
#define FORMAT	1
#define ADDBAD	2
#define WRITEVTOC 3
#define DISPLAY	4

#define DFLT_RPM	3600

/*
 * Diagnostic Block Structure
 */
#define	CSD_DIAG_PAT_BYTES	(512 - sizeof(uint))

struct csd_db {
	unchar	csd_db_pattern[CSD_DIAG_PAT_BYTES];	/* test pattern */
	uint	csd_db_blkno;				/* block number */
};

/*
 * Worse case csd pattern: e739c
 */
#define	CSD_DIAG_PAT_0		0xe7	/* must be in byte 0 of the block */
#define	CSD_DIAG_PAT_1		0x39	/* must be in byte 1 of the block */
#define	CSD_DIAG_PAT_2		0xce	/* must be in byte 2 of the block */
#define	CSD_DIAG_PAT_3		0x73	/* must be in byte 3 of the block */
#define	CSD_DIAG_PAT_4		0x9c	/* must be in byte 4 of the block */
#define	CSD_DIAG_PAT_SIZE	5	/* repeated pattern is 5 bytes long */

#define CCS_ALIGN	16	
#define LINELIMIT	128
#define NIL		-999

extern int atoi(char *);
extern void bcopy(void *, void *, size_t);
extern void bzero(void *, size_t);
extern caddr_t calloc(int);
extern void callocrnd(int);
extern void exit(int);
extern char *index(char *, char);
extern char *prompt(char *);
extern void strcpy(char *, char *);
extern int strlen(char *);
extern int strncmp(char *, char *, int);
extern long vtoc_get_cksum(struct vtoc *);

extern	struct	drive_type drive_table[];
extern	int	maxbadlist;		/* maximum number of bad blocks */
extern	int	list[];			/* list of bad blocks to be added */

static void get_disk_name(void);
static void printexamples (int);
static int is_CCS_disk(void);
static void get_disk_type(void);
static void display_menu(void);
static void addbad(void);
static int addlist(int);
static int getlist(void);
static void init_get_spot(void);
static int get_spot(int);
static void write_diag(void);
static void fill_pat(unchar *, uint, unchar *, uint);
static void write_min_vtoc(void);
static int xatoi(char *s);
static void initlist(void);
static int reassign_blocks(int);
static void format(void);

static int fd = -1;			/* fd for disk we are working on */
static char filename[LINELIMIT];	/* name of disk */
static int addindex, getindex;		/* ptrs into bad block list */
static struct drive_type *drive;	/* drive-specific information */
static int capacity = 0;

static struct task_type task_table[] = {
	{ EXIT,		"Exit from CCSformat",				},
	{ FORMAT,	"Format disk using existing bad block lists",	},
	{ ADDBAD,	"Addbad - add blocks to existing badlists",	},
	{ WRITEVTOC,	"Write a new minimal VTOC",			},
	{ -1,		},
};


/*
 * void
 * main(void)
 * 	Interactive format operations for a SCSI disk.
 *
 * Calling/Exit State:
 *	Depending on the tasks selected by the operator,
 *	may completely format, map out additional defects, 
 *	and/or write a VTOC on the specified disk(s). 
 *
 *	No return value.
 *
 * Description:
 *	This program is interactive with the console operator.
 *	It will prompt the operator for tasks to perform and
 *	devices upon which to perform them, displaying a task
 *	list menu to assist with selection.  Both the format
 *	and write VTOC operations write, so backup necessary
 *	data prior to invoking them.  The ADDBAD operation
 *	will locate a replacement for the physical sector
 *	currently mapped to the specified logical sector, while
 *	preserving the data from the original sector if possible.
 */
void
main(void)
{
	int token;
	char *cp;

	get_disk_name();
	if (! is_CCS_disk())
		exit(1);
	
	get_disk_type();

	/*
	 * main command loop
	 */
	
	display_menu();

	for(;;) {
		cp = prompt("task? ");
		if (!*cp) {
			display_menu();
			continue;
		}
		token = xatoi(cp);
		switch (token) {

		case FORMAT:
			format();
			cp = prompt("Write VTOC? [n/y] ");
			if (*cp == 'y' || *cp == 'Y')
				write_min_vtoc();
			cp = prompt("Write diagnostic patterns? [n/y] ");
			if (*cp == 'y' || *cp == 'Y')
				write_diag();
			break;

		case ADDBAD:
			addbad();
			break;

		case WRITEVTOC:
			write_min_vtoc();
			exit(0);
			/*NOTREACHED*/

		case EXIT:
			exit(0);
			/*NOTREACHED*/

		default:
			printf("CCSformat: bad selection\n");
			display_menu();
			break;
		}
	}
}

/*
 * static void
 * get_disk_name(void)
 * 	Solicit for the name of the disk to perform task(s) upon. 
 *
 * Calling/Exit State:
 *	Returns a file descriptor for the selected device in
 *	the global variable "fd" after opening it for the caller.
 *
 * Description:
 *	Prompts the console operator for the device to be opened
 *	and returned to the caller.  The device name is expected
 *	to be in the format of "wd(x,y)", where x is a SCSI device
 *	address corresponding to 8 * target i.d. of the device,
 *	and y is the partition number.  The partition number specified
 *	is irrelevant as it will be replaced with V_NUMPAR to tell 
 *	the driver to "ignore the VTOC and use the whole disk".  
 *	Then it attempts to open that device, storing its file 
 *	descriptor in "fd".
 *
 *	If the device name is invalid or the open fails, print out
 *	an explanitory message and try again.
 */
static void
get_disk_name(void)
{
	char *pp;
	char partbuf[20];
	char *dp, *cp, *fn;
	int n = V_NUMPAR;
	int example = 1;

	/*
	 * Load up partbuf with the character translation of V_NUMPAR.
	 */
	bzero(partbuf, 20);
	pp = partbuf + sizeof(partbuf) - 1;
	*pp-- = '\0';
	do {
		*pp-- = "0123456789"[n%10];
		n /= 10;
	} while (n);
	pp++;

	while (fd < 0) {
		cp = prompt("\nDevice to format? ");

		if (cp[0] == 'w' && cp[1] == 'd') {
			/* 
			 * Copy name to filename
			 */
			for (fn = filename; (*fn = *cp) != NULL; fn++, cp++)
				continue;
			/* 
			 * Replace partition number with V_NUMPAR
			 */
			dp = index(filename, ',');
			dp++;
			strcpy(dp, pp);
			dp = filename + strlen(filename);
			strcpy(dp, ")");
			/*
			 * Now open
			 */
			if ((fd = open(filename, O_RDWR)) < 0) {
				printf("Open failed. Check input, cables ");
				printf("and drive switch settings.\n");
				printexamples(example++);
			}
		} else
			printexamples(example++);

		if (example > 7)
			example = 0;
	}
}

/*
 * static void
 * printexamples(int)
 *	Display an example of acceptable input for a device specifier.
 *
 * Calling/Exit State:
 *	"targ" contains the SCSI target i.d. to use for the example.
 *
 *	No return value.
 */
static void
printexamples(int targ)
{
	printf("Examples for target adaptor %d:\n",targ);
	printf("\tad(%d,0), sd(%d,0), wd(%d,0), qd(%d,0)\nor\n",
		targ << 3,
		targ << 3,
		targ << 3,
		targ << 8);
	printf("\tad(0x%x,0), sd(0x%x,0), wd(0x%x,0), qd(0x%x,0)\n",
		targ << 3,
		targ << 3,
		targ << 3,
		targ << 8);
}

/*
 * static int
 * is_CCS_disk(void)
 * 	Decide if the opened device is a supported CCS disk or not.  
 *
 * Calling/Exit State:
 *	The device in question is already open with its descriptor
 *	specified by the global variable "fd".
 *
 *	Return 0 and display an error message if not a CCS disk.
 *	Return 1 otherwise.
 *
 * Description:
 *	Execute a SCSI-INQUIRY command to the specified device
 *	and decipher the returned data's "format" byte to determine
 *	if the device is CCS compliant.
 */
static int
is_CCS_disk(void)
{
	struct	scsiioctl sioctl;
	struct	sdinq inq;

	/*
	 * set up the command block.  A short INQUIRY will be done.
	 */
	bzero((caddr_t) &sioctl, sizeof (struct scsiioctl));
	bzero((caddr_t) &inq, sizeof (struct sdinq));
	sioctl.sio_datalength = SIZE_INQ;
	sioctl.sio_addr = (ulong)&inq;
	sioctl.sio_cmdlen = SCSI_CMD6SZ;
	sioctl.sio_cmd6.cmd_opcode = SCSI_INQUIRY;
	sioctl.sio_cmd6.cmd_length = SIZE_INQ;

	if (ioctl(fd, SAIOSCSICMD, (char *)&sioctl) < 0) {
		printf("CCSformat: problems doing INQUIRY\n");
		exit(1);
	}

	if (inq.sdq_format == 0) {
		printf("CCSformat: This is not a supported CCS disk.\n");
		return(0);
	}

	return(1);		/* It is CCS compliant. */
}


/*
 * static int
 * get_disk_type(void)
 * 	Decide if the opened device is a supported CCS disk or not.  
 *
 * Calling/Exit State:
 *	The device in question is already open with its descriptor
 *	specified by the global variable "fd".
 *
 *	Set the global variable "drive" to address the device's 
 *	corresponding entry in the drive type information table.
 *
 *	Set the global variable "capacity" to the last addressable
 *	block number of the device.
 *
 *	No return value.
 *
 * Description:
 *	Execute a SCSI-INQUIRY command to the specified device
 *	and decipher the returned data's vendor and product i.d.
 *	strings to determine if there is an entry in the drive_table
 *	for that device type.  Also, execute a SCSI READ-CAPACITY
 *	command to retreive its last addressable block.  This is used
 *	to determine the drive's format capacity, geometry, and
 *	minimal VTOC layout.
 */
static void
get_disk_type(void)
{
	int	i;
	struct	scsiioctl sioctl;
	struct	sdinq inq;
	struct	drive_type *dp;
	struct	readcarg readcarg;

	/*
	 * set up the inquiry command
	 */
	bzero((caddr_t) &sioctl, sizeof (struct scsiioctl));
	bzero((caddr_t) &inq, sizeof (struct sdinq));
	sioctl.sio_datalength = SIZE_INQ_XTND;
	sioctl.sio_addr = (ulong)&inq;
	sioctl.sio_cmdlen = SCSI_CMD6SZ;
	sioctl.sio_cmd6.cmd_opcode = SCSI_INQUIRY;
	sioctl.sio_cmd6.cmd_length = SIZE_INQ_XTND;

	if (ioctl(fd, SAIOSCSICMD, (char *)&sioctl) < 0) {
		printf("CCSformat: failed on INQUIRY command\n");
		exit(1);
	}

	/*
	 * print out the id string from the INQUIRY command.  Need to
	 * count the chars because drive does not null-terminate the
	 * strings.
	 */
	printf("\n%s ID string: vendor ", filename);
	for (i = 0; i < SDQ_VEND; i++)
		printf("%c", inq.sdq_vendor[i]);
	printf(", product ");
	for (i = 0; i < SDQ_PROD; i++)
		printf("%c", inq.sdq_product[i]);
	printf(", revision ");
	for (i = 0; i < SDQ_REV; i++)
		printf("%c", inq.sdq_revision[i]);
	printf("\n\n");

	/*
	 * search the drive table for the ID string
	 */
	for (dp = drive_table; dp->dt_vendor; dp++)
		if (strncmp(inq.sdq_vendor, dp->dt_vendor, SDQ_VEND) == 0
		    && strncmp(inq.sdq_product, dp->dt_product, SDQ_PROD) == 0)
			break;

	if (!dp) {
		printf("CCSformat: could not find ID string in drive table\n");
		exit(1);
	}

	drive = dp;

	if (inq.sdq_format != drive->dt_inqformat) {
		printf("%s: Not a supported CCS Disk.\n", filename);
		exit(1);
	}

	/*
	 * set up for READ CAPACITY
	 */
	bzero((caddr_t) &sioctl, sizeof (struct scsiioctl));
	bzero((caddr_t) &readcarg, sizeof (struct readcarg));
	sioctl.sio_datalength = sizeof (struct readcarg);
	sioctl.sio_addr = (ulong)&readcarg;
	sioctl.sio_cmdlen = SCSI_CMD10SZ;
	sioctl.sio_cmd10.cmd_opcode = SCSI_READC;

	if (ioctl(fd, SAIOSCSICMD, (char *)&sioctl) < 0) {
		printf("CCSformat: failed on READ CAPACITY command\n");
		exit(1);
	}

	capacity = (readcarg.nblocks[0] << 24) |
		(readcarg.nblocks[1] << 16) |
		(readcarg.nblocks[2] << 8) |
		(readcarg.nblocks[3]);
}
		
/*
 * static void
 * display_menu(void)
 *	Display the task menu to the console.
 *
 * Calling/Exit State:
 *	The "task_table" is initialized at compile time
 *	with the strings to display and their corresponding
 *	action code.  It is terminated with an record 
 *	containing a NULL string. 
 *
 *	No return value.
 */
static void
display_menu(void)
{
	struct task_type *tp;

	printf("Task Menu:\n\n");
	for (tp = task_table; tp->tt_desc; tp++)
		printf("(%d)       %s\n", tp->tt_cmd, tp->tt_desc);

	printf("\n");
}


/*
 * static void
 * addbad(void)
 * 	Add bad blocks to the disk's defect map.
 *
 * Calling/Exit State:
 *	The device to use is already open with its descriptor
 *	specified by the global variable "fd".
 *
 *	The global variable "capacity" contains the last addressable
 *	block number for the device.
 *
 *	The new defects will be added to the drive's defect
 *	map and replacement sectors found for them (the logical
 *	sector i.d. remains usable, but maps to a new physical
 *	sector including attempting to recover/restore its data ).  
 *
 *	No return value.
 *
 * Description:
 *	Prompts the console operator to determine if they would like
 *	to enter the defect numbers (logical sector #'s) manually or
 *	from a file located on the system.  If manually, then prompt
 *	for the defects one at a time, building up the list internally.
 *	Otherwise it parses the list from the file specified after
 *	prompting further for it, in the standard standalone path 
 *	notation, i.e., "wd(0,2)defects".  The maximum number of entries 
 *	that can be input at one time is currently 256 (MAXBADLIST).
 *
 *	Once the list is input, it invokes reasign_blocks() for each
 *	element to find a replacement.  It also uses list management
 *	abstractions to initialize the list, then add elements to and 
 *	retrieve elements from it.
 */
static void
addbad(void)
{
	int valid = 0;
	char *cp;
	char *file;
	int finished;
	int n;
	int spot;
	int bfd;


	while (! valid) {
		printf("Enter defect list manually (%d) or from a file (%d)? ",
			MANUALLY, FILE);
		cp = prompt("");

		switch (xatoi(cp)) {

		case MANUALLY:
			printf("Enter list of defects, %d maximum\n",
				maxbadlist);
			printf("Entries should be positive integers between ");
			printf("0 and %d inclusive\n\n", capacity);
			printf("(type RETURN to terminate list)\n");

			initlist();
			init_get_spot();
			finished = n = 0;

			while (! finished) {
				printf("Enter logical address of bad spot %d: ",
					n++);
				spot = get_spot(-1);

				if (spot == -1) {
					printf("illegal input\n");
					n--;
					continue;
				}
						
				if (spot > capacity) {
					printf("%d is greater than disk capacity (%d)\n",
						spot, capacity);
					n--;
					continue;
				}
				if (spot == NIL || addlist(spot))
					finished = 1;
			}

			cp = prompt("Proceed with addbad? [n/y] ");
			if (*cp == 'y' || *cp == 'Y')
				while ((spot = getlist()) != NIL)
					if (reassign_blocks(spot) < 0) {
						printf("aborting addbad\n");
						break;
					}
			valid = 1;
			break;

		case FILE:

			file = prompt("File with bad spot information? ");
			if ((bfd = open(file, O_RDONLY)) < 0) {
				printf("CCSformat: cannot open %s\n", file);
				continue;
			}

			/*
			 * get bad spots from the file, adding them to
			 * the list.  list is NIL terminated.
			 */
			
			initlist();
			init_get_spot();

			while ((spot = get_spot(bfd)) != NIL) {

				if (spot == -1)
					continue;
					
				if (spot > capacity) {
					printf("%d is greater than disk capacity (%d)\n",
						spot, capacity);
					continue;
				}

				if (addlist(spot) < 0)
					break;
			}

			cp = prompt("Proceed with addbad? [n/y] ");
			if (*cp == 'y' || *cp == 'Y')
				while ((spot = getlist()) != NIL)
					if (reassign_blocks(spot) < 0) {
						printf("aborting addbad\n");
						break;
					}
			close(bfd);

			valid = 1;
			break;

		default:
			printf("CCSformat: bad option\n");
			break;
		}
	}
}

/* 		badlist management functions 			*/

/*
 * static void
 * initlist(void)
 *	Initialize the internal badlist for adding to the defect map.
 *
 * Calling/Exit State:
 *	Resets the producer (addindex) and consumer (getindex) 
 *	indicies of the internal defect list to make the list
 *	appear empty, along with clearing its entries.
 *
 *	No return value.
 */
static void
initlist(void)
{
	int i;

	for (i = 0; i < maxbadlist; i++)
		list[i] = NIL;

	addindex = getindex = 0;

}

/*
 * static int
 * addlist(int)
 *	Add an block number to the internal badlist for deffect mapping.
 *
 * Calling/Exit State:
 *	Adjusts the producer (addindex) index of the internal defect 
 *	list after adding the specified block# to it in the next 
 *	available entry.
 *
 *	Returns zero if the defect was successfully added to the
 *	list.  Returns -1 if the list is full.
 */
static int
addlist(int spot)
{
	
	if (addindex == maxbadlist) {
		printf("No more bad blocks.  Add the existing list first\n");
		return(-1);
	}

	list[addindex++] = spot;
	if (spot != NIL)
		printf(" -- added %d to list\n", list[addindex - 1]);
	return(0);
}

/*
 * static int
 * getlist(int)
 *	Retrieve the next block number from the internal badlist 
 *	for deffect mapping.
 *
 * Calling/Exit State:
 *	Adjusts the consumer (getindex) index of the internal defect 
 *	list after retrieving the next block# from its next valid entry.
 *
 *	Returns NIL if the end of the defect list is reached, which is 
 *	marked by a NIL enry or maxbadlist elements reached. Otherwise, 
 *	returns the block number of the next consumer entry.
 */
static int
getlist(void)
{
	if ((getindex == maxbadlist) || (list[getindex] == NIL))
		return(NIL);
	else
		return(list[getindex++]);
}

static int  count;
static char addbuf[1024];
static char *cp;
static char *lastline;

/*
 * static void
 * init_get_spot(void)
 *	Initialize the buffers for soliciting defect entries.
 *
 * Calling/Exit State:
 *	No return value.
 */
static void
init_get_spot(void)
{
	count = 0;
	cp = lastline = addbuf;
}

/*
 * static int
 * get_spot(int)
 *	Input the address of a bad spot.
 *
 * Calling/Exit State:
 *	If ifd is non-negative, it is corresponds to an open file 
 *	descriptor from which input is retrieved.  Otherwise, 
 *	solicit input from the console.
 *
 *	init_get_spot() must have already  been invoked to initialize 
 *	the input buffers for this function.
 *
 *	Returns NIL when the end of input is reached.  
 * 	Returns -1 if the defect data is invalid.  
 *	Otherwise, returns the logical block number which was input.
 */
static int
get_spot(int ifd)
{
	int nread;
	int lba;

	if (ifd < 0) {				/* console input */

		cp = prompt("");
		if (!*cp)
			return(NIL);
		return(xatoi(cp));

	} else {
		if (cp == lastline) {
			count = count - (lastline - addbuf);
			bcopy(lastline, addbuf, count);
			lastline = &addbuf[count];
			/* note kludge for ts tape */
			nread = read(ifd, lastline,
				((sizeof addbuf) - count) & ~511);
			count += nread;

			/*
			 * Scan the adlist buffer replacing newlines 
			 * with NUL and set lastline to the end of the 
			 * last full line of input in the buffer.  If
			 * there is no further input then put a NUL
			 * in the buffer to terminate input.
			 */
			if (count == 0) {
			 	addbuf[0] = 0;
			} else {
				for (cp = addbuf; cp < addbuf + count; cp++) {
					if (*cp == '\n') {
						*cp = 0;
						lastline = cp + 1;
					}
				}
			}
			cp = addbuf;
		}

		lba = xatoi(cp);
		if (lba < 0) {
			printf("illegal data in input file:\n");
			printf("%s\n", cp);
			lba = -1;
		}

		if (!*cp)
			return(NIL);
		while (*cp++ != 0) 
			continue;
		return (lba);
	}
}

/*
 * static int
 * reassign_blocks(int)
 * 	Instruct the disk to replace a bad sector.
 *
 * Calling/Exit State:
 *	"block" specifies the logical block number for which
 *	the physical block requires replacement with another
 *	physical block (logical block remains usable).
 *	
 *	The device is already open with its descriptor
 *	specified by the global variable "fd".
 *
 *	Return zero if the SCSI REASSIGN-BLOCK command
 *	succeeds and the block is replaced.  Return -1 
 *	if it fails.
 */
static int
reassign_blocks(int block)
{
	struct	scsiioctl sioctl;
	struct	reassarg reassarg;

	bzero((caddr_t) &sioctl, sizeof (struct scsiioctl));
	bzero((caddr_t) &reassarg, sizeof (struct reassarg));
	sioctl.sio_datalength = 4 + drive->dt_reasslen;
	sioctl.sio_addr = (ulong)&reassarg;
	sioctl.sio_cmdlen = SCSI_CMD6SZ;
	sioctl.sio_cmd6.cmd_opcode = SCSI_REASS;
	reassarg.length[1] = drive->dt_reasslen;
	itob4(block, reassarg.defect);

	printf(" -- revectoring %d --\n", block);
	if (ioctl(fd, SAIOSCSICMD, (char *)&sioctl) < 0) {
		printf("CCSformat: problem with REASSIGN BLOCKS\n");
		return(-1);
	}
	return(0);
}

/*
 * static void
 * format(void)
 * 	(Re)format the disk using the manufactures defect list and
 *	added defect information, i.e., the drive's P and G lists.
 *
 * Calling/Exit State:
 *	The device to format is already open with its descriptor
 *	specified by the global variable "fd".
 *
 *	The drive already contains the defect lists internally.
 *
 *	Display a console message indicating the success or 
 *	failure of this operation.
 *
 *	No return value.
 *
 * Description:
 *	Prompt the console operator to confirm that they are
 *	still wanting to proceed, since formatting the drive
 *	overwrites its contents.  Abort the task if they respond
 *	"no".  Otherwise, issue a SCSI FORMAT command to the drive
 *	instructing it to reformat itself with its internal defect
 *	lists.  Formatting takes several minutes for this command
 *	to complete, during which time the system may appear hung
 *	because it must busy/wait for completion - give it time!
 */
static void
format(void)
{
	char	*chp;
	struct	scsiioctl sioctl;

	chp = prompt("Last chance before format ... proceed? [n/y] ");
	if (*chp == 'y' || *chp == 'Y') {
		
		printf("Beginning disk format ... \n");
		bzero((caddr_t) &sioctl, sizeof (struct scsiioctl));
		sioctl.sio_cmdlen = SCSI_CMD6SZ;
		sioctl.sio_cmd6.cmd_opcode = SCSI_FORMAT;
		sioctl.sio_cmd6.cmd_lun = drive->dt_formcode;

		if (ioctl(fd, SAIOSCSICMD, (char *)&sioctl) < 0)
			printf("CCSformat: format failed\n");
		else 
			printf("Format complete\n");
	}
}

/*
 * static void
 * write_diag(void)
 *	Write a diagnostic pattern in the last to cylinders of the disk.
 *
 * Calling/Exit State:
 *	The device to use is already open with its descriptor
 *	specified by the global variable "fd".
 *
 *	Display console messages indcating the success or failure
 *	of this operation.
 *
 *	No return value.
 *
 * Description:
 *	Request geometry information from the disk driver, to be
 *	used for locating the last two cylinders of the selected 
 *	drive.  Then format a buffer with the pattern which should
 *	be written to those two cylinders and then write them
 *	out.  The systems diagnostics expects a specific pattern
 *	in that specific location, so its imperitive to keep this
 *	code up to date with the diagnostic utilities.
 */
static void
write_diag(void)
{
	struct csd_db *db, *db_buf;
	int i, n, count;
	uint lba, diag_end, diag_start;
	struct st st;			/* device data */
	struct	scsiioctl sioctl;
	struct	readcarg readcarg;	/* read capacity data */
	static unchar pat_default[] = {	 /* diagnostic track pattern */
		CSD_DIAG_PAT_0, CSD_DIAG_PAT_1, CSD_DIAG_PAT_2,
		CSD_DIAG_PAT_3, CSD_DIAG_PAT_4
	};

	/* get the device data to determine where the diag blocks start */
	if (ioctl(fd, SAIODEVDATA, (char *)&st) < 0) {
		printf("CCSformat: can't get device data\n");
		exit(1);
	}

	/* read the capacity to determine where the diag blocks stop */
	bzero((caddr_t) &sioctl, sizeof (struct scsiioctl));
	bzero((caddr_t) &readcarg, sizeof (struct readcarg));
	sioctl.sio_datalength = sizeof (struct readcarg);
	sioctl.sio_addr = (ulong)&readcarg;
	sioctl.sio_cmdlen = SCSI_CMD10SZ;
	sioctl.sio_cmd10.cmd_opcode = SCSI_READC;

	if (ioctl(fd, SAIOSCSICMD, (char *)&sioctl) < 0) {
		printf("CCSformat: failed on READ CAPACITY command\n");
		exit(1);
	}
	diag_end = (readcarg.nblocks[0] << 24) | (readcarg.nblocks[1] << 16)
		 | (readcarg.nblocks[2] << 8)  | (readcarg.nblocks[3]);
	diag_end++;	/* one beyond the last block */
	/* two cylinders worth of diagnostic data */
	diag_start = diag_end - (st.nspc * 2);

	/*
	 * get a buffer to hold one cylinder -
	 * align it to 8-byte boundary for SSM
	 * based disks
	 */
	callocrnd(CCS_ALIGN);
	db_buf = (struct csd_db *)(void *)calloc(st.ncyl * sizeof(*db));
	if (!db_buf) {
		printf("CCSformat: calloc failed.  No buffer for patterns.\n");
		exit(1);
	}

	/* lay in the patterns; they're identical in all blocks */
	for (db = db_buf, i = 0; i < st.nspc; db++, i++)
		fill_pat(pat_default, sizeof pat_default,
			db->csd_db_pattern, sizeof(db->csd_db_pattern));

	printf("Writing diagnostic tracks (%d..%d)... \n",
		diag_start, diag_end-1);

	if (lseek(fd, diag_start * 512, 0)) {
		printf("CCSformat: Can't seek to diagnostic block %d\n",
			diag_start);
		exit(1);
	}

	n = st.nspc;
	for (lba = diag_start; lba < diag_end; lba += n) {
		if (lba + n > diag_end)
			n = diag_end - lba;
		for (db = db_buf, i = 0; i < n; db++, i++)
			db->csd_db_blkno = lba + i;
		i *= sizeof(*db);
		if ((count = write(fd, (char *)db_buf, i)) != i) {
			if (count > 0)
				lba += count / sizeof(*db);
			printf("CCSformat: Error writing diagnostic block %d\n",
				lba);
			break;
		}
	}

	printf("...Done\n");
}

/*
 * static void
 * fill_pat(unchar *, uint, unchar *, uint)
 *	Fill a buffer repeatedly with a copy of the specified pattern.
 *
 * Calling/Exit State:
 *	The pattern and destination buffer address/sizes are 
 *	provided by the caller
 *
 *	The pattern buffer will by copied a byte at a time to the
 *	destination buffer until it fills.  When the end of the
 *	pattern is reached, start back at its beginning and 
 *	continue to fill the destination buffer until full.
 *
 *	No return value.
 */
static void
fill_pat(unchar *src, uint nsrc, unchar *dst, uint ndst)
{
	unchar *s = src;
	unchar *d = dst;
	unchar *sx = s + nsrc;
	unchar *dx = d + ndst;

	while (d < dx) {
		*d++ = *s++;
		if (s >= sx)
			s = src;
	}
}

/*
 * static void
 * write_min_vtoc(void)
 *	Write a minimal VTOC on the disk.
 *
 * Calling/Exit State:
 *	The device to use is already open with its descriptor
 *	specified by the global variable "fd".
 *
 *	The global variable "drive" addresses the device's 
 *	corresponding entry in the drive type information table,
 *	which contains a default description of the minimal VTOC 
 *	for this device.
 *
 *	The global variable "capacity" contains the last addressable
 *	block number for the device.
 *
 *	Display a console message indicating the success or 
 *	failure of this operation.
 *
 *	No return value.
 *
 * Description:
 *	Display the default VTOC format for the device on the
 *	console and request the operator to either confirm that
 *	is what they wish to use or have them enter another layout.
 *	Then fill out a VTOC in a buffer to be written to the
 *	disk, seek to the standard VTOC location, and write the
 *	new VTOC onto the disk.
 *
 * Remarks:
 *	The minimal VTOC reserves areas for boot and diagnostic
 *	data, as well as one filesystem slice.  The filesystem
 *	slice is by default intended to be used for loading the
 *	bootstrapping software and normally corresponds to what
 *	will eventually become a swap partition.
 *
 *	The VTOC is allocated and written using V_SIZE bytes 
 *	instead of sizeof (struct vtoc) since the latter is
 *	not a multiple of device block size and to ensure that 
 *	padding reserved for furture use is zeroed and will
 *	not be misinterpreted for valid data.
 */
static void
write_min_vtoc(void)
{
	struct vtoc *v;
	int where;
	int partno = 1;
	struct partition part; 
	
	part = drive->dt_part;
	callocrnd(CCS_ALIGN);
	v = (struct vtoc *)(void *)calloc(V_SIZE);

	printf("Standard minimal VTOC for this disk:\n\n");
	do {
		printf("Partition: %d\n", partno);
		printf("Offset in sectors: %d\n", part.p_start);
		printf("Size in sectors: %d\n", part.p_size);
		cp = prompt("Use this layout (y/n)? ");
		if (*cp != 'y' && *cp != 'Y') {
			do {
				partno = atoi(
					prompt("Partition number (0-254)? "));
			} while (partno < 0 || partno > 254);
			part.p_start = atoi(prompt("Offset (sectors)? "));
			part.p_size = atoi(prompt("Size (sectors)? "));
		}
	} while (*cp != 'y' && *cp != 'Y');

	v->v_sanity = VTOC_SANE;
	v->v_version = V_VERSION_1;
	v->v_size = sizeof(struct vtoc);
	v->v_nparts = partno + 1;
	v->v_secsize = DEV_BSIZE;
	v->v_ntracks = drive->dt_st.ntrak;
	v->v_nsectors = drive->dt_st.nsect;
	v->v_ncylinders = drive->dt_st.ncyl;
	v->v_rpm = DFLT_RPM;
	v->v_capacity = capacity + 1;   /* last lba + 1 */
	v->v_nseccyl = drive->dt_st.nspc;
	strcpy(v->v_disktype, drive->dt_diskname); 
	v->v_part[partno] = part;
	v->v_cksum = 0;
	v->v_cksum = vtoc_get_cksum(v);

	printf("Writing VTOC to sector %d.\n", V_VTOCSEC);

	where = V_VTOCSEC << DEV_BSHIFT;
	(void)lseek(fd, where, 0);
	if (write(fd, (char *)v, V_SIZE) != V_SIZE) {
		printf("Unable to write VTOC\n");
		return;
	}
	printf("VTOC written.\n");
}

/*
 * static int
 * xatoi(char *)
 *	Convert a string to integer.
 *
 * Calling/Exit State:
 *	The argument specifies an ascii string which should
 *	contain decimal characters 0..9 only.
 *
 *	Returns -1 if the string contains non-decimal characters.
 *	Returns the integer representation for the string otherwise.
 *
 * Remarks:
 *	This eventually invokes atoi(), but only after confirming
 *	that all of the input is numeric.
 */
static int
xatoi(char *s)
{
	char *chp;

	for (chp = s; *chp; chp++)
		if (*chp < '0' || *chp > '9')
			return(-1);
	return(atoi(s));
}
