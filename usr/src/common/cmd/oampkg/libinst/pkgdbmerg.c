/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



/*LINTLIBRARY*/
#ident	"@(#)oampkg:common/cmd/oampkg/libinst/pkgdbmerg.c	1.18.11.11"
#ident  "$Header: $"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/errno.h>
#include <pkginfo.h>
#include <limits.h>
#include <sys/types.h>
#include <pkgstrct.h>
#include "install.h"
#include <sys/stat.h>

#include <pfmt.h>

#define LSIZE	256
#define ERR_OUTPUT	"uxpkgtools:72:unable to update contents file"
#define ERR_MEMORY	"uxpkgtools:6:memory allocation failure, errno=%d"
#define	ERR_PINFO	"uxpkgtools:82:missing pinfo structure for <%s>"

extern char	*pkginst, *errstr, errbuf[];
extern int	nosetuid, 
		nocnflct,
		errno,
		exist_pkginst,
		otherstoo,
		usedbycore;

extern struct pinfo	
		*eptstat();
extern void	*calloc();
extern void	echo(),	
		progerr(),
		logerr(),
		quit(),
		free();
extern int	alarm(),
		access(),
		srchcfile(),
		putcfile(),
		averify(),
		cverify();

char dbst = '\0';
int installed;

static int	errflg = 0;
static int eptnum;
static FILE *fpproc;
static long sizetot;
static int seconds;
static struct pinfo	
		*pkgpinfo = (struct pinfo *)0;

static int	is_setuid(),
		is_setgid(),
		merg(),
		do_like_entry(),
		do_new_entry(),
		typechg();
static void	set_change(), chgclass(), output();

/*ARGSUNUSED*/
void
notice(n)
int n;
{
	(void) signal(SIGALRM, SIG_IGN);
	if(sizetot)
		echo("uxpkgtools:83:   %2d%% of information processed; continuing ...", 
		ftell(fpproc) * 100L / sizetot);
	(void) signal(SIGALRM, notice);
	alarm(seconds);
}

/*ARGSUSED*/
int
pkgdbmerg(mapfp, tmpfp, eptlist, mstat, notify)
FILE	*mapfp, *tmpfp;
struct cfent **eptlist;
struct mergstat	*mstat;
int notify;
{
	struct cfent *inp;
	struct cfent entry;
	int	n, changed;

	entry.pinfo = (NULL);
	errflg = 0;
	eptnum = 0;
	installed = changed = 0;

	fpproc = mapfp;
	if(notify) {
		seconds = notify;
		(void) signal(SIGALRM, notice);
		(void) alarm(seconds);
	}
	
	sighold(SIGALRM);
	fseek(mapfp, 0L, 2); /* seek to end of file */
	sizetot = ftell(mapfp); /* store number of bytes in open file */
	fseek(mapfp, 0L, 0); /* rewind */
	fseek(tmpfp, 0L, 0); /* rewind */
	sigrelse(SIGALRM);

	for(;;) {
		sighold(SIGALRM);
		/* get next package entry to match */
		while(inp = eptlist[eptnum]) {
			if(!strchr("in", inp->ftype))
				break;
			eptnum++;
		}

		/* get next entry from contents file */
		n = srchcfile(&entry, inp ? inp->path : NULL, mapfp, tmpfp);
		if(n < 0) {
			logerr("uxpkgtools:84:bad entry read from contents file");
			if (entry.path)
				logerr("uxpkgtools:85:- pathname: %s", entry.path);
			logerr("uxpkgtools:86:- problem: %s", errstr);
			return(-1);
		} else if(n == 0) {
			break; /* EOF */
		} else if(n == 1) {
			/* both entries have like pathnames */
			if(do_like_entry(tmpfp, &mstat[eptnum], 
				eptlist[eptnum], &entry))
				changed++;
			eptnum++;
		} else {
			if(do_new_entry(tmpfp, &mstat[eptnum], eptlist[eptnum]))
				changed++;
			eptnum++;
		}
		sigrelse(SIGALRM);
	}

	while(eptlist[eptnum]) {
		sighold(SIGALRM);
		if(!strchr("in", eptlist[eptnum]->ftype)) {
			if(do_new_entry(tmpfp, &mstat[eptnum], eptlist[eptnum]))
				changed++;
		}
		eptnum++;
		sigrelse(SIGALRM);
	}

	if(notify) {
		(void) alarm(0);
		(void) signal(SIGALRM, SIG_IGN);
	}

	(void) fflush(tmpfp);
	return(errflg ? -1 : changed);
}

static int
do_like_entry(fpo, mstat, inp, entry)
FILE	*fpo;
struct mergstat *mstat;
struct cfent *entry, *inp;
{
	int	stflag, ignore, changed;

	ignore = changed = 0;
	pkgpinfo = eptstat(entry, pkginst, '#');

	/*
	 * If the package instance with which this pathname is
	 * associated was not previously listed in the contents
	 * entry, mark this as a change.  This is so that even 
	 * if no attributes are different, the new pkginst to
	 * be associated with the pathname will be added to the
	 * contents file entry for this pathname.
	 */
	if(!exist_pkginst)
		changed++;

	stflag = pkgpinfo->status;

	if(otherstoo)
		mstat->shared++;

	if(inp->ftype == '-') {
		if(!errflg) {
			pkgpinfo = eptstat(entry, pkginst, '-');
			if(putcfile(entry, fpo)) {
				progerr(ERR_OUTPUT);
				quit(99);
			}
		}
		return(1);
	}

	if(is_setuid(inp) || is_setuid(entry))
		mstat->setuid++;
	if(is_setgid(inp) || is_setgid(entry))
		mstat->setgid++;

	if(!pkgpinfo) {
		progerr(ERR_PINFO, entry->path);
		quit(99);
	}

	if((nocnflct && mstat->shared) ||
	   (nosetuid && (mstat->setgid || mstat->setuid))) {
		/* do not allow installation if nocnflct is set and
		 * other packages reference this pathname, or if
		 * nosetuid is set and this is a set[ug]id process
		 */
		char *save_path = inp->path;
		*inp = *entry; /* structure copy to non-volatile memory */
		/*
		 * structure copy: do not forget to duplicate strings
		 */
		inp->path = save_path;
		ignore++;
	} else if(merg(mstat, inp, entry))
		changed++;

	/* inp structure now contains updated entry */

	if(!mstat->contchg && !ignore) {
		/* we know the DB entry matches the pkgmap, so now
		 * we need to see if object really matches the DB entry
		 */
		set_change(inp, mstat);
	}

	if(!errflg) { 
		if(mstat->contchg)
			pkgpinfo = eptstat(inp, pkginst, (dbst ? dbst : '*'));
		else if(mstat->attrchg)
			pkgpinfo = eptstat(inp, pkginst, (dbst ? dbst : '~'));
		else if(stflag != '#') {
			pkgpinfo = eptstat(inp, pkginst, '\0');
			changed++;
		}
		output(fpo, inp, entry, pkgpinfo);
	}

	if(pkgpinfo->aclass[0])
		(void) strcpy(inp->class, pkgpinfo->aclass);
	
	/* free up list of packages which reference this entry */
	entry->pinfo = inp->pinfo;
	inp->pinfo = NULL;
	if(!mstat->attrchg && !mstat->contchg)
		installed++;
	return(changed);
}

static int
do_new_entry(fpo, mstat, inp)
FILE	*fpo;
struct mergstat *mstat;
struct cfent *inp;
{
	struct pinfo *pinfo;

	if(inp->ftype == '-')
		return(0);

	if(access(inp->path, 0) == 0) {
		/* path exists, and although its not referenced by 
		 * any package we make it look like it is so it appears
		 * as a conflicting file in case the user doesn't want 
		 * it installed
		 */
		mstat->shared++;
		set_change(inp, mstat);
	} else {
		/* since path doesn't exist, we're changing everything */
		mstat->contchg++;
		mstat->attrchg++;
	}

	if(is_setuid(inp)) 
		mstat->setuid++;
	if(is_setgid(inp)) 
		mstat->setgid++;

	if((nocnflct && mstat->shared) ||
	   (nosetuid && (mstat->setgid || mstat->setuid))) {
		/* do not allow installation if nocnflct is set and
		 * other packages reference this pathname, or if
		 * nosetuid is set and this is a set[ug]id process;
		 * since this entry is new, we don't ouput anything
		 * to the database we're building
		 */
		return(0);
	}

	if(!errflg) {
		inp->npkgs = 1;
		pinfo = (struct pinfo *)calloc(1, sizeof(struct pinfo));
		if(!pinfo) {
			progerr(ERR_MEMORY, errno);
			quit(99);
		}
		inp->pinfo = pinfo;
		(void) strncpy(pinfo->pkg, pkginst, 14);
		pinfo->status = dbst ? dbst : '*';
		output(fpo, inp, (struct cfent *)0, pinfo);
		free(pinfo);
		inp->pinfo = NULL;
	}
	if(!mstat->attrchg && !mstat->contchg)
		installed++;
	return(1);
}

static void
set_change(p, mstat)
struct cfent *p;
struct mergstat *mstat;
{
	int	n;

	if(strchr("fev", p->ftype)) {
		if(cverify(0, &p->ftype, p->path, &p->cinfo))
			mstat->contchg++;
		else if(!mstat->contchg && !mstat->attrchg) {
			if(averify(0, &p->ftype, p->path, &p->ainfo, 1))
				mstat->attrchg++;
		}
	} else if(!mstat->attrchg && strchr("dxcbp", p->ftype)) {
		n = averify(0,&p->ftype, p->path, &p->ainfo, 1);
		if(n == VE_ATTR)
			mstat->attrchg++;
		else if(n && (n != VE_EXIST))
			mstat->contchg++;
	}
}

static int
is_setuid(p)
struct cfent *p;
{
	return(strchr("fve", p->ftype) && (p->ainfo.mode > 0) && 
		(p->ainfo.mode & S_ISUID));
}

static int
is_setgid(p)
struct cfent *p;
{
	return(strchr("fve", p->ftype) && (p->ainfo.mode > 0) && 
		(p->ainfo.mode & S_ISGID) && 
		(p->ainfo.mode & S_IXGRP));
}

char *types[] = {
	"fev", /* type 1, regular files*/
	"s", 	/* type 2, symbolic links*/
	"l", 	/* type 3, linked files*/
	"dx",	/* type 4, directories */
	"c",	/* type 5, character special devices */
	"b",	/* type 6, block special devices */
	"p",	/* type 7, named pipes */
	NULL
};

static int
typechg(inp, entry, flag)
struct cfent *inp, *entry;
int	flag;
{
	int	i, etype, itype;

	/* inp and entry ftypes are different if this routine is called */
	if(inp->ftype == '?') {
		inp->ftype = entry->ftype;
		return(0); /* do nothing; not really different */
	}

	if(entry->ftype == '?')
		return(0);

	etype = itype = 0;
	for(i=0; types[i]; ++i) {
		if(strchr(types[i], entry->ftype)) {
			etype = i+1;
			break;
		}
	}
	for(i=0; types[i]; ++i) {
		if(strchr(types[i], inp->ftype)) {
			itype = i+1;
			break;
		}
	}
	
	if(itype == etype) {
		/* same basic object type */
		return(0);
	}

	if(flag)
		return(-1); /* major type change disallowed */

	/* allow change, but warn user of possible problems */
	switch(etype) {
	  case 1:
		logerr("uxpkgtools:87:WARNING:%s <no longer a regular file>",
			entry->path);
		break;

	  case 2:
		logerr("uxpkgtools:88:WARNING:%s <no longer a symbolic link>",
			entry->path);
		break;

	  case 3:
		logerr("uxpkgtools:89:WARNING:%s <no longer a linked file>",
			entry->path);
		break;

	  case 4:
		logerr("uxpkgtools:90:WARNING:%s <no longer a directory>",
			entry->path);
		break;

	  case 5:
		logerr("uxpkgtools:91:WARNING:%s <no longer a character special device>",
			entry->path);
		break;

	  case 6:
		logerr("uxpkgtools:92:WARNING:%s <no longer a block special device>",
			entry->path);
		break;

	  case 7:
		logerr("uxpkgtools:93:WARNING:%s <no longer a named pipe>",
			entry->path);
		break;
	}
	return(1);
}

static int
merg(mstat, inp, entry)
struct mergstat *mstat;
struct cfent *inp, *entry;
{
	int	changed,
		n,
		no_achange;

	changed = 0;

	/* we need to change the original entry to make
	 * it look like the new entry (the eptstat() routine
	 * has already added appropriate package information,
	 * but not about 'aclass'
	 */
	
	inp->pinfo = entry->pinfo;

	/* do not allow installation of attribute changes
	 * if this path is used by the core utilties ????
	 */
	no_achange = (usedbycore && strcmp(pkginst, COREPKG));

	if(entry->ftype != inp->ftype) {
		n = typechg(inp, entry, no_achange);
		if(n < 0) {
			logerr("uxpkgtools:94:WARNING:object type change ignored <%s>", 
				entry->path);
			*inp = *entry;
			return(0);	/* don't change current entry */
		} else if(n)
			mstat->contchg++;
		changed++;
	}

	if(strcmp(entry->class, inp->class)) {
		/* we always allow a class change as long as we have
		 * consistent ftypes, which at this point we must
		 */
		changed++;
 		if(strcmp(entry->class, "?")) {
			(void) strcpy(pkgpinfo->aclass, inp->class);
			(void) strcpy(inp->class, entry->class);
			chgclass(inp, pkgpinfo);
		}
	}

	if(strchr("sl", entry->ftype)) {
		/*
		 * If we get here, it is because a file is no longer linked
		 * and we expect it to be -- ainfo.local is the link.
		 */
		if(entry->ainfo.local == NULL || inp->ainfo.local == NULL ||
			strcmp(entry->ainfo.local, inp->ainfo.local)) {
			changed++;
			if(inp->ainfo.local != NULL && entry->ainfo.local != NULL &&
			   !strcmp(inp->ainfo.local, "?"))
				strcpy(inp->ainfo.local, entry->ainfo.local);
			else
				mstat->contchg++;
		}
		/*
		 * If the link doesn't now exist at all but should, fill in 
		 * the inp->ainfo.local element to look as though it does.
		 * This is to prevent "bad read" errors in the contents file.
		 */
		if(inp->ainfo.local == NULL && entry->ainfo.local != NULL) {
			inp->ainfo.local = calloc(1, strlen(entry->ainfo.local)+1);
			(void) strcpy(inp->ainfo.local, entry->ainfo.local);
		}
		return(changed);
	} else if(inp->ftype == 'e') {
		/* the contents of edittable files assumed to be changing */
		mstat->contchg++;
		changed++;  /* content info is changing */
	} else if(strchr("fv", entry->ftype)) {
		/* look at content information; a '?' in this field
		 * indicates the contents are unknown -- thus we 
		 * assume that they are changing
		 */
		if(entry->cinfo.size != inp->cinfo.size) {
			changed++; /* content info is changing */
			mstat->contchg++;
		}
		if(entry->cinfo.modtime != inp->cinfo.modtime) {
			changed++; /* content info is changing */
			mstat->contchg++;
		}
		if(entry->cinfo.cksum != inp->cinfo.cksum) {
			changed++; /* content info is changing */
			mstat->contchg++;
		}
	} else if(strchr("cb", entry->ftype)) {
		if(entry->ainfo.major != inp->ainfo.major) {
			changed++;  /* attribute info is changing */
		   	if(inp->ainfo.major == BADMAJOR)
				inp->ainfo.major = entry->ainfo.major;
			else
				mstat->contchg++;
		}
		if(entry->ainfo.minor != inp->ainfo.minor) {
			changed++;  /* attribute info is changing */
		   	if(inp->ainfo.minor == BADMINOR)
				inp->ainfo.minor = entry->ainfo.minor;
			else
				mstat->contchg++;
		}
	}

	if(mstat->contchg && no_achange)
		no_achange = 0; /* since content is changing, attr's can */

	if(entry->ainfo.mode != inp->ainfo.mode) {
		changed++;  /* attribute info is changing */
	   	if(no_achange || (inp->ainfo.mode == BADMODE))
			inp->ainfo.mode = entry->ainfo.mode;
		else
			mstat->attrchg++;
	}
	if(strcmp(entry->ainfo.owner, inp->ainfo.owner)) {
		changed++;  /* attribute info is changing */
		if(no_achange || !strcmp(inp->ainfo.owner, "?"))
			(void) strcpy(inp->ainfo.owner, entry->ainfo.owner);
		else
			mstat->attrchg++;
	}
	if(strcmp(entry->ainfo.group, inp->ainfo.group)) {
		changed++;  /* attribute info is changing */
	   	if(no_achange || !strcmp(inp->ainfo.group, "?"))
			(void) strcpy(inp->ainfo.group, entry->ainfo.group);
		else
			mstat->attrchg++;
	}
	if(entry->ainfo.macid != inp->ainfo.macid) {
		changed++;  /* attribute info is changing */
	   	if(no_achange || (inp->ainfo.macid <= 0))
			inp->ainfo.macid = entry->ainfo.macid;
		else
			mstat->attrchg++;
	}
	if(strcmp(entry->ainfo.priv_fix, inp->ainfo.priv_fix)) {
		changed++;  /* attribute info is changing */
	   	if(no_achange || !strcmp(inp->ainfo.priv_fix, "?") 
		   || !strcmp(inp->ainfo.priv_fix, "NONE"))
			(void) strcpy(inp->ainfo.priv_fix, entry->ainfo.priv_fix);
		else
			mstat->attrchg++;
	}
	if(strcmp(entry->ainfo.priv_inh, inp->ainfo.priv_inh)) {
		changed++;  /* attribute info is changing */
	   	if(no_achange || !strcmp(inp->ainfo.priv_inh, "?") 
		   || !strcmp(inp->ainfo.priv_inh, "NONE"))
			(void) strcpy(inp->ainfo.priv_inh, entry->ainfo.priv_inh);
		else
			mstat->attrchg++;
	}
	return(changed);
}

/*
 * Added argument entry (old entry in contents file):
 * If the old filetype is volatile then filetype should remain volatile.
 * If there is no old entry then the argument is NULL.
 */
static void
output(fpo, inp, entry, pinfo)
FILE *fpo;
struct cfent *inp;
struct cfent *entry;
struct pinfo *pinfo;
{
	short	svvolno;
	char	*svpt;

	/* output without volume information */
	svvolno = inp->volno;
	inp->volno = 0;

	pinfo->editflag = 0;
	if(strchr("sl", inp->ftype)) {
		if(putcfile(inp, fpo)) {
			progerr(ERR_OUTPUT);
			quit(99);
		}
	} else {
		/*
		 * We want volatile files to remain volatile.
		 */
		if(inp->ftype == 'e') {
			if(entry && entry->ftype == 'v')
				inp->ftype = 'v';
			pinfo->editflag++;
		}
		else if(inp->ftype == 'f' && entry && entry->ftype == 'v') {
			/* change ftype to 'v' if shared file is volatile */
			logerr("uxpkgtools:95:WARNING:%s <shared file is volatile>", entry->path);
			inp->ftype = 'v';
		}
		/* output without local pathname */
		svpt = inp->ainfo.local;
		inp->ainfo.local = NULL;
		if(putcfile(inp, fpo)) {
			progerr(ERR_OUTPUT);
			quit(99);
		}
		inp->ainfo.local = svpt;
		/* if this entry represents a file which is being
		 * edited, we need to store in memory the fact
		 * that it is an edittable file so that when we
		 * audit it after installation we do not worry
		 * about its contents; we do this by resetting the
		 * ftype to 'e' in the memory array which is later
		 * used to control the audit 
		 */
		if(pinfo->editflag)
			inp->ftype = 'e';
	}
	/* restore volume information */
	inp->volno = svvolno;
}

static void
chgclass(entry, pinfo)
struct cfent *entry;
struct pinfo *pinfo;
{
	struct pinfo *pp;
	char	*oldclass, newclass[CLSSIZ+1];
	int	newcnt, oldcnt;

	/* we use this routine to minimize the use of the aclass element
	 * by optimizing the use of the entry->class element
	 */
	strcpy(newclass, pinfo->aclass);
	newcnt = 1;

	oldclass = entry->class;
	oldcnt = 0;

	/* count the number of times the newclass will be used and see
	 * if it exceeds the number of time the oldclass is referenced
	 */
	pp = entry->pinfo;
	while(pp) {
		if(pp->aclass[0]) {
			if(!strcmp(pp->aclass, newclass))
				newcnt++;
			else if(!strcmp(pp->aclass, oldclass))
				oldcnt++;
		}
		pp = pp->next;
	}
	if(newcnt > oldcnt) {
		pp = entry->pinfo;
		while(pp) {
			if(pp->aclass[0] == '\0')
				strcpy(pp->aclass, oldclass);
			else if(!strcmp(pp->aclass, newclass))
				pp->aclass[0] = '\0';
			pp = pp->next;
		}
		(void) strcpy(entry->class, newclass);
	}
}
