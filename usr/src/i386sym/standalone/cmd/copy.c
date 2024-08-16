/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* 
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ident	"@(#)stand:i386sym/standalone/cmd/copy.c	1.1"

/*
 * copy
 *	Standalone program to copy all or part of a file from one 
 *	place to another - kind of like "dd" to some extent.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/sysmacros.h>
#include <sys/saio.h>

#define MAX_MEM		(4*1024*1024) /* limit ourselves to 4M since some 
				       * drivers may have problems otherwise */

static int _buf_size = 0;
static char *bufptr;
static uint checksum = 0;
static uint gchecksum = 0;

extern char *calloc(int);	
extern void exit(int);
extern void bzero(void *, size_t);
extern void callocrnd(int);
extern int atoi(char *);

static void set_buf_size(char *);
static void sum(short *, int n);

/*
 * void
 * main(void)
 *	Copy the specified file from one place to another specified location.
 *
 * Calling/Exit State:
 *	No return value;
 *
 * Description:
 *	This program will interact with the console operator to 
 * 	solicit the parameters of the this copy, then copy the
 *	file from one location to the other.  It also checksums
 *	the file as it copies it and can optionally be requested
 *	to read the new clone of the file back to checksum it and
 *	report mismatches.  The following is a description of the
 *	input which will be solicited:
 *
 *	"Input file?"	
 *		standalone device/pathname of the source file.
 *	"Output file?"
 *		standalone device/pathname of the destination file.
 *	"Verify?"
 *		enter <cr> or "y" to have it checksum both the source
 *		and resulting destination files for comparison.  Enter
 *		"n" to bypass it for performance.
 *		
 *	"Seek to Output block?"
 *		enter <cr> to start writing at block zero of an existing
 *		destination file or device, or a block number <n> if
 *		the first <n> should be skipped prior to writting.
 *		
 *	"Seek to Input block?"
 *		enter <cr> to start read at block zero of the source
 *		file or device, or a block number <n> if the first <n> 
 *		should be skipped prior to reading..
 *		
 *	"Limit total transfer count?"
 *		enter <cr> to copy the entire file or enter <n> to
 *		copy no more than <n> blocks of it.
 *		
 *	"Buffer by?"
 *		this allows the blocks size of the buffer used for
 *		reading/writting to be overridden from the displayed
 *		default.  Enter <cr> to use the default <n> to make
 *		it smaller than the default.
 *		
 *	"Count?"
 *		enter <cr> to copy the entire file or enter <n> to
 *		read and write no more than <n> buffers of it, the
 *		size of which was determined by the response to the
 *		"Buffer by?" prompt.
 */
void
main(void)
{
	int pos, ipos, opos, i, n, o, bs, count;
	int limit, left, verifypass;
	int gcount, gbs, gleft;
	int bytecount, vbytecount;
	char buf[128], input[128], output[128];

	vbytecount = bytecount = verifypass = pos = 0;
	printf("Copy program\n");

	do {
		printf("Input file?");
		(void)gets(input);
		i = open(input, O_RDONLY);
	} while (i <= 0);

	do {
		printf("Output file?");
		(void)gets(output);
		o = open(output, O_RDWR|O_CREAT, 0666);
	} while (o <= 0);

	printf("Verify?"); 
	(void)gets(buf);
	if (buf[0] == 'y' || buf[0] == 'Y')
		verifypass = 1;

	printf("Seek to Output block?"); 
	(void)gets(buf); 
	opos = atoi(buf) * 512;

	printf("Seek to Input block?"); 
	(void)gets(buf); 
	ipos = atoi(buf) * 512;

	printf("Limit total transfer count?"); 
	(void)gets(buf); 
	limit = atoi(buf);

	if (limit < 0)
		limit = 0;
	gleft = left = limit;

	set_buf_size(buf);
	printf("%d blocks available (%d.%d Meg)\n", 
		_buf_size/512, _buf_size >> 20, 
		(_buf_size % (1024*1024)) / ((1024*1024)/10));
	printf("Buffer by?", _buf_size); 
	(void)gets(buf); 
	bs = atoi(buf);
	if (bs <= 0) {
		bs = _buf_size/512;
		printf(">>> %d used\n", bs);
	}
	gbs = bs;

	printf("Count?"); 
	(void)gets(buf); 
	count = atoi(buf);
	if (count == 0) {
		count = -1;
		printf(">>> infinite count used\n");
	}
	gcount = count;

	printf("Last chance....continue? (default 'y') "); 
	(void)gets(buf);
	if (buf[0] == 'n' || buf[0] == 'N') {
		printf("    ...exiting without doing copy\n");
		exit(0);
	}


verifyloop:
	bs *= 512;
	left *= 512;
	while(count--) {
		if (count == -2)
			count = -1;
		if (limit) {
			if (left <= 0) {
				printf(">>> hit limit of %d blocks\n", limit);
				goto done;
			}
			if (left < bs)
				bs = left;
		}
		(void)lseek(i, pos+ipos, 0);
		if (vbytecount && (bytecount + bs) > vbytecount)
			bs =  vbytecount - bytecount;
		if ((n=read(i, bufptr, bs)) < 0) {
			printf("read err blk %d %d\n", (ipos+pos)/512, n);
			exit(errno);
		}
		if (n == 0) {			/* EOF */
			printf(">>> EOF\n");
			goto done;
		}
		/* 
		 * Handle short reads by zeroing rest of buffer and 
		 * rounding up read size by 512 bytes.
		 */
		if (n < bs) {
			bzero(bufptr+n, bs-n);
			bs = n = roundup(n, 512);
		}
		if (verifypass) { 
			sum((short *)((void *)bufptr), n);
		}
		if (verifypass != 2) {
			(void)lseek(o, pos+opos, 0);
			if ((n=write(o, bufptr, bs)) != bs) {
				printf("write err blk %d\n", (pos+opos)/512, n);
				exit(errno);
			}
		}
		pos += bs;
		bytecount += bs;
		if (limit)
			left -= bs;
		if (vbytecount && bytecount >= vbytecount)
			goto done;
	}
done:
	close(i);
	if (verifypass == 1) {
		do {
			checksum = (checksum & 0xffff) | (checksum >> 16);
		} while (checksum & ~0xffff);
		if (bytecount % 512) {
			printf("internal error: non 512 byte multiple ");
			printf("byte count (%d)\n", bytecount);
		}

		printf("Pass 1, count \"%d\", checksum \"%x\", doing verify\n",
			bytecount/512, checksum);
		close(o);
		i = open(output, 0);
		if (i < 0) {
			printf("open error on %s\n", output);
			exit(errno);
		}
		ipos = opos;
		gchecksum = checksum;
		pos = checksum = 0;
		verifypass = 2;
		count = gcount;
		left = gleft;
		bs = gbs;
		vbytecount = bytecount;
		bytecount = 0;
		goto verifyloop;
	}
	if (verifypass == 2) {
		do {
			checksum = (checksum & 0xffff) | (checksum >> 16);
		} while (checksum & ~0xffff);
		if (bytecount % 512) {
			printf("Non 512 byte multiple byte count (%d)\n", 
				bytecount);
		}

		printf("Pass 2, count \"%d\", checksum \"%x\"\n", 
			bytecount/512, checksum);
		if (bytecount != vbytecount)
			printf("*** ERROR *** bytecount mismatch\n");
		if (checksum != gchecksum)
			printf("*** ERROR *** checksum mismatch\n");
	} else {
		close(o);
	}
	printf("Done\n");
	exit(0);
}

/* 
 * static void
 * set_buf_size(char *)
 *	Allocate remaining usable memory for the copy operation.
 *
 * Calling/Exit State:
 *	The global variable "bufptr" is set to the start address
 *	of the allocated buffer and "_buf_size" set to the size
 *	of the usable buffer space.
 *
 *	The argument "x" is a rough estimate of the current stack
 * 	pointer, used in computing room for future stack growth.
 *
 *	No return value;
 *
 * Remarks:
 *	The buffer start addr is the next 1k boundary that calloc()
 *	would have allocated, if we knew the size to request of it.
 *
 *	It assumes that rest of memory, less the is available for 
 *	the buffer. That is, no driver (execpt when opened early 
 *	in main()) will attempt to allocate more memory.
 *
 *	It also allows up to 50K for possible stack growth and then
 *	rounds the upper bound to under 4Meg due to some special
 *	controller addressing limitations.
 */
static void
set_buf_size(char *x)
{
	char *p;

	callocrnd(1024);
	bufptr = calloc(0);			/* The start address */

	/* allow 50k for stack growth */
	p = (char *) ((int)&x - (50 * 1024));

	/* round below 4M so SCSI devices happy */
	p = (char *) MIN((int)p, MAX_MEM);
	p = (char *) roundup((int)p, 1024);
	_buf_size = ((int)p - (int)bufptr);	/* The buffer size */
}

/* 
 * static void
 * sum(short *, int)
 *	Checksum the specified buffer onto the current global checksum.
 *
 * Calling/Exit State:
 *	"p" addresses a series of "n" words to checksum, where 
 *	"n" must be an even count.
 *
 *	The global variable "checksum" contains the running checksum
 * 	for the caller and is augmented with checksums for the new
 *	list of words passed to this function, using a simple shift
 *	and add algorithm.
 *
 *	No return value.
 */
static void
sum(short *p, int n)
{
	unsigned int sum = checksum;

	if (n & 01)
		printf("copy: internal error: summing an odd count\n");
	n /= sizeof(short);
	while (n-- > 0) {
		if (sum & 01)
			sum = (sum>>1) | 0x80000000;
		else
			sum >>= 1;
		sum += (*p++ & 0xffff);
	}
	checksum = sum;
}
