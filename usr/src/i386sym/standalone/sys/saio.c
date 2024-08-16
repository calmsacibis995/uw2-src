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

#ident	"@(#)stand:i386sym/standalone/sys/saio.c	1.1"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <sys/saio.h>

#define B_MOD_FLUSH     if (modify_flag) { \
                                flush_writes(); \
                                modify_flag = 0; \
                        }

struct iob	iob[NFILES];
struct inode	inode[NINODES];
struct inode	*ino_list;
int	errno;
int	modify_flag = 0;		/* set if filesys use may modify it */ 
static int setupfirst = 1;

static struct buf *in_use, *mru, *lru;

static int	parsedev(struct iob *, char **);
static void 	init_structs(void);
static void	write_buf(struct buf *);
static void	breset(void);
static void 	flush_writes(void);


extern int 	bfs_open(struct iob *, char *, mode_t);
extern int 	bfs_read(struct iob *, char *, int);
extern int 	bfs_write(struct iob *, char *, int);
extern int 	bfs_lseek(struct iob *, off_t, int);
extern void 	bzero(void *, size_t);
extern caddr_t 	calloc(int);
extern void 	callocrnd(int);
extern void 	devclose(struct iob *);
extern int 	devioctl(struct iob *, int, caddr_t);
extern void 	devopen(struct iob *);
extern int 	devread(struct iob *);
extern int 	devwrite(struct iob *);
extern off_t 	devlseek(struct iob *, off_t, int);
extern int 	getchar(void);
extern int 	printf(const char *, ...);
extern int 	putchar(int);
extern int 	stoi(char **);
extern void 	_stop(char *);
extern int 	strcmp(char *, char *);
extern void 	strcpy(char *, char *);
extern int 	vtoc_is_vdiag(daddr_t);

/*
 * External I/O functions.
 */

/*
 * int
 * open(const char *, int, ...)
 *	Open the specified device or file.
 *
 * Calling/Exit State: 
 *	The string arg specifies a standalone pathname for 
 *	the object to be opened.
 *
 *	"how" specifies the mode of access to use on the file,
 *	i.e., read-only, read-write, etc.
 *
 * 	When creating a new file, there is a mode argument on the
 * 	stack as well for use in setting the filesystem protection
 *	mask of the new file.
 *
 *	Returns -1 if the open fails.  Otherwise it is an index into
 *	the file descriptor table is returned for use in further
 *	accesses of the same open file/device.
 *
 * Description:
 *	Locate a usable file descriptor table entry and allocate
 *	it to this open.  Then invoke parsedev() to parse the
 *	device portion of the path string and log its characteristics
 *	in the file descriptor, such as its index in the device-
 *	switch table, minor device number, and device type.  For
 *	tape and packet devices, the remainder of the path string
 *	is irrelevant; just use the indicated physical device with
 *	the minor number indicating which file on the tape to use.
 *	If that is the end of the pathname, just use the indicated
 *	raw device or raw device partition.
 *
 *	Otherwise, determine if the raw device contains a valid 
 *	filesystem, which contains the specified file by invoking
 *	the filesystem open routine.  It will determine if the
 *	pathname is valid, the file is present or should be
 *	created, and also validates permissions.  Finally, flush
 *	the i/o buffer cache to ensure that it does not contain
 *	stale data from this file.
 *
 * Remarks:
 *	Only a raw device or a filesystem located on a "diagnostic"
 *	filesystem partition may be written to for integrity reasons.
 */
int
open(const char *str, int how, ...)
{
	char *cp;
	struct iob *file;
	int fdesc;
	mode_t mode;			/* only used when creating a file */

	errno = 0;
	if (setupfirst) {
		init_structs();
		setupfirst = 0;
	}

	for (fdesc = 0; fdesc < NFILES; fdesc++)
		if (iob[fdesc].i_flgs == 0)
			goto gotfile;
	_stop("No more file slots");
gotfile:
	(file = &iob[fdesc])->i_flgs |= F_ALLOC;
	fdesc += 3;		/* To skip over stdin,stdout,stderr */

	cp = (char *)str;
	if (parsedev(file, &cp) < 0) {
		B_MOD_FLUSH;
		return(-1);
	}
	file->i_howto = how;

	/*
	 * If raw device or tape or packet device, stop here.
	 */
	if (*++cp == '\0' || (file->i_flgs&(F_PACKET|F_TAPE))) {
		file->i_flgs |= how + 1;
		file->i_fname[cp-str] = '\0';
		file->i_cc = 0;
		file->i_offset = 0;
		B_MOD_FLUSH;
		return (fdesc);
	}

	/*
	 * Otherwise, determine if the device has a valid
	 * filesystem and contains the specified file.
	 * Note that only diagnostic partitions may be 
	 * written to.
	 */
	errno = 0;
	mode = (mode_t) (((int *)&how)[1]);
        if ((file->i_howto & O_MODIFY) != 0
        &&  vtoc_is_vdiag(file->i_partno) == 0){
              errno = EACCES;
		B_MOD_FLUSH;
        	return(-1);
        } 

	if (bfs_open(file, cp, mode) != 0) {
		if (errno == 0) {
			errno = ENOENT;
        		printf("Invalid filesystem on specified device.\n");
		} else {
        		printf("Invalid file path specification.\n");
		}
        	file->i_flgs = 0;
		B_MOD_FLUSH;
        	return(-1);
	}

	file->i_cc = 0;
	file->i_offset = 0;
	B_MOD_FLUSH;
	return (fdesc);	/* Successful open within filesystem */
}

/*
 * int
 * read(int, char *, int)
 * 	Read data from an open file into a buffer.
 *
 * Calling/Exit State:
 *	"fdesc" is an index into the file descriptor table
 *	for a previously opened file.
 *
 *	The other arguments define the memory location and
 *	size in bytes of the buffer into which data is to
 *	be read.
 *
 *	Returns the number of bytes read if any were or
 *	-1 if the read failed, in which case the global
 *	variable "errno" is set to a value which indicates
 *	why the attempt failed.
 *
 * Description:
 *	If fdesc corresponds to stdin, stdout, or stderr, then
 *	attempt to read a line of input from the console.  Otherwise,
 *	determine if the read is from a raw device or a filesystem.
 *	For a raw devices, generate a driver read request using an
 *	iob structure and invoke the driver to execute it.  Otherwise,
 *	invoke the filesystem read interface to perform the read.
 */
int
read(int fdesc, char *buf, int count)
{
	struct iob *file;
	int i;

	errno = 0;
	if (fdesc >= 0 && fdesc <= 2) {
		i = count;
		do {
			*buf = getchar();
		} while (--i && *buf++ != '\n');
		B_MOD_FLUSH;
		return (count - i);
	}
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0) {
		errno = EBADF;
		B_MOD_FLUSH;
		return (-1);
	}

	if ((file->i_flgs&F_READ) == 0) {
		errno = EBADF;
		B_MOD_FLUSH;
		return (-1);
	}
	if (count <= 0) {
		B_MOD_FLUSH;
		return(0);
	}

	if ((file->i_flgs & F_FILE) == 0) {
		file->i_cc = count;
		file->i_ma = buf;
		file->i_bn = file->i_boff + (file->i_offset / DEV_BSIZE);
		i = devread(file);
		file->i_offset += count;
		if (i < 0)
			errno = file->i_error;
		B_MOD_FLUSH;
		return (i);
	} else {
		B_MOD_FLUSH;
		return (bfs_read(file, buf, count));
	}
}

/*
 * int
 * write(int, char *, int)
 * 	Write data from a buffer to an open file.
 *
 * Calling/Exit State:
 *	"fdesc" is an index into the file descriptor table
 *	for a previously opened file.
 *
 *	The other arguments define the memory location and
 *	size in bytes of the buffer from which data is to
 *	be written.
 *
 *	Returns the number of bytes written if any were or
 *	-1 if the request failed, in which case the global
 *	variable "errno" is set to a value which indicates
 *	why the attempt failed.
 *
 * Description:
 *	If fdesc corresponds to stdin, stdout, or stderr, then
 *	attempt to write output to the console.  Otherwise,
 *	determine if the write is to a raw device or a filesystem.
 *	For a raw devices, generate a driver write request using an
 *	iob structure and invoke the driver to execute it.  Otherwise,
 *	invoke the filesystem write interface to perform the write.
 */
int
write(int fdesc, char *buf, int count)
{
	int i;
	struct iob *file;

	errno = 0;
	if (fdesc >= 0 && fdesc <= 2) {
		i = count;
		while (i--)
			(void)putchar(*buf++);
		B_MOD_FLUSH;
		return (count);
	}
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0) {
		errno = EBADF;
		B_MOD_FLUSH;
		return (-1);
	}
	if ((file->i_flgs&F_WRITE) == 0) {
		errno = EBADF;
		B_MOD_FLUSH;
		return (-1);
	}
	if (count <= 0) {
		B_MOD_FLUSH;
		return(0);
	}
	if ((file->i_flgs & F_FILE) == 0) {
		file->i_cc = count;
		file->i_ma = buf;
		file->i_bn = file->i_boff + (file->i_offset / DEV_BSIZE);
		i = devwrite(file);
		file->i_offset += count;
		if (i < 0)
			errno = file->i_error;
		B_MOD_FLUSH;
		return (i);
	} else {
		B_MOD_FLUSH;
		return (bfs_write(file, buf, count));
	}
}

/*
 * off_t
 * lseek(int, off_t, int)
 * 	Reposition the next I/O location of a file or device.
 *
 * Calling/Exit State:
 *	"fdesc" is an index into the file descriptor table
 *	for a previously opened file.
 *
 *	The other arguments define where to seek to.
 *
 *	Returns -1 and sets "errno" accordingly if the seek 
 *	fails due to an invalid file descriptor or a device 
 *	which cannot support the specified seek.  Otherwise, 
 *	it returns the resulting absolute file positioning 
 *	after the seek. 
 *
 * Description:
 *	If the device appears to support the operation, invoke
 *	the underlying driver lseek routine if its a raw device.
 *	Or invoke the filesystem lseek routine for normal files.
 *	In those cases, the invoked routine sets errno and 
 *	provides the final return value.
 */
off_t
lseek(int fdesc, off_t addr, int ptr)
{
	struct iob *file;

	errno = 0;
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((file = &iob[fdesc])->i_flgs & F_ALLOC) == 0) {
		errno = EBADF;
		B_MOD_FLUSH;
		return (-1);
	}
	if (ptr != 0 && !(file->i_flgs & F_FILE)) {
		printf("Seek not from beginning of file\n");
		errno = EOFFSET;
		B_MOD_FLUSH;
		return (-1);
	}
	/*
	 * if packet, call the device seeker
	 * Otherwise, seek on a regular file.
	 */
	if (file->i_flgs & F_PACKET) {
		B_MOD_FLUSH;
		return(devlseek(file, addr, ptr));
	} else {
		B_MOD_FLUSH;
		return (bfs_lseek(file, addr, ptr));
	}
}

/*
 * int
 * close(int)
 * 	Close a previously opened file.
 *
 * Calling/Exit State:
 *	"fdesc" is an index into the file descriptor table
 *	for the previously opened file.
 *
 *	Returns -1 and sets errno to EBADF if fdesc is
 *	invalid.  Otherwise returns zero after invoking
 *	the underlying device close routine for raw
 *	device files, and flushing the buffer cache to
 *	finish delayed filesystem writes and clear out
 *	stale data.
 */
int
close(int fdesc)
{
	struct iob *file;

	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0) {
		errno = EBADF;
		B_MOD_FLUSH;
		return (-1);
	}
	if ((file->i_flgs&F_FILE) == 0)
		devclose(file);
	flush_writes();		/* Make certain writes get to the device */
	breset();		/* Clear bufs that are not currently in use */
	file->i_flgs = 0;
	B_MOD_FLUSH;
	return (0);
}

/*
 * int
 * ioctl(int, int, char *)
 * 	Perform a device special control request on a open device.
 *
 * Calling/Exit State:
 *	"fdesc" is an index into the file descriptor table
 *	for a previously opened file.
 *
 *	The other arguments define a driver specific command
 *	and optional data buffer or arguments for that command.
 *
 *	Returns -1 and sets errno accordingly if fdesc is
 *	invalid or corresponds to stdin, stdout, stderr, or
 *	the driver rejects the request when passed to it.
 *	Otherwise it returns a non-negative value returned
 *	after invoking the underlying driver's ioctl routine.
 */
int
ioctl(int fdesc, int cmd, char *arg)
{
	struct iob *file;
	int error = 0;

	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES ||
	    ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0) {
		errno = EBADF;
		return (-1);
	} else if ((file->i_flgs & F_FILE) != 0) {
		errno = EINVAL;
		return (-1);
	}

	if ((error = devioctl(file, cmd, arg)) < 0)
		errno = file->i_error;
	return (error);
}

/*
 * Utility functions.
 */

/*
 * static int
 * parsedev(struct iob *, char **)
 *	Parse a standalone device specification and locate
 *	the device and subunit to which it pertains.
 *
 * Calling/Exit State:
 *	iob is a file descriptor in which to store the 
 *	characteristics of the device and subunit specified
 *	in the string located at the address pointed to by
 *	str.  The characteristics include the corresponding
 *	driver's index in the device-switch table, minor
 *	device number, and starting address on the device
 *	which may be a block number or file number, depending
 *	upon the device type.
 *
 *	"str" is updated to the character after the device
 *	specification in the pathname string.
 *	
 *	Returns zero if the name is syntactically valid and
 *	corresponds to an existing device (determined by
 *	invoking the device's driver open routine).  Returns
 *	-1 and sets errno otherwise.
 */
static int
parsedev(struct iob *file, char **str)
{
	char *cp;
	struct devsw *dp = devsw;
	int i;

	for (cp = *str; *cp && *cp != '('; cp++)
		continue;

	if (*cp != '(') {
		printf("Bad device\n");
		file->i_flgs = 0;
		errno = EDEV;
		return(-1);
	}
	*cp++ = '\0';
	for (i = n_devsw; i > 0; i--) {
		if (strcmp(*str, dp->dv_name) == 0)
			goto gotdev;
		++dp;
	}
	printf("Unknown device\n");
	file->i_flgs = 0;
	errno = ENXIO;
	return(-1);	
gotdev:
	*(cp-1) = '(';
	file->i_dev = dp - devsw;
	file->i_flgs |= (dp->dv_flags & D_PACKET ? F_PACKET : 0);
	file->i_flgs |= (dp->dv_flags & D_TAPE ? F_TAPE : 0);
	file->i_unit = stoi(&cp);
	if (file->i_unit < 0 || file->i_unit > 32768) {
		printf("Bad unit specifier\n");
		file->i_flgs = 0;
		errno = EUNIT;
		return(-1);	
	}
	if (*cp++ != ',') 
		goto badoff;

	file->i_partno = file->i_boff = stoi(&cp);
	for (;;) {
		if (*cp == ')')
			break;
		if (*cp++)
			continue;
		goto badoff;
	}
	strcpy(file->i_fname, *str);
	file->i_error = 0;
	devopen(file);
	if (file->i_error) {
		file->i_flgs = 0;
		errno = file->i_error;
		return(-1);	
	}
	*str = cp;
	return(0);
badoff:
	printf("Missing offset specification\n");
	file->i_flgs = 0;
	errno = EOFFSET;
	return(-1);
}

/*
 * static void
 * init_structs(void)
 *	Initialize the file descriptor table, inode table, and buffer cache.
 *
 * Calling/Exit State:
 *	Called once prior to opening the first device.
 *
 *	The file descriptor table is statically allocated, but
 *	still need to ensure that each elements i_flags are 
 *	cleared so appear unused.
 *
 *	The inode table is also statically allocated, but needs
 *	its elements to be linked together to form the inode
 *	free list for the filesystem code's use.
 *
 *	Lastly, the buf structures for the buffer cache must be
 *	allocated using calloc() and then linked into the LRU
 *	MRU lists used by the buffer cache management functions
 *	getblk(), bwrite(), brelse(), breset(), and flush_writes().
 *
 *	No return value.
 */
static void
init_structs(void)
{
	struct buf *bptr, *ptr;
	int i;

	/*
	 * Init iob structures
	 */
	for (i = 0; i < NFILES; i++) {
		iob[i].i_flgs = 0;	/* This indicates iob is not in use. */
	}

	/*
	 * Chain linked lists of available inodes together.
	 */
	ino_list = &inode[0];
	for (i = 0; i < NINODES-1; i++)
		inode[i].i_next = &inode[i+1];
	inode[i].i_next = NULL;

	/*
	 * Allocate bufs for the buffer cache and 
	 * stick them all on the lru/mru allocation 
	 * list.
	 */
	callocrnd(RAWALIGN);
	lru = bptr = (struct buf *)(void *)calloc(sizeof(struct buf)); 
	for (i = 0; i < NBUFS-1; i++) {
		callocrnd(RAWALIGN);
		ptr = (struct buf *)(void *)calloc(sizeof(struct buf));
		bptr->b_next = ptr;
		ptr->b_prev = bptr;
		ptr->b_next = NULL;
		bptr = ptr;
	}
	mru = bptr;
	in_use = NULL;
	return;
}

/*
 * struct buf *
 * getblk(dev_t, daddr_t, int, int)
 *	Locate a buffer cache buffer for the specified filesystem block(s).
 *
 * Calling/Exit State:
 *	"dev" designates an open physical device upon which a
 *	filesytem is located.
 *
 *	"bn" is the starting filesystem block address for which the 
 *	request is being made, for as many as "size" bytes.
 *
 *	"action" determines whether the caller is requesting an
 *	empty buffer, an empty buffer which has been zeroed, or
 *	a buffer with an up-to-date copy of the blocks(s) requested.
 *	
 * 	Always returns a buf structure address, as per the request,
 *	although it may indicate a failure to retrieve the data by
 *	setting the buf's b_flags and b_error members to B_ERROR and
 *	an errno appropriate for the type of failure that occurred.
 *
 * Description:
 *	This routine is essentially a filesystem buffer allocator and
 *	a part of the filesystem cache manager for data which has
 *	already been read or written such that the in-core version is
 *	up-to-date.  This provides reasonable performance while allowing
 *	the standalone utilities to perform cooked I/O on fixed block
 *	size devices.
 *
 *	This routine searches for the specified block in the buffer cache.
 *      If an exact match is found (same block, device, and size), then
 *	it uses the found block.  Then once found (or not found), it 
 *	performs the specified action.  'action' is one of DOREAD, 
 *	DONOTHING, or ZERO.  An action of DOREAD causes the block to be 
 *	read from the physical device if not already found in the cache.
 *	An action of ZERO causes the buffer to be zero'ed.  This means if
 *	it isn't found, it doesn't have to be read.  DONOTHING says we don't
 *	need to zero or read it.  Just assign the buffer to the block and
 *	return it.
 *
 *	Since delayed writes are also supported by this cache scheme, it
 *	may be necessary to write out the contents of a buffer in the
 *	cache in order to allocate that buffer to a new request.
 */
struct buf *
getblk(dev_t dev, daddr_t bn, int size, int action)
{
	struct buf *bp = NULL;
	daddr_t start, last;
	int found = 0;
	struct iob iob;

	if (bn <= 0) {		/* just find empty block */
		goto searchdone;
	}
	start = bn;
	last = start + btodb(size) - 1;

	/*
	 * First search the in-use list
	 */
	for (bp = in_use; bp; bp = bp->b_next) {
		if (bp->b_dev == dev) {
			if (bp->b_bcount == 0 || bp->b_blkno > last
			    || bp->b_blkno + btodb(bp->b_bcount) <= start)
				continue;

			/*
			 * found an overlap.  Exact match is
			 * okay, otherwise, yikes!  we're
			 * not prepared to deal with an inuse
			 * overlap.
			 */
			if (bp->b_blkno != bn || bp->b_bcount != size
			    || action == ZERO) {
				if (action == ZERO) {
					printf("PANIC: zeroing in-use buffer!\n");
				} else {
					printf("PANIC: overlap blkno in use!\n");
					printf("blk %d, size %d in use, ",
						bp->b_blkno, bp->b_bcount);
					printf("need blk %d, size %d\n", bn, size);
				}
				/*
				 *+ we're not prepared to handle this!
				 */
				errno = EIO;
				bp->b_flags |= B_ERROR;
			}
			bp->b_usecnt++;
			bp->b_flags &= ~B_READ;
			return(bp);
		}
	}
	/*
	 * now search free list, starting at MRU end
	 */
	for (bp = mru; bp; bp = bp->b_prev) {
		if (bp->b_dev == dev) {
			if (bp->b_bcount == 0 || bp->b_blkno > last
			    || bp->b_blkno + btodb(bp->b_bcount) <= start)
				continue;
			/*
			 * found overlap.  If exact match, good,
			 * otherwise must invalidate
			 */
			if (bp->b_blkno == bn && bp->b_bcount == size) {
				found = 1;
				break;
			} else {
				bp->b_blkno = 0;
				bp->b_dev = 0;
				bp->b_bcount = 0;
			}
		}
	}

	/*
	 * extract from free list
	 */
searchdone:
	if (bp) {
		if (bp->b_prev)
			bp->b_prev->b_next = bp->b_next;
		if (bp->b_next)
			bp->b_next->b_prev = bp->b_prev;
		if (bp == lru)
			lru = bp->b_next;
	} else {
		/*
		 * grab buffer from lru end
		 */
		if (!lru) {
			printf("PANIC: out of bufs\n");
			/* 
			 * use head of in-use list to return the error 
			 */
			errno = EIO;
			in_use->b_flags |= B_ERROR;
			in_use->b_usecnt++;
			return(in_use);
		}
		bp = lru;
		lru = bp->b_next;
		if (lru)
			lru->b_prev = NULL;
	}
	if (bp == mru)
		mru = bp->b_prev;

	if (!found) {
		/* fill in buffer fields */
		if (bp->b_flags & B_MOD)
			write_buf(bp);	
		bp->b_blkno = bn;
		bp->b_dev = dev;
		bp->b_bcount = size;
		bp->b_flags = 0;
	} else
		bp->b_flags &= ~B_READ;

	bp->b_usecnt++;

	/* put in the in-use list */
	bp->b_next = in_use;
	bp->b_prev = NULL;
	if (in_use)
		in_use->b_prev = bp;
	in_use = bp;

	switch (action) {
	case DOREAD:
		if (!found) {
			bzero((char *)&iob, sizeof (struct iob));
			iob.i_dev = major(dev);
			iob.i_unit = minor(dev);
			iob.i_bn = bn;
			iob.i_cc = size;
			iob.i_ma = bp->b_buf;
			if (devread(&iob) < 0) {
				errno = iob.i_error;
				bp->b_flags |= B_ERROR;
			}
			bp->b_flags |= B_READ;
		}
		break;

	case DONOTHING:
		break;

	case ZERO:
		bzero(bp->b_buf, size);
		break;

	}
	return(bp);
}
	
/*
 * void
 * brelse(struct buf *, int)
 * 	Release the specified buffer from the caller's ownership.
 *
 * Calling/Exit State:
 *	The buf must have previously been allocated by getblk().
 *
 *	"kflag" is a hint as to the priority the buffer should
 *	be given for new allocations from the buffer cache, i.e.,
 *	how likely it is that its contents will be requested again
 *	shortly.
 *
 *	No Return value.
 *
 * Description:
 *	Decrement the use count on the buffer, and if no longer
 *	in use, put back in the buffer cache free list.  If 
 *	'kflag' is set to B_NOAGE, the buffer is put at the MRU 
 *	end of the list, otherwise at LRU end of list.  (LRU end 
 *	is where buffers are taken from in getblk).
 */
void
brelse(struct buf *bp, int kflag)
{
	if (--bp->b_usecnt > 0) {		/* still in use */
		return;
	}

	if (bp->b_flags & B_ERROR) {
		bp->b_dev = bp->b_blkno = 0;
		bp->b_bcount = bp->b_flags = 0;
	}

	/*
	 * take out of inuse list
	 */
	if (bp->b_prev)
		bp->b_prev->b_next = bp->b_next;
	else
		in_use = bp->b_next;
	if (bp->b_next)
		bp->b_next->b_prev = bp->b_prev;

	if (kflag != MRU) {
		/* put at the lru end of list */
		bp->b_next = lru;
		bp->b_prev = NULL;
		if (lru) {
			lru->b_prev = bp;
			lru = bp;
		} else
			lru = mru = bp;
	} else {
		/* put at mru end of list */
		bp->b_prev = mru;
		bp->b_next = NULL;
		if (mru) {
			mru->b_next = bp;
			mru = bp;
		} else
			lru = mru = bp;
	}
	return;
}

/*
 * void
 * bwrite(struct buf *, int, int)
 *	Write the contents of the specified buffer to disk.  
 *
 * Calling/Exit State:
 *	"bp" is the buffer whose contents are to be written.
 *	the buffer indicates the physical block location and
 *	#of bytes on the device that it corresponds to.
 *
 *	'delayflag' specifies whether the buf-contents are
 *	to be written immediately or may be cached until 
 *	required to be flushed (W_DELAY set).
 *
 *	'releaseflag' indicates if the buffer is to be released
 *	via brelse() after its contents are written or scheduled
 *	for writing (by setting B_MOD in bp->b_flags).  If released
 *	it also indicates to b_relse which allocation list it should
 *	go on (LRU vs. MRU).  
 *
 *	No return value.
 */ 
void
bwrite(struct buf *bp, int releaseflag, int delayflag)
{
	if (delayflag == W_DELAY)
		bp->b_flags |= B_MOD;
	else 
		write_buf(bp);
	if (releaseflag != DONTFREE)
		brelse(bp, releaseflag);
}

/*
 * static void
 * write_buf(struct buf *)
 *	Write the contents of a buffer to a physical device.
 *	
 * Calling/Exit State:
 *	'bp' addresses a buffer cache buffer whose data
 *	is now to be written out to the physical device 
 *	specified within the same buf structure.
 *
 *	Assume the device specified in the buf is
 *	currently open to the caller.
 *
 *	No return value, although upon failure errno is
 *	set to the errno returned by the driver write
 *	function in the iob, and B_ERROR is set in bp->b_flags
 *	to indicate to future buf-cache accesses that the
 *	buf operation did fail.
 *
 * Description:
 *	Dummy up an iob structure to support the request and
 *	then pass it along when calling the designated device's
 *	driver write routine.  Clear the B_MOD b_flag of the
 *	buffer to indicate it no longer needs to be written.
 */
static void
write_buf(struct buf *bp)
{
	struct iob iob;

	bp->b_flags &= ~B_MOD;
	bzero((char *)&iob, sizeof (struct iob));
	iob.i_dev = major(bp->b_dev); 
	iob.i_unit = minor(bp->b_dev);
	iob.i_bn = bp->b_blkno; 
	iob.i_cc = bp->b_bcount; 
	iob.i_ma = bp->b_buf;
	if (devwrite(&iob) < 0) {
		errno = iob.i_error;
		bp->b_flags |= B_ERROR;
	}
	return;
}

/*
 * static void
 * flush_writes(void)
 *	Perform all delayed writes currently pending in the buffer cache.
 * 
 * Calling/Exit State:
 *	There may be buffers in the buffer cache for which
 *	writes have been delayed following data updates.  
 *	They have B_MOD set in their b_flags field.
 *	
 *	No return value, although all buffers marked B_MOD
 *	on the buf-cache free list (MRU and LRU) are located 
 *	and written using write_buf() prior to returning.
 */
static void
flush_writes(void)
{
	struct buf *bp;

	for (bp = lru; bp; bp = bp->b_next) {
		if (bp->b_flags & B_MOD)
			write_buf(bp);
	}
}

/*
 * static void
 * breset(void)
 *	Reset the buffer cache so all subsequent accesses
 *	force data to be re-read from their devices.
 *
 *  Calling/Exit State:
 *	All buffers not currently in use are marked as
 *	as empty, so subsequent calls to getblk() are
 *	forced to read the data from the device again, 
 *	instead of using a cached (possibly stale) copy.
 *
 *	No return value.
 */
static void
breset(void)
{
	struct buf *ptr;

	for (ptr = lru; ptr != NULL; ptr = ptr->b_next) {
		ptr->b_bcount = 0;
	}
	return;
}
