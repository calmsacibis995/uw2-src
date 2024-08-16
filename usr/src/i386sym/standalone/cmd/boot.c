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

#ident	"@(#)stand:i386sym/standalone/cmd/boot.c	1.2"

/*
 * Boot
 *	Standalone utility to load and execute other standalone programs.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/boot.h>
#include <sys/cfg.h>
#include <sys/elftypes.h>
#include <sys/elf.h>
#include <sys/saio.h>

/* Definitions used for internally mapping ELF boot information  */
enum lfsect { TLOAD, DLOAD, NOLOAD };
typedef enum lfsect lfsect_t;

struct bootsect {
	lfsect_t type;			/* type of section */
	unsigned long addr;		/* virtual address to load section */
	unsigned long fsize;		/* file size of section */
	unsigned long msize;		/* memory size of section */
	unsigned long offset;		/* offset in file */
};
#define NBSECT	13	/* number of sec/segments */

static struct bootsect bsect_array[NBSECT];
static Elf32_Ehdr elfhdr;
static int sec_cnt;
static void copyunix(int, int);
static void getsect(int);
static int bt_read(int, char *, int, off_t);

extern caddr_t calloc(int);
extern void callocrnd(int);
extern void bcopy(void *, void *, size_t);
extern void gsp(caddr_t, caddr_t, long, caddr_t);
extern void _stop(char *);
extern char *prompt(char *);

/*
 * void
 * main(void)
 *	locate, load, and execute another standalone program.
 *
 * Calling/Exit State:
 *	The system configuration table written by the firmware
 *	contains the boot-string and boot-flags used to determine
 *	specific actions of this loader.  
 *
 *	Starts execution of the specified program, after which
 *	boot never returns.
 *
 * Description:
 * 	The boot-flags indicate if the program to load is specified
 *	in the boot-string from the configuration table or if it
 *	should be solicited from the console operator.   Once obtained,
 *	attempt to open the file, retrying using solicitation upon 
 *	failure.  
 *
 * 	If the bootfile name is solicited interactively, then the 
 *	boot-string in the firmware table is updated to reflect the 
 *	interactive input so the booted program may access it.  While
 *	updating the boot-string, replace whitespace with NULs to 
 *	separate the boot arguments into NUL-terminated strings, as
 *	the firmware would have had it been provided by it.
 *
 *	Once opened, invoke copyunix to finish loading the program
 *	and start its execution; it never returns.
 */
void
main(void)
{
	struct config_desc *cd = CD_LOC;
	int i;
	char *r, *s;
	int io;
	static	int retry;

	printf("Boot\n");
	for (;;) {
		if (cd->c_boot_flag & RB_ASKNAME) {
			s = prompt(": ");
			r = cd->c_boot_name;
			/* skip leading white space */
			while (*s == ' ' || *s == '\t')
				s++;
			/* copy new line into config structure */
			for (i = cd->c_bname_size; i-- > 0; ) {
				switch (*s) {
				case '\0':	/* end of line */
					*r++ = 0;
					break;
				case '\t':	/* tab */
				case ' ':	/* space */
					*r++ = 0; s++;
					break;
				default:	/* character */
					*r++ = *s++;
					break;
				}
			}
		} else
			printf(": %s\n", cd->c_boot_name);
		io = open(cd->c_boot_name, 0);
		if (io >= 0)
			copyunix(cd->c_boot_flag, io);
		else
			printf("Can't open %s\n", cd->c_boot_name);
		if (++retry > 2)
			cd->c_boot_flag = RB_ASKNAME;
	}
}

/*
 * static void
 * copyunix(int, int)
 *	Attempt to load the executable into memory and run it.
 *
 * Calling/Exit State:
 *	"io" is the opened file descriptor to read the program from.
 *
 *	"howto" currently is ignored.
 *
 *	Never returns; either the program is successfully loaded
 *	and its execution started, or this program is aborted.
 *
 * Description:
 *	Attempt to read the object header and use its magic number to
 *	validate that it is an ELF binary.  Then invoke getsect() to
 *	read the binary's segment headers into the global "bsect_array" 
 *	and set "sec_cnt" to the number of segments in it.  Then cycle
 *	through those headers reading all loadable DATA and TEXT segments
 *	into sequential/unused core memory, and reserving space for the 
 *	non-loadable data segments (bss).  Lastly, extract the entry
 *	execution address from the ELF-header and start its execution
 *	there by invoking gsp() to copy the loaded object to "low" memory,
 *	actually overwriting this program, and start its execution.
 *
 * Remarks:
 * 	Since the tape driver can only read in DEV_BSIZE blocks, all 
 * 	data is read with DEV_BSIZE or larger granularity into
 * 	a buffer cache, then bcopy()'ed into the desired location.
 * 	Also, the file must be read in ascending order to work for tape.
 */
/*ARGSUSED*/
static void
copyunix(int howto, int io)
{
	int i, j, ok, length;
	unsigned long lastaddr;
	long offset, entry, text1_size, data_size, bss_size;

	bss_size = 0L;
	data_size = 0L;
	text1_size = 0L;

	/*
	 * Start reading file at 1K boundary past 
	 * the end of allocated memory.
	 */
	callocrnd(1024);
	offset = (long)calloc(0);

	/* Read the object header and validate its magic number */
	if (bt_read(io, (char *)&elfhdr, sizeof(elfhdr), 0) != sizeof(elfhdr))
		goto sread;

        if (elfhdr.e_ident[EI_MAG0] != ELFMAG0
        ||  elfhdr.e_ident[EI_MAG1] != ELFMAG1
        ||  elfhdr.e_ident[EI_MAG2] != ELFMAG2
        ||  elfhdr.e_ident[EI_MAG3] != ELFMAG3)
		_stop("Bad a.out magic number");

	printf ("loading an ELF binary\n");

	getsect(io);

	length = 0;
	lastaddr = 0;
	for(i = 0; i < sec_cnt; i++) {
		switch (bsect_array[i].type) {
		case TLOAD:
		case DLOAD:
			/* 
			 * Check to see if this section 
			 * overlaps another 
			 */
			for (ok = 0, j = 0; j < i; j++) {
				if (bsect_array[i].addr + bsect_array[i].msize 
				    <= bsect_array[j].addr) { 
					ok = 1;
				} else if (bsect_array[i].addr 
					   >= bsect_array[j].addr + 
					   bsect_array[j].msize) { 
					ok = 1;
				}
				if (ok != 1) {
					printf("WARNING: section %d ", i);
					printf("overlaps section %d\n", j);
				}
			}
			if(bt_read(io, (char *)offset + bsect_array[i].addr, 
				bsect_array[i].fsize,  bsect_array[i].offset)
			   != bsect_array[i].fsize)
				goto sread;

			if (bsect_array[i].type == TLOAD) {
				text1_size += bsect_array[i].fsize;
			} else {
				data_size += bsect_array[i].fsize;
				bss_size += bsect_array[i].msize - 
						bsect_array[i].fsize;
			}

			if (bsect_array[i].addr > lastaddr) {
				lastaddr = bsect_array[i].addr;
				length = bsect_array[i].addr 
					 + bsect_array[i].msize;
			}
			break;
		default:
			break;
		}
	}
	entry = elfhdr.e_entry;
	printf("%d+%d+%d start 0x%x\n", text1_size, data_size, bss_size, entry);

	/*
	 * Close the binary object file, copy the
	 * configuration table, then exec the new
	 * program over ourselves.
	 */
	close(io);
	bcopy(CD_LOC, (void *)(offset + (int)CD_LOC), 
		CD_STAND_ADDR - (int)CD_LOC);
	gsp((caddr_t)offset, 0, length, (caddr_t)entry);
	/*NOTREACHED*/
sread:
	_stop("short read");
}

/*
 * static void
 * getsect(int)
 *	Read segment headers from the executable into the bsect_array.
 *
 * Calling/Exit State:
 *	"io" is the opened file descriptor to read the program from.
 *
 *	the global variables "bsect_array" and "sec_cnt" are loaded with
 *	applicable segment header information from the binary and the 
 *	number of records that are valid.
 *
 *	No return value.
 *
 * Remarks:
 * 	Interprets each header from ELF format into a generalized
 *	form that copyunix() understands.  Historically, this is to 
 *	support multiple format types.
 */
static void
getsect(int io)
{
	Elf32_Phdr elfphdr;
	int i, nbytes;

	/* 
	 * In order for sequential read of the segments to work, 
	 * we store them in bsect_array[].  Global var sec_cnt
	 * indicate the number of segments in the file.
	 */

	sec_cnt=0;
	for (i=0; i < (int)elfhdr.e_phnum; i++) {
		nbytes = bt_read(io, (char *)&elfphdr, elfhdr.e_phentsize,
				elfhdr.e_phoff + i * elfhdr.e_phentsize);
		if (nbytes != elfhdr.e_phentsize) {
			_stop("short section hdr read");
		}

		switch (elfphdr.p_type) {
		case PT_LOAD:
			if(elfphdr.p_filesz == 0)
				continue;
			switch (elfphdr.p_flags & (PF_R | PF_W | PF_X)) {
			case (PF_R | PF_W | PF_X):
				bsect_array[i].type = DLOAD;
				break;
			case (PF_R | PF_X):
				bsect_array[i].type = TLOAD;
				break;
			default:
				bsect_array[i].type = NOLOAD;
				break;
			}	
			break;
		default:
			bsect_array[i].type = NOLOAD;
			break;
		}
		bsect_array[i].addr = elfphdr.p_paddr;
		bsect_array[i].fsize = elfphdr.p_filesz;
		bsect_array[i].msize = elfphdr.p_memsz;
		bsect_array[i].offset = elfphdr.p_offset;
		sec_cnt++;
	}
}

/* 
 * static int
 * bt_read(int, char *, int, off_t)
 * 	Perform a "cooked" read into the specified caller' buffer.  
 *
 * Calling/Exit State:
 *	"io" is the opened file descriptor to read from.
 *
 *	"offset" is the location with that file at which to
 *	read the specified amount of data.
 *
 *	Upon success, returns the number of bytes successfully
 *	read into the caller's buffer, up the size specified.
 *	Returns -1 upon failure.
 *
 * Remarks:
 *	This function is essential for tape support, to
 *	cache data so that I/O works on strict sequential
 *	devices and non-aligned request boundaries. 
 *
 *	This is done by reading all data blocks between
 *	the current location and the end of the request
 * 	into a cache while moving requested bytes into 
 * 	the caller's buffer.
 */
static int
bt_read(int io, char *buf, int nbytes, off_t offset)
{
#define CACHE_SZ	(16 * DEV_BSIZE)	/* Size of input buf cache */
	static char cache[CACHE_SZ];
	static int curblock = -1;
	static int bytes_in = CACHE_SZ;
	int icount = 0;
	int eblock, count;
	off_t pos;
	
	eblock = (offset + nbytes - 1) / CACHE_SZ;
	if (eblock < curblock) return (-1);	/* Non-sequential read */

	for ( ; curblock != eblock && bytes_in == CACHE_SZ; curblock++) {
		if (offset / CACHE_SZ == curblock) {
			pos = offset % CACHE_SZ;
			if ((count = bytes_in - pos) > 0) {
				bcopy(&cache[pos], buf, count);
				buf += count;
				offset += count;
				icount += count;
				nbytes -= count;
			}
		}

		if ((bytes_in = read(io, &cache[0], CACHE_SZ)) < 0)
			return (-1);		/* Read error */
	}

	/* 
	 * Either end-of-data has been reached or the
	 * last input block is in the cache.  Return
	 * the expected portion of that block.
	 */
	if (offset / CACHE_SZ == curblock) {
		pos = offset % CACHE_SZ;
		count = bytes_in - pos;
		if (count > nbytes)
			count = nbytes;		/* Reduce xfer size */

		bcopy(&cache[pos], buf, count);
		icount += count;		
	}
	return (icount);			/* Return #bytes put in buf */
}
