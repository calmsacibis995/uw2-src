/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgchk/ckentry.c	1.2.10.10"
#ident  "$Header: $"

#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <pkgstrct.h>
#include "install.h"

extern size_t	cmdlen;
extern int	Lflag, lflag,
		aflag, cflag,
		fflag, qflag,
		nflag, xflag,
		compressed,
		vflag;
extern char	*errstr,
		*basedir,
		*device,
		pkgspool[],
		cmdline[],
		tempfile[],
		errbuf[];

extern void	mappath(),
		logerr(),
		progerr(),
		basepath(),
		mappath(),
		mapvar(),
		canonize(),
		tputcfent(),
		free(),
		exit();
extern int	access(),
		unlink(),
		putcfile(),
		gpkgmap(),
		srchcfile(),
		averify(),
		cverify();

#define nxtentry(p) \
		(maptyp ? srchcfile(p, "*", fp, NULL) : gpkgmap(p, fp, 0))

#define ERR_SPOOLED	":218:unable to locate spooled object <%s>"
#define ERR_OPEN	":46:unable to open <%s> for reading, errno=%d"
#define ERR_UNCOMPR	":370:unable to uncompress file <%s>"

static int	xdir();
static char	*findspool();

int
ckentry(envflag, maptyp, ept, fp)
int	envflag, maptyp;
struct cfent	*ept;
FILE	*fp;
{
	int	a_err, c_err,
		errflg;
	char	*path;
	char *cmd1=cmdline;
	char *cmd2, *ptr;
	char *tmpfile;
	char *filename;
	FILE *spoolfp;

	if(envflag && (ept->ftype != 'i')) {
		mappath(2, ept->path);
		(void)basepath(ept->path, basedir);
	}
	canonize(ept->path);
	if(strchr("sl", ept->ftype)) {
		if(envflag) {
			mappath(2, ept->ainfo.local);
			if( 'l' == ept->ftype ||
			   (!isdot(ept->ainfo.local) &&
			    !isdotdot(ept->ainfo.local)))
				(void)basepath(ept->ainfo.local, basedir);
		}
		if('l' == ept->ftype ||
		   (!isdot(ept->ainfo.local) &&
		    !isdotdot(ept->ainfo.local)))
			canonize(ept->ainfo.local);
	}
	if(envflag) {
		if(!strchr("isl", ept->ftype)) {
			mapvar(2, ept->ainfo.owner);
			mapvar(2, ept->ainfo.group);
		}
	}

	if(lflag) {
		tputcfent(ept, stdout);
		return(0);
	} else if(Lflag)
		return(putcfile(ept, stdout));

	errflg = 0;
	if(device) {
		if(strchr("dxslcbp", ept->ftype))
			return(0);
		if((path = findspool(ept)) == NULL) {
			progerr(ERR_SPOOLED, ept->path);
			return(-1);
		}
		tmpfile=NULL;
		filename=path;
		if ( compressed ) {
			/* if path is a compress file, lets uncompress it
		   	into tmpfile, and then use tmpfile as arg
		   	to cverify
			*/
			if ( (spoolfp=fopen(path,"r")) == NULL ) {
				progerr(ERR_OPEN, path, errno);
				quit(99);
			}
			/* if filename ends in .Z, don't uncompress */
			/* for new R5 server font files */
			ptr=strrchr(path,NULL);
			ptr-=2;
			if ( strncmp(ptr,".Z",2) && ( getc(spoolfp) == 037 ) &&
					( getc(spoolfp) == 0235) ) {
				(void) fclose(spoolfp);
				tmpfile=tempfile;
				cmd2=cmd1+cmdlen;
				(void) strcpy(cmd2,path);
				(void) strcat(cmd2," > ");
				(void) strcat(cmd2,tmpfile);
				if(esystem(cmd1, -1, -1)) {
					/*
			 		* The uncompress command failed.
			 		*/
					rpterr();
					progerr(ERR_UNCOMPR, path);
					quit(99);
				}
				filename=tmpfile;
			}
			else 
				(void) fclose(spoolfp);
		}

		c_err = cverify(0, &ept->ftype, filename, &ept->cinfo);
		if ( tmpfile )
			unlink(tmpfile);
		if(c_err) {
			logerr(":219:ERROR:%s", path);
			logerr(errbuf);
			return(-1);
		}
	} else {
		a_err = 0;
		if(aflag && !strchr("in", ept->ftype)) {
			/* validate attributes */
			if(a_err = averify(fflag, &ept->ftype, 
			  ept->path, &ept->ainfo, 0)) {
				errflg++;
				if(!qflag || (a_err != VE_EXIST)) {
					logerr(":219:ERROR:%s", ept->path);
					logerr(errbuf);
				}
				if(a_err == VE_EXIST)
					return(-1);
			}
		}
		if(cflag && strchr("fev", ept->ftype) &&
		  ( !nflag ||  (ept->ftype == 'f')  ) ) {
			/* validate contents */
			if(c_err = cverify(fflag, &ept->ftype, 
			  ept->path, &ept->cinfo)) {
				errflg++;
				if(!qflag || (c_err != VE_EXIST)) {
					if(!a_err) 
						logerr(":219:ERROR:%s", ept->path);
					logerr(errbuf);
				}
				if(c_err == VE_EXIST)
					return(-1);
			}
		}
		if(xflag && (ept->ftype == 'x')) {
			/* must do verbose here since ept->path will change */
			path = strdup(ept->path);
			if(xdir(maptyp, fp, path))
				errflg++;
			(void) strcpy(ept->path, path);
			free(path);
		}
	}
	if(vflag) 
		(void) fprintf(stderr, "%s\n", ept->path);
	return(errflg);
}

static int
xdir(maptyp, fp, dirname)
int	maptyp;
FILE	*fp;
char	*dirname;
{
	struct dirent *drp;
	struct cfent mine;
	struct pinfo *pinfo;
	DIR	*dirfp;
	long	pos;
	int	n, len, dirfound,
		errflg;
	char	badpath[PATH_MAX+1];

	pos = ftell(fp);

	if((dirfp = opendir(dirname)) == NULL) {
		progerr(":220:unable to open directory <%s>", dirname);
		return(-1);
	}
	len = strlen(dirname);

	errflg = 0;
	(void) memset((char *)&mine, '\0', sizeof(struct cfent));
	while((drp = readdir(dirfp)) != NULL) {
		if(!strcmp(drp->d_name, ".") || !strcmp(drp->d_name, ".."))
			continue;
		dirfound = 0;
		while(n = nxtentry(&mine)) {
			if(n < 0) {
				logerr(":216:ERROR:garbled entry");
				if(mine.path)
					logerr(":199:pathname: %s", mine.path);
				logerr(":200:problem: %s", errstr);
				exit(99);
			}
			if(strncmp(mine.path, dirname, len) || 
			(mine.path[len] != '/'))
				break;
			if(!strcmp(drp->d_name, &mine.path[len+1])) {
				dirfound++;
				break;
			}
		}
		if(fseek(fp, pos, 0)) {
			progerr(":221:unable to reset file position from xdir()");
			exit(99);
		}
		if(!dirfound) {
			(void) sprintf(badpath, "%s/%s", dirname, drp->d_name);
			if(fflag) {
				if(unlink(badpath)) {
					errflg++;
					logerr(":219:ERROR:%s", badpath);
					logerr(":222:unable to remove hidden file");
				}
			} else {
				errflg++;
				logerr(":219:ERROR:%s", badpath);
				logerr(":223:hidden file in exclusive directory");
			}
		}
	}

	if(maptyp) {
		/* clear memory we've used */
		while(pinfo = mine.pinfo) {
			mine.pinfo = pinfo->next;
			free((char *)pinfo);
		}
	}

	(void) closedir(dirfp);
	return(errflg);
}

static char *
findspool(ept)
struct cfent *ept;
{
	static char path[2*PATH_MAX+1];
	char host[PATH_MAX+1];

	(void) strcpy(host, pkgspool);
	if(ept->ftype == 'i') {
		if(strcmp(ept->path, "pkginfo") && strcmp(ept->path, "setinfo"))
			(void) strcat(host, "/install");
	} else if(ept->path[0] == '/')
		(void) strcat(host, "/root");
	else
		(void) strcat(host, "/reloc");

	(void) sprintf(path, "%s/%s", host, 
		ept->path + (ept->path[0] == '/')); 
	if(access(path, 0) == 0)
		return(path);

	if((ept->ftype != 'i') && (ept->volno > 0)) {
		(void) sprintf(path, "%s.%d/%s", host, ept->volno,
			ept->path + (ept->path[0] == '/')); 
		if(access(path, 0) == 0)
			return(path);
	}
	return(NULL);
}

