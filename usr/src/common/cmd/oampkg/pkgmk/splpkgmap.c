/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgmk/splpkgmap.c	1.15.10.13"
#ident  "$Header: $"

#include	<stdio.h> 
#include	<string.h> 
#include	<limits.h>
#include	<sys/types.h>
#include	<pkgdev.h>
#include	<pkgstrct.h>

#include	<pfmt.h>

extern int	optind, errno;
extern int	sflag;
extern char	*optarg, *errstr;

extern void	*calloc(), *realloc();
extern char	*qstrdup();
extern void	progerr(),
		quit(),
		free();
extern long	atol();
extern int	ppkgmap();


extern struct pkgdev
		pkgdev;

#define	NBPSCTR		512
#define	SCTRSHFT	9

#define PBLK	512	/* 512 byte "physical" block */
#define MALSIZ	500
#define EFACTOR	128L	/* typical size of a single entry in a pkgmap file */

#define ERR_MEMORY	":6:memory allocation failure, errno=%d"
#define ERR_TOOBIG	":519:%s (%ld blocks) does not fit on a volume"
#define ERR_INFOFIRST	":520:information file <%s> must appear on first part"
#define ERR_INFOSPACE	":521:all install files must appear on first part"
#define	ERR_OVERFLOW	":522:part %d: selected files do not fit on volume"
#define ERR_FREE	":474:package does not fit space currently available in <%s>"
#define INDIRECT	10	/*Number of logical blocks before indirection*/

struct data {
	long	blks;
	struct cfent *ept;
};

struct class {
	char *name;
	int first;
	int last;
};

static long	btotal, 	/* blocks stored on current part */
		ftotal, 	/* files stored on current part */
		bmax, 		/* maximum number of blocks on any part */
		fmax, 		/* maximum number of files on any part */
		bpkginfo;	/* blocks used by pkginfo file */
static char	**dirlist;
static int	volno = 0;	/* current part */
static int	nclass;
static ulong 	DIRSIZE;
static struct class
		*cl;

static void	allocnode(),
		addclass(),
		newvolume(),
		sortsize();
static int	store(), nodecount();
static long	nblk();

int
splpkgmap(eptlist, eptnum, order, bsize, plimit, pilimit, pllimit)
struct cfent	**eptlist;
int	eptnum;
char	*order[];
ulong	bsize;
long	*plimit, *pllimit;
int	*pilimit;
{
	struct data	*f, **sf;
	struct cfent	*ept;
	register int	i, j;
	int	flag, errflg, total;
	long	btemp;
	int	uservol = 0;	/* # of volumes specified in prototype */

	f = (struct data *) calloc((unsigned) eptnum, sizeof(struct data));
	if(f == NULL) {
		progerr(ERR_MEMORY, errno);
		quit(99);
	}

	sf = (struct data **) calloc((unsigned) eptnum, sizeof(struct data *));
	if(sf == NULL) {
		progerr(ERR_MEMORY, errno);
		quit(99);
	}

	nclass = 0;
	cl = (struct class *) calloc(MALSIZ, sizeof(struct class));
	if(cl == NULL) {
		progerr(ERR_MEMORY, errno);
		quit(99);
	}

	errflg = 0;

	/*The next bit of code checks to see if, when creating a package
	 *on a directory, there is enough free blocks and inodes before
	 *continuing 
	 */
	total=0;
	DIRSIZE = ((bsize - 1) / PBLK) + 1; /*DIRSIZE takes up 1 logical block*/
	if (!pkgdev.mount) {
		allocnode(NULL);
		if (eptnum+1 > *pilimit) {
			progerr(ERR_FREE, pkgdev.dirname);
			quit(1);
		}
		if(sflag && nblk(eptnum + 1, bsize) > *plimit) {
			progerr(ERR_FREE, pkgdev.dirname);
			quit(1);
		}

		for(i=0; i<eptnum; i++) {
			if(strchr("dxslcbp", eptlist[i]->ftype))
				continue;
			else {
				total += (nodecount(eptlist[i]->path) * DIRSIZE);
				total += nblk(eptlist[i]->cinfo.size, bsize);
				if (total > *plimit && !sflag) {
					progerr(ERR_FREE, pkgdev.dirname);
					quit(1);
				}
				allocnode(eptlist[i]->path);
			}
		}
	}
	/* if there is a value in pllimit, use that 
	 * for the limit from now on
	 */
	if (*pllimit)
		*plimit = *pllimit;
	else if(sflag) /* don't volumize if -s option is used with no -l option
*/
		*plimit = LONG_MAX;



	/* calculate number of physical blocks used by each object */
	for(i=0; i < eptnum; i++) {
		f[i].ept = ept = eptlist[i];
		if(ept->volno > uservol)
			uservol = ept->volno;	/* last volume in prototype */
		addclass(ept->class, 0);
		if(strchr("dxlcbps", ept->ftype))
			f[i].blks = 0; /* virtual object (no contents) */
		else
			f[i].blks = nblk(ept->cinfo.size, bsize);
	}

	/* establish an array sorted by decreasing file size */
	sortsize(f, sf, eptnum);

	/* initialize first volume */
	newvolume(sf, eptnum);

	/* check if pre-selected files fit on volume */
	if(btotal > *plimit) {
		progerr(ERR_OVERFLOW, volno);
		quit(1);
	}
	
	/* reserve room on first volume for pkgmap */
	btotal += 2 * ( nblk(eptnum * EFACTOR, bsize) + 2L ) ;
	ftotal++;

	/* initialize directory info */
	allocnode(NULL);


	/* place installation files on first volume! */
	flag = 0;
	for(j=0; j < eptnum; ++j) {
		if(f[j].ept->ftype != 'i')
			continue;
		if(!strcmp(f[j].ept->path, "pkginfo"))
			bpkginfo = f[j].blks;
		else if(!flag++) {
			/* save room for install directory */
			ftotal++;
			btotal += 2;
		}
		if(!f[j].ept->volno) {
			f[j].ept->volno = 1;
			ftotal++;
			btotal += f[j].blks;
		} else if(f[j].ept->volno != 1) {
			progerr(ERR_INFOFIRST, f[j].ept->path);
			errflg++;
		}
	}

	/*
	 * Any volume must contain the pkginfo file.  Make sure each file
	 * plus the pkginfo file does not fit on a volume.  Reserve 6 blocks
	 * for the package directory and the directories <package>/root and
	 * <package>/reloc.
	 */
	for(j=0; j < eptnum; ++j) {
		btemp = nodecount(f[j].ept->path) * DIRSIZE;
		if(f[j].blks + btemp + bpkginfo + 6L > *plimit) {
			errflg++;
			progerr(ERR_TOOBIG, f[j].ept->path, f[j].blks);
		}
	}
	if(errflg)
		quit(errflg);


	/* place classes listed on command line */
	if(order) {
		for(i=0; order[i]; ++i)  {
			while(store(sf, eptnum, order[i], *plimit, *pilimit))
				; /* stay in loop until store is complete */
		}
	}

	/*
	 * Loop while there are some more volumes.
	 */
	do {
		while(store(sf, eptnum, (char *)0, *plimit, *pilimit))
			; /* stay in loop until store is complete */

		newvolume(sf, eptnum);

		/* check if pre-selected files fit on volume */
		if(btotal > *plimit) {
			progerr(ERR_OVERFLOW, volno);
			quit(1);
		}
	} while (uservol >= volno);

	/* place all virtual objects, e.g. links and spec devices */
	for(i=0; i < nclass; ++i) {
		/* if no objects were associated, attempt to 
		 * distribute in order of class list
		 */
		if(cl[i].first == 0)
			cl[i].last = cl[i].first = (i ? cl[i-1].last : 1);
		for(j=0; j < eptnum; j++) {
			if((f[j].ept->volno == 0) && 
			 !strcmp(f[j].ept->class, cl[i].name)) {
				if(strchr("sl", f[j].ept->ftype))
					f[j].ept->volno = cl[i].last;
				else
					f[j].ept->volno = cl[i].first;
			}
		}
	}

	*plimit = bmax;
	*pilimit = fmax;

	/* free up dynamic space used by this module */
	free(f);
	free(sf);
	for(i=0; i < nclass; ++i)
		free(cl[i].name);
	free(cl);
	for(i=0; dirlist[i]; i++)
		free(dirlist[i]);
	free(dirlist);

	return(errflg ? -1 : volno-1);
}

static long 
nblk(size, bsize)
long	size;
ulong	bsize;
{
	long	tot, count, count1, d_indirect, t_indirect, ind;
	/*Need to keep track of indirect blocks.
	 *However, for convenience, will assume an s5 type
	 *of structure, which should be approximately
	 *correct for most file system types
	 */

	if (size == 0)
		return(1);

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
	return(tot);
}

static int
store(sf, eptnum, class, limit, ilimit)
struct data **sf;
int	eptnum;
char	*class;
long	limit;
int	ilimit;
{
	int	i, svnodes, choice, select;
	long	btemp, ftemp;

	select = 0;
	choice = (-1);
	for(i=0; i < eptnum; ++i) {
		if(sf[i]->ept->volno) 
			continue; /* defer storage until class is selected */
		if(class && strcmp(class, sf[i]->ept->class)) 
			continue;
		if(strchr("sldxcbp", sf[i]->ept->ftype)) {
			sf[i]->ept->volno = (char) volno;
			addclass(sf[i]->ept->class, volno);
			continue;
		}
		select++; /* we need to place at least one object */
		ftemp = nodecount(sf[i]->ept->path);
		btemp = sf[i]->blks + (ftemp * DIRSIZE);
		if(((limit <= 0) || ((btotal + btemp) <= limit)) &&
		  ((ilimit <= 0) || ((ftotal + ftemp) < ilimit))) {
			/* largest object which fits on this volume */
			choice = i;
			svnodes = ftemp;
			break;
		}
	}
	if(!select)
		return(0); /* no more to objects to place */

	if(choice < 0) {
		newvolume(sf, eptnum);
		/* check if pre-selected files fit on volume */
		if(btotal > limit) {
			progerr(ERR_OVERFLOW, volno);
			quit(1);
		}
		return(store(sf, eptnum, class, limit, ilimit));
	}
	sf[choice]->ept->volno = (char) volno;
	ftotal += svnodes + 1;
	btotal += sf[choice]->blks + (svnodes*DIRSIZE);
	allocnode(sf[choice]->ept->path);
	addclass(sf[choice]->ept->class, volno);
	return(++choice); /* return non-zero if more work to do */
}

static void
allocnode(path)
char *path;
{
	register int i;
	int	found;
	char	*pt;

	if(path == NULL) {
		if(dirlist) {
			/* free everything */
			for(i=0; dirlist[i]; i++)
				free(dirlist[i]);
			free(dirlist);
		}
		dirlist = (char **) calloc(MALSIZ, sizeof(char *));
		if(dirlist == NULL) {
			progerr(ERR_MEMORY, errno);
			quit(99);
		}
		return;
	}

	pt = path;
	if(*pt == '/')
		pt++;
	/* since the pathname supplied is never just 
	 * a directory, we store only the dirname of
	 * of the path
	 */
	while(pt = strchr(pt, '/')) {
		*pt = '\0';
		found = 0;
		for(i=0; dirlist[i] != NULL; i++) {
			if(!strcmp(path, dirlist[i])) {
				found++;
				break;
			}
		}
		if(!found) {
			/* insert this path in node list */
			dirlist[i] = qstrdup(path);
			if((++i % MALSIZ) == 0) {
				dirlist = (char **) realloc(dirlist, 
					(i+MALSIZ)*sizeof(char *));
				if(dirlist == NULL) {
					progerr(ERR_MEMORY, errno);
					quit(99);
				}
			}
			dirlist[i] = (char *) NULL;
		}
		*pt++ = '/';
	}
}

static int
nodecount(path)
char *path;
{
	char	*pt;
	int	i, found, count;

	pt = path;
	if(*pt == '/')
		pt++;

	/* we want to count the number of path
	 * segments that need to be created, not
	 * including the basename of the path;
	 * this works only since we are never
	 * passed a pathname which itself is a
	 * directory
	 */
	count = 0;
	while(pt = strchr(pt, '/')) {
		*pt = '\0';
		found = 0;
		for(i=0; dirlist[i]; i++) {
			if(!strcmp(path, dirlist[i])) {
				found++;
				break;
			}
		}
		if(!found)
			count++;
		*pt++ = '/';
	}
	return(count);
}

static void
newvolume(sf, eptnum)
struct data **sf;
{
	register int i;
	int	newnodes;

	if(volno) {
		(void) pfmt(stdout, MM_NOSTD, ":523:part %2d -- %ld blocks, %ld entries\n",
			volno, btotal, ftotal);
		if(btotal > bmax)
			bmax = btotal;
		if(ftotal > fmax)
			fmax = ftotal;
		/*
		 * Reserve blocks and inodes for the following files or
		 * directories: package directory, pkginfo, reloc[.%d],
		 * root[.%d].
		 */
		btotal = bpkginfo + 6L;
		ftotal = 4;
	} else {
		/*
		 * Reserve blocks and inodes for the following files or
		 * directories: package directory, reloc[.%d], root[.%d].
		 */
		btotal = 6L;
		ftotal = 3;
	}
	volno++;

	/* zero out directory storage */
	allocnode((char *)0);

	/* force storage of files whose volume 
         * number has already been assigned
	 */
	for(i=0; i < eptnum; i++) {
		if(sf[i]->ept->volno == volno) {
			newnodes = nodecount(sf[i]->ept->path);
			ftotal += newnodes + 1;
			btotal += sf[i]->blks + (newnodes*DIRSIZE);
			allocnode(sf[i]->ept->path);
			addclass(sf[i]->ept->class, volno);
		}
	}
}

static void
addclass(class, vol)
char *class;
int vol;
{
	int i;

	for(i=0; i < nclass; ++i) {
		if(!strcmp(cl[i].name, class)) {
			if(vol <= 0)
				return;
			if(!cl[i].first || (vol < cl[i].first))
				cl[i].first = vol;
			if(vol > cl[i].last)
				cl[i].last = vol;
			return;
		}
	}
	cl[nclass].name = qstrdup(class);
	cl[nclass].first = vol;
	cl[nclass].last = vol;
	if((++nclass % MALSIZ) == 0) {
		cl = (struct class *) realloc((char *)cl, 
			sizeof(struct class)*(nclass+MALSIZ));
		if(!cl) {
			progerr(ERR_MEMORY, errno);
			quit(99);
		}
	}
	return;
}

static void
sortsize(f, sf, eptnum)
struct data *f, **sf;
int	eptnum;
{
	int	nsf;
	int	i, j, k;

	nsf = 0;
	for(i=0; i < eptnum; i++) {
		for(j=0; j < nsf; ++j) {
			if(f[i].blks > sf[j]->blks) {
				for(k=nsf; k > j; k--) {
					sf[k] = sf[k-1];
				}
				break;
			}
		}
		sf[j] = &f[i];
		nsf++;
	}
}
