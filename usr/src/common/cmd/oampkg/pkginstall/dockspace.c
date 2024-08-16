/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkginstall/dockspace.c	1.7.13.12"
#ident  "$Header: $"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/param.h>
#include <limits.h>
#include <mnttab.h>
#include <pkgstrct.h>
#include <pfmt.h>

extern struct cfent
		**eptlist;
extern char	*basedir;
extern char	**class;

#define PBLK	512	/* 512 byte "physical" block */
#define LSIZE	256
#define NFSYS 10
#define LIM_BFREE	150
#define LIM_FFREE	25
#define WRN_STATVFS	":336:WARNING: unable to stat filesystem mounted on <%s>"
#define INDIRECT	10	/*Number of logical blocks before indirection*/

extern char	*strrchr(),
		*dirname(),
		*malloc();
extern void	progerr(),
		logerr(),
		mappath(),
		basepath();

static int	nfsys = 0;
static int	rootfsys = -1;
static void	warn();
static int	fsys(), readspace(), readmap();
static long	nblk();

static struct tbl {
	char	name[PATH_MAX];
	u_long	bsize;	/* fundamental file system block size */
	u_long	bfree;	/* total # of free blocks */
	u_long	bused;	/* total # of free blocks */
	u_long	ffree;	/* total # of free file nodes */
	u_long	fused;	/* total # of free file nodes */
	dev_t	dev;	/* device id of filesystem */
} *table;

static int error, repeat;

int
dockspace(spacefile)
char	*spacefile;
{
	struct statvfs statvfsbuf;
	struct stat statbuf;
	FILE	*fp;
	char	*path;
	long	bfree, ffree, frsize;
	int	i, lastfsys, count;
	struct	mnttab mntent;


	if((table = (struct tbl *)malloc(NFSYS * sizeof(struct tbl))) == (struct tbl *)NULL) {
		progerr(":338:unable to malloc space\n");
		return -1;
	}
	nfsys = error = 0;
	repeat = 1;

	if(readmap() < 0)
		return -1;
	if((spacefile) && (readspace(spacefile) < 0))
		return -1;
	
	if((fp=fopen("/etc/mnttab", "r")) == NULL) {
		progerr(":777:unable to open /etc/mnttab");
		return -1;
	}

	count = 0;
	lastfsys = i = 0;

	while(getmntent(fp, &mntent) == 0) {
		if(stat(mntent.mnt_mountp, &statbuf)) {
			continue;
		}
		for(i = 0; i< nfsys; i++)
			if(statbuf.st_dev == table[i].dev) {
				(void) strcpy(table[i].name, mntent.mnt_mountp);
				count++;
				break;
			}
	}

	if(fclose(fp) || count < nfsys - 1) {
		progerr(":778:unable to determine mounted filesystems");
		return -1;
	}
	for(i=0; i < nfsys; ++i) {
		if((!table[i].fused) && (!table[i].bused))
			continue; /* not used by us */
		bfree = (long) table[i].bfree - (long) table[i].bused;
		ffree = (long) table[i].ffree - (long) table[i].fused;
		if(bfree < LIM_BFREE) {
			warn(gettxt(":340", "blocks"), table[i].name, 
				table[i].bused + LIM_BFREE, table[i].bfree);
			error++;
		}
		if(ffree < LIM_FFREE) {
			warn(gettxt(":341", "file nodes"), table[i].name, 
				table[i].fused + LIM_FFREE, table[i].ffree);
			error++;
		}
	}
	return error;
}

static void
warn(type, name, need, avail)
char *type, *name;
ulong need, avail;
{
	extern	int logmode;
	int	reset = 0;

	if(logmode) {
		reset++;
		logmode = 0;
	}

	logerr(":52:WARNING:");
	logerr(":342:%lu free %s are needed in the %s filesystem,",
		 need, type, name);
	logerr(":343:but only %lu %s are currently available.", avail, type);
	if(reset)
		logmode++;
}

/* cache of last directory */
static char lastpath[PATH_MAX];
static int lastfsys;

static int
fsys(ept, statbuf)
struct cfent *ept;
struct stat *statbuf;
{
	register int i;
	int n, ret = -1;
	char pathname[PATH_MAX], *path;
	struct stat statbuf2;
	struct statvfs statvfsbuf;
	long frsize;
	
	path = pathname;
	strcpy(path, ept->path);
	if(!statbuf) {
		/* file doesn't exist, check if dirname matches cache first */
		path = dirname(path);
		if(strcmp(path, lastpath) == 0) {
			if(ept->ftype == 'd' || ept->ftype == 'x')
				strcpy(lastpath, ept->path);
			return lastfsys;
		}
		while(strcmp(path, "/") != 0) {
			if(stat(path, &statbuf2) == 0) {
				statbuf = &statbuf2;
				break;
			}
			if(errno != ENOENT)
				return -1;
			path = dirname(path);
		}
		if(!statbuf) {
			/* file is under root filesystem */
			if(rootfsys >= 0)
				ret = rootfsys;
			else {
				rootfsys = nfsys;
				(void) stat("/", &statbuf2);
				statbuf = &statbuf2;
			}
		}
	}

	for(i = lastfsys; ret < 0 && i < nfsys; i++) {
		if(table[i].dev == statbuf->st_dev) 
			ret = i;
	}

	for(i = 0; ret < 0 && i < lastfsys; i++) {
		if(table[i].dev == statbuf->st_dev) {
			ret = i; 
		}
	}

	if(ret < 0) {
		/* not in existing filesystem entry */
		if(statvfs(path, &statvfsbuf)) {
			logerr(WRN_STATVFS, path);
			error++;
		}
		if(nfsys == (repeat * NFSYS)) {
			repeat++;
			if((table = (struct tbl *)realloc(table, NFSYS * repeat * sizeof(struct tbl))) == (struct tbl *)NULL) {
				progerr(":338:unable to malloc space\n");
				return -1;
			}
		}	
		ret = nfsys;
		table[nfsys].dev = statbuf->st_dev;
		(void) strcpy(table[nfsys].name, path);
		/* statvfs returns number of fragment size blocks
		 * so will change this to number of 512 byte blocks
		 */
		table[nfsys].bfree = statvfsbuf.f_bavail;
		frsize = statvfsbuf.f_frsize;
		table[nfsys].bfree *= (((frsize - 1) / PBLK) + 1);
		table[nfsys].ffree = statvfsbuf.f_favail;
		table[nfsys].bsize = frsize;
		table[nfsys].bused = (u_long) 0;
		table[nfsys].fused = (u_long) 0;
		nfsys++;
	}

	if(ept->ftype == 'd' || ept->ftype == 'x') {
		strcpy(lastpath, ept->path);
	} else {
		strcpy(pathname, ept->path);
		strcpy(lastpath, dirname(pathname));
	}
	lastfsys = ret;
	return ret;
}

static int
readmap()
{
	struct cfent *ept;
	struct stat statbuf;
	long	blk;
	int	i, n;

	for(i=0; (ept = eptlist[i]) != NULL; i++) {
		if(ept->ftype == 'i')
			continue;
		if( ept->ftype == 's' ? lstat(ept->path, &statbuf) : stat(ept->path, &statbuf)) {
			/* path cannot be accessed */
			if((n = fsys(ept, (struct stat *)0)) < 0)
				return n;
			table[n].fused++;
			if(strchr("dxlspcb", ept->ftype))
				blk = nblk((long)table[n].bsize,table[n].bsize);
			else if((ept->ftype != 'e') && 
			(ept->cinfo.size != BADCONT))
				blk = nblk(ept->cinfo.size, table[n].bsize);
			else
				blk = 0;
		} else {
			/* path already exists */
			if((n = fsys(ept, &statbuf)) < 0)
				return n;
			if(strchr("dxlspcb", ept->ftype))
				blk = 0;
			else if((ept->ftype != 'e') && 
			(ept->cinfo.size != BADCONT)) {
				blk = nblk(ept->cinfo.size, table[n].bsize);
				blk -= nblk(statbuf.st_size, table[n].bsize);
				/* negative blocks show room freed, but since
				 * order of installation is uncertain show
				 * 0 blocks usage 
				 */
				if(blk < 0)
					blk = 0;
			} else
				blk = 0;
		}
		table[n].bused += blk;
	}
	return 0;
}

static long 
nblk(size, bsize)
long	size;
ulong	bsize;
{
	long	tot, count, count1, d_indirect, t_indirect, ind;

	if(size == 0)
		return 1;

	/*Need to keep track of indirect blocks.
	 *However, for convenience, will assume an s5 type
	 *of structure, which should be approximately
	 *correct for most file system types
	 */
	ind=(bsize + 1)/sizeof(daddr_t);
	d_indirect=ind + INDIRECT; 	/*double indirection*/
	t_indirect=ind*(ind + 1) + INDIRECT;	/*triple indirection*/

	tot = ((size - 1) / bsize) + 1; 
	if (tot > t_indirect) {
		count1 = (tot - ind*ind - (INDIRECT + 1))/ind;
		count = count1 + count1/ind + ind + 3;
	}
	else if (tot > d_indirect) 
		count = (tot - (INDIRECT + 1))/ind + 2;
	     else if (tot > INDIRECT)
			count = 1;
	     	  else
			count = 0;	


	/*Accounting for the indirect blocks, the total becomes*/
	tot += count;

	/* calculate number of 512 byte blocks */
	tot *= (((bsize-1)/PBLK)+1);
	return tot;
}

static int
readspace(spacefile)
char	*spacefile;
{
	FILE	*fp;
	char	*pt, path[256], line[LSIZE];
	long	blocks = 0, nodes = 0;
	int	n;
	struct cfent entry;
	struct stat statbuf;


	if(spacefile == NULL)
		return 0;

	if((fp=fopen(spacefile, "r")) == NULL) {
		progerr(":344:unable to open spacefile %s", spacefile);
		return -1;
	}

	entry.ftype = 'd';

	while(fgets(line, LSIZE, fp)) {
		for(pt=line; isspace(*pt);)
			pt++;
		if((*line == '#') || !*line)
			continue;

		(void) sscanf(line, "%s %ld %ld", path, &blocks, &nodes); 
		mappath(2, path);
		(void)basepath(path, basedir);

		entry.path = path;
		if(stat(path, &statbuf))
			n = fsys(&entry, (struct stat *)0);
		else
			n = fsys(&entry, &statbuf);
		table[n].bused += blocks;
		table[n].fused += nodes;
	}
	(void) fclose(fp);
	return 0;
}
