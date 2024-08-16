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

#ident	"@(#)stand:i386sym/standalone/sys/vtoc.c	1.1"

/*
 * vtoc.c
 *	Stand-alone VTOC support
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/vtoc.h>
#include <sys/unistd.h>
#include <sys/saio.h>

#define PARTBUFSIZE		20	/* Big enough for "xx(yy,255)" */
#define INPUTBUFSIZE		80	/* Max size of SCED input buffer */
#define	ignore_part(type)	((type) == V_NOPART || (type) == V_RESERVED)

static char *vbuf;

extern int      atoi(char *);
extern void     bzero(void *, size_t);
extern caddr_t  calloc(int);
extern void     callocrnd(int);
extern int      devread(struct iob *);
extern char 	*index(char *, char);
extern int	open(const char *, int, ...);
extern int      printf(const char *, ...);
extern void     strcpy(char *, char *);
extern int 	strlen(char *);

/* Forward references */
int vtoc_setboff(struct iob *, int);
int vtoc_getpsize(char *);
int vtoc_pread(int, struct vtoc **);
long vtoc_get_cksum(struct vtoc *);
int vtoc_is_vdiag(daddr_t);

/*
 * int
 * vtoc_setboff(struct iob *, int)
 *	Set a slice's start block address based upon the device's VTOC.
 *
 * Calling/Exit State:
 *	Upon entry io->boff contains the device's slice to
 *	be referenced in the VTOC.  Upon successful completion,
 *	it is reset to the start block address of that slice.
 *
 *	"offset" is the physical block address on the device
 *	specified in the iob at which the VTOC should be located.
 *
 *	io->error may be set to an errno upon failure of this
 *	function.
 *	
 *	Returns zero upon success; -1 upon failure, such as
 *	when the VTOC cannot be read or the requested slice is
 *	not valid in the VTOC.
 *	
 * Description:
 * 	If the slice requested is V_NUMPAR, then they are requesting
 *	access to the entire drive.  In that case, don't read the
 *	VTOC and set io->boff to allow access starting at sector
 *	zero of the physical device and return success.
 *
 *	Otherwise, read the VTOC from the specified location and
 *	validate it.  If the VTOC is correct and the partition is 
 *	valid, vtoc_setboff resets io->i_boff to the offset (in 
 *	sectors) of the partition start relative to the beginning
 *	of the unpartitioned device.  io->i_boff is then used by the 
 *	rest of standalone to be added to the block number in all 
 *	subsequent IO operations.
 *
 * 	If the VTOC is missing or invalid, io->i_error gets set to EDEV 
 *	and vtoc_setboff returns -1.  If the VTOC is correct but 
 *	the partition is not valid, io->i_error gets set to EUNIT 
 *	and vtoc_setboff returns -1.
 */
int
vtoc_setboff(struct iob *io, int offset)
{
	struct vtoc *v;
	ulong cksum;
	int tries;

	/*
	 * Partition numbers one larger than the max are assumed to
	 * refer to the whole disk.  Skip the VTOC read.
	 */
	if (io->i_boff == V_NUMPAR) {
		io->i_boff = 0;
		return(0);
	}

	if (vbuf == NULL) {
		callocrnd(DEV_BSIZE);
		vbuf = (char *)calloc(V_SIZE);
	}

	/*
	 * Work around for scan dump problem. Every so often the VTOC_SANE
	 * field would return bogus value. A second read seems to come out
	 * ok. This is somewhat related to the SSM-2 mod made to allow
	 * resetting of the scsi bus. It is suspected that sometimes the
	 * reset doesn't take care of everything.
	 */
	for (tries = 0; ; tries++) {
		io->i_ma = vbuf;
		io->i_cc = V_SIZE;
		io->i_bn = V_VTOCSEC + offset;
		if (devread(io) < 0) {
			io->i_boff = 0;
			return(-1);
		}

		v = (struct vtoc *)(void *)vbuf;
		if (v->v_sanity == VTOC_SANE && v->v_version == V_VERSION_1) {
			break;		/* VTOC appears to be readable */
		} else if (tries == 4) {
			/* Enough retries; fail the request */
			printf("No VTOC present on device\n");
			io->i_error = EDEV;
			return(-1);
		}	/* Otherwise, try reading the VTOC again */
	}
	cksum = v->v_cksum;
	v->v_cksum = 0;
	if (cksum != vtoc_get_cksum(v)) {
		printf("VTOC checksum mismatch - suspect a corrupted VTOC\n");
		io->i_error = EDEV;
		return(-1);
	}

	if ((ushort)io->i_boff >= v->v_nparts ||
	   v->v_part[io->i_boff].p_type == V_NOPART) {
		printf("Partition %d not in VTOC\n", io->i_boff);
		io->i_error = EUNIT;
		return(-1);
	}
	io->i_boff = v->v_part[io->i_boff].p_start;
	return(0);
}

/*
 * int
 * vtoc_getpsize(char *)
 *	Return the the size (in sectors) of the specified partition.
 *
 * Calling/Exit State:
 * 	The string passed in contains the name of a device
 *	to be looked up in the VTOC.  It is in standalone 
 *	device/pathname format.
 *
 *	Returns the number of sectors on the specified
 *	minor device if successful.  Returns -1 if a valid
 *	VTOC cannot be read from the device or if the specified
 *	slice is invalid.
 *
 * Remarks:
 * 	Assumes that the driver will try to open() a partition 
 *	before trying to figure out what its partition size is, 
 *	since vtoc_getpsize() does not sanity check the VTOC.
 */
int
vtoc_getpsize(char *device)
{
	char		*dp;
	char		*pp;
	int		i;
	struct	vtoc	*v = (struct vtoc *)NULL;
	char		partbuf[PARTBUFSIZE];	/* character version of V_NUMPAR */
	char		savedev[INPUTBUFSIZE];	/* local copy of device */
	int		n = V_NUMPAR;
	ushort		partno;

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

	/*
	 * save the device name to a buffer where we can mess with it.
	 */
	strcpy(savedev, device);

	/*
	 * Replace the partition number with V_NUMPAR
	 */
	dp = index(savedev, ',');
	if (dp == 0) {
		printf("Must specify a device name of the form:");
		printf(" XX(y,z)\n");
		return(-1);
	}
	dp++;
	strcpy(dp, pp);
	dp = savedev + strlen(savedev);
	strcpy(dp, ")");

	/*
	 * Now try opening this device and reading the VTOC
	 */
	i = open(savedev, 0);
	if (i < 0) {
		printf("Unable to open device %s\n", savedev);
		return(-1);
	}
	if (vtoc_pread(i, &v) < 0) {
		printf("Error reading VTOC from %s\n", savedev);
		return(-1);
	}
	close(i);

	/*
	 * Determine which partition needs to be looked up.  partbuf[]
	 * is reused here to house partition number string.
	 */
	bzero(partbuf, sizeof(partbuf));
	dp = index(device, ',');
	dp++;
	strcpy(partbuf, dp);
	dp = index(partbuf, ')');
	*dp = '\0';
	partno = (ushort)atoi(partbuf);
	if (partno >= v->v_nparts) {
		printf("Unable to find partition %d in VTOC\n", partno);
		return(-1);
	}
	if (ignore_part(v->v_part[partno].p_type)) {
		printf("Unable to find partition %d in VTOC\n", partno);
		return(-1);
	}
	return(v->v_part[partno].p_size);
}

/*
 * int
 * vtoc_pread(int, struct vtoc **)
 *	Read the VTOC off the specified device.
 *
 * Calling/Exit State:
 * 	Allocate a buffer for the VTOC to be read into, if the
 *	buffer pointer addressed by "buf" is NULL, in which
 *	case that pointer is updated to address the new allocation.
 *	The vtoc will be read into this buffer.
 *
 *	Assumes that fd corresponds to an open file descriptor,
 *	indicating the physical device from which the VTOC is
 *	to be read from a predetermined location.
 *
 * 	Returns: -1 on failure, 0 upon success.
 */
int
vtoc_pread(int fd, struct vtoc **buf)
{
	unsigned long	block = V_VTOCSEC;
	unsigned long	len = V_SIZE;
	int	err, offset;

	if (*buf == (struct vtoc *)NULL) {
		callocrnd(DEV_BSIZE);
		*buf = (struct vtoc *)(void *)calloc(V_SIZE);
	}
	err = ioctl(fd, SAIOFIRSTSECT, (char *)&offset);
	if (err < 0) {
		return(-1);
	}
	block += offset;
	if (lseek(fd, (long)(block * DEV_BSIZE), 0) < (long)0) {
		printf("vtoc: Cannot seek disk to block %d\n",
								block);
		return(-1);
	}
	if (read(fd, (char *)*buf, (unsigned)len) != len) {
			printf("vtoc: Cannot read VTOC from disk block %d\n",
				block);
			return(-1);
	}
	return(0);
}

/*
 * long
 * vtoc_get_cksum(struct vtoc *)
 * 	checksum a VTOC.
 *
 * Calling/Exit State:
 *	Assume a VTOC has been read or created at the location
 *	specified.
 *
 *	Return a checksum computed by simply adding together
 *	the values of all the longwords that make up that VTOC.
 */
long
vtoc_get_cksum(struct vtoc *v)
{
	long sum;
	int  nelem = sizeof(struct vtoc) / sizeof(long);
	long *lptr = (long *)v;

	sum = 0;
	while (nelem-- > 0) {
		sum += *lptr;
		++lptr;
	}
	return (sum);
}

/*
 * int
 * vtoc_is_vdiag(daddr_t)
 *
 * Calling/Exit State: 
 *	assume that the VTOC to use is still in the 
 *	structure addressed by the global "vbuf"; it
 * 	should not been used for anything else since 
 *	its  devopen().
 *
 *	"boff" is an index referencing a slice within
 *	the VTOC in 'vbuf'.
 *
 *	Return zero if the index exceeds the maximum
 *	allowed slice number or if the slice' type is
 *	not V_DIAG.  Otherwise return 1, indicating 
 *	that the slice is marked as diagnostic.
 */
int
vtoc_is_vdiag(daddr_t boff)
{
	struct vtoc *v = (struct vtoc *)(void *)vbuf;

	if (boff == V_NUMPAR)
		return(0);
	if (v->v_part[boff].p_type == V_DIAG)
		return(1);
	else
		return(0);
}
