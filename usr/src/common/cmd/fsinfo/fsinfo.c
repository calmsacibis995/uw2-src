/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)fsinfo:common/cmd/fsinfo/fsinfo.c	1.2.8.2"
#ident "$Header: fsinfo.c 1.2 91/07/08 $"
/*
 * fsinfo.c
 *
 * Print various filesystem information. Avoids stdio to help in
 * the first-3B2-restore-floppy space crunch.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
/*
#include <sys/vnode.h>
#include <sys/fs/s5inode.h>
*/
#include <sys/fs/sfs_fs.h>
#include <sys/fs/s5ino.h>
#include <sys/fs/s5param.h>
#undef getfs
#undef FsOKAY
#undef FsACTIVE
#include <sys/fs/s5filsys.h>
#include <sys/fs/s5dir.h>
#include <fcntl.h>

/*
 * Definitions.
 */
#define	reg	register		/* Convenience */
#define	uint	unsigned int		/* Convenience */
#define	ulong	unsigned long		/* Convenience */
#define	STDERR	2			/* fileno(stderr) */
#define	STDOUT	1			/* fileno(stdout) */

/*
 * External functions.
 */
void	exit();
long	lseek();
char	*strrchr();

/*
 * Internal functions.
 */
int	fsinfo();
void	prn();
void	prs();
char	*syserr();
void	usage();
int	warn();

/*
 * External variables.
 */
extern int	errno;			/* System error code */
extern char	*sys_errlist[];		/* Table of system error messages */
extern int	sys_nerr;		/* Number of sys_errlist entries */

/*
 * Static variables.
 */
static char	*myname;		/* Last qualifier of arg0 */
static short	fflag;			/* Free block count */
static short	iflag;			/* Total inode count */
static short	sflag;			/* File system sanity */
static short	lflag;			/* Number of free blocks */
static short	sfsflag;		/* SFS file system */

main(ac, av)
int		ac;
reg char	**av;
{
	reg int		errors = 0;

	if (myname = strrchr(*av, '/'))
		++myname;
	else
		myname = *av;
	while (*++av && **av == '-')
		if (strcmp(*av, "-f") == 0)
			++fflag;
		else if (strcmp(*av, "-i") == 0)
			++iflag;
		else if (strcmp(*av, "-s") == 0)
			++sflag;
		else if (strcmp(*av, "-l") == 0)
			++lflag;
		else if (strcmp(*av, "-F") == 0) {
			if (strcmp(*++av, "sfs") == 0)
				sfsflag++;
			else if (strcmp(*av, "s5") != 0)
				usage();
		}
		else
			usage();
	if ((fflag == 0 && iflag == 0 && sflag == 0 && lflag == 0) || *av == 0)
		usage();
	while (*av) 
		if (sfsflag) {
			if (sfs_fsinfo(*av++) < 0)
				++errors;
		} else {
			if (fsinfo(*av++) < 0)
				++errors;
		}
	exit(errors);
}

/*
 * fsinfo()
 *
 * Print the number of 512-byte blocks needed by a given filesystem.
 */
static int
fsinfo(path)
char		*path;
{
	reg daddr_t		bsize;
	reg int			oops;
	reg int			fd;

	auto struct filsys	fs;

	if ((fd = open(path, O_RDONLY)) < 0)
		return (warn(path, syserr()));
	oops = (lseek(fd, SUPERBOFF, 0) < 0
	    || read(fd, (char *) &fs, sizeof(fs)) != sizeof(fs));
	if (oops || fs.s_magic != FsMAGIC)
		return (warn(path, "Not a filesystem"));

        switch (s5bsize(fd, &fs)){
	case Fs1b:
		bsize = 512;
		break;
	case Fs2b:
		bsize = 1024;
		break;
	case Fs4b:
		bsize = 2048;
		break;
	default:
		return (warn(path, "Unknown filesystem type"));
	}
	(void) close(fd);
	if (fflag)
		prn(bsize * (fs.s_fsize - fs.s_tfree) / 512);
	if (iflag)
		prn(bsize * (fs.s_isize - 2) / sizeof(struct dinode));
	if (lflag)
		prn( (bsize * fs.s_tfree) / 512 );
	if (sflag)
		if ((fs.s_state + (long)fs.s_time) != FsOKAY) 
			return(-1);
	return (0);
}

/*
 * sfs_fsinfo()
 *
 * Print the number of 512-byte blocks needed by a given filesystem.
 */
static int
sfs_fsinfo(path)
char		*path;
{

/* temporary hack to get SFS to work for s11 boot. */

	reg daddr_t		bsize;
	reg int			oops;
	reg int			fd;
	int			result;
	auto union {
		struct fs	fs;
		char 		pad[SBSIZE];
	} sblock;

	if ((fd = open(path, O_RDONLY)) < 0)
		return (warn(path, syserr()));
	oops = (lseek(fd, BBSIZE, 0) < 0
	    || read(fd, (char *) &sblock, SBSIZE) != SBSIZE);
	if (oops || sblock.fs.fs_magic != SFS_MAGIC)
		return (warn(path, "Not an SFS filesystem"));
	(void) close(fd);
	if (fflag)
		prn(sblock.fs.fs_size * sblock.fs.fs_fsize / DEV_BSIZE);
	if (iflag)
		prn(sblock.fs.fs_ipg * sblock.fs.fs_ncg);
	if (lflag)
		prn(sblock.fs.fs_fsize / DEV_BSIZE * sblock.fs.fs_fpg * 
			sblock.fs.fs_ncg);
	if (sflag)
		if ((sblock.fs.fs_state + (long)sblock.fs.fs_time) != FSOKAY) 
			return(-1);
	return (0);
}
/*
 * prn()
 *
 * Print a decimal number with a trailing newline.
 */
static void
prn(number)
reg ulong	number;
{
	reg char	*idx;
	auto char	buf[64];

	idx = buf + sizeof(buf);
	*--idx = '\0';
	*--idx = '\n';
	do {
		*--idx = "0123456789abcdef"[number % 10];
		number /= 10;
	} while (number);
	prs(idx);
}

/*
 * prs()
 *
 * Print a string.
 */
static void
prs(str)
reg char	*str;
{
	(void) write(STDOUT, str, (uint) strlen(str));
}

/*
 * syserr()
 *
 * Return a pointer to a system error message.
 */
static char *
syserr()
{
	return (errno <= 0 ? "No error (?)"
	    : errno < sys_nerr ? sys_errlist[errno]
	    : "Unknown error (!)");
}

/*
 * usage()
 *
 * Print a helpful message and exit.
 */
static void
usage()
{
	static char	before[] = "Usage:\t";

	static char	after[] = " [-F fstype -f -i -s -l ] filesystem ...\n";

	(void) write(STDERR, before, (uint) strlen(before));
	(void) write(STDERR, myname, (uint) strlen(myname));
	(void) write(STDERR, after, (uint) strlen(after));
	exit(1);
}

/*
 * warn()
 *
 * Print an error message. Always returns -1.
 */
static int
warn(what, why)
reg char	*what;
reg char	*why;
{
	static char	between[] = ": ";
	static char	after[] = "\n";

	(void) write(STDERR, myname, (uint) strlen(myname));
	(void) write(STDERR, between, (uint) strlen(between));
	(void) write(STDERR, what, (uint) strlen(what));
	(void) write(STDERR, between, (uint) strlen(between));
	(void) write(STDERR, why, (uint) strlen(why));
	(void) write(STDERR, after, (uint) strlen(after));
	return (-1);
}
