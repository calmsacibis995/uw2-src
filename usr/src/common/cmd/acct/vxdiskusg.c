/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)acct:common/cmd/acct/vxdiskusg.c	1.8"
#ident "$Header: $"

/*
 * Copyright (c) 1991, 1992 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 * 
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 * 
 *               RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *               VERITAS SOFTWARE
 * 4800 GREAT AMERICA PARKWAY, SUITE 420, SANTA CLARA, CA 95054
 */

/*
 * Portions Copyright 1992, 1991 UNIX System Laboratories, Inc.
 * Portions Copyright 1990 - 1984 AT&T
 * All Rights Reserved
 */

/*
 * The diskusg program for FSType vxfs.  This allows running against a
 * vxfs snapshot file system - though the script that usually drives
 * diskusg (dodisk) should have no call to do so.
 */

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <macros.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/param.h>
#include <sys/mnttab.h>
#include <sys/vnode.h>
#include <sys/fs/vx_inode.h>
#include <sys/fs/vx_fs.h>
#include <sys/fs/vx_param.h>
#include <sys/fs/vx_ioctl.h>
#include "acctdef.h"
#include <stdarg.h>
#ifdef VXI18N
#include <locale.h>
#include <pfmt.h>
#endif /*VXI18N*/

#ifndef max
#define max(a,b)	((int)(a) > (int)(b) ? (a) : (b))
#endif  /* max */
#define IBUFSIZE 32768

/*
 * a bit pattern for fs_open() and fs_cksblock() to control
 * what fields are actually validated in the superblock.
 */

#define	FS_CKMAGIC	0x01
#define	FS_CKVERSION	0x02
#define	FS_CKSUM	0x04
#define	FS_NOSNAPERR	0x10

/*
 * holds all the information about an open file system.  
 * if the file system is a snapshot, fs_mntpt and fs_mntfd
 * are also filled in
 */

struct fs_dat {
	struct vx_fs	fsd_fs;		/* superblock data */
	char		*fsd_spec;	/* special file */
	int		fsd_fd;		/* file descriptor for special */
	off_t		fsd_off;	/* current offset */
	int		fsd_issnapshot;	/* true if snapshot */
	char 		*fsd_mntpt;	/* mount point if snapshot */
	int		fsd_mntfd;	/* fd for mount point if snapshot */
	int		fsd_nindex;	/* number of filesets aggregate */
	ulong		*fsd_index;	/* table of indices of filesets */
};

struct fs_dat	*fs_datalloc();
void	fs_datfree(struct fs_dat *);
char	*fs_open(struct fs_dat *, char *, int, int);
int	fs_close(struct fs_dat *);
off_t	fs_lseek(struct fs_dat *, off_t, int);
int	fs_read(struct fs_dat *, void *, unsigned);
int	fs_write(struct fs_dat *, void *, unsigned);
int	fs_readi(struct fs_dat *, struct vx_dinode *, off_t,
			 void *, int);
char	*fs_cksblock(struct vx_fs *, int);
char	*fs_findbspec(char *);
char	*fs_convto_cspec(char *);
char	*fs_convto_bspec(char *);
int	fs_ckspecmnt(char *, char *);
int	fs_ckspecs(char *, char *);
int	fs_bmap(struct fs_dat *, ino_t, ulong, struct vx_dinode *,
		        off_t, off_t *, struct vx_extent *);

/* other VERSION2 layout extentions */

struct fset_dat {
	struct vx_fsethead	fst_head;	/* fileset header */
	struct vx_dinode	fst_ilino;	/* fileset ilist inode */
	struct vx_cuent		fst_cuent;	/* current usage table entry */
};

struct fset_dat	*fset_datalloc();
void 		fset_datfree(struct fset_dat *);
char	 	*fset_open(struct fs_dat *, struct fset_dat *, ulong);
int		fset_close(struct fset_dat *);
char		*fset_iread(struct fs_dat *, struct fset_dat *, ino_t,
				   void *, ulong, int *);

ino_t	ino;			/* used by ilist() and count() */
long	lseek();
int	VERBOSE = 0;
FILE	*ufd = 0;	/* file ptr for file where unacct'd for fileusg goes */
char	*ignlist[MAXIGN];	/* ignore list of filesystem names */
int	igncnt = {0};
char	*cmd;
unsigned hash();
void	process();
void	ilist();
struct	vx_fs *fs;
struct	fs_dat *fs_datp;
struct	fset_dat *fset_datp;

struct acct  {
	uid_t	uid;
	long	usage;
	char	name [MAXNAME+1];
} userlist [MAXUSERS];

main(argc, argv)
int argc;
char **argv;
{
	extern	int	optind;
	extern	char	*optarg;
	register c;
	register FILE	*fd;
	int	sflg = {FALSE};
	char 	*pfile = NULL;
	int	errfl = {FALSE};
	int	i; 
	size_t	len;
	char 	*errmsg;

	cmd = argv[0];
	while((c = getopt(argc, argv, "vu:p:si:")) != EOF) switch(c) {
	case 's':
		sflg = TRUE;
		break;
	case 'v':
		VERBOSE = 1;
		break;
	case 'i':
		ignore(optarg);
		break;
	case 'u':
		ufd = fopen(optarg, "a");
		break;
	case 'p':
		pfile = optarg;
		break;
	case '?':
		errfl++;
		break;
	}
	if(errfl) {
		fprintf(stderr, "Usage: %s [-sv] [-p pw_file] [-u file] [-i ignlist] [file ...]\n", cmd);
		exit(10);
	}

	hashinit();
	if(sflg == TRUE) {
		if(optind == argc){
			adduser(stdin);
		} else {
			for( ; optind < argc; optind++) {
				if((fd = fopen(argv[optind], "r")) == NULL) {
					fprintf(stderr, "%s: Cannot open %s\n", cmd, argv[optind]);
					continue;
				}
				adduser(fd);
				fclose(fd);
			}
		}
	} else {
		setup(pfile);
		for( ; optind < argc; optind++) {
			if ((fs_datp = fs_datalloc()) == NULL) {
				fprintf(stderr, 
					"%s: Cannot allocate space for super block.\n", 
					cmd);
				exit(15);
			}
			if ((fset_datp = fset_datalloc()) == NULL) {
				fprintf(stderr,
					"%s: Cannot allocate space for fset.\n",					cmd);
				exit(15);
			}

			process(argv[optind]);
			
		}
	}
	output();
	exit(0);
	/*NOTREACHED*/
}

/*
 * Run through each of the filesets of the aggregate. In the case of
 * a version 1 layout there is only one "fileset" of interest, which 
 * the libreary presents to us as the one and only index in the list
 * of fileset indexes.
 */

void
process(spec)
char	*spec;
{
	char	*errmsg;
	int	i, len, fsind = 0;
	struct	stat statb;

	sync();
	
	if (stat(spec, &statb) < 0) {
		fprintf(stderr, "%s : %s - ", cmd, spec);
		perror(" ");
		return;
	}

	if ((statb.st_mode & S_IFMT) != S_IFBLK &&
	    (statb.st_mode & S_IFMT) != S_IFCHR) {
		fprintf(stderr,"%s is neither block nor character special\n",
			spec);
		return;
	}

	errmsg = fs_open(fs_datp, spec, O_RDONLY,
			FS_CKMAGIC | FS_CKVERSION | FS_CKSUM);
	if (errmsg) {
		fprintf(stderr," %s %s \n", cmd, errmsg);
		return;
	}

	fs = &fs_datp->fsd_fs;
	for (i = 0; i < igncnt; i++) {
		len = max(strlen(fs->fs_fname), strlen(ignlist[i]));
		if (strncmp(fs->fs_fname, ignlist[i], len) == 0) {
			return;
		}
	}
	
	if (fs->fs_version == VX_VERSION2) {
		/*
		 * In the version 2 case, the list of fileset indexes
		 * gives us the attribute fileset as the first entry.
		 */ 
		fsind = 1;
	}

	for (; fsind < fs_datp->fsd_nindex; fsind++) {
		if ((errmsg = fset_open(fs_datp, fset_datp,
				fs_datp->fsd_index[fsind])) != NULL) {
			fprintf(stderr,"%s %s : couldn't open fileset : %s\n", cmd, spec, errmsg);
			return;
		}	
		ilist(spec, fsind);
	}
	return;
}

void
ilist(fsname, fsind)
char	*fsname;
int	fsind;
{
	int	i, relino, mode, dinosize, bufino = 0;
	ulong	ino = 0;
	char	ibuf[IBUFSIZE];
	daddr_t	bno;
	struct	vx_dinode *dp;
	char	*errmsg;

	if (fs_datp->fsd_fs.fs_version == VX_VERSION) {
		dinosize = fs_datp->fsd_fs.fs_bsize / fs_datp->fsd_fs.fs_inopb;
	} else {
		dinosize = fs_datp->fsd_fs.fs_dinosize;
	}

	memset(ibuf, '\0', IBUFSIZE);
	while(((errmsg = fset_iread(fs_datp, fset_datp,
			ino, ibuf, IBUFSIZE, &bufino)) == NULL) &&
		bufino > 0) {
		if (errmsg) { 
			fprintf(stderr,"%s\n", errmsg);
			return;
		}
		relino = 0;

		for (dp = (struct vx_dinode *)ibuf;
		     dp < (struct vx_dinode *)(ibuf + IBUFSIZE) &&
		     	relino < bufino; 
		     dp = (struct vx_dinode *)((char *)dp + dinosize), 
			ino++, relino++) {

			if ((mode = (dp->di_mode & IFMT)) == 0)
				continue;
			if (count(dp) == FAIL) {
				if(VERBOSE) {
					fprintf(stderr,"BAD UID: file system = %s, inode = %u, uid = %ld\n", 
						fsname, ino, dp->di_uid);
				}
				if(ufd) {
					fprintf(ufd, "%s %u %ld\n", 
						fsname, ino, dp->di_uid);
				}
			} 
		}
	}
	return;
}

			
count(ip)
struct vx_dinode *ip;
{
	int index;
	static int	secperblk = 0;

	if (!secperblk)
		secperblk = fs->fs_bsize / DEV_BSIZE;

	if (ip->di_nlink == 0 || ip->di_mode == 0)
		return(SUCCEED);
	if((index = hash(ip->di_uid)) == FAIL || 
		userlist[index].uid == UNUSED)
		return (FAIL);
	userlist[index].usage += ip->di_blocks * secperblk;
	return (SUCCEED);
}


adduser(fd)
register FILE	*fd;
{
	uid_t	usrid;
	long	blcks;
	char	login[MAXNAME+10];
	int 	index;

	while(fscanf(fd, "%ld %s %ld\n", &usrid, login, &blcks) == 3) {
		if((index = hash(usrid)) == FAIL) return(FAIL);
		if(userlist[index].uid == UNUSED) {
			userlist[index].uid = usrid;
			strncpy(userlist[index].name, login, MAXNAME);
		}
		userlist[index].usage += blcks;
	}
}

ignore(str)
register char	*str;
{
	char	*skip();

	for( ; *str && igncnt < MAXIGN; str = skip(str), igncnt++)
		ignlist[igncnt] = str;
	if(igncnt == MAXIGN) {
		fprintf(stderr, "%s: ignore list overflow. Recompile with larger MAXIGN\n", cmd);
	}
}


output()
{
	int index;

	for (index=0; index < MAXUSERS ; index++)
		if (userlist[index].uid != UNUSED && userlist[index].usage != 0)
			printf("%ld	%s	%ld\n",
			    userlist[index].uid,
			    userlist[index].name,
			    userlist[index].usage);
}

unsigned
hash(j)
uid_t j;
{
	register unsigned start;
	register unsigned circle;
	circle = start = (unsigned)j % MAXUSERS;
	do
	{
		if (userlist[circle].uid == j || userlist[circle].uid == UNUSED)
			return (circle);
		circle = (circle + 1) % MAXUSERS;
	} while (circle != start);
	return (FAIL);
}

hashinit() 
{
	int index;

	for(index=0; index < MAXUSERS ; index++) {
		userlist[index].uid = UNUSED;
		userlist[index].usage = 0;
		userlist[index].name[0] = '\0';
	}
}

static FILE *pwf = NULL;

setup(pfile)
char	*pfile;
{
	register struct passwd	*pw;
	void end_pwent();
	struct passwd *	(*getpw)();
	void	(*endpw)();
	int index;

	if (pfile) {
		if(!stpwent(pfile)) {
			fprintf(stderr, "%s: Cannot open %s\n", cmd, pfile);
			exit(5);
		}
		getpw = fgetpwent;
		endpw = end_pwent;
	} else {
		setpwent();
		getpw = getpwent;
		endpw = endpwent;
	}
	while ((pw=getpw(pwf)) != NULL) {
		if ((index=hash(pw->pw_uid)) == FAIL) {
			fprintf(stderr,"%s: INCREASE SIZE OF MAXUSERS\n", cmd);
			return (FAIL);
		}
		if (userlist[index].uid == UNUSED) {
			userlist[index].uid = pw->pw_uid;
			strncpy(userlist[index].name, pw->pw_name, MAXNAME);
		}
	}

	endpw();
}

char	*
skip(str)
register char	*str;
{
	while(*str) {
		if(*str == ' ' ||
		    *str == ',') {
			*str = '\0';
			str++;
			break;
		}
		str++;
	}
	return(str);
}

stpwent(pfile)
register char *pfile;
{
	if(pwf == NULL)
		pwf = fopen(pfile, "r");
	else
		rewind(pwf);
	return(pwf != NULL);
}

void
end_pwent()
{
	if(pwf != NULL) {
		(void) fclose(pwf);
		pwf = NULL;
	}
}

/*
 * Below this point are the routines taken from the standard vxfs fs
 * library.  These routines, in the main, are used for encapsulating
 * the handling of snapshot mounted file systems.
 */

static char fsopenfmtstr[] = ":1:fs_open: internal error\n";
static char statfmtstr[] = ":2:Cannot access %s: %s\n";
static char openfmtstr[] = ":3:Cannot open %s: %s\n";
static char mallocfmtstr[] = ":4:malloc of space for super block failed\n";
static char rsuperfmtstr[] = ":5:read of super-block on %s failed: %s\n";
static char rsuperbfmtstr[] = ":6:read of super-block on %s failed: not enough bytes\n";
static char notborcfmtstr[] = ":7:%s is not a block or character special file\n";
static char nosnmntfmtstr[] =  ":8:snapshot mountpoint for %s (%s) not found\n";
static char notsnapfmtstr[] = ":9:mountpoint %s for %s (%s) not snapshot file system\n";
static char rsnapsbfmtstr[] = ":10:snapshot read of super-block on %s (%s) failed: %s\n";
static char rsnapsbbfmtstr[] = ":11:snapshot read of super-block on %s (%s) failed: not enough bytes\n";
static char magicfmtstr[] = ":12:invalid vxfs super block magic\n";
static char versfmtstr[] = ":13:unrecognized vxfs version number\n";
static char csumfmtstr[] = ":14:invalid vxfs super-block checksum\n";

static char fsetofmtstr[] = ":15:fset_open: internal error\n";
static char fsetnotstr[] = ":16:fileset not found\n";
static char irangestr[] = ":17:inode %d out of range\n";
static char irdblkstr[] = ":18:read failed for block %d: %s\n";
static char irdbufstr[] = ":19:inode extent buffer allocation failed\n";
static char irdextstr[] = ":20:bad ilist extent\n";

struct indbuf {
	daddr_t	bno;
	char	buf[IADDREXTSIZE];
};

#undef		bzero
#define bzero(cp, size)	memset((void *) cp, 0, size)

static ulong	unnamed_index = VX_FSET_UNNAMED_INDEX;

static char	errbuf[MAXPATHLEN + 80];	/* for generated errors */
static struct indbuf	indbuf[2];

char		*fs_findmntpt(char *);
static char	*fs_cksnapmnt(struct fs_dat *);
static void	fs_err_fmt(const char *, ...);

/* static function prototypes for VERSION2 layout support */
static int	fset_iread_v1(struct fs_dat *, struct fset_dat *, ino_t inum,
			      void *, ulong);
static int	fset_iread_v2(struct fs_dat *, struct fset_dat *, ino_t inum,
			      void *, ulong);
static int	fset_indices(struct fs_dat *);
static int	fset_find_index(struct fs_dat *, struct fset_dat *, ulong);
static int	fset_checksum(struct vx_fsethead *);
static int	iget_inode(struct fs_dat *, ino_t, struct vx_dinode *, daddr_t *);
static int	olt_read(struct fs_dat *, ino_t *, ino_t *, daddr_t *);
static int	olt_checksum(caddr_t, int);
static int	b_readi(struct fs_dat *, struct vx_dinode *, char *, off_t, int);
static int	bmapindir(struct fs_dat *, ino_t, ulong, off_t, off_t *,
			  off_t, off_t *, struct vx_extent *, daddr_t,
			  long, int);
static int	bad_extent(struct fs_dat *, daddr_t, long);


/*
 * read the super-block of the target file system.  it should
 * be of type vxfs.  if it's a snapshot file system, try to
 * figure out the mountpoint and setup values for future
 * snapreads.
 *
 * if the routine returns successfully, datp is filled  with valid information,
 * including super-block of the file system (snapshot or not).
 *
 * this routine will not necessarily work correctly if spec is
 * a tape, and will leave spec open (until an fs_close() is done),
 * inhibiting rewind.
 *
 * fs_read() and fs_write() won't work on tapes (since they attempt to seek).
 * snapshot file systems on tape don't work.
 *
 * returns NULL on success, or an error message on failure.
 */

char *
fs_open(datp, spec, oflag, ckflags)
	struct fs_dat	*datp;
	char	 *spec;
	int	oflag;
	int	ckflags;
{
	int	fd, len;
	int	fsopt;
	char	*buf;
	char	*bspec;
	char	*errmsg;
	struct stat	statbuf;
	struct vx_fs	*fs;

	if (datp == NULL) {
		fs_err_fmt(fsopenfmtstr, NULL);
		return errbuf;
	}
	if (stat(spec, &statbuf) != 0) {
		fs_err_fmt(statfmtstr, spec, strerror(errno));
		return errbuf;
	}

	if (oflag == O_WRONLY) {
		oflag = O_RDWR;
	}
	if ((fd = open(spec, oflag)) < 0) {
		fs_err_fmt(openfmtstr, spec, strerror(errno));
		return errbuf;
	}

	buf = (char *) malloc(VX_MAXBSIZE);
	if (!buf) {
		close(fd);
		fs_err_fmt(mallocfmtstr, NULL);
		return errbuf;
	}
	len = read(fd, buf, VX_MAXBSIZE);
	if (len != VX_MAXBSIZE) {
		if (len < 0) {
			fs_err_fmt(rsuperfmtstr, spec, strerror(errno));
		} else {
			fs_err_fmt(rsuperbfmtstr, spec);
		}
		goto errout;
	}

	/*
	 * note that ckflags may be 0, and the magic number
	 * may not be valid, even though no error is returned
	 * for Version 2 layout, we cannot initialize filesets
	 * safely unless the super-block checksum is checked and 
	 * valid
	 */
	 
	fs = (struct vx_fs *) (buf + VX_SUPERBOFF);
	errmsg = fs_cksblock(fs, ckflags);
	if (errmsg) {
		goto errout;
	}

	datp->fsd_fd = fd;
	datp->fsd_spec = strdup(spec);
	datp->fsd_fs = *fs;

	if (fs->fs_version == VX_VERSION) {
		datp->fsd_nindex = 1;
		datp->fsd_index = &unnamed_index;
	} else if ((ckflags & FS_CKSUM) && fs->fs_magic != VX_SNAPMAGIC) {

		/*
		 * If it's not a snapshot, set up the filset table.
		 */

		fset_indices(datp);
	}

	if (fs->fs_magic != VX_SNAPMAGIC) {

		/*
		 * If it's not a snapshot, we're done.
		 */

		fs_lseek(datp, 0, 0);
		datp->fsd_issnapshot = 0;
		free(buf);
		return NULL;
	}

	/*
	 * It's a snapshot file system; try to determine the mountpoint.
	 */

	datp->fsd_issnapshot = 1;
	if ((statbuf.st_mode & S_IFMT) == S_IFCHR) {
		bspec = fs_convto_bspec(spec);
	} else if ((statbuf.st_mode & S_IFMT) == S_IFBLK) {
		bspec = spec;
	} else {
		fs_err_fmt(notborcfmtstr, spec);
		goto errout;
	}
	datp->fsd_mntpt = fs_findmntpt(bspec);
	if (!datp->fsd_mntpt) {
		if (ckflags & FS_NOSNAPERR) {
			goto out;
		}
		fs_err_fmt(nosnmntfmtstr, spec, bspec);
		goto errout;
	}

	if (fs_cksnapmnt(datp)) {
		if (ckflags & FS_NOSNAPERR) {
			goto out;
		}
		fs_err_fmt(notsnapfmtstr, datp->fsd_mntpt, spec, bspec);
		goto errout;
	}

	/*
	 * now we get the "real" super-block for the snapshot.
	 */

	len = fs_read(datp, (void *) buf, VX_MAXBSIZE);
	if (len != VX_MAXBSIZE) {
		if (len < 0) {
			fs_err_fmt(rsnapsbfmtstr, datp->fsd_mntpt, spec,
				   strerror(errno));
		} else { 
			fs_err_fmt(rsnapsbbfmtstr, datp->fsd_mntpt, spec);
		}
		goto errout;
	}
	fs_lseek(datp, 0, 0);
	fs = (struct vx_fs *) (buf + VX_SUPERBOFF);

	errmsg = fs_cksblock(fs, FS_CKMAGIC | FS_CKVERSION | FS_CKSUM);
	if (errmsg) {
		goto errout;
	}

	/*
	 * Now we can set up the fileset table from the primary's 
	 * data, reset the seek pointer and be done.
	 */

	fset_indices(datp);
	fs_lseek(datp, 0, 0);
	datp->fsd_fs = *fs;
out:
	free(buf);
	return NULL;

errout:
	free(buf);
	close(fd);
	return errbuf;
}


/*
 * Check the putative snapshot mountpoint for perjury.
 *
 * We "round up" the checks since we can't check the version number
 * unless we have a valid magic number, and can't check the checksum
 * unless we have a valid version number.
 */

char *
fs_cksblock(fsp, ckflags)
	struct vx_fs	*fsp;
	int		ckflags;
{
	if (ckflags & (FS_CKMAGIC | FS_CKVERSION | FS_CKSUM)
	    && fsp->fs_magic != VX_MAGIC && fsp->fs_magic != VX_SNAPMAGIC) {
		fs_err_fmt(magicfmtstr, NULL);
		return errbuf;
	}
	if (ckflags & (FS_CKVERSION | FS_CKSUM) &&
	    ((fsp->fs_magic == VX_MAGIC &&
	      (fsp->fs_version != VX_VERSION &&
	       fsp->fs_version != VX_VERSION2)) ||
	     (fsp->fs_magic == VX_SNAPMAGIC &&
	      (fsp->fs_version != VX_SNAPVERSION &&
	       fsp->fs_version != VX_SNAPVERSION2)))) {
		fs_err_fmt(versfmtstr, NULL);
		return errbuf;
	}
	if (ckflags & FS_CKSUM &&
	    (fsp->fs_checksum != VX_FSCHECKSUM(fsp) ||
	     fsp->fs_checksum2 != VX_FSCHECKSUM2(fsp))) {
		fs_err_fmt(csumfmtstr, NULL);
		return errbuf;
	}

	return NULL;
}


/*
 * verify that the special file and the mounted snapshot filesystem 
 * correspond to the same file system.
 *
 * return NULL if they match, an errmsg on failure.
 */

static char *
fs_cksnapmnt(datp)
	struct fs_dat *datp;
{
	int	fsopt;
	struct statvfs	vfsbuf;

	if ((datp->fsd_mntfd = open(datp->fsd_mntpt, O_RDONLY)) < 0) {
		return "open of snapshot mountpoint %s failed:";
	}
	if (fstatvfs(datp->fsd_mntfd, &vfsbuf) != 0) {
		return "fstatvfs of snapshot mountpoint %s failed:";
	}
	if (strcmp(vfsbuf.f_basetype, "vxfs") != 0) {
		return "snapshot mountpoint %s is not a vxfs file system";
	}
	if (ioctl(datp->fsd_mntfd, VX_GETFSOPT, &fsopt) != 0) {
		return "VX_GETFSOPT ioctl on snapshot mountpoint %s failed:";
	}
	if (!(fsopt & VX_FSO_SNAPSHOT)) {
		return "file system %s is not a snapshot!";
	}
	if (fs_ckspecmnt(datp->fsd_mntpt, datp->fsd_spec)) {
		return "special file %s doesn't match file system %s";
	}
	return NULL;
}


/*
 * Try to convert a character special pathname of the
 * form /dev/rdsk/0s7, /dev/rroot, or /dev/sd01/rc0t0s1
 * to the corresponding block special pathname.
 * Essentially strip the 'r' from the first '/r' 
 * combination found, and verify that it's a block device.
 * If we're passed a relative pathname, first convert
 * it to absolute before doing our thing.
 *
 * return malloc'ed space holding the path of the block special.
 * return NULL on failure.
 */

char *
fs_convto_bspec(spec)
	char *spec;
{
	char *sp, *bp, *bspec;
	char cdp[2 * MAXPATHLEN];
	struct stat statbuf;

	if (strlen(spec) >= (size_t) MAXPATHLEN) {
		return NULL;
	}
	sp = spec;
	if (*sp != '/') {
		if (getcwd(cdp, 2 * MAXPATHLEN) == NULL) {
			return NULL;
		}
		strcat(cdp, "/");
		strcat(cdp, sp);
		sp = cdp;
	}
		
	bspec = malloc(2 * MAXPATHLEN);
	if (bspec == NULL) {
		return NULL;
	}
	bp = bspec;
	while (*sp) {
		*bp = *sp;
		if (*sp == '/' && *(sp + 1) == 'r') {
			sp += 2;
			bp++;
			break;
		}
		sp++;
		bp++;
	}
	if (!*sp) {
		goto errout;
	}
	while (*bp++ = *sp++) {
	}
	if (stat(bspec, &statbuf) != 0) {
		goto errout;
	}
	if ((statbuf.st_mode & S_IFMT) == S_IFBLK) {
		return bspec;
	}

errout:
	free(bspec);
	return NULL;
}


/*
 * Try to convert a block special pathname of the
 * form /dev/dsk/0s7, /dev/root, or /dev/sd01/c0t0s1
 * to the corresponding character special pathname, e.g.
 * /dev/rdsk/0s7, /dev/rroot, or /dev/sd01/rc0t0s1.
 * Essentially try adding an 'r' after the 2nd to
 * last '/' and after the last '/' and see if we've
 * found a character special file.
 * If we're passed a relative pathname, first convert
 * it to absolute before doing our thing.
 *
 * This should really parse things like "/dev//vol/./v0" correctly,
 * but it doesn't.
 *
 * return malloc'ed space holding the path of the character special.
 * return NULL on failure.
 */

char *
fs_convto_cspec(bspec)
	char *bspec;
{
	char *tp, *cp, *cspec;
	char c;
	struct stat statbuf;

	if (strlen(bspec) >= (size_t) MAXPATHLEN) {
		return NULL;
	}
	cspec = malloc(2 * MAXPATHLEN);
	if (cspec == NULL) {
		return NULL;
	}
	if (*bspec != '/') {
		if (getcwd(cspec, 2 * MAXPATHLEN) == NULL) {
			free(cspec);
			return NULL;
		}
		strcat(cspec, "/");
		strcat(cspec, bspec);
	} else {
		strcpy(cspec, bspec);
	}
		
	/*
	 * try adding an 'r' after the second-to-last '/'
	 */

	cp = strrchr(cspec, '/');
	if (cp) {
		*cp = 0;
		tp = strrchr(cspec, '/');
		*cp = '/';
		cp = tp;
	}
	if (cp) {
		cp++;
		tp = cp + strlen(cp);
		while (tp >= cp) {
			*(tp + 1) = *tp;
			tp--;
		}
		*cp = 'r';

		if (stat(cspec, &statbuf) == 0
		    && (statbuf.st_mode & S_IFMT) == S_IFCHR) {
			return cspec;
		}
	}

	/*
	 * That didn't work.  Fix up the string, and try adding
	 * a 'r' after the last '/'.  cp (if set) still points
	 * at the 'r' we added earlier.
	 */

	while (cp && *cp) {
		*cp = *(cp + 1);
		cp++;
	}
	cp = strrchr(cspec, '/');
	if (cp) {
		cp++;
		tp = cp + strlen(cp);
		while (tp >= cp) {
			*(tp + 1) = *tp;
			tp--;
		}
		*cp = 'r';

		if (stat(cspec, &statbuf) == 0
		    && (statbuf.st_mode & S_IFMT) == S_IFCHR) {
			return cspec;
		}
	}

	free(cspec);
	return NULL;
}


/*
 * Verify that the passed special corresponds to the passed
 * file system by reading the super-block from the special
 * device and comparing the unchanging fields.  This is only
 * an approximate test.  We remember that the writable fields 
 * may be changing, and that snapshot file systems have a
 * slightly different super-block.
 *
 * Return 0 if they match, 1 if they don't, and -1 if some
 * kind of error prevents us from figuring it out.
 */

int
fs_ckspecmnt(mntpt, spec)
	char *mntpt;
	char *spec;
{
	char *mbufp, *cp;
	struct vx_fs *fs;
	struct statvfs stvbuf;
	int fd;
	int error;

	if ((mbufp = malloc(VX_MAXBSIZE)) == NULL) {
		return -1;
	}
	error = -1;
	if ((fd = open(spec, O_RDONLY)) < 0) {
		goto out;
	}
	if (read(fd, mbufp, VX_MAXBSIZE) != VX_MAXBSIZE) {
		close(fd);
		goto out;
	}
	(void) close(fd);
	if (statvfs(mntpt, &stvbuf) < 0) {
		goto out;
	}

	/*
	 * we've collected all the information for our tests,
	 * now perform them
	 */

	error = 1;
	fs = (struct vx_fs *) (mbufp + VX_SUPERBOFF);
	if (strcmp("vxfs", stvbuf.f_basetype) != 0 ||
	    fs_cksblock(fs, FS_CKMAGIC)) {
		goto out;
	}

	if (stvbuf.f_blocks != fs->fs_size ||
	    stvbuf.f_frsize != fs->fs_bsize) {
		goto out;
	}

	if (fs->fs_version == VX_VERSION &&
	    stvbuf.f_files != fs->fs_ninode) {
		goto out;
	}

	if (strncmp(stvbuf.f_fstr, fs->fs_fname, sizeof(fs->fs_fname)) != 0) {
		goto out;
	}
	cp = stvbuf.f_fstr + strlen(stvbuf.f_fstr) + 1;
	if (strncmp(cp, fs->fs_fpack, sizeof(fs->fs_fpack)) != 0) {
		goto out;
	}
	error = 0;

out:
	free(mbufp);
	return error;
}


/*
 * Verify that the passed block and character special files
 * correspond to the same file system device by reading the
 * super-block from each and comparing the contents of fields.  
 *
 * vxfs writes to the file system directly, bypassing the 
 * page cache used by the block device, so we expect the
 * character device to be more current than the block device.
 * The file system can write the super-block at any time.
 *
 * Return 0 if they match, 1 if they don't, and -1 if some
 * kind of error prevents us from figuring it out.
 */

int
fs_ckspecs(cspec, bspec)
	char *cspec, *bspec;
{
	struct vx_fs *cfs, *bfs;
	char *cbufp, *bbufp;
	char *mbufp, *cp;
	int fd;
	int error;

	if ((cbufp = malloc(VX_MAXBSIZE)) == NULL) {
		return -1;
	}
	if ((bbufp = malloc(VX_MAXBSIZE)) == NULL) {
		free(cbufp);
		return -1;
	}

	error = -1;
	if ((fd = open(bspec, O_RDONLY)) < 0) {
		goto out;
	}
	if (read(fd, bbufp, VX_MAXBSIZE) != VX_MAXBSIZE) {
		close(fd);
		goto out;
	}
	(void) close(fd);
	bfs = (struct vx_fs *) (bbufp + VX_SUPERBOFF);

	if ((fd = open(cspec, O_RDONLY)) < 0) {
		goto out;
	}
	if (read(fd, cbufp, VX_MAXBSIZE) != VX_MAXBSIZE) {
		close(fd);
		goto out;
	}
	(void) close(fd);
	cfs = (struct vx_fs *) (cbufp + VX_SUPERBOFF);

	/*
	 * we've collected all the information for our tests,
	 * now perform them
	 */

	error = 1;
	if (cfs->fs_magic != bfs->fs_magic ||
	    VX_FSCHECKSUM(cfs) != VX_FSCHECKSUM(bfs) ||
	    VX_FSCHECKSUM2(cfs) != VX_FSCHECKSUM2(bfs) ||
	    cfs->fs_size != bfs->fs_size ||
	    cfs->fs_ninode != bfs->fs_ninode) {
		goto out;
	}
	if (cfs->fs_ctime != bfs->fs_ctime ||
	    cfs->fs_ectime != bfs->fs_ectime) {
		goto out;
	}
	if (cfs->fs_time < bfs->fs_time ||
	    (cfs->fs_time == bfs->fs_time &&
	     cfs->fs_etime < bfs->fs_etime)) {
		goto out;
	}
	if (memcmp(bfs->fs_fname, cfs->fs_fname,
	           sizeof(bfs->fs_fname)) != 0 ||
	    memcmp(bfs->fs_fpack, cfs->fs_fpack,
		      sizeof(bfs->fs_fpack)) != 0) {
		goto out;
	}
	error = 0;

out:
	free(cbufp);
	free(bbufp);
	return error;
}


/*
 * set a specified offset for reading or writing to the
 * the file system; undefined when used on tape drives.
 */

off_t
fs_lseek(datp, off, whence)
	struct fs_dat	*datp;
	off_t	off;
	int	whence;
{
	if (!datp->fsd_issnapshot) {
		datp->fsd_off = lseek(datp->fsd_fd, off, whence);
	} else {
		switch (whence) {
		case 0:
			datp->fsd_off = off;
			break;
		case 1:
			datp->fsd_off += off;
			break;
		case 2:
			
			/*
			 * special files seem to have a size of 0
			 */

			datp->fsd_off = off;
			break;
		default:
			datp->fsd_off = -1;
			break;
		}
	}

	return datp->fsd_off;
}


/*
 * read the specified blocks from a file system; handle snapshot appropriately.
 * off and len are in bytes.
 *
 * Since this is either reading a device or using the VX_SNAPREAD ioctl to
 * simulate reading a device, the offset and the length must both be in
 * multiples of DEV_BSIZE.
 */

int
fs_read(datp, buf, len)
	struct fs_dat *datp;
	void *buf;
	unsigned len;
{
	struct vx_fs *fs;
	struct vx_snapread args;
	int ret;

	if (datp->fsd_off & DEV_BOFFSET || len & DEV_BOFFSET) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * If this is a snapshot file system, then use the VX_SNAPREAD
	 * ioctl to read the snapshot image of the file system.  Otherwise
	 * use read to read the device.
	 */

	if (datp->fsd_issnapshot) {
		args.sr_buf = (char *) buf;
		args.sr_off = datp->fsd_off >> DEV_BSHIFT;
		args.sr_len = len;
		ret = ioctl(datp->fsd_mntfd, VX_SNAPREAD, &args);
	} else {
		ret = read(datp->fsd_fd, buf, len);
	}
	if (ret != -1) {
		datp->fsd_off += ret;
	}
	return ret;
}

/*
 * write the specified blocks to a file system; snapshots are read-only.
 * off and len are in bytes.
 */

int
fs_write(datp, buf, len)
	struct fs_dat *datp;
	void *buf;
	unsigned len;
{
	struct vx_snapread args;
	int	ret;

	if (datp->fsd_issnapshot) {
		errno = EROFS;
		return -1;
	}
	ret = write(datp->fsd_fd, buf, len);
	if (ret != -1) {
		datp->fsd_off += ret;
	}
	return ret;
}


/*
 * Given the pathname of a block special file, try to find where
 * its mounted based on the mount table.
 *
 * Space for the returned mountpoint is malloc'ed.
 */

char *
fs_findmntpt(bspec)
	char *bspec;
{
	FILE *fp;
	struct mnttab	mntent;
	struct mnttab	mntpref;

	fp = fopen(MNTTAB, "r");
	if (!fp) {
		return NULL;
	}

	bzero(&mntpref, sizeof(struct mnttab));
	mntpref.mnt_special = bspec;
	if (getmntany(fp, &mntent, &mntpref)) {
		fclose(fp);
		return NULL;
	}

	fclose(fp);
	return strdup(mntent.mnt_mountp);
}


/*
 * Given the mountpoint, try to find the block special file 
 * in the mount table.
 *
 * Space for the returned pathname is malloc'ed.
 */

char *
fs_findbspec(mntpt)
	char *mntpt;
{
	FILE *fp;
	struct mnttab	mntent;
	struct mnttab	mntpref;

	fp = fopen(MNTTAB, "r");
	if (!fp) {
		return NULL;
	}

	bzero(&mntpref, sizeof(struct mnttab));
	mntpref.mnt_mountp = mntpt;
	if (getmntany(fp, &mntent, &mntpref)) {
		fclose(fp);
		return NULL;
	}

	fclose(fp);
	return strdup(mntent.mnt_special);
}

/*
 * Given the mountpoint of a snapshot file system, try to find
 * the block special file of the primary in the mount table.
 *
 * The primary may have been referenced as either the device
 * or mountpoint when the snapshot was mounted.
 */

char *
fs_find_primarybspec(mntpt)
	char *mntpt;
{
	FILE *fp;
	struct mnttab	mntent;
	struct mnttab	mntpref;
	char		*cp;
	struct stat	statbuf;

	fp = fopen(MNTTAB, "r");
	if (!fp) {
		return NULL;
	}

	bzero(&mntpref, sizeof(struct mnttab));
	mntpref.mnt_mountp = mntpt;
	if (getmntany(fp, &mntent, &mntpref)) {
		fclose(fp);
		return NULL;
	}

	fclose(fp);
	cp = strtok(mntent.mnt_mntopts, ",= ");
	do {
		if (strcmp(cp, "snapof") == 0) {

			/*
			 * The next token is the object of intrest.
			 * If this is a device file we're done.
			 * Otherwise, find the device associated
			 * with the mountpoint.
			 */

			cp = strdup(strtok(NULL, ", "));
			if (stat(cp, &statbuf) != 0) {
				return NULL;
			} 
			if ((statbuf.st_mode & S_IFMT) == S_IFBLK) {
				return cp;
			}
			if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
				return fs_findbspec(cp);
			}

			return NULL;
		}
	} while (cp = strtok(NULL, ",= "));

	return NULL;
}

/*
 * allocate an fs_dat structure and initialize it for use by our callers
 */

struct fs_dat *
fs_datalloc()
{
	struct fs_dat	*datp;

	datp = (struct fs_dat *) malloc(sizeof(struct fs_dat));
	if (datp) {
		bzero((char *) datp, sizeof(struct fs_dat));
		datp->fsd_fd = -1;
		datp->fsd_mntfd = -1;
	}
	return datp;
}

/*
 * close an open file system; and clear the snapshot mountpoint.
 */

int
fs_close(datp)
	struct fs_dat	*datp;
{
	close(datp->fsd_fd);
	datp->fsd_fd = -1;
	free(datp->fsd_spec);
	datp->fsd_spec = NULL;
	if (datp->fsd_issnapshot) {
		datp->fsd_issnapshot = 0;
		close(datp->fsd_mntfd);
		datp->fsd_mntfd = -1;
	}
	if (datp->fsd_mntpt) {
		free(datp->fsd_mntpt);
		datp->fsd_mntpt = NULL;
	}
	if ((datp->fsd_nindex > 1) &&
	    (datp->fsd_index != (ulong *)NULL)) {
		free(datp->fsd_index);
	} 
	datp->fsd_nindex = 0;
	datp->fsd_index = (ulong *)NULL;
	datp->fsd_off = 0;

	return 0;
}


/*
 * free an allocated fs_dat structure; it's assumed to have
 * already been closed.
 */

void
fs_datfree(datp)
	struct fs_dat *datp;
{
}

/*
 * general error message formatting
 * accepts standard pfmt() message strings and produces a formatted
 * error message in the static global errbuf[].  If this is the
 * international library, output the message using vpfmt() also.
 */

static void
fs_err_fmt(const char *fmt, ...)
{
	va_list ap;
	char *cp;

	va_start(ap, fmt);

	/*
	 * skip over the first 2 ':' separated elements of fmt
	 */

	if ((cp = strchr(fmt, ':')) == NULL ||
	    (cp = strchr(++cp, ':')) == NULL) {

		/*
		 * this is a usage error, but is not serious unless
		 * we are internationalized, so let vpfmt() complain
		 */ 

		cp = (char *) fmt;
	} else {
		cp++;
	}
	vsprintf(errbuf, cp, ap);
#ifdef VXI18N
	vpfmt(stderr, MM_ERROR, fmt, ap);
#endif
	va_end(ap);
}

/*
 * read from an inode at a specific offset into a buffer
 */

int
fs_readi(fsdp, dp, off, buf, cnt)
	struct fs_dat		*fsdp;
	struct vx_dinode	*dp;
	off_t			off;
	void			*buf;
	int			cnt;
{
	struct vx_fs	*fs;
	int	sz, coff, boff, rc;
	char	*bp, *up;
	
	fs = &fsdp->fsd_fs;
	up = (char *)buf;

	/*
	 * check inode mode
	 */

	if (dp->di_mode == 0) {
		errno = EINVAL;
		return -1;
	}

	/*
	 *  check offset
	 */

	if (off > dp->di_size)  {
		errno = EINVAL;
		return -1;
	} else if (off == dp->di_size) {
		return 0;
	}

	/*
	 * check inode org - handle immediate data quickly
	 */

	if (dp && dp->di_orgtype == IORG_IMMED) {
		if (off + cnt > dp->di_size) {
			sz = dp->di_size - off;
		} else {
			sz = cnt;
		}
		memcpy(buf, dp->di_immed + off, sz);
		return sz;
	}
	
	/*
	 * handle non-aligned requests
	 */

	if ((off & fs->fs_boffmask) ||
	    (cnt & fs->fs_boffmask)) {
		bp = malloc(fs->fs_bsize);
		if (bp == (char *)NULL) {
			return -1;
		}
		boff = off & fs->fs_boffmask;
		coff = 0;
		while (coff < cnt) {
			rc = b_readi(fsdp, dp, bp, off & fs->fs_bmask,
				     fs->fs_bsize);
			if (rc < 0) {
				coff = rc;
				break;
			}
			sz = min(cnt - coff, fs->fs_bsize);
			memcpy(up + coff, bp + boff, sz);
			coff += sz;
			boff = 0;
		}
		free(bp);
	} else {
		if ((coff = b_readi(fsdp, dp, up, off, cnt)) == 0) {
			coff = cnt;
		}
	}
	return coff;
}

/*
 * allocate an fset structure
 */

struct fset_dat *
fset_datalloc()
{
	struct fset_dat *fsetdp;
	fsetdp = (struct fset_dat *) malloc(sizeof(struct fset_dat));
	if (fsetdp) {
		bzero((char *) fsetdp, sizeof (struct fset_dat));
	}
	return fsetdp;
}

/*
 * free an fset structure
 */

void
fset_datfree(fsetdp)
	struct fset_dat *fsetdp;
{
	free((char *) fsetdp);
}

/*
 * fill in an fset structure
 */

char *
fset_open(fsdp, fsetdp, fsid)
	struct fs_dat	*fsdp;
	struct fset_dat *fsetdp;
	ulong	fsid;
{
	struct vx_fs		*fs = &fsdp->fsd_fs;
	struct vx_fsethead	*fsh = &fsetdp->fst_head;
	int	iextsize, niau, inopiau;

	if (fsdp == (struct fs_dat *)NULL ||
	    fsetdp == (struct fset_dat *)NULL) {
		fs_err_fmt(fsetofmtstr, NULL);
		return errbuf;
	}
	if (fsdp->fsd_nindex == 0) {
		fs_err_fmt(fsetnotstr, NULL);
		return errbuf;
	}
	indbuf[0].bno = -1;
	indbuf[1].bno = -1;
	if (fs->fs_version != VX_VERSION2) {
		if (fsid != VX_FSET_UNNAMED_INDEX) {
			fs_err_fmt(fsetnotstr, NULL);
			return errbuf;
		}

		/*
		 * build a dummy fset_dat structure
		 */

		inopiau = fs->fs_inopau;
		niau = (fs->fs_ninode + inopiau - 1) / inopiau;
		iextsize = fs->fs_iaddrlen;
		while (fs->fs_auilen % iextsize) {
			iextsize >>= 1;
		}
		fsh->fsh_version = VX_FSET_VERSION;
		fsh->fsh_fsindex = VX_FSET_UNNAMED_INDEX;
		fsh->fsh_time = fs->fs_ctime;
		fsh->fsh_etime = fs->fs_ectime;
		fsh->fsh_fsextop = 0;
		fsh->fsh_ninode = fs->fs_ninode;
		fsh->fsh_nau = niau;
		fsh->fsh_ilesize = iextsize;
		fsh->fsh_dflags = VX_FSET_UNNAMED;
		fsh->fsh_quota = fs->fs_dsize;
		fsh->fsh_iauino = 0;
		fsh->fsh_ilistino[0] = 0;
		fsh->fsh_ilistino[1] = 0;
		fsh->fsh_checksum = fset_checksum(fsh);

		/*
		 * fill in rest of fset structure
		 */

		fsetdp->fst_cuent.cu_fsindex = VX_FSET_UNNAMED_INDEX;
		fsetdp->fst_cuent.cu_curused = fs->fs_dsize - fs->fs_free;
		return NULL;
	}
	
	if (fset_find_index(fsdp, fsetdp, fsid)) {
		fs_err_fmt(fsetnotstr, NULL);
		return errbuf;
	}

	return NULL;
}

/*
 * close an fset structure
 */

int
fset_close(fsetdp)
	struct fset_dat	*fsetdp;
{
	bzero((char *) fsetdp, sizeof (struct fset_dat));
	return 0;
}

/*
 * read a portion of the ilist for a fileset into user supplied buffer
 * returns number of inodes read in caller's nino variable
 */

char *
fset_iread(fsdp, fsetdp, inum, buf, bsize, nino)
	struct fs_dat	*fsdp;
	struct fset_dat	*fsetdp;
	ino_t	inum;
	void	*buf;
	ulong	bsize;
	int	*nino;
{
	struct vx_fs		*fs = &fsdp->fsd_fs;
	struct vx_fsethead	*fsh = &fsetdp->fst_head;
	char	*rbuf, *cp;
	off_t	coff;
	int	i, numi, blks, brem, rsize, rinum, cnt;
	int	inpe, ioff, isize, icnt;

	/*
	 * validate inumber
	 */

	if (inum < 0 || inum > fsh->fsh_ninode) {
		*nino = 0;
		fs_err_fmt(irangestr, inum);
		return errbuf;
	}
	if (inum == fsh->fsh_ninode) {
		*nino = 0;
		return NULL;
	}

	isize = fs->fs_bsize / fs->fs_inopb;
	inpe = fsh->fsh_ilesize * fs->fs_inopb;
	ioff = inum % inpe;
	blks = bsize / fs->fs_bsize;
	brem = blks % fsh->fsh_ilesize;
	if (blks == 0 || brem != 0 || ioff != 0) {

		/*
		 * doing non-extent aligned/sized reads of the
		 * ilist
		 */

		rsize = fs->fs_bsize * fsh->fsh_ilesize;
		rbuf = malloc(rsize);
		if (rbuf == (char *)NULL) {
			*nino = 0;
			fs_err_fmt(irdbufstr, NULL);
			return errbuf;
		}
		rinum = inum & ~(inpe - 1);
		icnt = 0;
		coff = 0;
		while (coff < bsize) {
			if (fs->fs_version != VX_VERSION2) {
				i = fset_iread_v1(fsdp, fsetdp, rinum,
						  rbuf, rsize);
			} else {
				i = fset_iread_v2(fsdp, fsetdp, rinum,
						  rbuf, rsize);
			}
			if (i <= 0) {
				break;
			}
			cp = rbuf + (ioff * isize);
			cnt = rsize - (ioff * isize);
			if (cnt > (bsize - coff)) {
				cnt = bsize - coff;
			}
			memcpy((char *) buf + coff, cp, cnt);
			rinum += i;
			ioff = 0;
			coff += cnt;
			icnt += cnt / isize;
		}
		free(rbuf);
		*nino = icnt;
		if (*nino > 0) {
			return NULL;
		}
		return errbuf;
	} else {
		if (fs->fs_version != VX_VERSION2) {
			*nino = fset_iread_v1(fsdp, fsetdp, inum, buf, bsize);
			if (*nino > 0) {
			        return NULL;
			}
		} else {
			*nino = fset_iread_v2(fsdp, fsetdp, inum, buf, bsize);
			if (*nino > 0) {
				return NULL;
			}
		}
		return errbuf;
	}
}

/*
 * real iread for VERSION1 layout - reads in extents
 */

static int
fset_iread_v1(fsdp, fsetdp, inum, buf, bsize)
	struct fs_dat	*fsdp;
	struct fset_dat	*fsetdp;
	ino_t	inum;
	void	*buf;
	ulong bsize;
{
	struct vx_fs		*fs = &fsdp->fsd_fs;
	struct vx_fsethead	*fsh = &fsetdp->fst_head;
	off_t	coff, size;
	int	aun, i, boff, blks, rino, numi;
	daddr_t	blk;

	/*
	 * calculate number of inodes that fit in buffer and adjust
	 * to the end of the ilist
	 */

	numi = bsize / (fs->fs_bsize / fs->fs_inopb);
	if ((inum + numi) > fsh->fsh_ninode) {
		numi = fsh->fsh_ninode - inum;
	}
	size = (numi / fs->fs_inopb) * fs->fs_bsize;

	coff = 0;
	rino = 0;
	while (coff < size) {
		aun = (inum + rino) / fs->fs_inopau;
		blk = fs->fs_fistart + aun * fs->fs_aulen;
		boff = ((inum + rino) % fs->fs_inopau) / fs->fs_inopb;
		blk += boff;
		blks = (size - coff) / fs->fs_bsize;
		if ((boff + blks) > fs->fs_auilen) {
			blks = fs->fs_auilen - boff;
			boff = 0;
		}
		fs_lseek(fsdp, blk << fs->fs_bshift, 0);
		if (fs_read(fsdp, (char *) buf + coff,
			    blks << fs->fs_bshift) < 0) {
			fs_err_fmt(irdblkstr, blk, strerror(errno));
			return 0;
		}
		rino += blks * fs->fs_inopb;
		coff += blks << fs->fs_bshift;
	}
	return rino;
}

/*
 * real iread for VERSION2 layout - reads in extents
 */

static int
fset_iread_v2(fsdp, fsetdp, inum, buf, bsize)
	struct fs_dat	*fsdp;
	struct fset_dat	*fsetdp;
	ino_t	inum;
	void	*buf;
	ulong bsize;
{
	struct vx_fs		*fs = &fsdp->fsd_fs;
	struct vx_fsethead	*fsh = &fsetdp->fst_head;
	struct vx_extent	ext;
	off_t	size, coff, maxlen, extoff;
	int	numi, bno;
	int	iloff;

	/*
	 * calculate number of inodes that fit in buffer and adjust
	 * to the end of the ilist
	 */

	maxlen = fsh->fsh_ilesize << fs->fs_bshift;
	numi = bsize / (fs->fs_bsize / fs->fs_inopb);
	if ((inum + numi) > fsh->fsh_ninode) {
		numi = fsh->fsh_ninode - inum;
	}
	size = (numi / fs->fs_inopb) * fs->fs_bsize;
	iloff = inum * (fs->fs_bsize / fs->fs_inopb);

	coff = 0;
	while (coff < size) {
		if (fs_bmap(fsdp, fsh->fsh_ilistino[0], fsh->fsh_fsindex,
			  &fsetdp->fst_ilino, iloff + coff, &extoff, &ext)) {
			fs_err_fmt(irdextstr, NULL);
			return 0;
		}
		bno = ext.ex_st;
		if (extoff) {
			bno += extoff >> fs->fs_bshift;
		}
		if (maxlen > size) {
			maxlen = size;
		}
		fs_lseek(fsdp, bno << fs->fs_bshift, 0);
		if (fs_read(fsdp, (char *) buf + coff, maxlen) < maxlen) {
			fs_err_fmt(irdblkstr, bno, strerror(errno));
			return 0;
		}
		coff += maxlen;
	}
	return numi;
}

/*
 * read the fileset struct and ilist extent for the requested fileset
 */

static int
fset_find_index(fsdp, fsetdp, fsid)
	struct fs_dat	*fsdp;
	struct fset_dat	*fsetdp;
	ulong		fsid;
{
	struct vx_fs		*fs = &fsdp->fsd_fs;
	struct vx_fsethead	*fhp;
	struct vx_dinode	ip;
	struct vx_cuent		*cup;
	struct fset_dat		*attrdp;
	int	sblk, iblk, sz, inopile;
	ino_t	fsinum, cuinum;
	off_t	off, ioff;
	daddr_t	bno, ext[2];
	char	*fbuf;

	if (olt_read(fsdp, &fsinum, &cuinum, ext) ||
	    iget_inode(fsdp, fsinum, &ip, ext)) {
		return -1;
	}
	attrdp = fset_datalloc();
	if (attrdp == (struct fset_dat *)NULL) {
		return -1;
	}

	/*
	 * allocate a general purpose buffer
	 */

	fbuf = malloc(fs->fs_bsize * fs->fs_iaddrlen);
	if (fbuf == (char *) NULL) {
		fset_datfree(attrdp);
		return -1;
	}

	/*
	 * read the fileset header file one block at a time
	 */

	off = 0;
	while (off < ip.di_size) {
		if (b_readi(fsdp, &ip, fbuf, off, fs->fs_bsize) < 0) {
			goto errout;
		}
		off += fs->fs_bsize;
		fhp = (struct vx_fsethead *)fbuf;
		if ((fhp->fsh_fsindex != fsid) &&
		    (fhp->fsh_fsindex != VX_FSET_ATTR_INDEX)) {
			continue;
		}
		if ((fhp->fsh_version != VX_FSET_VERSION) ||
		    (fset_checksum(fhp) != fhp->fsh_checksum)) {
			goto errout;
		}

		/*
		 * keep an eye out for the attribute fileset
		 */

		if (fhp->fsh_fsindex == VX_FSET_ATTR_INDEX) {
			if (attrdp->fst_head.fsh_fsindex != 0) {
				goto errout;
			}
			attrdp->fst_head = *fhp;
			sz = fs->fs_iaddrlen << fs->fs_bshift;

			/*
			 * now read an ilist inode
			 * it must be in the first two ilist extents
			 */

			inopile = fs->fs_inopb * fhp->fsh_ilesize;
			sblk = fhp->fsh_ilistino[0] / inopile;
			ioff = fhp->fsh_ilistino[0] % inopile;
			if (sblk > 2) {
				goto errout;
			}
			
			/*
			 * read block containing fileset inode
			 */

			fs_lseek(fsdp, (ext[sblk] << fs->fs_bshift), 0);
			if (fs_read(fsdp, fbuf, sz) < sz) {
				goto errout;
			}
			attrdp->fst_ilino = *(struct vx_dinode *)(fbuf +
					(ioff * fs->fs_dinosize));

			if ((attrdp->fst_ilino.di_mode & IFMT) != IFILT) {
				goto errout;
			}
			if (fsid == VX_FSET_ATTR_INDEX) {

				/*
				 * just copy what we've got
				 */

				fsetdp->fst_head = attrdp->fst_head;
				fsetdp->fst_ilino = attrdp->fst_ilino;
				break;
			}
		} else {

			/*
			 * attribute better be filled in by now
			 */

			fsetdp->fst_head = *fhp;
			sz = fhp->fsh_ilesize << fs->fs_bshift;

			/*
			 * calculate which block of the ilist extent file the
			 * contains the inode
			 */

			inopile = fs->fs_inopb * fhp->fsh_ilesize;
			sblk = fhp->fsh_ilistino[0] / inopile;
			ioff = fhp->fsh_ilistino[0] % inopile;
			
			/*
			 * read the inode from the attribute ilist file
			 */

			if (b_readi(fsdp, &attrdp->fst_ilino, fbuf,
				    sblk * sz, sz)) {
				goto errout;
			}
			fsetdp->fst_ilino = *(struct vx_dinode *)(fbuf +
				(ioff * fs->fs_dinosize));
			if ((attrdp->fst_ilino.di_mode & IFMT) != IFILT) {
				goto errout;
			}
			break;
		}
	}
	if (fsetdp->fst_head.fsh_fsindex != fsid) {
		goto errout;
	}

	/*
	 * read the CUT a block at a time
	 */

	if (iget_inode(fsdp, cuinum, &ip, ext) ||
	    (ip.di_mode & IFMT) != IFCUT) {
		goto errout;
	}
	for (off = 0; off < ip.di_size; off += fs->fs_bsize) {
		if (b_readi(fsdp, &ip, fbuf, off, fs->fs_bsize) < 0) {
			goto errout;
		}
		cup = (struct vx_cuent *) fbuf;
		while (cup < (struct vx_cuent *) (fbuf + fs->fs_bsize)) {
			if (cup->cu_fsindex == fsid) {
				memcpy(&fsetdp->fst_cuent, cup, sizeof (*cup));
				break;
			}
			cup++;
		}
	}
	if (fsetdp->fst_cuent.cu_fsindex != fsid) {
		goto errout;
	}
	fset_datfree(attrdp);
	free(fbuf);
	return 0;

errout:	
	fset_datfree(attrdp);
	free(fbuf);
	return -1;
}

/*
 * allocate and fill an array of fileset indices
 */

static int
fset_indices(fsdp)
	struct fs_dat	*fsdp;
{
	off_t	off;
	struct vx_dinode	fsip;
	struct vx_fs		*fs = &fsdp->fsd_fs;
	struct vx_fsethead 	*fsh;
	ino_t	fsinum, cuinum;
	daddr_t	ext[2];
	char	*buf;
	ulong 	*ixp;

	if (olt_read(fsdp, &fsinum, &cuinum, ext) ||
	    iget_inode(fsdp, fsinum, &fsip, ext)) {
		return -1;
	}
	buf = malloc(fs->fs_bsize);
	if (buf == (char *)NULL) {
		return -1;
	}
	
	/*
	 * Make sure that the inode is really a fileset header that size
	 * and blocks match.
	 */
	if ((fsip.di_mode & IFMT) != IFFSH ||
	    (ulong)(fsip.di_size / fs->fs_bsize) != fsip.di_blocks) {
		free(buf);
		return -1;
	}
	fsdp->fsd_nindex = fsip.di_size / fs->fs_bsize;
	fsdp->fsd_index = (ulong *) malloc(fsdp->fsd_nindex * sizeof (ulong));
	if (fsdp->fsd_index == (ulong *)NULL) {
		free(buf);
		fsdp->fsd_nindex = 0;
		return -1;
	}

	/*
	 * read the fileset file a block at a time, saving the indices of
	 * filesets
	 */

	ixp = fsdp->fsd_index;
	off = 0;
	while (off < fsip.di_size) {
		if (b_readi(fsdp, &fsip, buf, off, fs->fs_bsize)) {
			goto errout;
		}
		off += fs->fs_bsize;
		fsh = (struct vx_fsethead *) buf;
		if (fsh->fsh_version == VX_FSET_VERSION &&
		    fsh->fsh_checksum == fset_checksum(fsh)) {
			*ixp++ = fsh->fsh_fsindex;
		}
	}
	free(buf);
	return 0;
errout:
	free(buf);
	free(fsdp->fsd_index);
	fsdp->fsd_index = (ulong *)NULL;
	fsdp->fsd_nindex = 0;
	return -1;
}

/*
 * checksum a fileset structure
 */

static int
fset_checksum(fshd)
	struct vx_fsethead	*fshd;
{
	char	*cp;
	int	i, cksum;

	cksum = fshd->fsh_version +
		fshd->fsh_fsindex +
		fshd->fsh_time +
		fshd->fsh_etime +
		fshd->fsh_fsextop +
		fshd->fsh_ninode +
		fshd->fsh_nau +
		fshd->fsh_ilesize +
		fshd->fsh_dflags +
		fshd->fsh_quota +
		fshd->fsh_maxinode +
		fshd->fsh_iauino +
		fshd->fsh_ilistino[0] +
		fshd->fsh_ilistino[1] +
		fshd->fsh_lctino;
	for (cp = fshd->fsh_name, i = 0; i < FSETNAMESZ; i++, cp++) {
		cksum += *cp;
	}
	return cksum;
}

/*
 * find and read the requested inode into the supplied inode buffer
 * from the initial inode extent in ext
 */

static int
iget_inode(fsdp, inum, ip, ext)
	struct fs_dat	*fsdp;
	ino_t	inum;
	struct vx_dinode *ip;
	daddr_t	ext[];
{
	struct vx_fs	*fs = &fsdp->fsd_fs;
	int 	i, bno;
	char	*buf;

	buf = malloc(fs->fs_bsize);
	if (buf == (char *)NULL) {
		return -1;
	}

	/*
	 * figure out which block to read, we only have
	 * 2 entries in initial extent so choices are limited...
	 */

	i = inum / (fs->fs_iaddrlen * fs->fs_inopb);
	if (i > 1) {
		free(buf);
		return -1;
	}
	bno = (int) ext[i];
	bno += inum / fs->fs_inopb;
	i = inum % fs->fs_inopb;
	fs_lseek(fsdp, bno << fs->fs_bshift, 0);
	if (fs_read(fsdp, buf, fs->fs_bsize) < fs->fs_bsize) {
		free(buf);
		return -1;
	}
	*ip = *(struct vx_dinode *)(buf + (i * fs->fs_dinosize));
	return 0;
}

/*
 * read the olt and extract the useful information
 */

static int
olt_read(fsdp, fino, cino, ext)
	struct fs_dat		*fsdp;
	ino_t			*fino;
	ino_t			*cino;
	daddr_t			ext[];
{
	int			len;
	off_t			off;
	char			*buf;
	struct vx_olthead	*olth;
	union vx_oltent		*oltp;

	len = fsdp->fsd_fs.fs_oltsize << fsdp->fsd_fs.fs_bshift;
	off = (off_t)fsdp->fsd_fs.fs_oltext[0] << fsdp->fsd_fs.fs_bshift;
	buf = malloc(len);
	if (buf == (char *)NULL) {
		return -1;
	}
	fs_lseek(fsdp, off, 0);
	if (fs_read(fsdp, buf, len) < len) {
		free(buf);
		return -1;
	}
	oltp = (union vx_oltent *)buf;
	olth = (struct vx_olthead *)oltp;
	
	/*
	 * Make sure that we have a valid OLT
	 */

	if (olth->olt_magic != VX_OLTMAGIC ||
	    olth->olt_size != sizeof (struct vx_olthead) ||
	    olth->olt_checksum != olt_checksum(buf, len)) {
		free(buf);
		return -1;
	}

	while ((char *)oltp < (buf+len)) {
		switch (oltp->oltcommon.type) {
		case VX_OLTFREE:
		case VX_OLTMAGIC:

			/*
			 * skip these records
			 */

			oltp = (union vx_oltent *)((char *)oltp +
						   oltp->oltcommon.size);
			break;
		
		case VX_OLTCUT:
			*cino = oltp->oltcut.olt_cutino;
			oltp = (union vx_oltent *)((char *)oltp +
						   oltp->oltcommon.size);
			break;
		case VX_OLTFSHEAD:
			*fino = oltp->oltfshead.olt_fsino[0];
			oltp = (union vx_oltent *)((char *)oltp +
						   oltp->oltcommon.size);
			break;
		case VX_OLTILIST:
			ext[0] = oltp->oltilist.olt_iext[0];
			ext[1] = oltp->oltilist.olt_iext[1];
			oltp = (union vx_oltent *)((char *)oltp +
						   oltp->oltcommon.size);
			break;
		}
	}
	free(buf);
	return 0;
}

/*
 * generate the checksum for an olt extent; the first 3 longs are
 * magic, size, and checksum and are not included in the checksum.
 */

static int
olt_checksum(data, len)
	caddr_t	data;
	int	len;
{
	long	chksum = 0;
	long	*lp, *ep;

	ep = (long *)(data + len);
	lp = (long *)data + 3;
	while (lp < ep) {
		chksum += *lp++;
	}
	return chksum;
}

/*
 * read an inode's data into supplied buffer
 */

static int
b_readi(fsdp, ip, buf, off, len)
	struct	fs_dat	*fsdp;
	struct vx_dinode *ip;
	char		*buf;
	off_t		off;
	int		len;
{
	off_t	extoff, endoff, curoff, toff;
	int	size, rc;
	struct vx_extent ext;

	/*
	 * no support for non block size reads at this level
	 */

	if ((off & ~fsdp->fsd_fs.fs_bmask) ||
	    (len & ~fsdp->fsd_fs.fs_bmask)) {
		return -1;
	}
	curoff = 0;
	endoff = off + len;
	while (off < endoff) {
		if (fs_bmap(fsdp, 0, 0, ip, off, &extoff, &ext)) {
			return -1;
		}
		size = (ext.ex_sz << fsdp->fsd_fs.fs_bshift) - extoff;
		toff = extoff + (ext.ex_st << fsdp->fsd_fs.fs_bshift);
		fs_lseek(fsdp, toff, 0);
		if ((rc = fs_read(fsdp, buf + curoff, min(size, len))) < 0) {
			return rc;
		}
		off += rc;
		curoff += rc;
	}
	return 0;
}

/*
 * the real block mapping function
 */

int
fs_bmap(fsdp, ino, fset, ip, offset, exoffp, exp)
	struct fs_dat	*fsdp;
	ino_t 	ino;
	ulong	fset;
	struct vx_dinode	*ip;
	off_t 	offset;
	off_t 	*exoffp;
	struct vx_extent	*exp;
{
	off_t 	coff;
	off_t	want;
	int	i;
	daddr_t de;
	long	des;

	if (offset > ip->di_size || offset < 0) {
		return 1;
	}
	
	coff = 0;
	*exoffp = 0;
	exp->ex_st = exp->ex_sz = 0;
	want = offset >> fsdp->fsd_fs.fs_bshift;
	 
	for (i = 0 ; i < fsdp->fsd_fs.fs_ndaddr; i++) {
		des = ip->di_dext[i].ic_des;
		de = ip->di_dext[i].ic_de;
		if (!des) {
			return 1;
		}
		if (des + coff > want) {
			if (bad_extent(fsdp, de, des)) {
				return 1;
			}
			exp->ex_st = de;
			exp->ex_sz = des;
			*exoffp = offset - (coff << fsdp->fsd_fs.fs_bshift);
			return 0;
		}
		coff += des;
	}

	if (!ip->di_ies) {
		return 1;
	}

	for (i = 0; i < NIADDR; i++) {
		if (bmapindir(fsdp, ino, 0, offset, exoffp, want, &coff, exp,
			      ip->di_ie[i], ip->di_ies, i + 1)) {
			return 1;
		}
		if (exp->ex_st != 0) {
			return 0;
		}
	}

	return 0;
}

/*
 * find offset in indirect extents
 */

static int
bmapindir(fsdp, ino, fset, offset, exoffp, want, coff, exp, bno, esize, lvl)
	struct fs_dat	*fsdp;
	ino_t	ino;
	ulong	fset;
	off_t 	offset;
	off_t 	*exoffp;
	off_t 	want;
	off_t 	*coff;
	struct vx_extent	*exp;
	daddr_t	bno;
	long	esize;
	int 	lvl;
{
	struct vx_fs	*fs = &fsdp->fsd_fs;
	char *bufp = indbuf[lvl - 1].buf;
	daddr_t	*bnop;
	off_t	iwant;
	int	i, nindext;

	if (bad_extent(fsdp, bno, fs->fs_iaddrlen)) {
		return -1;
	}

	nindext = (fs->fs_iaddrlen << fs->fs_bshift) / sizeof(daddr_t);
	iwant = want - *coff;
	if (lvl == 1) {
		if ((iwant / esize) >= nindext) {
			*coff += nindext * esize;
			return 0;
		}
		if (bno != indbuf[lvl - 1].bno) {
			fs_lseek(fsdp, (bno << fs->fs_bshift), 0);
			if (fs_read(fsdp, bufp, IADDREXTSIZE) < 0) {
				return -1;
			}
			indbuf[lvl - 1].bno = bno;
		}
		bnop = (daddr_t *)bufp;
		bnop += iwant / esize;
		*coff += (iwant / esize) * esize;
		exp->ex_st = *bnop;
		exp->ex_sz = esize;
		exp->ex_off = *coff << fs->fs_bshift;
		*exoffp = offset - exp->ex_off;
		return 0;
	}
	
	lvl--;
	if (bno != indbuf[lvl].bno) {
		fs_lseek(fsdp, (bno << fs->fs_bshift), 0);
		if (fs_read(fsdp, bufp, IADDREXTSIZE) < 0) {
			return -1;
		}
		indbuf[lvl].bno = bno;
	}
	bnop = (daddr_t *)bufp;
	bnop += iwant / (nindext * esize);
	*coff += (iwant / (nindext * esize)) * (nindext * esize);
	if (bmapindir(fsdp, ino, fset, offset, exoffp, want, coff, exp,
		      *bnop, esize, lvl))  {
		return -1;
	}

	return 0;
}

/*
 * bad_extent - ensure that extent is in the range of blocks for the filesystem
 */

static int
bad_extent(fsdp, bno, len)
	struct fs_dat	*fsdp;
	daddr_t	bno;
	long	len;
{
	if (len <= 0 || len > fsdp->fsd_fs.fs_aublocks) {
		return 1;
	}
	if (bno >= fsdp->fsd_fs.fs_size) {
		return 1;
	}
	if ((bno + len) > fsdp->fsd_fs.fs_size) {
		return 1;
	}
	return 0;
}	
