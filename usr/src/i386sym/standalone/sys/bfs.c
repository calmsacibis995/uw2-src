/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386sym/standalone/sys/bfs.c	1.1"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <sys/bfs.h>
#include <sys/saio.h>

extern void bcopy(void *, void *, size_t);
extern void bzero(void *, size_t);
extern caddr_t calloc(int);
extern int printf(const char *, ...);
extern int strncmp(char *, char *, int);
extern void _stop(char *);

extern struct inode *ino_list;
static struct bfs_info	*bfs_info; 	/* Linked list of allocated BFS 
					 * info structs */
static int free_ldir;			/* Index used to note a recently 
					 * encounted available entry in 
					 * filesystems ldirs array. */

static int 		bfs_creat(struct iob *, char *, mode_t);
static int 		bfs_get_fs(struct iob *iob, int rdwr);
static int 		bfs_find(char *, struct iob *, ino_t *, char **);
static struct inode 	*bfs_iget(dev_t, struct bfs_info *, ino_t);
static void 		bfs_iput(struct inode *);
static void 		bfs_iupdat(struct inode *);
static void		bfs_itrunc(struct inode *, ulong);
static int		bfs_creat(struct iob *, char *, mode_t);
static int		bfs_rwip(struct iob *, char *, int, int);
static void		bfs_compact(struct bfs_info *, struct inode *);

/*
 * External Filesystem entry points.
 */

/* 
 * int
 * bfs_open(struct iob *, char *, mode_t)
 *	Open a file on a BFS filesystem.
 *
 * Calling/Exit State:
 *	"file" is an open file descriptor for the physical device
 * 	upon which the filesystem is located.  file->i_boff is set
 *	to the start block address on that device where the 
 *	filesystem partition is located.
 *
 *	"cp" is a string containing the filesystem pathname
 *	of the file to be opened on the filesystem.  It must
 *	be of the form: "[x][/]*y", although subdirectory
 *	components ("x") are irrelevant since BFS is a flat 
 *	filesystem.
 *
 *	"mode" must contain protection to use if the file
 *	must be created.
 *
 *	Returns zero, saves inode and access mode information 
 *	in the file descriptor if the open is successful.  
 *	Returns -1 otherwise.
 *
 * Description:
 *	Invoke bfs_get_fs() to determine if the device specified in
 *	the file descriptor contains a valid BFS filesystem.  If so,
 *	invoke bfs_find to parse the pathname string to determine
 *	if the specified file already exists, then read its inode.
 *	
 *	If the file does not exist, call bfs_creat() to create it
 *	if requested and allowed to do so.  Othewise, determine if
 *	the access modes are permitted based on the file's protection
 *	mode settins from its inode and fail incompatable attempts.
 *	Finally, note the access mode in the file descriptor and
 *	truncate the file if requested, updating its on-disk inode
 *	afterwards.
 */
int 
bfs_open(struct iob *file, char *cp, mode_t mode)
{
	ino_t pino;			/* parent inode of file to open */
	char *filename;			/* final component of path name */

	if (bfs_get_fs(file, ((file->i_howto & O_MODIFY)?B_WRITE:B_READ)) < 0) {
		file->i_flgs = 0;
		return(-1);	
	}
	if (bfs_find(cp, file, &pino, &filename) != 0) {
		if ((file->i_howto & O_CREAT) == 0) {
			errno = ENOENT;
			return (-1);

		} else if (bfs_creat(file, filename, mode) != 0) {
			return (-1);	/* couldn't create file */
		}

	} else if (file->i_ino->i_dir.d_fattr.va_type != VREG) {
		/*
		 * can only access regular files.
		 */
		errno = EACCES;
		bfs_iput(file->i_ino);
		return (-1);
	} else if ((file->i_howto & O_MODIFY) != 0
	       &&  (file->i_ino->i_dir.d_fattr.va_mode & 0222) == 0) {
		/*
		 * Can only modify writable files.
		 */
		errno = EACCES;
		bfs_iput(file->i_ino);
		return (-1);

	} else if ((file->i_howto & (O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL)) {
		errno = EEXISTS;
		bfs_iput(file->i_ino);
		return (-1);
	}

	file->i_inodenum = file->i_ino->i_number;

	if (file->i_howto & O_RDWR)
		file->i_flgs |= F_WRITE | F_READ | F_FILE;
	else if (file->i_howto & O_WRONLY)
		file->i_flgs |= F_WRITE | F_FILE;
	else
		file->i_flgs |= F_READ | F_FILE;

	if ((file->i_howto & O_TRUNC) != 0) {
		/* truncate the file */
		bfs_itrunc(file->i_ino, (ulong)0);
	}

	bfs_iput(file->i_ino);
	return (0);
}

/* 
 * int
 * bfs_read(struct iob *, char *, int)
 *	Read from a file on a BFS filesystem.
 *
 * Calling/Exit State:
 *	"file" is an open file descriptor containing information
 *	about the filesystem's physical location and the file's 
 *	in-core inode, current access offset, and file-size.
 *
 *	"buf" is the address of a buffer into which the caller
 *	would like to have the next "count" bytes transfered from 
 *	the specified file.
 *
 *	Returns -1 if the file cannot be read, zero if there
 *	is no more data that can be read from the specified
 *	location (end-of-file), or the actual number of bytes
 *	successfully read otherwise.  In the case of an error
 *	from the physical device, file->i_error may be set to
 *	an applicable errno by the underlying device driver.
 *
 * Description:
 *	Locate the file's inode with bfs_iget() to determine the 
 *	file's current byte size and whether there is any data 
 *	to read from it.  If so, invoke bfs_rwip() to attempt
 *	reading the requested data into the caller's buffer and
 *	also return a count of the bytes successfully read. 
 *	Prior to returning to the caller, release the inode
 *	with bfs_iput().
 *
 * Remarks:
 *	The filesystem data blocks and inode may actually be 
 *	be cached by cache managers hidden behind bfs_iget(),
 *	bfs_iput(), and bfs_rwip().  This should have a positive
 *	impact since we support cooked I/O on a fixed block device.
 *	So, the physical device may not really being accessed as
 *	often as it might otherwise appear that it is.
 */
int 
bfs_read(struct iob *file, char *buf, int count)
{
	int filesize;

	file->i_ino = bfs_iget(makedev(file->i_dev, file->i_unit),
			file->i_bfs, file->i_inodenum);
	if (file->i_ino == NULL) {
		printf("can't read inode <%d>\n", file->i_inodenum);
		return (-1);
	}
	file->i_ino->i_bfs = file->i_bfs;

	if (file->i_ino->i_dir.d_sblock == 0)
		filesize = 0;
	else
		filesize = file->i_ino->i_dir.d_eoffset + 1 -
			(file->i_ino->i_dir.d_sblock * BFS_BSIZE);

	if (file->i_offset + count > filesize) {
		/* Scale back read size to match remaining file size */
		count = filesize - file->i_offset;
	}
	if (count <= 0) {
		/* Nothing to read; return end-of-data */
		bfs_iput(file->i_ino);
		return (0);
	}
	count = bfs_rwip(file, buf, count, READ);
	bfs_iput(file->i_ino);
	return (count);
}

/* 
 * int
 * bfs_write(struct iob *, char *, int)
 *	Read from a file on a BFS filesystem.
 *
 * Calling/Exit State:
 *	"file" is an open file descriptor containing information
 *	about the filesystem's physical location and the file's 
 *	in-core inode, current access offset, and file-size.
 *
 *	"buf" is the address of a buffer from which the caller
 *	would like to have the next "count" bytes transfered to 
 *	the specified file.
 *
 *	Returns -1 if the file cannot be written, zero if the
 *	write attempt fails entirely, or the actual number of 
 *	bytes successfully written otherwise.  In the case of 
 *	an error from the physical device, file->i_error may 
 *	be set to an applicable errno by the underlying device 
 *	driver.
 *
 * Description:
 *	Locate the file's inode with bfs_iget(), since it must
 *	be updated to reflect the results of the attempted
 *	transfer and to located the file itself.  Then invoke 
 *	bfs_rwip() to attempt writting the requested data from 
 *	the caller's buffer into the file.  bfs_rwip() will
 *	take care of expanding the file and updating the inode
 *	as needed to accomodate the request.  It also returns 
 *	a count of the bytes successfully written, which is 
 *	in-turn returned by this function after releasing the 
 *	inode with bfs_iput().
 *
 * Remarks:
 *	The filesystem data blocks and inode may actually be 
 *	be cached by cache managers hidden behind bfs_iget(),
 *	bfs_iput(), and bfs_rwip().  This should have a positive
 *	impact since we support cooked I/O on a fixed block device.
 *	So, the physical device may not really being accessed as
 *	often as it might otherwise appear that it is.
 */
int 
bfs_write(struct iob *file, char *buf, int count)
{
	file->i_ino = bfs_iget(makedev(file->i_dev, file->i_unit),
			file->i_bfs, file->i_inodenum);
	if (file->i_ino == NULL) {
		printf("can't read inode <%d>\n", file->i_inodenum);
		return (-1);
	}
	file->i_ino->i_bfs = file->i_bfs;

	count = bfs_rwip(file, buf, count, WRITE);

	bfs_iput(file->i_ino);
	return (count);
}

/*
 * off_t
 * bfs_lseek(int, off_t, int)
 *      Reposition the next I/O location of a file.
 *
 * Calling/Exit State:
 *	"file" is an open file descriptor containing information
 *	about the filesystem's physical location and the file's 
 *	in-core inode, current access offset, and file-size.
 *
 *      The other arguments define where to seek to.
 *
 *      Returns -1 and sets "errno" accordingly if the seek
 *      parameters are invalid or the seek fails for any reason.
 *      Otherwise, it returns zero after updating the file 
 *	descriptor to reflect the new access location.
 *
 * Remarks:
 *      When seeking relative to the end of the file, the file's
 *	inode must be retrieved to determine its current size.
 *
 *	When the seek goes past the current end of the file, 
 *	the expansion does not actually occur until an attempt
 *	is made to write at a location past that end-of-file.
 */
int 
bfs_lseek(struct iob *file, off_t addr, int ptr)
{
	off_t new_offset;
	int filesize;

	switch (ptr) {
	case SEEK_SET:
		new_offset = addr;
		break;
	case SEEK_CUR:
		new_offset = addr + file->i_offset;
		break;
	case SEEK_END:

		file->i_ino = bfs_iget(makedev(file->i_dev, file->i_unit),
			file->i_bfs, file->i_inodenum);
		if (file->i_ino == NULL) {
			printf("can't read inode <%d>\n", file->i_inodenum);
			return (-1);
		}
		file->i_ino->i_bfs = file->i_bfs;
		if (file->i_ino->i_dir.d_sblock == 0)
			filesize = 0;
		else
			filesize = file->i_ino->i_dir.d_eoffset + 1 -
				(file->i_ino->i_dir.d_sblock * BFS_BSIZE);

		new_offset = filesize + addr;
		bfs_iput(file->i_ino);
		break;
	default:
		errno = EINVAL;
		return (-1);
	}

	if (new_offset < 0) {
		printf("Negative seek offset\n");
		errno = EINVAL;
		return (-1);
	}
	file->i_offset = new_offset;
	file->i_bn = new_offset / DEV_BSIZE;
	file->i_cc = 0;
	return (0);
}

/*
 * Utility functions.
 */

/*
 * static int
 * bfs_get_fs(struct iob *, int)
 *	Validate the BFS on the specified device and setup in-core 
 *	structures for accessing it.
 *
 * Calling/Exit State:
 *	"iob" is a file descriptor which is being opened, for which
 *	the BFS is being validated.  Its i_dev, i_unit, and i_boff
 *	members describe the physical device location of the
 *	expected filesystem. 
 *
 *	"rdwr" indicates if the caller intends to write on portions
 *	of the filesystem.  It helps determine whether this function
 *	must generate an allocation list for facilitating file
 *	expansion. 
 *
 *	If successful, sets iob->i_bfs to the address of the filesystem's
 *	in-core superblock information for future references, then 
 *	returns zero.  Otherwise, it returns -1 indicating failure.
 *	
 * Description:
 *	Since the filesystem may already be in use, search the linked
 *	list of BFS information structures to determine if its 
 *	information is already in-core.  The search attempts to match 
 *	up the i.d. of the underlying device and its BFS start address.  
 *	If not found, allocate a new BFS structure and link it onto
 *	that list if the the real filesystem is valid.
 *
 *	Validate the BFS by reading its superblock and verifying that
 *	it has a BFS-specific magic number in a predetermined location.
 *	If valid, use the superblock to determine and save the size of the
 *	BFS, along with the locations of its root directory, containing
 *	its existing file inodes, and the data area for the files.
 *
 *	If the filesystem is to be written to, then an accounting
 *	of unused inodes and data blocks must be generated, from which 
 *	to allocate new files space.  The specific resource to be
 *	allocated will be located when it is actually needed.  This 
 *	requires that all the inodes be read, since only they contain
 *	the information needed for this calculation.
 */
static int
bfs_get_fs(struct iob *iob, int rdwr)
{
	struct buf *bp;
	struct bfs_info *bfs;
	struct inode *ip;
	dev_t  devno = makedev(iob->i_dev, iob->i_unit);
	ino_t  ino;
	int    n_inodes;

	/* 
	 * Search for pre-existing BFS information.
	 */
	for (bfs = bfs_info; bfs != NULL; bfs = bfs->bsup_next) {
		if (bfs->bsup_dev == devno && bfs->bsup_boff == iob->i_boff)
			break;		/* Superbuf info exists */
	}

	if (bfs == NULL) {
		/* 
		 * Allocate a new bfs_info structure, then read
		 * and validate the superbuff from the disk,
		 * extracting needed info from it.
		 */
		bfs = (struct bfs_info *) (void *)calloc(sizeof (struct bfs_info));
		if (bfs == NULL) 
			return (-1);

		bp = getblk(devno, (BFS_SUPEROFF/BFS_BSIZE) + iob->i_boff, 
			BFS_BSIZE, DOREAD);
		if ((bp->b_flags & B_ERROR) != 0) {
			printf("super block read error\n");
			brelse(bp, MRU);
			return (-1);
		} else if (((struct bdsuper *)(void *)bp->b_buf)->bdsup_bfsmagic != BFS_MAGIC){
			brelse(bp, MRU);
			return (-1);		 /* Superblock unrecognizable */
		}

		bfs->bsup_start = ((struct bdsuper *)(void *)bp->b_buf)->bdsup_start;
		bfs->bsup_end = ((struct bdsuper *)(void *)bp->b_buf)->bdsup_end;
		bfs->bsup_dev = devno;
		bfs->bsup_boff = iob->i_boff;

		/* 
		 * Read the root inode to determine the size of the
		 * BFS directory entry array which follows the superbuf
		 * in the next sector on the disk.
		 */
		ip = bfs_iget(devno, bfs, BFSROOTINO);
		if (ip == NULL) {
			printf("Can't read root inode.\n");
			brelse(bp, MRU);
			return (-1);
		}
        	bfs->bsup_uid = ip->i_dir.d_fattr.va_uid;
        	bfs->bsup_gid = ip->i_dir.d_fattr.va_gid;
   	    	bfs->bsup_dirsblk = ip->i_dir.d_sblock;
		n_inodes = (bfs->bsup_dirsblk * BFS_BSIZE - BFS_DIRSTART)
				  / sizeof (struct bfs_dirent);
		bfs->bsup_ndirs = n_inodes + BFSROOTINO - 1; /* max inode # */
		bfs->bsup_eldirs = ip->i_dir.d_eoffset;
        	bfs->bsup_freeblocks = -1;
		bfs_iput(ip);
		brelse(bp, MRU);

		/* Insert the new BFS info into the reusable list */
		bfs->bsup_next = bfs_info;
		bfs_info = bfs;
	} else {
		n_inodes = (bfs->bsup_dirsblk * BFS_BSIZE - BFS_DIRSTART)
				  / sizeof (struct bfs_dirent);
	}

	if (rdwr == B_WRITE && bfs->bsup_freeblocks < 0) {
		/* 
		 * Determine the amount of free resources
		 * available during writes and file creation.
		 */
        	bfs->bsup_freedrents = n_inodes;
        	bfs->bsup_freeblocks = 
			(bfs->bsup_end - bfs->bsup_start + 1) / BFS_BSIZE;
        	bfs->bsup_lastfile = BFSROOTINO;
        	bfs->bsup_lasteblk = (bfs->bsup_start / BFS_BSIZE) - 1;
		for (ino = BFSROOTINO; ino < bfs->bsup_ndirs; ino++) {
			ip = bfs_iget(devno, bfs, ino);
			if (ip == NULL) {
				printf("inode table read failed.\n");
				return (-1);
			}
			if (ip->i_dir.d_ino != 0) {
        			bfs->bsup_freedrents--;
				if ((ip->i_dir.d_fattr.va_type == VREG
				    || ip->i_dir.d_fattr.va_type == VDIR)
				&& ip->i_dir.d_sblock != 0) {

        				bfs->bsup_freeblocks -= 
						(ip->i_dir.d_eblock - 
						ip->i_dir.d_sblock) + 1;

					if (ip->i_dir.d_eblock > 
						bfs->bsup_lasteblk) {
        					bfs->bsup_lasteblk = 
							ip->i_dir.d_eblock;
        					bfs->bsup_lastfile = ino;
					}
				}
			} 
			bfs_iput(ip);
		}
	}

	/*
	 * Set modify flag according to access mode
	 * so that modified buffers will be flushed.
	 */
	modify_flag = (rdwr == B_WRITE) ? 1 : 0;
	iob->i_bfs = (void *) bfs;

	return(0);
}

/*
 * static int
 * bfs_find(char *, struct iob *, ino_t *, char **)
 * 	Attempt to locate a file within a BFS filesystem.
 *
 * Calling/Exit State:
 *	"file" is a file descriptor which is being opened, for which
 *	the BFS has been validated, and in which information for
 *	referencing it and the underlying physical device has
 *	been stored already.
 *
 *	"path" is a string containing the full pathname portion
 *	of the file standalone file specifier.  Since BFS is
 *	a flat filesystem, subdirectory components within it are
 *	ignored and the address of the actual filename component
 *	within it is returned to the caller by storing it in
 *	the location addressed by "fname".
 *
 *	If the search for the named file within the BFS is
 *	successful, then the inode number for the file is
 *	stored within the location addressed by "pino" and
 *	the value zero is returned to the caller.  Otherwise,
 *	the value -1 is returned indicating failure, and the
 *	global variable "free_ldir" is set to an available inode 
 *	number in the event the caller then wishes to create 
 *	the file (saving a search for it later).
 *
 * Description:
 *	Skip to the last component of the pathname, which is the
 *	name of the file to look up in the given BFS root directory.	
 *	Then invoke getblk() to read the directory listing from
 *	the device into memory, so it can be searched for an entry
 *	with that filename, while noting the index of free entries.
 *	If a match is found, pass the inode value from the directory
 *	entry to bfs_iget(), which fetches it into core.  Save the
 *	inode number in *pino and the in-core inode's address in the
 *	file descriptor prior to returning to the caller.
 */
static int
bfs_find(char *path, struct iob *file, ino_t *pino, char **fname)
{
	struct buf *bp;
	daddr_t bn;
	struct bfs_ldirs *ld;
	struct bfs_info *bfs = file->i_bfs;
	char *c, *q;
	int n_ldirs, index;
	off_t byte_off;

        /* Take last component of the fname */
        for (c = q = path; *q != '\0'; ) {
                if (*q++ == '/')
                        c = q;
        }
	*fname = c;

	/*
	 * Search for the file name in the directory. 
	 * Upon success, set the inode number in pino
	 * and return zero.
	 * 
	 * If failure, return -1, but set free_ldir
	 * to the index of the first free entry in
	 * the ldir array to save repeating this work
	 * if a new file is subsequently created.
	 */
	bn = bfs->bsup_dirsblk + file->i_boff;
	n_ldirs = (bfs->bsup_eldirs + 1 - (bfs->bsup_dirsblk * BFS_BSIZE))
		/ sizeof (struct bfs_ldirs);

	free_ldir = -1;
	for (index = 0; index < n_ldirs; bn++) {
        	bp = getblk(makedev(file->i_dev, file->i_unit),
			bn, BFS_BSIZE, DOREAD);

		for (byte_off = 0, ld = (struct bfs_ldirs *)bp;
		     byte_off < BFS_BSIZE && index < n_ldirs;
		     byte_off += sizeof (struct bfs_ldirs), ld++, index++) {

			if (ld->l_ino == 0) {
				if (free_ldir < 0)
					free_ldir = index;
			} else if (strncmp(ld->l_name, c, BFS_MAXFNLEN) == 0) {
				*pino = ld->l_ino;
				brelse(bp, MRU);
				/*
				 * The inode has been located, 
				 * so let's retrieve it.
				 */
		   		file->i_ino = bfs_iget(
					makedev(file->i_dev, file->i_unit), 
					bfs, ld->l_ino);
				if (file->i_ino == NULL) {
					return (-1);
				}
				return (0);
			}
		}
		brelse(bp, MRU);
	}
	if (free_ldir < 0)
		free_ldir = n_ldirs;	/* This will expand the ldirs array */
	return (-1);
}

/*
 * static struct inode *
 * bfs_iget(dev_t, struct bfs_info *, ino_t)
 *	Look up and read in an inode by device, inumber.
 *
 * Calling/Exit State:
 *	The arguments describe the physical device upon which
 *	the BFS is located, in-core information about the
 *	BFS (such as its size and start address on the device),
 *	and the number of the BFS' inode to be read and returned
 *	to the caller.
 *
 *	Returns the address of the in-core inode if successful;
 *	NULL otherwise.
 *
 * Description:
 *	Allocate a structure from a predetermined list for reading
 *	the desired inode into.  Use the parameters to calculate the 
 *	the address of the block and offset within it of the inode
 *	data on the physical device.  Then have getblk() read that
 *	block from the device into core, extracting and saving its 
 *	relevant inode information prior to invoking brelse() to 
 *	release the buf back to the buffer cache.  Finally, return
 *	the address of the newly allocated/initialized inode to the
 *	caller.
 */
static struct inode *
bfs_iget(dev_t dev, struct bfs_info *bfs, ino_t	ino)
{
	struct inode *ip;
	struct buf *bp;
	daddr_t bn;
	struct bfs_dirent *dir;
	off_t byte_off;

	if (ino_list == NULL) {
		printf("PANIC: no free inodes\n");
		errno = EIO;
		return(NULL);
	}

	ip = ino_list;
	ino_list = ino_list->i_next;

	byte_off = BFS_INO2OFF(ino);
	bn = BYTE_TO_BLK(byte_off) + bfs->bsup_boff;
	bp = getblk(dev, bn, BFS_BSIZE, DOREAD);
	if ((bp->b_flags & B_ERROR) != 0) {
		printf("Inode read error\n");
		brelse(bp, LRU);
		ip->i_next = ino_list;
		ino_list = ip;
		errno = EIO;
		return(NULL);
	}
	/*
	 * Now locate the inode itself within the buffer just read.
	 * and save it within ip.
	 */
	dir = (struct bfs_dirent *) (void *)(bp->b_buf + EXC_BLK(byte_off));

	ip->i_devunit = dev;
	ip->i_number = ino;
	ip->i_flag = 0;
	ip->i_bfs = bfs;
	bcopy(dir, &ip->i_dir, sizeof (struct bfs_dirent));
	brelse(bp, MRU);
	return(ip);
}

/*
 * static void
 * bfs_iput(struct inode *)
 *	Release a previously allocated in-core BFS inode.
 *
 * Calling/Exit State:
 *	"ip" addresses the BFS in-core inode to be released.
 *	Its contents must be written back to disk if it has
 *	been updated since being read, which is indicated
 *	by its i_flags field.  This is done by calling
 *	bfs_iupdat().  Afterwards, link the inode structure
 *	onto a list of free inodes for new re-use by bfs_iget().
 *
 *	No return value.
 */
static void
bfs_iput(struct inode *ip)
{
	if (ip->i_flag & (IUPD|IACC|ICHG))
		bfs_iupdat(ip);

	ip->i_next = ino_list;
	ino_list = ip;
	return;
}

/*
 * static void
 * bfs_iupdat(struct inode *)
 *	Update an inode on-disk if its in-core copy has been modified.
 *
 * Calling/Exit State:
 *	"ip" addresses the BFS in-core inode to be checked.
 *	Write its contents back to disk if it has been updated 
 *	since being read, which is indicated by its i_flags 
 *	field.  Also use those flags to update its access,
 *	modification, and creation times accordingly.
 *
 *	No return value.
 *
 * Description:
 *	Check accessed and update flags on an inode structure.
 *	If any thing is changed, update the inode with the 
 *	current time and information prior to writing its 
 *	contents out to the disk.
 *
 *	The inode is written to disk by using information saved
 *	within the in-core inode about its physical device location
 *	to calculate its block address and offset within that block
 *	for the on the physical device.  Use that information to
 *	have getblk() read that block from the device into core, 
 *	update its relevant inode information, and then call
 *	bwrite() write the block back to disk and release its buf
 *	back to the buffer cache.
 */
static void
bfs_iupdat(struct inode *ip)
{
	daddr_t bn;
	struct buf *bp;
	struct bfs_dirent *dir;
	off_t byte_off;
	struct bfs_info *bfs = ip->i_bfs;


	if (ip->i_flag & (IUPD|IACC|ICHG)) {

#ifdef SET_TIME
		if (ip->i_flag & IUPD) {
			SET_TIME(&ip->i_dir.d_fattr.va_mtime);
		}
		if (ip->i_flag & IACC) {
			SET_TIME(&ip->i_dir.d_fattr.va_atime);
		}
		if (ip->i_flag & ICHG) {
			SET_TIME(&ip->i_dir.d_fattr.va_ctime);
		}
#endif /* SET_TIME */

		/*
		 * Read in the disk block that the inode
		 * is part of so it can be updated.
		 */
		bfs = ip->i_bfs;
		byte_off = BFS_INO2OFF(ip->i_number);
		bn = BYTE_TO_BLK(byte_off) + bfs->bsup_boff;
		bp = bread(ip->i_devunit, bn, BFS_BSIZE);
		if ((bp->b_flags & B_ERROR) != 0) {
			printf("Inode update read error\n");
			brelse(bp, LRU);
			return;
		}

		/*
		 * Now locate the inode itself within the buffer 
		 * just read, copy the updated version into it, 
		 * then write it out.
		 */
		dir = (struct bfs_dirent *) (void *)(bp + EXC_BLK(byte_off));
		bcopy(&ip->i_dir, dir, sizeof (struct bfs_dirent));
		ip->i_flag &= ~(IUPD|IACC|ICHG);
		bwrite(bp, MRU, W_NODELAY); 
		return;
	}
}

/*
 * static void
 * bfs_itrunc(struct inode *, ulong)
 *	Truncate a file to at most a specified size.
 *
 * Calling/Exit State:
 *	"ip" addresses an in-core BFS inode noting the current 
 *	characteristics of an existing. Its size is to be 
 *	truncated to, at most, the first "length" bytes.
 *	Released portions of the file become available for
 *	future filesystem allocation.
 *	
 *	No return value.
 *
 * Description:
 * 	If the file's size is not changing, then do nothing since
 * 	there is no space change and no need to update inode on-disk.  
 *	Otherwise, adjust its new data block allocation, and perform 
 *	bfs_info bookkeeping and compaction as necessary.
 */
static void
bfs_itrunc(struct inode *ip, ulong length)
{
	struct bfs_info *bfs;
	daddr_t oeblock;
	int filesize;
	struct inode *tip;
	ino_t ino;

	if (ip->i_dir.d_sblock == 0)
		filesize = 0;
	else
		filesize = ip->i_dir.d_eoffset + 1 -
			(ip->i_dir.d_sblock * BFS_BSIZE);
	if (filesize <= length)
		return;		/* No change - nothing to do */

	/* Otherwise, truncate the file...*/
	bfs = ip->i_bfs;
	oeblock = ip->i_dir.d_eblock;
	ip->i_flag |= IACC | IUPD | ICHG;
	ip->i_dir.d_eoffset = (ip->i_dir.d_sblock * BFS_BSIZE) + length - 1;
	ip->i_dir.d_eblock = ip->i_dir.d_eoffset / BFS_BSIZE;

	if (oeblock > ip->i_dir.d_eblock)
		bfs->bsup_freeblocks += oeblock - ip->i_dir.d_eblock;

	if (length == 0) {
		ip->i_dir.d_sblock = 0;
		ip->i_dir.d_eoffset = 0;
		ip->i_dir.d_eblock = 0;
		bfs_iupdat(ip);
		
		if (ip->i_number == bfs->bsup_lastfile) {
			if (oeblock != bfs->bsup_lasteblk)
				_stop("BFS endfile/endblock mismatch."); 
			/*
			 * Recalculate the last file and allocated 
			 * in the BFS by searching all the inodes.
			 */
        		bfs->bsup_lastfile = BFSROOTINO;
        		bfs->bsup_lasteblk = (bfs->bsup_start / BFS_BSIZE) - 1;
			for (ino = BFSROOTINO; ino < bfs->bsup_ndirs; ino++) {
				tip = bfs_iget(ip->i_devunit, bfs, ino);
				if (tip == NULL) 
					return;
				if (tip->i_dir.d_ino != 0
				&& (tip->i_dir.d_fattr.va_type == VREG
				    || tip->i_dir.d_fattr.va_type == VDIR)
				&& tip->i_dir.d_eblock > bfs->bsup_lasteblk) {
					bfs->bsup_lastfile = ino;
        				bfs->bsup_lasteblk = 
							tip->i_dir.d_eblock;
				}
			}
			bfs_iput(tip);
		} else if (filesize / BFS_BSIZE > BIGFILE) {
			/* 
			 * Perform compaction if we have freed
			 * up a large area adjacent to the last
			 * allocated file and it is small.  
			 * Otherwise, note that we could perform
			 * compaction in the future.
			 */
			tip = bfs_iget(ip->i_devunit, bfs, bfs->bsup_lastfile);
			if (tip == NULL) 
				return;
			if (tip->i_dir.d_sblock == oeblock + 1
			&&  tip->i_dir.d_eblock - tip->i_dir.d_sblock 
			    < SMALLFILE) {
				bfs_iput(tip);
				bfs_compact(bfs, ip);
			} else { 
				bfs_iput(tip);
			}
		}
	}
}

/* 
 * static int
 * bfs_creat(struct iob *, char *, mode_t)
 *	Create a new file in a BFS filesystem.
 *
 * Calling/Exit State:
 *	"file" is a descriptor for a file being opened.  It
 *	contains information about the physical location of
 *	the BFS and where its in-core superblock information
 *	has been stored.  Once the file is created, it is
 *	updated to address its new inode and inode number as
 *	well. 
 *
 *	"name" is the string name for the file which will
 *	be written to the disk in its newly allocated directory
 *	entry.
 *
 *	"mode" contains the BFS protection bits to set for the 
 *	newly created file.
 *
 *	Assume that bfs_find() had been called prior to this
 *	function and that it has left the index of an available
 *	directory entry in the global variable "free_ldir".
 *
 *	Returns zero if the file is successfully created. 
 *	Upon failure, set errno to an applicable error status and
 *	return -1.
 *
 * Description:
 *	Fail the request immediately if filename contains invalid 
 *	characters, its length exceeds the maximum for our BFS,
 *	or the BFS superblock has no allocatable directory entries
 *	for the new file.  
 *
 *	An earlier call to bfs_find() has already located an empty 
 *	directory entry for us to use, so we just need to locate an 
 *	unused inode to go with it and we are all set; read the inodes 
 *	with bfs_iget() until an unused one is located.  Then initialize
 *	it and its directory entry to reflect the new file's name and
 *	that it contains no data, writting both back to disk afterwards.
 *
 *	Lastly, update the superblock to reflect the allocation and,
 *	if neccessary, update the root inode if the newly allocated
 *	directory entry extended the in-use portion of its directory
 *	contents.
 */
static int
bfs_creat(struct iob *file, char *name, mode_t mode)
{
	struct inode *ip;
	struct bfs_info *bfs = file->i_bfs;
	char *cp;
	ino_t ino;
	int namelen;
	off_t byte_off;
	daddr_t bn;
	struct buf *bp;
	struct bfs_ldirs *ld;

	/*
	 * Check for valid name.
	 */
	for (cp = name, namelen = 0; *cp; namelen++, cp++) {
		if (*cp & 0x80) {
			errno = EACCES;
			return (-1);
		}
	}
	if (namelen > BFS_MAXFNLEN) {
		errno = EACCES;
		return (-1);
	}

	if (bfs->bsup_freedrents <= 0) {
		errno = ENOSPC;		/* Directory is full */
		return (-1);
	}

	/*
	 * Re-read the free_ldir, already found by, bfs_find()
	 * from the buf cache and locate an inode to go with it.
	 */
	for (ino = BFSROOTINO + 1; ino < bfs->bsup_ndirs; ino++) {
		ip = bfs_iget(makedev(file->i_dev, file->i_unit), bfs, ino);
		if (ip == NULL) {
			printf("inode table read failed.\n");
			return (-1);
		}
		if (ip->i_dir.d_ino == 0) {
			goto ino_found;
		}
		bfs_iput(ip);
	} 
	_stop("bfs_creat: Directory is unexpectedly full.");

ino_found:
	bfs->bsup_freedrents--;

	byte_off = (free_ldir * sizeof (struct bfs_ldirs))
		+ (bfs->bsup_dirsblk * BFS_BSIZE);
	bn = (byte_off / BFS_BSIZE) + file->i_boff;

	bp = bread(ip->i_devunit, bn, BFS_BSIZE);
	if (bp == NULL) {
		_stop("bfs_creat: cannot reread ldir array.");
	}
	ld = (struct bfs_ldirs *) (void *)(((char *)bp) + EXC_BLK(byte_off));

	/* 
	 * Initialize the the the newly allocated ldir and inode. 
	 */
	bzero(&ld->l_name[0], BFS_MAXFNLEN);
	bcopy(name, &ld->l_name[0], namelen);
	ld->l_ino = (ino_t)ino;
	bwrite(bp, LRU, W_NODELAY); 
	free_ldir = -1;

	ip->i_flag |= IUPD | IACC | ICHG;
	ip->i_dir.d_ino = (ino_t)ino;
	ip->i_dir.d_sblock = ip->i_dir.d_eblock = 0;
	ip->i_dir.d_eoffset = 0;
	ip->i_dir.d_fattr.va_type = VREG;
	ip->i_dir.d_fattr.va_mode = mode;
	ip->i_dir.d_fattr.va_uid = bfs->bsup_uid;
	ip->i_dir.d_fattr.va_gid = bfs->bsup_gid;
	ip->i_dir.d_fattr.va_nlink = 1;
	bfs_iupdat(ip);
	file->i_ino = ip;	/* Leave inode for the caller to use */

	byte_off += sizeof (struct bfs_ldirs) - 1;	/* end of the ldir */
	if (byte_off > bfs->bsup_eldirs) {
		/* 
		 * The root inode must be updated to
		 * reflect that the ldirs array has
		 * grown.
		 */
		ip = bfs_iget(ip->i_devunit, bfs, BFSROOTINO);
		if (ip == NULL) {
			printf("Root inode read failed.\n");
			return (-1);
		}
		ip->i_dir.d_eoffset = byte_off;
		ip->i_flag |= IACC | IUPD | ICHG;
		bfs_iput(ip);
		bfs->bsup_eldirs = byte_off;
	}
	return(0);
}

/*
 * static struct inode * 
 * bfs_cnxtinode(struct bfs_info *, daddr_t, struct inode *)
 *	Locate the inode for the first file in the BFS located
 *	after the specified BFS block.
 *
 * Calling/Exit State:
 *	Called during BFS compaction to determine which file
 *	to shift over free BFS space and how far.  Such a
 *	file may not exist if "curblock" is the start of the
 *	end chunk of free filesystem blocks.
 *
 *	"bfs" addresses the applicable BFS's in-core superblock.
 *
 *	"sip" addresses an inode already owned by the caller,
 *	which must not be re-read or released by this function.
 *
 *	Returns the address of the closes file's inode, if one
 *	exits.  Otherwise returns NULL.
 *
 * Description:
 * 	Loop through all of the inodes of the specified BFS until 
 *	the inode with a start block greater than and nearest to
 *	"curblock" is found.  Then return the address of its in-core
 *	inode to the caller.  Don't attempt to reaquire or release the 
 *	inode addressed by "sip", as would be done with others.  Just
 *	use it when appropriate.
 */
static struct inode * 
bfs_cnxtinode(struct bfs_info *bfs, daddr_t curblock, struct inode *sip)
{
	struct inode *nip;
	int ino, closest_ino = -1;
	off_t loc = bfs->bsup_lasteblk;

	for (ino = BFSROOTINO; ino < bfs->bsup_ndirs; ino++) {
		if (ino == sip->i_number) {
			nip = sip;
		} else if ((nip = bfs_iget(sip->i_devunit, bfs, ino)) == NULL) {
			_stop("bfs_cnxtinode: inode table read failed.");
		}

		if (nip->i_dir.d_ino != 0
		&& nip->i_dir.d_sblock >= curblock && nip->i_dir.d_sblock < loc
		&& (nip->i_dir.d_fattr.va_type == VREG
		    || nip->i_dir.d_fattr.va_type == VDIR)) {
			closest_ino = ino;
			loc = nip->i_dir.d_sblock;
		}
		if (nip != sip)
			bfs_iput(nip);
	}
	if (closest_ino == sip->i_number)
		return (sip);
	else if (closest_ino == -1)
		return (NULL);
	else
		return (bfs_iget(sip->i_devunit, bfs, closest_ino));
}

/*
 * static void
 * bfs_shiftfile(struct bfs_info *, struct inode *, long)
 * 	Relocate the file described by "ip", "gapsize" blocks forward in
 *	the BFS filesystem.
 *
 * Calling/Exit State:
 *	"bfs" addresses the applicable BFS's in-core superblock.
 *	Presumably it is undergoing compaction when this function 
 *	is called.
 *
 *	"ip" addresses a file's inode, the data for which needs
 *	to be copied "gapsize" blocks forward in the BFS prior to
 *	updating the this inode to reflect its new data location.
 *
 *	No return value.
 *
 * Remarks:
 * 	Move one block of the file at a time to its new location.
 * 	Use two bufs and a bcopy() due to the way the buf cache 
 *	works.  There are faster methods, but since writes in 
 *	standalone should be rare, this allows the standalone code 
 *	to remain simpler.
 *
 *	If gapsize is less than file size, we must write "-1" to the 
 *	first 2 sanity words of the BFS superblock and the "to"
 *	and "from" block addresses to the other sanity words first 
 *	(updating them as we go) to let fsck know where compaction 
 *	was in the event of failure.  Then update the inode to 
 *	reflect its new data location.  Upon completion, Write write 
 *	"-1" to all 4 sanity words for fsck to denote that compaction 
 *	is no longer in progress.  
 *
 *	The sanity word updates are not needed when gapsize is larger
 *	than the file being moved, since the move itself will not
 *	corrupt the original copy.
 */
static void
bfs_shiftfile(struct bfs_info *bfs, struct inode *ip, long gapsize)
{
	long filebsize = ip->i_dir.d_eblock - ip->i_dir.d_sblock + 1;
	struct buf *fbp, *tbp, *sb_bp;
	daddr_t fbn, tbn;

	fbn = ip->i_dir.d_sblock;
	tbn = ip->i_dir.d_sblock - gapsize;
	if (gapsize < filebsize) {
		/* Get a copy of the superblock for modification */
		sb_bp = bread(ip->i_devunit, 
				BFS_SUPEROFF+bfs->bsup_boff, BFS_BSIZE);
		if ((sb_bp->b_flags & B_ERROR) != 0)
			_stop("bfs_shiftfile: super block read error");
		((struct bdsuper *)(void *)sb_bp->b_buf)->bdcp_fromblock = fbn;
		((struct bdsuper *)(void *)sb_bp->b_buf)->bdcp_toblock = tbn;
	}

	do {
		/*
		 * If gapsize is less than file size, must write words for fsck
		 * to denote that compaction is in progress (i.e which blocks
		 * are being shifted.)
		 * Otherwise, there is no need to write sanity words. If the
		 * system crashes during compaction, fsck can take it from 
		 * the top without data lost. 
		 */
		if (gapsize < filebsize) {
			((struct bdsuper *)(void *)sb_bp->b_buf)->bdcpb_fromblock = fbn;
			((struct bdsuper *)(void *)sb_bp->b_buf)->bdcpb_toblock = tbn;
			bwrite(sb_bp, DONTFREE, W_NODELAY);
		}
		/*
		 * Move a block of the file to its new location.
		 * Use two bufs and a bcopy() due to the way the 
		 * buf cache works.
		 */
		fbp = bread(ip->i_devunit, fbn + bfs->bsup_boff, 
			BFS_BSIZE);
		tbp = getblk(ip->i_devunit, tbn + bfs->bsup_boff, 
			BFS_BSIZE, DONOTHING);
		if (fbp == NULL || tbp == NULL)
			_stop("bfs_shiftfile: I/O failed during compaction.");
		bcopy((char *)fbp, (char *)tbp, BFS_BSIZE);
		bwrite(tbp, LRU, W_NODELAY);
		brelse(fbp, LRU);
		/*
		 * If gapsize is less than file size, must write 
		 * "-1" to the first 2 sanity words to let fsck 
		 * know where compaction was.
		 */
		if (gapsize < filebsize) {
			((struct bdsuper *)(void *)sb_bp->b_buf)->bdcp_fromblock = -1;
			((struct bdsuper *)(void *)sb_bp->b_buf)->bdcp_toblock = -1;
			bwrite(sb_bp, DONTFREE, W_NODELAY);
		}
		fbn++;
		tbn++;
	} while (fbn != ip->i_dir.d_eblock + 1);

	/*
	 * Update the inode to reflect the new data location.
	 */
	ip->i_dir.d_sblock -= gapsize;
	ip->i_dir.d_eblock -= gapsize;
	ip->i_dir.d_eoffset -= gapsize * BFS_BSIZE;
	ip->i_flag |= IACC | ICHG;
	if (ip->i_number == bfs->bsup_lastfile)
        	bfs->bsup_lasteblk = ip->i_dir.d_eoffset;
	bfs_iupdat(ip);

	/*
	 * Must write "-1" to all 4 sanity words for fsck to denote that
	 * compaction is not in progress.  Note that the primary sanity
	 * words should already be -1, so just do the backups now.
	 */
	if (gapsize < filebsize) {
		((struct bdsuper *)(void *)sb_bp->b_buf)->bdcpb_fromblock = -1;
		((struct bdsuper *)(void *)sb_bp->b_buf)->bdcpb_toblock = -1;
		bwrite(sb_bp, MRU, W_NODELAY); /* Releases the superblock */
	}
}

/*
 * static void
 * bfs_compact(struct bfs_info *, struct inode *)
 *	Perform compaction of the specified BFS filesystem.
 *
 * Calling/Exit State:
 *	"bfs" addresses in-core superblock information about
 *	the filesystem being compacted.  Its data for the
 *	last-allocated-block must be updated during compaction
 *	and written to the superblock on the disk.
 *
 *	"sip" addresses an in-core inode which the caller already
 *	owns.  This algorithm and functions it calls must not
 *	deallocate it (with bfs_iput()), since the caller is 
 *	already dependent upon its current location.  However,
 *	if compaction relocates its data then it must be updated 
 * 	both in-core and on-disk (using bfs_iupdat()) since the
 *	caller may continue using it afterwards.
 *
 *	No return value, but upon completion all the free blocks 
 *	in the specified BFS are located sequentially at starting 
 *	at bfs->bsup_lasteblock + 1.
 *
 * Description:
 *	Call bfs_cnxtinode() to fetch the inode of the file
 *	closest to the start of the filesystem data area.
 *	Then invoke bfs_shiftfile() to compress unallocated
 *	space out from between it and the start block, if
 *	necessary.  Adjust the location of the gap-start-block
 *	and repeat the algorithm until no more such files/inodes 
 *	can be located, in which case the compaction is complete.  
 */
static void
bfs_compact(struct bfs_info *bfs, struct inode *sip)
{
	struct inode *nip;
	daddr_t nblock;
	long gapsize;

	/*
	 * Fetch the inode of the file closest to
	 * the start of the filesystem data area.
	 */
	nblock = bfs->bsup_start / BFS_BSIZE;
	nip = bfs_cnxtinode(bfs, nblock, sip);
	do {
		/*
		 * If file does not start at nblock compress
		 * the gap between them out by shifting this 
		 * file over and updating its inode.
		 */
		if ((gapsize = nip->i_dir.d_sblock - nblock) > 0) {
			bfs_shiftfile(bfs, nip, gapsize);
		}
		nblock = nip->i_dir.d_eblock + 1;
		if (nip != sip) 
			bfs_iput(nip);		/* Release this inode */
		nip = bfs_cnxtinode(bfs, nblock, sip);/* Get next file */
	} while (nip != NULL);
}

/*
 * static int
 * bfs_filetoend(struct inode *)
 *	Relocate the specified file to the end of filesystems allocated space.
 *
 * Calling/Exit State:
 *	"ip" addresses an in-core copy of the file's inode which
 *	also addresses information about the BFS and physical
 *	device upon which it is located. 
 *
 * 	There is no guarantee that there is enough room left in 
 * 	the filesystem to support the copy, in which case it fails
 * 	and returns -1 if this is still the case after then compacting
 *	the filesystem.  If the copy is successful, then the inode is 
 *	updated to reflect the file's new location, and the BFS's in-core
 *	superblock must be adjusted to reflect its new last-allocated-
 *	data-block location for future allocations, and zero is returned.
 *
 */
static int
bfs_filetoend(struct inode *ip)
{
	struct bfs_info *bfs = ip->i_bfs;
	int nbytes, nblocks, count;
	daddr_t from_bn, to_bn;
	struct buf *nbp, *obp;

	if (bfs->bsup_lastfile == ip->i_number)
		return (0);	/* Already the last file */

	if (ip->i_dir.d_sblock == 0)
		return (0);	/* There is no data to move yet */

	/*
 	 * Determine if there is enough space left
	 * to perform the move.  Compact the BFS
	 * if necessary.
	 */
	nbytes = ip->i_dir.d_eoffset - (ip->i_dir.d_sblock * BFS_BSIZE) + 1;
	nblocks = (ip->i_dir.d_eblock - ip->i_dir.d_sblock) + 1;
        if (nblocks > bfs->bsup_freeblocks) 
		return (-1);		/* Not enough blocks left in BFS */

	if (nblocks > (bfs->bsup_end / BFS_BSIZE) - bfs->bsup_lasteblk) {
		bfs_compact(bfs, ip);	/* Make more room at the end */
		if (nblocks > (bfs->bsup_end / BFS_BSIZE) - bfs->bsup_lasteblk)
			_stop("BFS free block count incorrect.");
	}

	/*
	 * Copy the file to its new location.
	 * We must use two bufs and a bcopy() 
	 * due to the way the buf cache works.
	 */
	from_bn = ip->i_dir.d_sblock + bfs->bsup_boff;
	to_bn = bfs->bsup_lasteblk + 1 + bfs->bsup_boff;
	for (count = 0; count < nblocks; count++, from_bn++, to_bn++) {
		obp = bread(ip->i_devunit, from_bn, BFS_BSIZE);
		if (obp == NULL)
			return (-1);	/* Read failed */
		nbp = getblk(ip->i_devunit, to_bn, BFS_BSIZE, DONOTHING);
		if (nbp == NULL) {
			brelse(obp, MRU);
			return (-1);	/* Read failed */
		}
		bcopy((char *)obp, (char *)nbp, BFS_BSIZE);
		bwrite(nbp, LRU, W_NODELAY);
		brelse(obp, LRU);
	}

	/*
	 * Update the inode to reflect the new location.
	 */
	ip->i_dir.d_sblock = bfs->bsup_lasteblk + 1;
	ip->i_dir.d_eblock = bfs->bsup_lasteblk + nblocks;
	ip->i_dir.d_eoffset = (ip->i_dir.d_sblock * BFS_BSIZE) + nbytes - 1;
	ip->i_flag |= IACC | ICHG;

	/*
	 * Update the BFS statistics to reflect the new
	 * allocation.
	 */
	bfs->bsup_lastfile = ip->i_number;
        bfs->bsup_lasteblk += nblocks;
	return (0);		/* Success */
}

/*
 * static struct buf *
 * bfs_expand(struct inode *, int, daddr_t, int)
 *	Expand the data area for the associated file/inode. 
 *
 * Calling/Exit State:
 *	"ip" addresses an in-core copy of the file's inode which
 *	also addresses information about the BFS and physical
 *	device upon which it is located.  It is updated if
 *	new data blocks are allocated to the file.
 *
 *	The other arguments are used to determine the difference
 *	between the files oldsize and desired expanded size.
 *	The in-core superblock for the BFS must also be updated
 *	if additional blocks are allocated to the file or the
 *	filesystem is compacted to allocate those blocks.
 *	
 * 	Return the final block's buf instead of writing it
 * 	out to the device; assume the caller will write it.  
 *	Upon failure, return NULL.
 *
 * Description:
 *	Determine the number of sequential blocks needed for 
 *	the file and fail the request if enough do not exist.
 *	Then relocate the file's data to the end of the allocated
 *	BFS for expansion, if its not already there.  Compress the
 *	BFS if there is room in the filesystem, but not enough at the
 *	end where we are trying to expand.  Finally, allocate any
 *	new blocks required for the file expansion, zeroing out all
 *	the as yet unused portions of the file, and update the
 *	inode to reflect the file's new size and location.
 */
static struct buf *
bfs_expand(struct inode *ip, int old_bsize, daddr_t lbn, int nbytes)
{
        struct bfs_info *bfs = ip->i_bfs;
        struct buf *bp;
        daddr_t bn, lastblk;
	int new_blks;
	off_t offset;

	/* 
	 * Determine how many more sequential blocks are 
	 * needed for the file and return failure if
	 * their are not enough left within the filesystem.
	 */
	new_blks = lbn + 1 - old_bsize;		/* How many more blks needed? */
	if (new_blks > bfs->bsup_freeblocks)
		return (NULL);		/* Not enough room within filesystem */

	/* 
	 * Relocate file to physical end of allocated 
	 * data blocks if it is not already there.
	 */
	if (bfs_filetoend(ip) != 0) 
		return (NULL);		/* Likely not enough room to move it */

	/*
	 * Compress the allocated filesystem if there
	 * is not enough room at the end of it for
	 * its expansion.  We already know there are
	 * enough free blocks in the filesystem; we
	 * just need to squeeze them out.
	 */
	if (new_blks > (bfs->bsup_end / BFS_BSIZE) - bfs->bsup_lasteblk) {
		bfs_compact(bfs, ip);
		if (new_blks > (bfs->bsup_end / BFS_BSIZE) - bfs->bsup_lasteblk)
			_stop("BFS free block count incorrect.");
	}

	/* 
 	 * If there were any blocks in the former
	 * file and part of the last block was 
	 * not in-use, then zero it out now.
	 */
	if (old_bsize > 0) {
		offset = ip->i_dir.d_eoffset % BFS_BSIZE;
		bn = ip->i_dir.d_eblock + bfs->bsup_boff;
		if (offset != BFS_BSIZE - 1) {
			bp = bread(ip->i_devunit, bn, BFS_BSIZE);
			if (bp == NULL) 
				return (NULL);	/* bread() failed */
			bzero((char *)bp + offset + 1, BFS_BSIZE - 1 - offset);
			bwrite(bp, LRU, W_NODELAY); 
		}
	} else {
		/* Note: the loop below will incr bn before starting */
		bn = bfs->bsup_lasteblk + bfs->bsup_boff;
	}

	/*
	 * Next allocate the remaining blocks, 
	 * zeroing them as well.  Hang onto
	 * the final block and return it to
	 * the caller after updating the inode.
	 */
	for (lastblk = old_bsize, bn++;  ;lastblk++, bn++) {
		bp = getblk(ip->i_devunit, bn, BFS_BSIZE, ZERO);
		if (bp == NULL) 
			return (NULL);	/* getblk() failed */
		if (lastblk == lbn)
			break;		/* Return the last block */
		bwrite(bp, LRU, W_NODELAY);
	}  

	/* Update the inode to reflect the allocation */
	if (ip->i_dir.d_sblock == 0) 
		ip->i_dir.d_sblock = bfs->bsup_lasteblk + 1;
	ip->i_dir.d_eblock = ip->i_dir.d_sblock + lbn;
	ip->i_dir.d_eoffset = (ip->i_dir.d_eblock * BFS_BSIZE) + nbytes - 1;
	ip->i_flag |= IACC | IUPD | ICHG;

	if (old_bsize == 0) {
		/* This is now the last physically allocated file */
		bfs->bsup_lastfile = ip->i_number;
	} else if (bfs->bsup_lastfile != ip->i_number) {
		_stop("BFS can only expand the last file.");
	}
	bfs->bsup_lasteblk = ip->i_dir.d_eblock;
	bfs->bsup_freeblocks -= new_blks;
	return (bp);
}

/*
 * static struct buf *
 * bfs_bmap(struct inode *, daddr_t, int, int) 
 *	Return a buf for writing/re-writing the specified 
 *	logical data bytes of a file.
 *
 * Calling/Exit State:
 *	"ip" addresses an in-core copy of the file's inode which
 *	also addresses information about the BFS and physical
 *	device upon which it is located.  It is updated if
 *	new data blocks are allocated to the file.
 *
 *	"lbn" is the logical block of the file in which the
 *	caller intends to read or write "nbytes", starting
 *	at byte position "offset" within it.  Assume that 
 *	0 < nbytes <= BFS_BSIZE and that file expansion may be 
 *	necessary to complete this request.
 *
 *	Returns a buf allocated by getblk() containing a copy of
 *	data from the file currently stored in that block, if
 *	successful.  Returns NULL if it fails for any reason.
 *
 * Remarks:
 *	Invoked during write operations to ensure that the specified 
 *	number of bytes of the logical bfs block of the given inode 
 *	have been allocated for it.  If it can accommodate requested
 *	expansion using the last data block assigned to the file, but
 *	only partially used, then it needs only to adjust the end-byte
 *	offset of the inode, then request a buf for the block from the
 *	buf-cache.  Otherwise, it invokes bfs_expand() to facilitate the 
 *	expansion and return a buf.
 */ 
static struct buf *
bfs_bmap(struct inode *ip, daddr_t lbn, int offset, int nbytes)
{
        struct buf *bp;
        int oblocks;
        daddr_t bn;
	off_t oend;

	/* 
	 * Make certain enough blocks have been
	 * allocated to the inode.
	 */
	if (ip->i_dir.d_sblock == 0) {
		/* Empty inode */
		oblocks = 0;
	} else {
		oblocks = ip->i_dir.d_eblock - ip->i_dir.d_sblock + 1;
	}

	if (lbn >= oblocks)
		return (bfs_expand(ip, oblocks, lbn, offset + nbytes));
	bn = ((struct bfs_info *)ip->i_bfs)->bsup_boff + 
		ip->i_dir.d_sblock + lbn;
	if (nbytes == BFS_BSIZE) {
		/* 
		 * Since the entire block is to be
		 * written just fetch a buf for it.
		 */
		bp = getblk(ip->i_devunit, bn, BFS_BSIZE, DONOTHING);
	} else {
		/* 
		 * Since the block is already part of the
		 * file, just retrieve it.
		 */
		bp = bread(ip->i_devunit, bn, BFS_BSIZE);
	}
	if (bp == NULL) 
		return (NULL);	/* getblk() or bread() failed */

	/* 
	 * For file expansion within the current block,
 	 * note the new end-offset in the inode and
	 * ensure that any new holes are zeroed.
	 */
	oend = ip->i_dir.d_eoffset % BFS_BSIZE;
	ip->i_dir.d_eoffset += (offset + nbytes - 1) - oend;
	if (offset > oend + 1)
		bzero(((char *)bp) + oend + 1, offset - (oend +1));
	return (bp);
}

/*
 * static int
 * bfs_rwip(struct iob *, char *, int, int)
 *	Transfer data between a BFS file and a memory buffer.
 *
 * Calling/Exit State:
 *	"file" is an open file descriptor for a BFS file.  It
 *	contains state information regarding the location and
 *	current access position of the file it is associated with.
 *	Its state is updated as a result of the transfer performed.
 *
 *	"ubuf" is the address of a user's data buffer to or from
 *	which "count" bytes of data are to be transfered.  If "rw" 
 *	is READ data is transfered from the file to ubuf.  Otherwise
 *	it is transferred from ubuf to the file.
 *
 *	Assume the caller will not try to read past the end of the
 *	specified file.
 *
 *	If the transfer succeeds, it returns a positive value
 *	indicating the number of bytes transferred.  In case of
 *	a failure, -1 is returned an errno is set to a value
 *	indicative of the reason for the failure.
 *
 * Description:
 *	For READ requests, determine the current size of the file
 *	and the starting location of transfer within that file.
 *	Then invoke bread() to fetch the first filesystem block
 *	involved in the transfer.  Release the buf returned and
 *	return failure to the caller if the attempted bread failed.
 *	Otherwise, copy the available portion of the requested
 *	transfer from the returned buf to ubuf, adjusting the
 *	remaining transfer count and file position accordingly.
 *	Then release the buf with brelse() and repeat this
 *	sequence until the request either enough data has been
 *	read and copied to ubuf, or the transfer fails.
 *
 *	WRITE requests are treated similarly, except that the
 *	direction of the data movement is from the buffer to
 *	the file and the file may need to be expanded to 
 *	accommodate data being appended to the file.  In this
 *	case, bfs_bmap() is invoked for each filesystem block
 *	required to ensure that it has been allocated to the
 *	file, expanding the file as needed, and returning
 *	a buf containing the current contents of that block for
 *	modification prior to being written back to the device.
 *	Also, blocks are written and released by calling bwrite(),
 *	which may cache the request until the file is closed or
 *	the buffer is needed to read/write another block.
 *	Finally, if an attempted WRITE errors out, then the
 *	file is truncated back to its former size prior to
 *	returning to the caller.
 */
static int
bfs_rwip(struct iob *file, char *ubuf, int count, int rw)
{
	struct inode *ip;
	struct buf *bp;
	int n, on, starting_count, keepflag, filesize;
	daddr_t lbn, bn;
	long osize;

	ip = file->i_ino;
	if (ip->i_dir.d_sblock == 0)
		filesize = 0;
	else
		filesize = ip->i_dir.d_eoffset + 1 -
			(ip->i_dir.d_sblock * BFS_BSIZE);

	starting_count = count;
	errno = 0;		/* just in case */

	do {
		lbn = file->i_offset / BFS_BSIZE;
		on = file->i_offset % BFS_BSIZE;
		n = MIN((unsigned)(BFS_BSIZE - on), count);


		if (rw == READ) {
			bn = file->i_boff + ip->i_dir.d_sblock + lbn;
			bp = bread(ip->i_devunit, bn, BFS_BSIZE);
		} else {
			osize = filesize;   /* Since bfs_bmap may change it */
			bp = bfs_bmap(ip, lbn, on, n);
			if (bp == NULL) {
				if (starting_count != count) {
					errno = 0;
					return(starting_count - count);
				} else
					return(-1);
			}
			filesize = ip->i_dir.d_eoffset + 1 - 
				(ip->i_dir.d_sblock * BFS_BSIZE);
		} 

		if (bp->b_flags & B_ERROR) {
			brelse(bp, LRU);
			if (errno && rw == WRITE)
				bfs_itrunc(ip, osize);
			return (-1);
		}
		count -= n;
		file->i_offset += n;
		if (n + on == BFS_BSIZE)
			keepflag = LRU;
		else
			keepflag = MRU;

		if (rw != READ) {
			bcopy(ubuf, (caddr_t)bp+on, n);
			bwrite(bp, keepflag, W_DELAY);
			ip->i_flag |= IACC | IUPD;
		} else {
			bcopy((caddr_t)bp+on, ubuf, n);
			if (file->i_offset == filesize)
				keepflag = LRU;
			brelse(bp, keepflag);
		}
		ubuf += n;
	} while (errno == 0 && count > 0);
	
	if (errno) {
		if (rw == WRITE)
			bfs_itrunc(ip, osize);
		return(-1);
	}
	return(starting_count - count);
}
