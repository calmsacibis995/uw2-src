/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/verify.c	1.21.12.27"
#ident  "$Header: $"

#include <stdio.h>
#include <limits.h>
#include <utime.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mkdev.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <varargs.h>
#include <pkgstrct.h>
#include <pkglocs.h>
#include <mac.h>
#include <sys/types.h>
#include "priv.h"

#include <pfmt.h>

extern struct group	*getgrnam(), *getgrgid();
extern struct passwd	*getpwnam(), *getpwuid();
extern void		setgrent(), setpwent();
extern char		*privname();

extern int	cftime(),
		link(),
		unlink(),
/************************************************************************
 * Comment out this lstat() declaration since it's non-ANSI compliant.	*
 * An __STDC__ wrapper will be inserted here in the near future.	*
 *									*
 *		lstat(),						*
 ************************************************************************/
		readlink(),
		mkdir(),
		symlink(),
		chmod(),
		chown(),
		lvlfile(),
		lvlin(),
		secsys(),
		getprivs(),
		setprivs(),
		reset();

#define WDMSK 0177777L
#define BUFSIZE 512
#define DATEFMT	"%D %r"
#define TDELTA 15*60
#define user_pub 4
#define	ABUFSIZ	(((NPRIVS + 1) * PRVNAMSIZ) + (NPRIVS + 1))

#define ERR_UNKNOWN	"uxpkgtools:732:unable to determine object type"
#define ERR_EXIST	"uxpkgtools:733:pathname does not exist"
#define ERR_FTYPE	"uxpkgtools:734:file type <%c> expected <%c> actual"
#define ERR_LINK	"uxpkgtools:735:pathname not properly linked to <%s>"
#define ERR_SLINK	"uxpkgtools:736:pathname not symbolically linked to <%s>"
#define ERR_MTIME	"uxpkgtools:737:modtime <%s> expected <%s> actual"
#define ERR_SIZE	"uxpkgtools:738:file size <%ld> expected <%ld> actual"
#define ERR_CKSUM	"uxpkgtools:739:file cksum <%ld> expected <%ld> actual"
#define ERR_MAJMIN	"uxpkgtools:740:major/minor device <%d,%d> expected <%d,%d> actual"
#define ERR_PERM	"uxpkgtools:741:permissions <%04o> expected <%04o> actual"
#define ERR_GROUP	"uxpkgtools:742:group name <%s> expected <%s> actual"
#define ERR_OWNER	"uxpkgtools:743:owner name <%s> expected <%s> actual"
#define	ERR_MAC		"uxpkgtools:744:MAC level <%u> expected <%u> actual"
#define	ERR_MACGET	"uxpkgtools:745:unable to get MAC level for <%s>"
#define ERR_INVMAC \
	"uxpkgtools:746:the MAC level <%d> is invalid - setting level for <%s> to <%d>"
#define	ERR_FIXPRV	"uxpkgtools:747:fixed privileges <%s> expected <%s> actual"
#define	ERR_INHPRV	"uxpkgtools:748:inheritable privileges <%s> expected <%s> actual"
#define ERR_GETPRV	"uxpkgtools:749:unable to get file privileges for <%s>"
#define ERR_INVPRV 	"uxpkgtools:750:could not set privileges on <%s>" 
#define ERR_RESET	"uxpkgtools:547:unable to reset privileges on linked file <%s>"
#define	ERR_INST \
	"uxpkgtools:751:unable to set security attributes for installation file <%s>"
#define ERR_MODFAIL	"uxpkgtools:752:unable to fix modification time"
#define ERR_LINKFAIL	"uxpkgtools:753:unable to create link to <%s>"
#define ERR_SLINKFAIL	"uxpkgtools:754:unable to create symbolic link to <%s>"
#define ERR_DIRFAIL	"uxpkgtools:755:unable to create directory"
#define ERR_CDEVFAIL	"uxpkgtools:756:unable to create character-special device"
#define ERR_BDEVFAIL	"uxpkgtools:757:unable to create block-special device"
#define ERR_PIPEFAIL	"uxpkgtools:758:unable to create named pipe"
#define ERR_ATTRFAIL	"uxpkgtools:759:unable to fix attributes"
#define ERR_BADGRPID	"uxpkgtools:760:unable to determine group name for gid <%d>"
#define ERR_BADUSRID	"uxpkgtools:761:unable to determine owner name for gid <%d>"
#define ERR_BADGRPNM	"uxpkgtools:762:group name <%s> does not exist in /etc/group"
#define ERR_BADUSRNM	"uxpkgtools:763:owner name <%s> does not exist in /etc/passwd"
#define ERR_PIPE	"uxpkgtools:764:cannot check file contents because file type has changed from <%c> to <%c>"

char	errbuf[PATH_MAX+512];

static int	cksumerr;
static unsigned docksum();
static int	cmppriv();
static int	setnum, getnum;
static int	fixprob, inhprob;
static char	fixstr[ABUFSIZ], inhstr[ABUFSIZ];
static char	*fixstrp=&fixstr[0], *inhstrp=&inhstr[0];
static int	getfix, getinh;

struct part { short unsigned hi,lo; };
static union hilo { /* this only works right in case short is 1/2 of long */
	struct part hl;
	long	lg;
} tempa, suma;

#include <ctype.h>

#define MSGID_SZ 64

char *
getmsg(char *fmt)
{
	char	msgid[MSGID_SZ + 1], *p = msgid;
	char	*defmsg = fmt;

	/* scan catalog name */
	while (*defmsg && *defmsg != ':' && (p - msgid) < MSGID_SZ)
		*p++ = *defmsg++;

	if (*defmsg != ':' || (p - msgid) >= MSGID_SZ)
		return fmt;
	else
		*p++ = *defmsg++;

	/* scan message number */
	while (*defmsg && isdigit(*defmsg) && (p - msgid) <MSGID_SZ)
		*p++ = *defmsg++;

	if (*defmsg != ':' || (p - msgid) >= MSGID_SZ)
		return fmt;
	else
	{ *p = '\0'; defmsg++; }

	return gettxt(msgid, defmsg);
}

/*VARARGS*/
static void
reperr(va_alist)
va_dcl
{
	char *fmt, *pt;
	va_list	ap;
	int	n;

	va_start(ap);
	if(n = strlen(errbuf)) {
		pt = errbuf + n;
		*pt++ = '\n';
		*pt = '\0';
	} else
		pt = errbuf;
	if(fmt = va_arg(ap, char *)) {
		fmt = getmsg(fmt);
		(void) vsprintf(pt, fmt, ap);
		pt += strlen(pt);
	} else
		errbuf[0] = '\0';
	va_end(ap);
}
/*ARGSUSED*/

int
cverify(fix, ftype, path, cinfo)
int fix;
char *ftype, *path;
struct cinfo *cinfo;
{
	int		alg = 0;
	struct stat	status;	/* file status buffer */
	struct utimbuf	times;
	unsigned	mycksum;
	int		setval, retcode;
	char		*actkey;
	char		tbuf1[26], tbuf2[26];

	setval = (*ftype == '?');
	retcode = 0;
	reperr(NULL);

	/*
	 * If a file type package object exists as a symbolic link,
	 * don't bother to check whether the linked to file has the
	 * same size, checksum; thus leading to the conclusion that
	 * both objects have the same contents.
	 *
	 * That is, object types must be part of the equation in order
	 * to determine that two objects have the same contents.
	 */
	if (*ftype == 'f' && ((retcode = lstat(path, &status)) == 0) &&
	    (status.st_mode & S_IFMT) == S_IFLNK) {
		if (cinfo->modtime != BADCONT || cinfo->size != BADCONT ||
		    cinfo->cksum != BADCONT) {
			reperr(ERR_PIPE, *ftype, 's'); 
			return(VE_FTYPE);
		}
	}
	if ((*ftype == 'f' && retcode != 0) || stat(path, &status) < 0) {
		reperr(ERR_EXIST);
		return(VE_EXIST);
	}

	/*
	 * Determine the actual file type of existing object.
	 */
	switch(status.st_mode & S_IFMT) {
		case S_IFIFO:
			reperr(ERR_PIPE, *ftype, 'p'); 
			return(VE_FTYPE);
	}

	/* -1	requires modtimes to be the same */
	/*  0   reports modtime failure */
	/*  1   fixes modtimes */
	if(setval || (cinfo->modtime == BADCONT))
		cinfo->modtime = status.st_mtime;
	else if(status.st_mtime != cinfo->modtime) {
		if(fix > 0) {
			/* reset times on the file */
			times.actime = cinfo->modtime;
			times.modtime = cinfo->modtime;
			if(utime(path, &times)) {
				reperr(ERR_MODFAIL);
				retcode = VE_FAIL;
			}
		} else if(fix < 0) {
			/* modtimes must be the same */
			(void) cftime(tbuf1, DATEFMT, &cinfo->modtime);
			(void) cftime(tbuf2, DATEFMT, &status.st_mtime);
			reperr(ERR_MTIME, tbuf1, tbuf2);
			retcode = VE_CONT;
		} /*else
			retcode = VE_TIME;*/
	} 
	if(setval || (cinfo->size == BADCONT))
		cinfo->size = status.st_size;
	else if(status.st_size != cinfo->size) {
		if(!retcode /*|| (retcode == VE_TIME)*/)
			retcode = VE_CONT;
		reperr(ERR_SIZE, cinfo->size, status.st_size);
	} 

	if ((actkey = getenv("ACTKEY")) != NULL && !strcmp(actkey, "YES")) {
		alg++;
	}
	mycksum = docksum(path, alg);
	if(setval || (cinfo->cksum == BADCONT))
		cinfo->cksum = mycksum;
	else if (mycksum != cinfo->cksum) {
		/*
		 * Doing verification probably through pkgchk.  Try the
		 * other algorithm since this approach is better than 
		 * trying to figure out from the pkginfo file whether the
		 * package was installed with activation enabled.
		 */
		mycksum = docksum(path, alg == 0 ? 1 : 0);
	}
	if((mycksum != cinfo->cksum) || cksumerr) {
		if(!retcode /*|| (retcode == VE_TIME)*/)
			retcode = VE_CONT;
		reperr(ERR_CKSUM, cinfo->cksum, mycksum);
	}

	return(retcode);
}
	
static unsigned
docksum(path, alg)
char *path;
{
	register int	ca;
	register FILE *fp;
	register unsigned sum;
	unsigned lsavhi, lsavlo;

	cksumerr = 0;
	if((fp = fopen(path, "r")) == NULL) {
		cksumerr++;
		return(0);
	}

	sum = 0;
	suma.lg = 0;
	if (alg == 0) {
		while((ca = getc(fp)) != EOF)
			suma.lg += ca & WDMSK;
		tempa.lg = (suma.hl.lo & WDMSK) + (suma.hl.hi & WDMSK);
		lsavhi = (unsigned) tempa.hl.hi;
		lsavlo = (unsigned) tempa.hl.lo;
	} else {
		while((ca = getc(fp)) != EOF) {
			if (sum & 01)
				sum = (sum >> 1) + 0x8000;
			else
				sum >>= 1;
			sum += ca;
			sum &= 0xffff;
		}
	}
	(void) fclose(fp);
	return(alg == 0 ? (lsavhi+lsavlo) : sum);
}

extern	char	*prog;

int
averify(fix, ftype, path, ainfo, shared)
int	fix;
char	*ftype, *path;
struct ainfo *ainfo;
ushort	shared;
{
	struct stat		status;	/* file status buffer */
	struct group		*grp;	/* group entry buffer */
	struct passwd		*pwd;
	int		setlvl();
	int		n;
	int		setval;
	int		uid, gid;
	int		dochown;
	int		retcode;
	char		myftype;
	char		buf[PATH_MAX];
	ino_t		my_ino;
	dev_t		my_dev;
	level_t		level, lvl;
	int		i;
	char		create_obj = 0;		/* object creation flag */
	char		fixempty;	/* fixed privilege is empty flag */
	char		inhempty;	/* inheritable privilege is empty flag */

	setval = (*ftype == '?');
	retcode = 0;
	reperr(NULL);

	if(*ftype == 'l') {
		if(stat(path, &status) < 0) {
			retcode = VE_EXIST;
			reperr(ERR_EXIST);
		} 

		my_ino = status.st_ino;
		my_dev = status.st_dev;

		if(retcode || (status.st_nlink < 2) || 
		(stat(ainfo->local, &status) < 0) ||
		(my_dev != status.st_dev) || (my_ino != status.st_ino)) {
			if(fix) {
				(void) unlink(path);
				if(link(ainfo->local, path)) {
					reperr(ERR_LINKFAIL, ainfo->local);
					return(VE_FAIL);
				}
				if(reset(ainfo->local)) {
						reperr(NULL);
						reperr(ERR_RESET, ainfo->local);
						return(VE_ATTR);
				}
				retcode = 0;
			} else {
				reperr(ERR_LINK, ainfo->local);
				return(VE_CONT);
			}
		}
		return(retcode);
	}

	retcode = 0;

	if (lstat(path, &status) == 0) {
		/* determining actual type of existing object */
		switch(status.st_mode & S_IFMT) {
               	  case S_IFLNK:
                        myftype = 's';
                        break;
		  case S_IFIFO:
			myftype = 'p';
			break;

		  case S_IFCHR:
			myftype = 'c';
			break;

		  case S_IFDIR:
			myftype = 'd';
			break;

		  case S_IFBLK:
			myftype = 'b';
			break;

		  case S_IFREG:
		  case 0:
			myftype = 'f';
			break;

		  default:
			reperr(ERR_UNKNOWN);
			return(VE_FTYPE);
		}
	} else {
		reperr(ERR_EXIST);
		retcode = VE_EXIST;
		myftype = '?';
		create_obj++;
	}

	if(setval)
		*ftype = myftype;
	else if(!retcode && (*ftype != myftype) && 
	   ((myftype != 'f') || !strchr("ilevL", *ftype)) &&
	   ((myftype != 'd') || (*ftype != 'x'))) {
		reperr(ERR_FTYPE, *ftype, myftype);
		retcode = VE_FTYPE;
	}

	if(!retcode && (*ftype == 's')) {
		/* make sure that symbolic link is correct */
		n = readlink(path, buf, PATH_MAX);
		if(n < 0) {
			reperr(ERR_SLINK, ainfo->local);
			retcode = VE_CONT;
		} else {
			buf[n] = '\0';
			if(strcmp(buf, ainfo->local)) {
				reperr(ERR_SLINK, ainfo->local);
				retcode = VE_CONT;
			}
		}
	}

	if(retcode) {
		/* path doesn't exist or is different than it should be */
		if(fix) {
			if (*ftype == 'd' || *ftype == 'x') {
				/*
				 * Allow installation of a directory through
				 * a symbolic link.
				 */
				if ((myftype == 's') &&
				    ((stat(path, &status) < 0) ||
				     (status.st_mode & S_IFMT) != S_IFDIR)) {
					reperr(ERR_DIRFAIL);
					return(VE_FAIL);
				}
				if ((retcode == VE_EXIST) &&
				    (mkdir(path, ainfo->mode) != 0)) {
					reperr(ERR_DIRFAIL);
					return(VE_FAIL);
				} else {
					retcode = 0;
				}
			} else {
				(void) unlink(path); /* in case it exists */
				if(*ftype == 's') {
					if(symlink(ainfo->local, path)) {
						reperr(ERR_SLINKFAIL,
							ainfo->local);
						return(VE_FAIL);
					}

				} else if(*ftype == 'c' || *ftype == 'b' ||
					  *ftype == 'p') {
					dev_t dev;
					mode_t mode;
					char *errstr;

					mode = ainfo->mode;
					dev = makedev(ainfo->major,
							ainfo->minor);
					switch (*ftype) {

					case 'b':
						mode |= S_IFBLK;
						errstr = ERR_BDEVFAIL;
						break;

					case 'c':
						mode |= S_IFCHR;
						errstr = ERR_CDEVFAIL;
						break;

					case 'p':
						dev = NULL;
						mode |= S_IFIFO;
						errstr = ERR_PIPEFAIL;
						break;
					}
					if(mknod(path, mode, dev) ||
					   (stat(path, &status) < 0)) {
						reperr(errstr);
						return(VE_FAIL);
					}
				} else
					return(retcode);
				
			}
		} else {
			return(retcode);
		}
	}

	if(*ftype == 's')
		return(0); /* don't check anything else */

	if(*ftype == 'i') 
		return(0); /* don't check anything else */

	retcode = 0;

	if(strchr("cb", myftype)) {
		if(setval || (ainfo->major < 0))
			ainfo->major = major(status.st_rdev);
		if(setval || (ainfo->minor < 0))
			ainfo->minor = minor(status.st_rdev);
		/* check major & minor */
		if(status.st_rdev != makedev(ainfo->major, ainfo->minor)) {
			reperr(ERR_MAJMIN, ainfo->major, ainfo->minor,
				major(status.st_rdev), minor(status.st_rdev));
			retcode = VE_CONT;
			if (fix) {
				unlink(path);
				if(mknod(path, ainfo->mode | 
					((myftype == 'b') ? S_IFBLK : S_IFCHR),
					makedev(ainfo->major, ainfo->minor)) ||
				(stat(path, &status) < 0)) {
					reperr(ERR_BDEVFAIL);
					retcode = VE_FAIL;
				}
			}	
		}
	}

	/* compare specified mode w/ actual mode excluding sticky bit */
	if(setval || (ainfo->mode == BADMODE) 
		  || (ainfo->mode == -1))
	/*the last comparison put in for installf compatibility */
		ainfo->mode = status.st_mode & 07777;
	else if (create_obj ||
		 (ainfo->mode & 06777) != (status.st_mode & 06777)) {
		if(fix) {
			if((ainfo->mode < 0) || 
			   (chmod(path, ainfo->mode) < 0))
				retcode = VE_FAIL;
		} else {
			reperr(ERR_PERM, ainfo->mode, 
				status.st_mode & 07777);
			if(!retcode)
				retcode = VE_ATTR;
		}
	}

	/* rewind group file */
	setgrent();
	dochown = 0;

	/* get group entry for specified group */
	if(setval || !strcmp(ainfo->group, BADGROUP) 
		  || !strcmp(ainfo->group, "NONE")) {
	/*the last comparison put in for installf compatibility */
		grp = getgrgid(status.st_gid);
		if(grp)
			(void) strcpy(ainfo->group, grp->gr_name);
		else {
			if(!retcode)
				retcode = VE_ATTR;
			reperr(ERR_BADGRPID, status.st_gid);
		}
		gid = status.st_gid;
	} else if((grp = getgrnam(ainfo->group)) == NULL) {
		reperr(ERR_BADGRPNM, ainfo->group);
		if(!retcode)
			retcode = VE_ATTR;
	} else if (create_obj || (gid=grp->gr_gid) != status.st_gid) {
		/*
		 * Dealing with a newly created object or existing
		 * object's gid mismatch.
		 */
		if(fix) {
			/* save specified GID */
			gid = grp->gr_gid;
			dochown++;
		} else {
			if((grp = getgrgid((int)status.st_gid)) == (struct group *)NULL)
				reperr(ERR_GROUP, ainfo->group, "");
			else 
				reperr(ERR_GROUP, ainfo->group, grp->gr_name);

			if(!retcode)
				retcode = VE_ATTR;
		}
	}

	/* rewind password file */
	setpwent();

	/* get password entry for specified owner */
	if(setval || !strcmp(ainfo->owner, BADOWNER) 
		  || !strcmp(ainfo->owner, "NONE")) {
	/*the last comparison put in for installf compatibility */
		pwd = getpwuid((int)status.st_uid);
		if(pwd)
			(void) strcpy(ainfo->owner, pwd->pw_name);
		else {
			if(!retcode)
				retcode = VE_ATTR;
			reperr(ERR_BADUSRID, status.st_uid);
		}
		uid = status.st_uid;
	} else if ((pwd = getpwnam(ainfo->owner)) == NULL) {
		/* UID does not exist in password file */
		reperr(ERR_BADUSRNM, ainfo->owner);
		if(!retcode)
			retcode = VE_ATTR;
	} else if (create_obj || (uid=pwd->pw_uid) != status.st_uid) {
		/*
		 * Dealing with a newly created object or existing
		 * object's uid mismatch.
		 */
		/* get owner name for actual UID */
		if(fix) {
			uid = pwd->pw_uid;
			dochown++;
		} else {
			pwd = getpwuid((int)status.st_uid);
			if(pwd)
				reperr(ERR_OWNER, ainfo->owner, pwd->pw_name);
			else
				reperr(ERR_OWNER, ainfo->owner, "");

			if(!retcode)
				retcode = VE_ATTR;
		}
	}
	if(dochown && (chown(path, uid, gid) < 0))
		retcode = VE_FAIL; /* chown failed */

	/* get the MAC level of the file */
	/* do security attributes only if it is not for a
	 * prototype file(i.e. setval is not defined)
	 */
	/* lvl will be the default level */
	lvl = user_pub;
	/* initialize level to "no level" */
	level = 0;
	if(!setval) {
		/*
		 * If the calling program is 'pkgchk', determine if
		 * the MAC feature is installed -- if lvlin() returns
		 * non-zero it is not.  In this case, don't do MAC
		 * level checking.
		 */
		if(!strcmp(prog, "pkgchk") && lvlin("SYS_PRIVATE", &level) < 0)
			goto passmac;

		if(lvlfile(path, MAC_GET, &level) == -1) {
			/* ENOPKG check strictly for h loads -
			 * 4.1 packaging used on 4.0
			 */
			if(errno == ENOPKG)
				goto end;
			reperr(ERR_MACGET, path);
			if(!retcode)
				retcode = VE_ATTR;
		}
		
		if(ainfo->macid == BADMAC)
			ainfo->macid = level;
		if((ainfo->macid < 0) && (level != user_pub)) {
			if(fix) {
			/* this means that MAC level needs 
		 	* to be set to default 
		 	*/
				if(setlvl(path, &lvl) == 0) {
					if(shared && (level != 0)) 
						pfmt(stdout, MM_WARNING, "uxpkgtools:765:Changed MAC level from <%u> to <%u> for <%s>\n", level, user_pub, path);
				}
			} else {
				reperr(ERR_MAC, user_pub, level);
				if(!retcode)
					retcode = VE_ATTR;
			}
		} else if((ainfo->macid != level) && (ainfo->macid > 0)) {
			if(fix) {
				/* attempt to set the level on the file */
				 if(lvlfile(path, MAC_SET, (level_t *)&ainfo->macid) < 0) {
					if((errno != ENOSYS) && (errno != EBUSY)) {
						/* if the reason the lvlfile
						 * failed was not due to a non-
						 * sfs file system or file is
						 * a mountpoint
						 */
						if(errno == EINVAL) {
							reperr(ERR_INVMAC, ainfo->macid, path, user_pub);
							if(!retcode)
								retcode = VE_ATTR;
							setlvl(path, &lvl);
						} else
							if(!retcode)
								retcode = VE_FAIL;
					} 
				} else if(shared && (level != 0)) {
					pfmt(stdout, MM_WARNING, "uxpkgtools:765:Changed MAC level from <%u> to <%u> for <%s>\n", level, ainfo->macid, path);
				}
			} else {
				reperr(ERR_MAC, ainfo->macid, level);
				if(!retcode)
					retcode = VE_ATTR;
			}
		}

passmac:

		/* get the fixed and inheritable privileges */

		if(!strcmp(ainfo->priv_fix, BADPRIV) || !strcmp(ainfo->priv_inh, BADPRIV))
			goto end;	

		/* get any privilegs that exist on the file */
		getfix = 0;
		getinh = 0;
		getnum = getprivs(path, fixstrp, inhstrp, &getfix, &getinh);

		fixempty = (!strcmp(ainfo->priv_fix, "NONE") ||
			    !strcmp(ainfo->priv_fix, "NULL"));
		inhempty = (!strcmp(ainfo->priv_inh, "NONE") ||
			    !strcmp(ainfo->priv_inh, "NULL"));
		if (fixempty && fixstrp[0] == '\0' &&
		    inhempty && inhstrp[0] == '\0') {
			goto end;
		}

		/* set all the privileges */
		if(fix) {
			setnum = setprivs(path, ainfo->priv_fix, ainfo->priv_inh);
			switch(setnum) {
			case -1:
				reperr(ERR_INVPRV, path);
				if(!retcode)
					retcode = VE_FAIL;
				break;
			case 0:
				/* no privileges set (NULL NULL) */
				break;
			default:
				if(getnum > 0) {
					/* if privs existed before, compare 
					 * them to the new ones
					 */
					if(cmppriv(ainfo)) {
						/* if the fixed changed */
						if(fixprob) {
							pfmt(stdout, MM_WARNING, "uxpkgtools:766:Changed fixed privileges for <%s> from <%s> to <%s>\n", path, fixstrp, ainfo->priv_fix);
						}
						if(inhprob) {
						/* if the inherited changed */
							pfmt(stdout, MM_WARNING, "uxpkgtools:767:Changed inheritable privileges for <%s> from <%s> to <%s>\n", path, inhstrp, ainfo->priv_inh);
						}
					}				
				}
				break;
			}
				
		} else {
			if(cmppriv(ainfo)) {
				if(fixprob) {
					reperr(ERR_FIXPRV, ainfo->priv_fix, fixstrp);
					if(!retcode)
						retcode = VE_ATTR;
				}
				if(inhprob) {
					reperr(ERR_INHPRV, ainfo->priv_inh, inhstrp);
					if(!retcode)
						retcode = VE_ATTR;
				}
			}
		}
	} else if(setval && fix) {
		/* installf must install file with USER_PUBLIC */
		setlvl(path, &lvl);
	}

end:
	if(retcode == VE_FAIL)
		reperr(ERR_ATTRFAIL);
	return(retcode);
}

static	char	all_priv_buf[NPRIVS][PRVNAMSIZ];
static	char	**all_priv_tab	= (char **)NULL;

/*
** This routine is called by cmppriv() to generate a list of all privileges
** for an "allpriv comparison. It should only be called once, the values are
** saved in the static arrays (defined above) and reused.
*/
char	**gen_allprivs()
{
	register int	i;
	char		**apt;

	apt = (char **)malloc(NPRIVS*sizeof(char *));

	for(i=0; i<NPRIVS; i++)	{
		apt[i] = privname(all_priv_buf[i], i);
	}
	return(apt);
}

int
cmppriv(ainfo)
struct ainfo *ainfo;
{
	int i, j, match, gfixcnt, ginhcnt, sfixcnt, sinhcnt;
	char *getfixp[NPRIVS];
	char *getinhp[NPRIVS];
	char *setfixp[NPRIVS];
	char *setinhp[NPRIVS];
	char *setptr;
	char *getptr;
	char **setp;

	fixprob = 0;
	inhprob = 0;

	/* this routine compares the privileges already on the file
	 * with the new ones that are being set - however, we must be
	 * careful in regard to what privileges we can get from the filepriv
	 * system call with GETPRV
	 */
	
	if(getfix) {
		/* get the fixed privilege on the file */
		getptr = (char *)malloc(strlen(fixstrp) + 1);
		(void) strcpy(getptr, fixstrp);
		getfixp[0] = strtok(getptr, ",");
		i=0;
		while(getfixp[i]) {
			getfixp[++i] = strtok(NULL, ",");
		}
		gfixcnt = i;

		/* get the privileges you want to set */

		/*
		** If we want to set "allprivs", use a list of all privileges for the
		** comparison instead of the word "allprivs".
		**
		** NOTE:
		**	We can't convert the privileges we got from the file to the
		**	word "allprivs" when appropriate, because if all privileges
		**	were explicitly listed in the pkgmap file, the comparison
		**	would fail.
		*/
		if(!strcmp(ainfo->priv_fix, "allprivs"))	{
			if(!all_priv_tab)	{
				/*
				** We need only do this once.
				*/
				all_priv_tab = gen_allprivs();
			}
			sfixcnt = NPRIVS;
			setp = all_priv_tab;
		}
		else	{
			setptr = (char *)malloc(strlen(ainfo->priv_fix) + 1);
			(void) strcpy(setptr, ainfo->priv_fix);
			i=0;
			if(!strcmp(setptr, "NULL")) {
				setfixp[i] = '\0';
			} else {
				setfixp[0] = strtok(setptr, ",");
				while(setfixp[i]) 
					setfixp[++i] = strtok(NULL, ",");
			}
			sfixcnt = i;
			setp = setfixp;
		}

		/* compare privileges */
		if(gfixcnt != sfixcnt)
			fixprob++;
		else {
			for(i=0; i<gfixcnt; i++) {
				match = 0;
				for(j=0; j<sfixcnt; j++) {
					if(!strcmp(getfixp[i], setp[j]))
						match++;
				}
				if(!match) 
					fixprob++;
			}
		} 
	}

	if(getinh) {
		getptr = (char *)malloc(strlen(inhstrp)+1);
		(void) strcpy(getptr, inhstrp);
		getinhp[0] = strtok(getptr, ",");
		i=0;
		while(getinhp[i]) {
			getinhp[++i] = strtok(NULL, ",");
		}
		ginhcnt = i;

		if(!strcmp(ainfo->priv_inh, "allprivs"))	{
			if(!all_priv_tab)	{
				/*
				** We need only do this once.
				*/
				all_priv_tab = gen_allprivs();
			}
			sinhcnt = NPRIVS;
			setp = all_priv_tab;
		}
		else	{
			setptr = (char *)malloc(strlen(ainfo->priv_inh) + 1);
			(void) strcpy(setptr, ainfo->priv_inh);
			i=0;
			if(!strcmp(setptr, "NULL")) {
				setinhp[i] = '\0';
			} else {
				setinhp[0] = strtok(setptr, ",");
				while(setinhp[i]) 
					setinhp[++i] = strtok(NULL, ",");
			}
			sinhcnt = i;
			setp = setinhp;
		}

		if(ginhcnt != sinhcnt)
			inhprob++;
		else {
			for(i=0; i<ginhcnt; i++) {
				match = 0;
				for(j=0; j<sinhcnt; j++) {
					if(!strcmp(getinhp[i], setp[j]))
						match++;
				}
				if(!match) 
					inhprob++;
			}
		} 
	}


	if(fixprob || inhprob)	{
		return(1);
	}
	else	{
		return(0);
	}
}

int
setlvl(path, lvl)
char *path;
level_t *lvl;
{
	/* this routine sets the level of path to lvl */

	if(lvlfile(path, MAC_SET, lvl) < 0)
		return(1);
	else
		return(0);
}
