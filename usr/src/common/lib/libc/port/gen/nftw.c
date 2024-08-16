/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/nftw.c	1.19"
/*LINTLIBRARY*/
/***************************************************************
 *	nftw - new file tree walk
 *
 *	int nftw(char *path, int (*fn)(), int depth, int flags);
 *
 *	Derived from System V ftw() by David Korn
 *
 *	nftw visits each file and directory in the tree starting at
 *	path. It uses the generic directory reading library so it works
 *	for any file system type.  The flags field is used to specify:
 *		FTW_PHYS  Physical walk, does not follow symblolic links
 *			  Otherwise, nftw will follow links but will not
 *			  walk down any path the crosses itself.
 *		FTW_MOUNT The walk will not cross a mount point.
 *		FTW_DEPTH All subdirectories will be visited before the
 *			  directory itself.
 *		FTW_CHDIR The walk will change to each directory before
 *			  reading it.  This is faster but core dumps 
 *			  may not get generated.
 *
 *	fn is called with four arguments at each file and directory.
 *	The first argument is the pathname of the object, the second
 *	is a pointer to the stat buffer and the third is an integer
 *	giving additional information as follows:
 *
 *		FTW_F	The object is a file.
 *		FTW_D	The object is a directory.
 *		FTW_DP	The object is a directory and subdirectories
 *			have been visited.
 *		FTW_SL	The object is a symbolic link.
 *		FTW_SLN The object is a symbolic link pointing at a   
 *		        non-existing file.
 *		FTW_DNR	The object is a directory that cannot be read.
 *			fn will not be called for any of its descendants.
 *		FTW_NS	Stat failed on the object because of lack of
 *			appropriate permission. The stat buffer passed to fn
 *			is undefined.  Stat failure for any reason is
 *			considered an error and nftw will return -1. 
 *	The fourth argument is a struct FTW* which contains the depth
 *	and the offset into pathname to the base name.
 *	If fn returns nonzero, nftw returns this value to its caller.
 *
 *	depth limits the number of open directories that ftw uses
 *	before it starts recycling file descriptors.  In general,
 *	a file descriptor is used for each level.
 *
 **************************************************************/

#ifdef __STDC__
	#pragma weak nftw = _nftw
#endif
#include "synonyms.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<dirent.h>
#include	<errno.h>
#include	<limits.h>
#include	<ftw.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<stdlock.h>
#include 	"_thr_funcs.h"

#ifdef _REENTRANT
static StdLock nftw_lock;
#endif

#ifndef PATH_MAX
#define PATH_MAX	1023
#endif

static int walk();
static int oldclose();

#if 0
#ifdef __STDC__
static int (*statf)(const char *, struct stat *);
#else
static int (*statf)();
#endif
static char *fullpath;
static char *tmppath;
static int curflags;
static dev_t cur_mount;
#ifdef DSHLIB
static struct FTW *state;
#else
static struct FTW st_state;
static struct FTW *state = &st_state;
#endif
#endif   /* if 0  */

struct nftw {
	int (*statf)();
	char *fullpath;
	char *tmppath;
	int curflags;
	dev_t cur_mount;
	struct FTW *state;
};

#ifndef _REENTRANT
static struct nftw  **__nftw_data;
#endif

#define NFTWPTR		nftw_ptr

struct Save
{
	struct Save *last;
	DIR	*fd;
	char	*comp;
	long	here;
	dev_t	dev;
	ino_t	inode;
};

int nftw(path, fn, depth, flags)
const char *path;
int depth;
register int flags;
int (*fn)();
{
	struct stat statb;
	char home[2*(PATH_MAX+1)];
	register int rc = -1;
	register char *dp;
	char *base;
	char *endhome;
	const char *savepath = path;
	struct nftw nftw_buf;

	home[0] = 0;

	if ((nftw_buf.state = (struct FTW *)malloc(sizeof(struct FTW))) == NULL)
                return(-1);

	/* If the walk is going to change directory before
	 * reading it, save current woring directory.
	 */
	if(flags&FTW_CHDIR)
		if(getcwd(home,PATH_MAX+1)==0)
			return(-1);
	endhome = dp = home + strlen(home);
	if(*path=='/')
		nftw_buf.fullpath = dp;
	else
	{
		*dp++ = '/';
		nftw_buf.fullpath = home;
	}
	nftw_buf.tmppath =  dp;
	base = dp-1;
	while(*path && dp < &nftw_buf.tmppath[PATH_MAX])
	{
		if(*path=='/')
			base = dp;
		*dp++ = *path++;
	}
	*dp = 0;
	nftw_buf.state->base = base+1-nftw_buf.tmppath;
	if(*path)
	{
		errno = ENAMETOOLONG;
		return(-1);
	}
	nftw_buf.curflags = flags;

	/* If doing a physical walk (not following symbolic link),
	 * set statf to _lxstat(). Otherwise, set statf to _xstat().
	 */
	if((flags&FTW_PHYS)==0)
		nftw_buf.statf = _xstat;
	else
		nftw_buf.statf = _lxstat;

	/* If walk is not going to cross a mount point,
	 * save the current mount point.
	 */
	if(flags&FTW_MOUNT)
	{
		if((nftw_buf.statf)(_STAT_VER, savepath, &statb) >= 0)
			nftw_buf.cur_mount = statb.st_dev;
		else
			goto done;
	}
	nftw_buf.state->level = 0;

	/* Call walk() which does most of the work.
	 */
	STDLOCK(&nftw_lock);
	rc = walk(dp,fn,depth,(struct Save*)0, &nftw_buf);
done:
	*endhome = 0;
	if(flags&FTW_CHDIR)
		chdir(home);
	STDUNLOCK(&nftw_lock);
	return(rc);
}

/*
 * close the oldest directory.  It saves the seek offset.
 * return value is 0 unless it was unable to close any descriptor
 */

static int oldclose(sp)
register struct Save *sp;
{
	register struct Save *spnext;
	while(sp)
	{
		spnext = sp->last;
		if(spnext==0 || spnext->fd==0)
			break;
		sp = spnext;
	}
	if(sp==0 || sp->fd==0)
		return(0);
	sp->here = telldir(sp->fd);
	closedir(sp->fd);
	sp->fd = 0;
	return(1);
}
	

/*
 *This is the recursive walker.
 */
static int walk(component, fn, depth, last, NFTWPTR)
char *component;
int (*fn)();
int depth;
struct Save *last;
struct nftw *NFTWPTR;
{
	struct stat statb;
	register char *p;
	register int type;
	register char *comp;
	register struct dirent *dir;
	register char *q;
	int rc = 0;
	int cdval = -1;
	int oldbase;
	int skip;
	struct Save this;

	this.last = last;
	this.fd = 0;
	if((NFTWPTR->curflags&FTW_CHDIR) && last)
		comp = last->comp;
	else
		comp = NFTWPTR->tmppath;

	/* Determine the type of the component.
	*/
	if((NFTWPTR->statf)(_STAT_VER, comp, &statb) >= 0)
	{
		if((statb.st_mode & S_IFMT) == S_IFDIR)
		{
			type = FTW_D;
			if(depth <= 1)
				oldclose(last);
			if((this.fd = opendir(comp))==0)
			{
				if(errno==EMFILE && oldclose(last) &&
					(this.fd = opendir(comp)))
				{
					depth = 1;
				}
				else {
					type = FTW_DNR;
					goto fail;
				}
			}
								/*
								 * guarantee successful
								 * chdir. Useful for find
								 * command. FTW_NS in this
								 * case is the best of the
								 * possible flags to pass
								 * back, but not optimal.
								 */
			if ((NFTWPTR->curflags&FTW_CHDIR) &&
			    ((statb.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) !=
			    (S_IXUSR|S_IXGRP|S_IXOTH)) &&
			    access(comp, X_OK | EFF_ONLY_OK) == -1)
			{
				type = FTW_NS;
				goto fail;
			}
		}
#ifdef S_IFLNK
		else if((statb.st_mode & S_IFMT) == S_IFLNK)
			type = FTW_SL;
#endif
		else
			type = FTW_F;
	}
	else
	{
		/* Statf has failed. If stat was used instead of lstat,
		 * try using lstat. If lstat doesn't fail, "comp"
		 * must be a symbolic link pointing to a non-existent
		 * file. Such a symbolic link should be ignored.
		 * Also check the file type, if possible, for symbolic
		 * link.
		 */

		if ((NFTWPTR->statf == _xstat) && (lstat(comp, &statb) >= 0) 
#ifdef S_IFLNK
		   && ((statb.st_mode & S_IFMT) == S_IFLNK) 
#endif
		   ) {

			/* Ignore bad symbolic link, let "fn"
			 * report it.
			 */

			errno = ENOENT;
			type = FTW_SLN;
		}
		else {
		
			type = FTW_NS;
	fail:
			if(errno != EACCES)
				return(-1);
		}
	}

	/* If the walk is not supposed to cross a mount point,
	 * and it did, get ready to return.
	 */
	if((NFTWPTR->curflags&FTW_MOUNT) && type!=FTW_NS && statb.st_dev!=NFTWPTR->cur_mount)
		goto quit;
	NFTWPTR->state->quit = 0;

	/* If current component is not a directory, call user
	 * specified function and get ready to return.
	 */
	if(type!=FTW_D || (NFTWPTR->curflags&FTW_DEPTH)==0)
		rc = (*fn)(NFTWPTR->tmppath, &statb, type, NFTWPTR->state);
	skip = (NFTWPTR->state->quit&FTW_SKD);
	if(rc != 0 || type !=FTW_D || NFTWPTR->state->quit&FTW_PRUNE)
		goto quit;

	if(NFTWPTR->tmppath[0] != '\0' && component[-1] != '/')
		*component++ = '/';
	if(NFTWPTR->curflags&FTW_CHDIR)
	{
		*component = 0;
		if((cdval = chdir(comp)) >= 0)
			this.comp = component;
	}

	/* If the walk has followed a symbolic link, traverse
	 * the walk back to make sure there is not a loop.
	 */
	if((NFTWPTR->curflags&FTW_PHYS)==0)
	{
		register struct Save *sp = last;
		while(sp)
		{
			/* If the same node has already been visited, there
			 * is a loop. Get ready to return.
			 */
			if(sp->dev==statb.st_dev && sp->inode==statb.st_ino)
				goto quit;
			sp = sp->last;
		}
	}
	this.dev = statb.st_dev;
	this.inode = statb.st_ino;
	oldbase = NFTWPTR->state->base;
	NFTWPTR->state->base = component-NFTWPTR->tmppath;
	while((this.fd) && (dir = readdir(this.fd)))
	{
		if(dir->d_ino == 0)
			continue;
		q = dir->d_name;
		if(*q == '.')
		{
			if(q[1]==0)
				continue;
			else if(q[1]=='.' && q[2]==0)
				continue;
		}
		p = component;
		while(p < &NFTWPTR->tmppath[PATH_MAX] && *q != '\0')
			*p++ = *q++;

		if (p >= &NFTWPTR->tmppath[PATH_MAX])
		{
			errno = ENAMETOOLONG;
			return -1;
		}

		*p = '\0';
		NFTWPTR->state->level++;

		/* Call walk() recursively.
		 */
		rc = walk(p, fn, depth-1, &this, NFTWPTR);
		NFTWPTR->state->level--;
		if(rc != 0)
			goto quit;
		if(this.fd == 0)
		{ 
			char *toopen=((NFTWPTR->curflags&FTW_CHDIR)?NFTWPTR->fullpath:comp);
			*component = 0;
			if((this.fd = opendir(toopen))==0)
				return(-1);
			seekdir(this.fd, this.here);
		}
	}
	NFTWPTR->state->base = oldbase;
	*--component = 0;	/* may overwrite path == "/" */
	type = FTW_DP;
	if((NFTWPTR->curflags&(FTW_DEPTH)) && !skip)
	{
		if (NFTWPTR->tmppath[0] == 0) /* if overwrote '/' */
		{
			NFTWPTR->tmppath[0] = '/';
			NFTWPTR->tmppath[1] = 0;
		}
		rc = (*fn)(NFTWPTR->tmppath, &statb, type, NFTWPTR->state);
	}
quit:
	if(cdval >= 0 && last)
	{
		/* try to change back to previous directory */
		if((cdval = chdir("..")) >= 0)
		{
			if((NFTWPTR->statf)(_STAT_VER, ".",&statb)<0
			|| statb.st_ino!=last->inode
			|| statb.st_dev!= last->dev)
				cdval = -1;
		}
		*comp = 0;
		if(cdval <0 && chdir(NFTWPTR->fullpath) < 0)
			rc = -1;

	}
	if(this.fd)
		closedir(this.fd);
	return(rc);
}
