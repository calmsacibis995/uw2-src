/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/df/df.c	1.3.6.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fs.d/sfs/df/df.c,v 1.1 91/02/28 17:26:35 ccs Exp $"

/***************************************************************************
 * df
 * Inheritable Privileges: P_MACREAD,P_DACREAD,P_COMPAT,P_DEV
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/

#include <sys/param.h>
#include <sys/types.h>
#include <sys/mntent.h>
#include <sys/errno.h>
#include <sys/vnode.h>
#include <sys/acl.h>
#include <sys/fs/sfs_fs.h>
#include <sys/fs/sfs_inode.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/statvfs.h>
#include <locale.h>
#include <pfmt.h>

#include <stdio.h>
#include <priv.h>
#include <sys/mnttab.h>

void	usage(), pheader();
char	*mpath();
int	aflag = 0;		/* even the uninteresitng ones */
int	bflag = 0;		/* print only number of kilobytes free */
int	eflag = 0;		/* print only number of file entries free */
int	gflag = 0;		/* print entire statvfs structure */
int	iflag = 0;		/* information for inodes */
int	nflag = 0;		/* print VFStype name */
int	tflag = 0;		/* print totals */
int	errflag = 0;
char	*typestr = "sfs";
long	t_totalblks, t_avail, t_used;
int	t_inodes, t_iused, t_ifree;

extern	int	optind;
extern char	*optarg;
extern  int	errno;

union {
	struct fs iu_fs;
	char dummy[SBSIZE];
} sb;
#define sblock sb.iu_fs

/*
 * This structure is used to build a list of mntent structures
 * in reverse order from /etc/mnttab.
 */
struct mntlist {
	struct mnttab *mntl_mnt;
	struct mntlist *mntl_next;
};

struct mntlist *mkmntlist();
struct mnttab *mntdup();

char *subopts [] = {
#define A_FLAG		0
	"a",
#define I_FLAG		1
	"i",
	NULL
};

static const char badopen[] = ":4:Cannot open %s: %s\n";
static const char badstat[] = ":5:Cannot access %s: %s\n";
static const char mnttab[] = MNTTAB;

/*
 * Procedure:     main
 *
 * Restrictions:
                 getopt: none
                 fprintf: none
                 printf: none
                 fopen: P_MACREAD
                 perror: none
                 getmntent: none
                 fclose: none
                 stat(2): none
*/

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct mnttab 		mnt;
	int			opt;
	char	*suboptions,    *value;
	int			suboption;

	(void)setlocale(LC_ALL, "");
        (void)setcat("uxcore.abi");
        (void)setlabel("UX:df sfs");

	while ((opt = getopt (argc, argv, "begko:t")) != EOF) {
		switch (opt) {

		case 'b':		/* print only number of kilobytes free */
			bflag++;
			break;

		case 'e':
			eflag++;	/* print only number of file entries free */
			iflag++;
			break;

		case 'g':
			gflag++;
			break;

		case 'n':
			nflag++;
			break;

		case 'k':
			break;

		case 'o':
			/*
			 * sfs specific options.
			 */
			suboptions = optarg;
			while (*suboptions != '\0') {
				switch ((suboption = getsubopt(&suboptions, subopts, &value))) {

				case I_FLAG:		/* information for inodes */
					iflag++;
					break;

				default:
					usage ();
				}
			}
			break;

		case 't':		/* print totals */
			tflag++;
			break;

		case 'V':		/* Print command line */
			{
				char			*opt_text;
				int			opt_count;

				(void) fprintf (stdout, "df -F sfs ");
				for (opt_count = 1; opt_count < argc ; opt_count++) {
					opt_text = argv[opt_count];
					if (opt_text)
						(void) fprintf (stdout, " %s ", opt_text);
				}
				(void) fprintf (stdout, "\n");
			}
			break;

		case '?':
			errflag++;
		}
	}
	if (errflag)
		usage();
	if (gflag && iflag) {
		pfmt(stdout, MM_ERROR, ":301:'-g' and '-o i' are mutually exclusive\n");
		exit(32);
	}
	if (bflag || eflag)
		tflag = 0;
	sync();
	if (argc <= optind) {
		register FILE *mtabp;
		int	status;
		
		procprivl(CLRPRV,pm_work(P_MACREAD), (priv_t)0);
		if ((mtabp = fopen(MNTTAB, "r")) == NULL) {
			pfmt(stderr, MM_ERROR, badopen, mnttab, strerror(errno));
			exit(32);
		}
		procprivl(SETPRV,pm_work(P_MACREAD), (priv_t)0);
		pheader();
		while ((status = getmntent(mtabp, &mnt)) == NULL) {
			if (strcmp(mnt.mnt_fstype, MNTTYPE_IGNORE) == 0 ||
			    strcmp(mnt.mnt_fstype, MNTTYPE_SWAP) == 0)
				continue;
			if (strcmp(typestr, mnt.mnt_fstype) != 0) {
				continue;
			}
			dfreemnt(mnt.mnt_mountp, &mnt);
		}
		if (tflag)
			if (iflag)
				print_itotals();
			else
				print_totals ();
		(void) fclose(mtabp);
	} else {
		int num = argc ;
		int i ;
		struct mntlist *mntl ;

		pheader();
		aflag++;
		/*
		 * Reverse the list and start comparing.
		 */
		for (mntl = mkmntlist(); mntl != NULL && num ; 
				mntl = mntl->mntl_next) {
		   struct stat dirstat, filestat ;

		   memcpy(&mnt, mntl->mntl_mnt, sizeof (mnt));
		   if (stat(mnt.mnt_mountp, &dirstat)<0) {
			continue ;
		   }
		   for (i = optind; i < argc; i++) {
			if (argv[i]==NULL) continue ;
			if (stat(argv[i], &filestat) < 0) {
				pfmt(stderr, MM_ERROR, badstat, argv[i], strerror(errno));
				argv[i] = NULL ;
				--num;
			} else {
			       if ((filestat.st_mode & S_IFMT) == S_IFBLK ||
			          (filestat.st_mode & S_IFMT) == S_IFCHR) {
					char *cp ;

					cp = mpath(argv[i]);
					if (*cp == '\0') {
						dfreedev(argv[i]);
						argv[i] = NULL ;
						--num;
						continue;
					}
					else {
					  if (stat(cp, &filestat) < 0) {
						pfmt(stderr, MM_ERROR, badstat, argv[i], strerror(errno));
						argv[i] = NULL ;
						--num;
						continue ;
					  }
					}
				}
				if (strcmp(mnt.mnt_fstype, MNTTYPE_IGNORE) == 0 ||
				    strcmp(mnt.mnt_fstype, MNTTYPE_SWAP) == 0)
					continue;
				if ((filestat.st_dev == dirstat.st_dev) &&
				    (strcmp(typestr, mnt.mnt_fstype)==0)) {
					dfreemnt(mnt.mnt_mountp, &mnt);
					argv[i] = NULL ;
					--num ;
				}
			}
		}
	     }
		if (tflag)
			if (iflag)
				print_itotals ();
			else
				print_totals ();
	     if (num) {
		     for (i = optind; i < argc; i++) 
			if (argv[i]==NULL) 
				continue ;
			else
				(void) pfmt(stderr, MM_ERROR,
				":302:Could not find mount point for %s\n", argv[i]) ;
	     }
	}
	exit(0);
	/*NOTREACHED*/
}

/*
 * Procedure:     pheader
 *
 * Restrictions:
                 printf: none
*/
void
pheader()
{
	if (nflag)
		(void) pfmt(stdout, MM_NOSTD, ":1269:VFStype name - sfs\n");
	if (iflag)
		if (eflag)
			(void) pfmt(stdout, MM_NOSTD, ":304:Filesystem            ifree\n");
		else
			(void) pfmt(stdout, MM_NOSTD,":305:Filesystem             iused   ifree  %%iused");
	else {
		if (gflag)
			(void) pfmt(stdout, MM_NOSTD,":306:Filesystem        f_type f_fsize f_bfree f_bavail f_files f_ffree f_fsid f_flag f_fstr\n");
		else
			if (bflag)
				(void) pfmt(stdout, MM_NOSTD,":307:Filesystem            avail\n");
			else {
				(void) pfmt(stdout, MM_NOSTD,":308:Filesystem            kbytes    used   avail capacity");
			}
		}
	if ((!eflag) && (!bflag) && (!gflag))
		(void) pfmt(stdout, MM_NOSTD,":309:  Mounted on\n");
}

/*
 * Procedure:     dfreedev
 *
 * Restrictions:
                 open(2): none
                 fprintf: none
                 perror: none
                 printf: none
 * Report on a block or character special device.  Assumed not to be 
 * mounted.  N.B. checks for a valid SFS superblock.  
 */
dfreedev(file)
	char *file;
{
	long totalblks, availblks, avail, free, used;
	int fi;

	/* P_MACREAD,P_DACREAD - override access, P_DEV - access static dev */
	fi = open(file, 0);
	if (fi < 0) {
		pfmt(stderr, MM_ERROR, badopen, file, strerror(errno));
		return;
	}
	if (bread(file, fi, SBLOCK, (char *)&sblock, SBSIZE) == 0) {
		(void) close(fi);
		return;
	}
	if (sblock.fs_magic != SFS_MAGIC) {
		(void) pfmt(stderr, MM_ERROR, ":1270: %s: not a sfs file system\n", 
		    file);
		(void) close(fi);
		return;
	}
	(void) printf("%-20.20s", file);
	if (iflag) {
		/*
	 	 *	Divide by inodes per file for alternate inodes
		 */
		int inodes = sblock.fs_ncg * sblock.fs_ipg / NIPFILE;
		used = inodes - sblock.fs_cstotal.cs_nifree;
		if (eflag)
			(void) printf("%8ld\n",
			    sblock.fs_cstotal.cs_nifree);
		else
			(void) printf("%8ld%8ld%6.0f%% ", used, sblock.fs_cstotal.cs_nifree,
			    inodes == 0 ? 0.0 : (double)used / (double)inodes * 100.0);
		if (tflag) {
			t_inodes += inodes;
			t_iused += used;
			t_ifree += sblock.fs_cstotal.cs_nifree;
		}
	} else {
		totalblks = sblock.fs_dsize;
		free = sblock.fs_cstotal.cs_nbfree * sblock.fs_frag +
		sblock.fs_cstotal.cs_nffree;
		avail = free;
		availblks = avail ? avail : 0 ;
		used = totalblks - availblks;

		if (bflag) {
			(void) printf("%8d\n",
			    availblks* sblock.fs_fsize / 1024);
		} else {
			(void) printf("%8d%8d%8d",
			    totalblks * sblock.fs_fsize / 1024,
			    used * sblock.fs_fsize / 1024,
			    availblks * sblock.fs_fsize / 1024);
			(void) printf("%6.0f%%",
			    totalblks? (double)used/(double)totalblks * 100.0 : 0.0);
			(void) printf("  ");
		}
		if (tflag) {
			t_totalblks += totalblks * sblock.fs_fsize / 1024;
			t_used += used * sblock.fs_fsize / 1024;
			t_avail += availblks * sblock.fs_fsize / 1024;
		}
	}
	if ((!bflag) && (!eflag))
		(void) printf("  %s\n", mpath(file));
	(void) close(fi);
}

/*
 * Procedure:     dfreemnt
 *
 * Restrictions:
                 statvfs(2): none
                 fprintf: none
                 perror: none
                 printf: none
*/

dfreemnt(file, mnt)
	char *file;
	struct mnttab *mnt;
{
	struct statvfs fs;

	if (statvfs(file, &fs) < 0) {
		pfmt(stderr, MM_ERROR, ":250:statvfs() on %s failed: %s\n",
                        file, strerror(errno));
		return;
	}

	if (fs.f_blocks == 0) {
		return;
	}
	if (strlen(mnt->mnt_special) > 20) {
		(void) printf("%s\n", mnt->mnt_special);
		(void) printf("                    ");
	} else {
		(void) printf("%-20.20s", mnt->mnt_special);
	}
	if (iflag) {
		long files, used;

		files = fs.f_files;
		used = files - fs.f_ffree;
		if (eflag)
			(void) printf("%8ld\n",
			    fs.f_ffree);
		else
			(void) printf("%8ld%8ld%6.0f%% ", used, fs.f_ffree,
			    files == 0? 0.0: (double)used / (double)files * 100.0);
		if (tflag) {
			t_inodes += files;
			t_iused += used;
			t_ifree += fs.f_ffree;
		}
	} else {
		if (gflag) {
			print_statvfs (&fs);
		} else {
			long totalblks, avail, free, used;

			totalblks = fs.f_blocks;
			avail = fs.f_bavail;
			used = totalblks - avail;
			if (avail < 0)
				avail = 0;
				if (bflag) {
					(void) printf("%8d\n",
					    avail * fs.f_frsize / 1024);
				} else {
				(void) printf("%8d%8d%8d", totalblks * fs.f_frsize / 1024,
				    used * fs.f_frsize / 1024, avail * fs.f_frsize / 1024);
				(void) printf("%6.0f%%",
				    totalblks? (double)used/(double)totalblks * 100.0 : 0.0);
			(void) printf("  ");
			if (tflag) {
				t_totalblks += totalblks * fs.f_bsize / 1024;
				t_used += used * fs.f_frsize / 1024;
				t_avail += avail * fs.f_frsize / 1024;
			}
		}
		}
	}
	if ((!bflag) && (!eflag) && (!gflag))
		(void) printf("  %s\n", mnt->mnt_mountp);
}

/*
 * Procedure:     mpath
 *
 * Restrictions:
                 fopen: P_MACREAD
                 fprintf: none
                 perror: none
                 getmntent: none
                 fclose: none

 * Notes:
 * Given a name like /dev/dsk/c1d0s2, returns the mounted path, like /usr.
 */
char *
mpath(file)
	char *file;
{
	FILE *mntp;
	struct mnttab 	mnt;
	int	status;

	procprivl(CLRPRV,pm_work(P_MACREAD), (priv_t)0);
	if ((mntp = fopen(MNTTAB, "r")) == 0) {
		pfmt(stderr, MM_ERROR, badopen, mnttab, strerror(errno));
		exit(32);
	}
	procprivl(SETPRV,pm_work(P_MACREAD), (priv_t)0);

	while ((status = getmntent(mntp, &mnt)) == 0) {
		if (strcmp(file, mnt.mnt_special) == 0) {
			(void) fclose(mntp);
			return (mnt.mnt_mountp);
		}
	}
	(void) fclose(mntp);
	return "";
}

/*
 * Procedure:     bread
 *
 * Restrictions:
                 read(2): none
                 fprintf: none
                 perror: none
*/


long lseek();

int
bread(file, fi, bno, buf, cnt)
	char *file;
	int fi;
	daddr_t bno;
	char *buf;
	int cnt;
{
	register int n;

	(void) lseek(fi, (long)(bno * DEV_BSIZE), 0);
	if ((n = read(fi, buf, cnt)) < 0) {
		/* probably a dismounted disk if errno == EIO */
		if (errno != EIO) {
			pfmt(stderr, MM_ERROR, ":300:Read error in bread() (%x,count = %d): %s\n",
                                bno, n, strerror(errno));
			perror(file);
		} else {
			pfmt(stderr, MM_ERROR, ":311:Premature EOF on %s (%x, expected = %d, count = %d)\n",
                                file, bno, cnt, n);
		}
		return (0);
	}
	return (1);
}

/*
 * Procedure:     xmalloc
 *
 * Restrictions:
                 fprintf: none
*/
char *
xmalloc(size)
	unsigned int size;
{
	register char *ret;
	char *malloc();
	
	if ((ret = (char *)malloc(size)) == NULL) {
		pfmt(stderr, MM_ERROR, ":312:Out of memory: %s\n",
                        strerror(errno));
		exit(32);
	}
	return (ret);
}


/*
 * Procedure:     mntdup
 *
 * Restrictions: None
*/

struct mnttab *
mntdup(mnt)
	register struct mnttab *mnt;
{
	register struct mnttab *new;

	new = (struct mnttab *)xmalloc(sizeof(*new));

	new->mnt_special = (char *)xmalloc((unsigned)(strlen(mnt->mnt_special) + 1));
	(void) strcpy(new->mnt_special, mnt->mnt_special);

	new->mnt_mountp = (char *)xmalloc((unsigned)(strlen(mnt->mnt_mountp) + 1));
	(void) strcpy(new->mnt_mountp, mnt->mnt_mountp);

	new->mnt_fstype = (char *)xmalloc((unsigned)(strlen(mnt->mnt_fstype) + 1));
	(void) strcpy(new->mnt_fstype, mnt->mnt_fstype);

	new->mnt_mntopts = (char *)xmalloc((unsigned)(strlen(mnt->mnt_mntopts) + 1));
	(void) strcpy(new->mnt_mntopts, mnt->mnt_mntopts);

	return (new);
}

/*
 * Procedure:     usage
 *
 * Restrictions:
                 fprintf: None
*/

void
usage()
{

	(void) pfmt(stderr, MM_ACTION,
		":1271:Usage:\ndf [-F sfs] [generic options] [-o i] [directory | special]\n");
	exit(32);
}

/*
 * Procedure:     mkmntlist
 *
 * Restrictions:
                 fopen: none
                 fprintf: none
                 perror: none
                 getmntent: none
                 fclose: none
*/

struct mntlist *
mkmntlist()
{
	FILE *mounted;
	struct mntlist *mntl;
	struct mntlist *mntst = NULL;
	struct mnttab mnt;
	int	status;

	if ((mounted = fopen(MNTTAB, "r"))== NULL) {
		pfmt(stderr, MM_ERROR, badopen, mnttab, strerror(errno));
		exit(32);
	}
	while ((status = getmntent(mounted, &mnt)) == NULL) {
		mntl = (struct mntlist *)xmalloc(sizeof(*mntl));
		mntl->mntl_mnt = mntdup(&mnt);
		mntl->mntl_next = mntst;
		mntst = mntl;
	}
	(void) fclose(mounted);
	return(mntst);
}


/*
 * Procedure:     print_statvfs
 *
 * Restrictions:
                 printf: None
*/

print_statvfs (fs)
	struct statvfs	*fs;
{
	int	i;

	for (i = 0; i < FSTYPSZ; i++)
		(void) printf ("%c", fs->f_basetype[i]);
	(void) printf ("%8d%8d%8d",
	    fs->f_frsize,
	    fs->f_blocks,
	    fs->f_bavail);
	(void) printf ("%8d%8d%8d\n",
	    fs->f_files,
	    fs->f_ffree,
	    fs->f_fsid);
	(void) printf ("0x%x ",
	    fs->f_flag);
	for (i= 0; i < 14; i++)
	(void) printf ("%c", (fs->f_fstr[i] == '\0')? ' ':
	    fs->f_fstr[i]);
	printf("\n");
}

print_totals ()
{
	(void) printf ("Totals              %8d%8d%8d",t_totalblks, t_used, t_avail);
	(void) printf("%6.0f%%\n",
	    t_totalblks ?(double)t_used/(double)t_totalblks * 100.0 : 0.0);
}

/*
 * Procedure:     print_itotals
 *
 * Restrictions:
                 printf: None
*/

print_itotals ()
{
	(void) pfmt(stdout, MM_NOSTD,
		":315:Totals              %8d%8d%6.0f%%\n",
	    t_iused,
	    t_ifree,
	    t_inodes == 0 ? 0.0 : (double)t_iused / (double)t_inodes * 100.0);
}
