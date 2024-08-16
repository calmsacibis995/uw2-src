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

#ident	"@(#)stand:i386sym/standalone/cmd/dump.c	1.1"

/*
 * dump
 *	standalone memory dumper
 */

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/boot.h>
#include <sys/cfg.h>
#include <sys/slicreg.h>
#include <sys/sysmacros.h>
#include <sys/saio.h>

#define DUMP_MAGIC	0xdeadbabe	/* This magic number is expected to be
					 * placed in the first word of the
					 * first sector of the dump, indicating
					 * a valid dump exists after it.
					 */
#define VER1		101 
#define UNIT_SZ		(8*1024)
#define BITS		13

#define EOF_MAP		-2
#define SEG_MISSING	-1

struct dump_info {
	long dump_magic;		/* magic number */
	int dump_size;			/* size of dump */
};

/*
 * Version of dump_info for compressed crashes
 */
struct cdump_info {
	unsigned dump_magic;		/* magic number */
	int dump_size;			/* size of dump */
	int compressed_size;		/* size of dump */
	int revision;			/* version of compression algorithm */
	unsigned seg_size;		/* unit of compression */
	unsigned comp_size;		/* total size (unused) */
	unsigned int data_end;		/* offset to end of data in file */
	unsigned dmesg_end;		/* offset to end of dmesg (unused) */
};

#define K1		1024
#define K32		(32 * K1)
#define M1		(K1 * K1)
#define K32_PER_CLICK	(MC_CLICK / K32)

#define OPTARGS		2		/* Number of optional arguments */
#define TRUE		1
#define FALSE		0
#define	IS_TAPE(f)	(devsw[iob[(f)-3].i_dev].dv_flags == D_TAPE)
#define	IS_DIGIT(d)	((d) >= '0' && (d) <= '9')

static struct dump_info di = {			/* dump magic number and size */
	DUMP_MAGIC,
	-1,
};

static int	*magic;
static caddr_t	magic_buf;
static int	psize;				/* size of the partition */
static int	chunk;
static char	* mem_pointer;
static int	mem_left;

static char * next_word(char *, char *);
static int find_where(char **, uint *, int *);
static int already_a_dump(char *, uint);
static void set_dump_size(int, uint);
static int dumpmem(int, uint);
static void abort(char *);

extern int atoi(char *);
extern caddr_t calloc(int);
extern void callocrnd(int);
extern void _stop(char *);
extern int strcmp(char *, char *);
extern int vtoc_getpsize(char *);

/*
 * void
 * main(void)
 *	Standalone memory dumper.
 *
 * Calling/Exit State:
 *	Assumes that core memory still contains the image of the
 *	state to be dumped.  If it's been cleared, the dump is no good.
 *
 *	Upon successful completion, the dump-image is located in the
 *	specified media location(s).  Typically this is an area which
 *	the kernel uses for swapping, since there is no need to preserve
 *	it between kernel boots.  The dump must then be recovered from 
 *	there before being overwritten by swap activity after rebooting;
 *	The savecore utility typically does this during bootup to 
 *	multi-user mode.
 *
 *	No return value.
 *
 * Description:
 * 	Invoke dump using the following syntax:
 *
 *	    bh '<flags><dumpername> <dumpdev> <offset> <unixname> [<size>] [-o]'
 *	    bh '<flags><dumpername> -f <dumplistfile> <unixfile> [-o]'
 *
 * 	for example:
 *
 *	    bh '88wd(0,0)stand/dump wd(0,1) 1000 /dev/wd0s1'
 *	    bh '88wd(0,0)stand/dump -f wd(0,0)/etc/DUMPLIST /etc/DUMPLIST'
 *
 * 	It copies memory to <offset> blocks past the beginning of <dumpdev>.  
 *	<unixname> should mean the same device to unix as <dumpdev> does 
 *	to the standalone (for the convenience of savecore). -f allows 
 *	multiple dump devices to be specifed.  Each line in <unixfile> is 
 *	of the format:
 * 		dumpdev1 offset1 unixname1 [size1]
 * 		dumpdev2 offset2 unixname2 [size2]
 *
 * 	<size> is used to override the amount of memory to dump.
 * 	The size, specified in blocks, is limited to (size of the 
 *	partition) - <offset>) and is set to be the min of this limit 
 *	and the size of memory read from the config structure.  <size> 
 *	will not be allowed to be greater than the amount of memory 
 *	configured or the size of the partition.  -o forces an overwrite 
 *	of an existing dump 
 *
 * Remarks:
 *	Using bootflags of '88' for booting the dumper bypasses the 
 *	clearing of core memory by the firmware prior to loading dump.
 *	The dump string ("bh '88wd(0,0)stand/dump...") should be stored
 *	permanently via the firmware monitor command.  then "bh doAux0" 
 *	will be sufficient or it can occur automatically after a panic.
 *
 *	The dumper is loaded and executed in the same area that the 
 *	unix Kernel's text normally would have been.  This is because
 *	a copy of it can be obtained from the kernel's object file
 *	for debugging.
 *	
 *	For dumping the kernel, the config structure should be placed
 *	before the end of kernel text space by the firmware. 
 */
void
main(void)
{
	char *device;			/* name of device to dump on */
	unsigned int offset;		/* offset in bytes from start of dev */
	int overwrite = 0;		/* nonzero => dump even if one there */
	int fd;

	printf("\nMemory dumper\n");

	/*
	 * the firmware clears memory if RB_DUMP isn't set.
	 */
	if ((CD_LOC->c_boot_flag & RB_DUMP) == 0)
		printf("Warning:  memory was probably zeroed!\n");

	/*
	 * look at the boot string to find out where to dump
	 */
	while (find_where(&device, &offset, &overwrite) == TRUE) {

		/*
		 * Already a dump there?
		 */
		++chunk;
		if ( already_a_dump(device, offset) == TRUE )
			if ( !overwrite )
				abort("Dump already in dump area");
			else
				printf("Overwriting previous dump.\n");

		/*
		 * Open dump device for writing
		 */
		fd = open(device, 2);
		if (fd < 0)
			abort("Can't open dump device for writing");

		/*
		 * Dump this chunk to the device
		 */
		set_dump_size(fd, offset);
		if (di.dump_size) {
			printf("\nDumping %d.%d MB to %s offset %d",
				di.dump_size / M1,
				(di.dump_size % M1) * 10 / M1,
				device, offset);
			/*
			 * Write magic number and start dumping memory
			 */
			if ( chunk == 1 )
				*(struct dump_info *)0 = di;
			if (dumpmem(fd, offset) == FALSE) {
				_stop("\nDump failed.");
			}
		}
		close(fd);
		di.dump_size = -1;
	}
	if (mem_left > 0)
		printf("\nWarning...%d bytes of memory not dumped!\n",
			mem_left);
	else
		printf("\nDump succeeded.\n");
	_stop(0);
}

/*
 * static int
 * find_where(char **, uint *, int *)
 * 	parse the boot-name to find the dump device and offset. 
 *
 * Calling/Exit State:
 * 	"name_p" is a string pointer to be set by this function
 *	to the name of the device upon which to dump.
 *
 *	"offset_p" is a pointer to an offset to be set by this 
 *	function to the starting block offset on the device for
 *	the dump.
 *
 *	"overwrite_p" is a pointer to a flag to be set by this function
 *	indicating if an existing dump should be overwritten if encountered.
 *
 *	The firmware configuration table contains the boot-flags
 *	and boot-string with which the dumper was invoked and
 *	which specifies the options to parse.
 *
 * Remarks:
 * 	Aborts if it can't parse the string.
 *
 *	If a dumplist is specified, then fflag is set to true
 *	upon exit, which tells to subsequent re-entries here to
 *	continue parsing the dumplist file instead of the boot-string
 *	to locate the next dump device. 
 */
static int
find_where(char **name_p, uint *offset_p, int *overwrite_p)
{
	char *cp, *cpend;
	static int fflag = FALSE;
	static char buf[8192];
	static int size;
	static char *s_end;
	char *arg0, *arg1, *arg2, *arg3, *arg4, *arg5;
	int fd, i;

	if ( fflag == TRUE ) {
		cp = s_end + 1;
		if ((cp - buf) > size)
			return (FALSE);
		for (cpend = cp; cpend < &buf[size]; cpend++)
			if (*cpend == '\n')
				break;
		if (cpend >= &buf[size])
			return (FALSE);
		s_end = cpend;
		*cpend = '\0';
		arg1 = cp;
		arg2 = next_word( arg1, cpend );
		arg3 = next_word( arg2, cpend );
		arg4 = next_word( arg3, cpend );
		arg5 = NULL;
	} else {
		if ( chunk != 0 )
			return (FALSE);
		/*
		 * find the boot name in config table
		 */
		cp = CD_LOC->c_boot_name;
		i = CD_LOC->c_bname_size;
		cpend = &(CD_LOC->c_boot_name[i]);

		/*
		 * Crack line into arguments (up to 6 args).
		 */
		arg0 = cp;
		arg1 = next_word( arg0, cpend );
		arg2 = next_word( arg1, cpend );
		arg3 = next_word( arg2, cpend );
		arg4 = next_word( arg3, cpend );
		arg5 = next_word( arg4, cpend );

		/*
		 * get name of device to dump on into name_p.
		 */
		if (arg1 == NULL)
			abort("missing second argument");
		if (strcmp(arg1, "-f") == 0) {
			fflag = TRUE;
			if (arg2 == NULL)
				abort("No dump device file specified");
			fd = open(arg2, 0);
			if (fd < 0)
				abort("Can't open dump device file");
			size = read(fd, buf, sizeof(buf));
			close(fd);
			if (size < 0)
				abort("Error reading dump device file");
			for (i=0; i < size; i++) {
				if (buf[i] == ' ' || buf[i] == '\t')
					buf[i] = '\0';
			}
			if (arg4 != NULL && strcmp(arg4, "-o") == 0)
				*overwrite_p = TRUE;
			cp = buf;
			for (cpend = buf; cpend < &buf[size]; cpend++)
				if (*cpend == '\n')
					break;
			if (cpend >= &buf[size])
				abort("Format error in dump device file");
			s_end = cpend;
			*cpend = '\0';
			arg1 = cp;
			arg2 = next_word( arg1, cpend );
			arg3 = next_word( arg2, cpend );
			arg4 = next_word( arg3, cpend );
			arg5 = NULL;
		}
	}

	*name_p = arg1;
	if ((fd = open(arg1, 0)) < 0)
		abort("Can't open dump device");	
	if (!IS_TAPE(fd)) {
		close(fd);
		psize = vtoc_getpsize(arg1) * DEV_BSIZE;
		if (psize < 0) {
			printf("Bad partition or non-existent VTOC on device\n");
			printf("Add a VTOC to the disk using a formatter or use\n");
			printf("a legal partition number.\n\n");
			printf("Bad device %s", arg1);
			abort(0);
		}
	} else
		close(fd);
	if (arg2 == NULL)
		abort("Missing offset");
	*offset_p = atoi(arg2) * DEV_BSIZE;
	if (arg3 == NULL)
		abort("Missing unix device name");
	if (arg4 != NULL)
		if ( IS_DIGIT(*arg4) )
			di.dump_size = atoi(arg4) * DEV_BSIZE;
		else if ( strcmp(arg4, "-o") == 0)
			*overwrite_p = TRUE;
	if (arg5 != NULL)
		if ( strcmp(arg5, "-o") == 0)
			*overwrite_p = TRUE;
	return (TRUE);
}

/*
 * static char *
 * next_word(char *, char *)
 *	Locate the beginning of the next word withing the specified string.
 *
 * Calling/Exit State:
 *	"p" addresses the string to parse, presumably at the 
 *	start of a word to skip over.
 *
 *	"e" addresses the end of the string addressed by "p".
 *	
 *	Returns NULL if the string is exhausted before the
 *	next word is located; otherwise returns its start address.
 */
static char *
next_word(char *p, char *e)
{

	if ( p == NULL || p >= e )
		return( NULL );
	while ( *p != '\0' ) {		/* skip letters */
		if ( ++p >= e )
			return( NULL );
	}
	while ( *p == '\0' ) {		/* skip nulls */
		if ( ++p >= e )
			return( NULL );
	}
	return( p );
}

/*
 * static int
 * already_a_dump(char *, uint)
 * 	Check dump location to see if there's a dump there already.
 *
 * Calling/Exit State:
 *	"device" and "offset" specify the standalone device name
 *	and the block number upon it, at which to check for an
 *	existing dump image.
 *
 *	The global variable "chunk" indicates which section of 
 *	dump is now being performed, for dumps using a dumplist.
 *
 *	Returns FALSE if this is not the first chunk of the
 *	dump, the specified dump device is a tape device, or 
 *	if a dump is not already there; FALSE otherwise.
 *
 * Description:
 *	Only test the first device being dumped to for an existing
 *	dump.  Otherwise, open the specified device, verify that it
 *	is not a tape device, seek to the specified block offset, then
 *	read the dump header.  Check the dump header to determine
 *	if it contains a dump-magic number in the expected location.
 *	If so, return TRUE, otherwise return FALSE. 
 */
static int
already_a_dump(char *device, uint offset)
{
	int fd;

	/*
	 * Only check the first chunk for an existing dump
	 */
	if ( chunk != 1 )
		return ( FALSE );

	fd = open(device, 0);
	if (fd < 0)
		abort("Can't open dump device for reading");

	/*
	 * if it is tape, don't check for magic number
	 */
	if ( IS_TAPE(fd) ) {
		close(fd);
		return ( FALSE );
	}
	if (magic_buf == (caddr_t)NULL) {
		callocrnd(RAWALIGN);
		magic_buf = calloc(DEV_BSIZE);
	}
	(void)lseek(fd, offset, 0);
	if (read(fd, (caddr_t)magic_buf, DEV_BSIZE) != DEV_BSIZE)
		abort("Can't read magic number");
	close(fd);
	magic = (int *)((void *)magic_buf);
	if ( *magic == DUMP_MAGIC )
		return ( TRUE );
	else
		return ( FALSE );
}

/*
 * static void
 * set_dump_size(int, uint)
 *	Determine the amount of core memory to dump.
 *
 * Calling/Exit State:
 * 	Sets the global field di.dump_size to the number of
 *	bytes that should be dumpted.  Upon entry, it may
 *	already be set to a maximum which was specified on
 *	the command line when the dumper was booted.
 *
 *	"mem_pointer" is a global variable set to the
 *	address at which the dump is to be started.
 *
 *	No return value.
 *
 * Description:
 *	Calculate the amount of memory which is available to
 *	be dumped, then scale it down if it exceeds a maximum
 *	which may have been specified in the boot-string.  Then
 *	compensate for the physical limitations of the dump
 *	partition and round it up to a 32K boundary.
 *
 * Remarks:
 *	The utilities that will eventually read this dump
 *	expect the filesize to be a multiple of 32k.  Apparently
 *	this is for historical reasons due to the block sizes
 *	that were being used on half-inch tape.
 *	
 */
static void
set_dump_size(int fd, uint offset)
{
	int size;

	mem_left = CD_LOC->c_maxmem - (int)mem_pointer;
	if (di.dump_size >= 0)
		size = MIN(mem_left, di.dump_size);
	else
		size = mem_left;

	if (!IS_TAPE(fd))
		size = MIN(size, psize - (int)offset);	
	if (size < 0)
		size = 0;
	di.dump_size = size;

	di.dump_size = roundup(di.dump_size, K32);
}

/*
 * static int
 * dumpmem(int, uint)
 *	Dump a segment of the core memory to the output device.
 *
 * Calling/Exit State:
 * 	The global field di.dumpsize specifies the number of
 *	bytes that to be dumpted.  
 *
 *	The arguments to this function specify the device and
 *	block location on it at which to dump that data.
 *
 *	The global variable "mem_pointer" contains the core
 *	address at which to start copying di_dumpsize bytes
 *	to the dump device.
 *
 *	Returns TRUE if the copy succeeded; FALSE otherwise.
 *
 * Description:
 * 	Loop through memory (as indicated in the memory map 
 *	in bitmap), writing what's there and skipping the holes.
 * 	Do writes in 32K byte chunks for historical reasons.
 */
static int
dumpmem(int fd, uint offset)
{
	int count, m, b;

	/*
	 * If it's not a tape, seek to offset
	 */
	if ( !IS_TAPE(fd) ) 
		(void)lseek(fd, offset, 0);

	count = di.dump_size;
	m = (int)mem_pointer / K32;

	while (count > 0) {
		/*
		 * Set b equal to an index into the memory bitmap.
		 * Each bit in the memory bitmap represents MC_CLICK
		 * (512K) bytes.  Since we are writting out 32K bytes
		 * each time through this loop, the index, b, will
		 * change every 16th time through the loop.
		 */
		b = m / K32_PER_CLICK;
		if (MC_MMAP(b, CD_LOC)) {
			if (write(fd, mem_pointer, K32) != K32) {
				printf("write error at 0x%x\n", mem_pointer);
				return (FALSE);
			}
			mem_pointer += K32;
			count -= K32;
			m++;
			/*
			 * Print out a "." for every MC_CLICK bytes written.
			 */
			if (m % K32_PER_CLICK == 0)
				printf(".");
		} else {
			mem_pointer += K32;
			m++;
			/*
			 * Print out a "x" for every MC_CLICK bytes skipped
			 * due to a hole.
			 */
			if (m % K32_PER_CLICK == 0)
				printf("X");
		}
		/*
		 * track how close we are to having looped through c_maxmem
		 * bytes.  This gets decremented regardless of holes, since
		 * c_maxmem is real mem + holes.
		 */
		mem_left -= K32;
		if (mem_left == 0)
			break;
	}
	return (TRUE);
}

/*
 * static void
 * abort(char *)
 *	Display and error message on the console, then abort the dump.
 *
 * Calling/Exit State:
 *	The argument addresses a string to be appended to a common
 *	message indicating tha the dump failed, which will all be
 *	displayed to the system console.
 *	
 *	This function is not expected to return from its call to
 *	stop(), which halts the system (aborting this utility).
 */
static void
abort(char *abort_message)
{
	printf("%s.  No Dump.\n", abort_message);
	_stop(0);
}
