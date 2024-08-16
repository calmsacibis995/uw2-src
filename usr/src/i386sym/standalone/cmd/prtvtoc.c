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

#ident	"@(#)stand:i386sym/standalone/cmd/prtvtoc.c	1.1"

/*
 * prtvtoc
 * 	Standalone program to read and display a disk partition map ("VTOC").
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/vtoc.h>
#include <sys/saio.h>

/*
 * Definitions.
 */
#define	reg		register /* Convenience */
#define PARTBUFSIZE	20	 /* must be big enough for "xx(yy,255)" */


/*
 * Disk freespace structure.
 */
typedef struct {
	ulong	fr_start;		/* Start of free space */
	ulong	fr_size;		/* Length of free space */
} Freemap;

/*
 * External functions.
 */
extern void bcopy(void *, void *, size_t);
extern void bzero(void *, size_t);
extern void exit(int);
extern char *index(char *, char);
extern void qsort(char *, int, int, int (*)());
extern void strcpy(char *, char *);
extern int strlen(char *);
extern int strcmp(char *, char *);
extern int vtoc_pread(int, struct vtoc **);

/*
 * Internal functions.
 */
static int	partcmp(struct partition **, struct partition **);
static int	prtvtoc(int);
static void	puttable(struct vtoc *, Freemap *, char *);

/*
 * Static variables
 */
static char line[100];
static char *name = line;

/*
 * void
 * main(void)
 *      Read and display the disk partition map ("VTOC") for specified devices.
 *
 * Calling/Exit State:
 *	Displays VTOC data on the system console.
 *
 *      No return value.
 *
 * Description:
 *      This program interacts with the console operator to
 *	solicit standalone device names for which the operator
 *	would like to have their VTOC read and displayed in a
 *	human readable form.  It prompts the operator for devices 
 *	to display this data from, then invokes prtvtoc() to 
 *	actually perform the task once the device has been 
 *	successfully opened.  It performs this task over and
 *	over until the operator enters "exit" instead of a
 *	device's standalone name.
 */
void
main(void)
{
	reg int	idx = 0;
	reg int	i;
	char	*dp;
	char	*pp;
	char	partbuf[PARTBUFSIZE];
	int	n = V_NUMPAR;

	/*
	 * Load up partbuf with the character translation of V_NUMPAR.
	 */
	bzero(partbuf, PARTBUFSIZE);
	pp = partbuf + sizeof(partbuf) - 1;
	*pp-- = '\0';
	do {
		*pp-- = "0123456789"[n%10];
		n /= 10;
	} while (n);
	pp++;

	/*
	 * Now open the device
	 */
	printf("prtvtoc\n\n");
	printf("Type a device name which contains a VTOC at the prompt\n");
	printf("Type \"exit\" to exit\n");
	for (;;) {
		do  {
			printf(": ");
			(void)gets(line);
			if (strcmp("exit", line) == 0)
				exit(idx);
			/*
			 * Replace the partition number with V_NUMPAR
			 */
			dp = index(line, ',');
			if (dp == 0) {
				printf("Must specify a device name of the form:");
				printf(" XX(y,z)\n");
				i = -1;
				continue;
			}
			dp++;
			strcpy(dp, pp);
			dp = line + strlen(line);
			strcpy(dp, ")");

			i = open(line, 0);
		} while (i < 0);

		idx |= prtvtoc(i);
		close(i);
	}
	/*NOTREACHED*/
}

/*
 * static Freemap *
 * findfree(vtoc)
 * 	Find space on a disk, not used by the specified VTOC partioning.  
 *
 * Calling/Exit State:
 *	"vtoc" must contain a valid VTOC.
 *
 *	Returns a list of unused areas on the device, based on the VTOC.
 *	The size of the list is delimeted by an entry with zero for
 *	both the start block number and size of its free area.
 *	
 * Description:
 *	Transform the VTOC partition specification into a list 
 *	of partition start addresses and sizes, sorted by start
 *	addresses.  Then cycle sequentially through that list
 *	noting in the freemap the start address and size of gaps 
 *	between physically sequential VTOC partitions.
 */
static Freemap *
findfree(vtoc)
	reg struct vtoc		*vtoc;
{
	reg struct partition	*part;
	reg struct partition	**list;
	reg Freemap		*freeidx;
	ulong			fullsize;
	struct partition	*sorted[V_NUMPAR + 1];
	static Freemap		freemap[V_NUMPAR + 1];

	fullsize = vtoc->v_capacity;
	if (vtoc->v_nparts > V_NUMPAR) {
		printf("prtvtoc: findfree() - Too many partitions on disk!\n");
		exit(1);
	}
	list = sorted;
	for (part = vtoc->v_part; part < vtoc->v_part + vtoc->v_nparts; ++part)
		*list++ = part;
	*list = 0;
	qsort((char *) sorted, (uint) (list - sorted), sizeof(*sorted), partcmp);
	freeidx = freemap;
	freeidx->fr_start = 0;
	for (list = sorted; (part = *list) != NULL; ++list) {
		if (part->p_type == V_NOPART)
			continue;
		if (part->p_start == freeidx->fr_start) {
			freeidx->fr_start += part->p_size;
		} else {
			freeidx->fr_size = part->p_start - freeidx->fr_start;
			(++freeidx)->fr_start = part->p_start + part->p_size;
		}
	}
	if (freeidx->fr_start < fullsize) {
		freeidx->fr_size = fullsize - freeidx->fr_start;
		++freeidx;
	}
	freeidx->fr_start = freeidx->fr_size = 0;
	return (freemap);
}


/*
 * static int
 * partcmp(struct partition **, struct partition **)
 * 	Qsort() key comparison of partitions by starting sector numbers.
 *
 * Calling/Exit State:
 *	Simply subtacts the start sector locations of the two
 *	specified partion definitions, returning the difference
 *	for qsort() to use.
 */
static int
partcmp(struct partition **one, struct partition **two)
{
	return ((*one)->p_start - (*two)->p_start);
}

/*
 * static int
 * prtvtoc(int)
 * 	Read and print a VTOC.
 *
 * Calling/Exit State:
 *	"fd" is an open file descriptor of the device from which 
 *	a VTOC is to be displayed.
 *
 *	If successful, display a detailed listing of the device's
 *	disk geometry and VTOC partioning layout to the console,
 *	then return zero.
 *
 *	Otherwise, return -1 after displaying a failure message.
 *
 * Description:
 *	Invoke vtoc_pread() to attempt reading the VTOC from the
 *	device into a VTOC data structure.  If successful, validate
 *	the VTOC by checking it "sanity" and "version" fields for
 *	expected values.  Then invoke findfree() to locate areas on 
 *	the device that are not part of the VTOC's partitions, prior
 *	to calling puttable() to display the information in a human
 *	readable fashion on the console.
 *
 * Remarks:
 *	No check is made of the VTOC's checksum, so this tool can be
 * 	used for rebuilding a corrupted VTOC.
 */
static int
prtvtoc(int fd)
{
	reg int		idx;
	reg Freemap	*freemap;
	static struct	vtoc	*vtoc = 0;

	idx = vtoc_pread(fd, &vtoc);
	if (idx < 0) {
		return (-1);
	}

	if (vtoc->v_sanity != VTOC_SANE ||
	    vtoc->v_version != V_VERSION_1) {
		printf("prtvtoc: Invalid VTOC on device\n");
		return(-1);
	}
	freemap = findfree(vtoc);
	puttable(vtoc, freemap, name);
	return (0);
}

/*
 * static void
 * puttable(struct vtoc *, Freemap *, char *)
 * 	Display a human-readable VTOC on the console.
 *
 * Calling/Exit State:
 *	"vtoc" addresses an fully initialized in-core VTOC to display.
 *
 *	"freemap" addresses a listing of unused areas of the same device.
 *
 *	"name" is a string containing the corresponding device's path name.
 *
 *	No return value;
 *
 * Description:
 * 	Display type and geometry information about the specified
 *	disk from the VTOC to the console.  Then display the map
 *	of sections of the device not in use within the VTOC
 *	partitions, which are displayed afterwards.
 */
static void
puttable(struct vtoc *vtoc, Freemap *freemap, char *name)
{
	reg ushort idx;

	printf("* %s partition map\n", name);
	printf("*\n");
	printf("* Disk Type: %s\n", vtoc->v_disktype);
	printf("*\n* Dimensions:\n");
	printf("* %d bytes/sector\n", vtoc->v_secsize);
	printf("* %d sectors/track\n", vtoc->v_nsectors);
	printf("* %d tracks/cylinder\n", vtoc->v_ntracks);
	printf("* %d cylinders\n", vtoc->v_ncylinders);
	printf("* %d sectors/cylinder\n", vtoc->v_nseccyl);
	printf("* %d sectors/disk\n", vtoc->v_capacity);
	printf("*\n");
	printf("* Partition Types:\n");
	printf("* %d: Empty Slot\n", V_NOPART);
	printf("* %d: Regular Partition\n", V_RAW);
	printf("* %d: Bootstrap Area\n", V_BOOT);
	printf("* %d: Reserved Area\n", V_RESERVED);
	printf("* %d: Firmware Area\n", V_FW);
	printf("* %d: SCAN Dump Partition\n", V_DIAG);
	printf("*\n");
	if (freemap->fr_size) {
		printf("* Unallocated space:\n");
		printf("*\tFirst     Sector    Last\n");
		printf("*\tSector     Count    Sector \n");
		do {
			printf("*\t");
			printf("%d",freemap->fr_start);
			printf(" ");
			printf("%d",freemap->fr_size);
			printf(" ");
			printf("%d",freemap->fr_size +
						freemap->fr_start - 1);
			printf("\n");
		} while ((++freemap)->fr_size);
		printf("*\n");
	}
	printf("\
*		Start		Size		Block Sz	Frag Sz\n\
*	Type	Sector		in Sectors	in Bytes	in Bytes\n");
	for (idx = 0; idx < vtoc->v_nparts; ++idx) {
		if (vtoc->v_part[idx].p_type == V_NOPART)
			continue;
		printf("\
%d	%d	%d		%d		%d		%d",
		idx,
		vtoc->v_part[idx].p_type,
		vtoc->v_part[idx].p_start,
		vtoc->v_part[idx].p_size,
		vtoc->v_part[idx].p_bsize,
		vtoc->v_part[idx].p_fsize);
		printf("\n");
	}
}
