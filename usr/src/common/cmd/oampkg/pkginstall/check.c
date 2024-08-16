/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*              copyright       "%c%"   */

#ident	"@(#)oampkg:common/cmd/oampkg/pkginstall/check.c	1.13.11.17"
#ident  "$Header: $"

#include <stdio.h>
#include <limits.h>
#include <langinfo.h>
#include <string.h>
#include <errno.h>
#include <utmp.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/keyctl.h>
#include <pkgstrct.h>
#include <pkglocs.h>
#include <unistd.h>
#include "install.h"

extern struct admin adm;
extern struct cfent
		**eptlist;
extern struct mergstat
		*mstat;
extern int	errno, ckquit,
		nointeract, nocnflct,
		nosetuid, rprcflag;
extern char	ilockfile[], rlockfile[],
		instdir[], savlog[],
		tmpdir[], pkgloc[],
		pkgbin[], pkgsav[],
		errbuf[], 
		*pkginst, *category, *msgtext;

extern char	*prodname;
extern char	*pkgname;

extern char	*qstrdup(),
		*getenv();
extern struct utmp 
		*getutid();
extern void	progerr(), logerr(),
		echo(), ptext(),
		quit(), free();
extern int	access(), mkdir(),
		ckyorn(), dockdeps(),
		dockspace(), creat(),
		isdir(), isfile(),
		close(), cverify();
extern int	ckyorn_match();

#define DISPSIZ	20	/* number of items to display on one page */
#define	DEF_FILE_MOD	0644
#define	DEF_DIR_MOD	0755

#define MSG_RUNLEVEL \
	gettxt(":280", "\\nThe current run-level of this machine is <%s>, 	which is not a run-level suggested for installation of 	this package.  Suggested run-levels (in order of preference) include:")
#define HLP_RUNLEVEL \
	gettxt(":281", "If this package is not installed in a run-level which has been 	suggested, it is possible that the package may not install or 	operate properly.  If you wish to follow the run-level suggestions, 	answer 'n' to stop installation of the package.")
#define MSG_STATECHG \
	gettxt(":282", "\\nTo change states, execute\\n\\tshutdown -y -i%s -g0\\n	after exiting the installation process. 	Please note that after changing states you 	may have to mount appropriate filesystem(s) 	in order to install this package.")

#define ASK_CONFLICT	gettxt(":283", "Do you want to install these conflicting files")
#define MSG_CONFLICT \
	gettxt(":284", "\\nThe following files are already installed on the system and 	are being used by another package:")
#define HLP_CONFLICT \
	gettxt(":285", "If you choose to install conflicting files, the files listed above 	will be overwritten and/or have their access permissions changed.  	If you choose not to install these files, installation will proceed 	but these specific files will \
not be installed.  Note that sane 	operation of the software being installed may require these files 	be installed; thus choosing to not to do so may cause inappropriate 	operation.  If you wish to stop installation of this package, 	enter 'q' to quit.")

#define ASK_SETUID	gettxt(":286", "Do you want to install these files")
#define MSG_SETUID \
	gettxt(":287", "\\nThe following files are being installed with setuid and/or setgid 	permissions or are overwriting files which are currently setuid/setgid:")
#define HLP_SETUID \
	gettxt(":288", "The package being installed appears to contain processes which will 	have their effective user or group ids set upon execution.  History 	has shown that these types of processes can be a source of major 	security impact on your system.\
If you choose not to install these files, installation will proceed 	but these specific files will not be installed.  Note that sane 	operation of the software being installed may require these files 	be installed; thus choosing to not to do so may cause inappropriate 	operation.  If you wish to stop installation of this package, 	enter 'q' to quit.")

#define MSG_PARTINST \
	gettxt(":289", "\\nThe installation of this package was previously terminated 	and installation was never successfully completed.")
#define MSG_PARTREM \
	gettxt(":290", "\\nThe removal of this package was terminated at some point 	in time, and package removal was only partially completed.")
#define HLP_PARTIAL \
	gettxt(":291", "Installation of partially installed packages is normally allowable, 	but some packages providers may suggest that a partially installed 	package be completely removed before re-attempting installation. 	Check the documentation provided with this package, and then answer 	'y' if you feel it is advisable to continue the installation process.")

#define HLP_SPACE \
	gettxt(":292", "It appears that there is not enough free space on your system in 	which to install this package.  It is possible that one or more 	filesystems are not properly mounted.  Neither installation of the 	package nor its operation can be guaranteed under these conditions. 	If you choose to disregard this warning, enter 'y' to continue the 	installation process.")
#define HLP_DEPEND \
	gettxt(":293", "The package being installed has indicated a dependency on 	the existence (or non-existence) of another software package. 	If this dependency is not met before continuing, the package 	may not install or operate properly.  If you wish to disregard 	this dependency, answer 'y' to continue the installation process.")

#define MSG_PRIV \
	gettxt(":294", "\\nThis package contains scripts which may have a security 	impact and which will be executed during the process of installing 	this package.")
#define HLP_PRIV \
	gettxt(":295", "During the installation of this package, certain scripts provided 	with the package will be executed. These scripts may modify or 	otherwise change your system without your knowledge.  If you are 	certain of the origin and trustworthiness of the package being 	installed, answer 'y' to continue the installation process.")

#define ASK_CONT \
	gettxt(":296", "Do you want to continue with the installation of this package")
#define HLP_CONT \
	gettxt(":297", "If you choose 'y', installation of this package will continue.  	If you want to stop installation of this package, choose 'n'.")

#define ERR_EXIST \
	":298:\nThis package expects <%s> to have been previously installed."

#define	MSG_HITRET \
	":299:[Hit [RETURN] to continue display]"

#define MSG_KEYCTL \
	"uxpkgtools:806:- Validation of <%s> package failed, errno=%d"

static int	mkpath();
void
ckpartial()
{
	char	ans[INPBUF];
	int	n;

	if(ADM(partial, "nocheck"))
			return;

	if(access(ilockfile, 0) == 0) {
		msgtext = MSG_PARTINST;
		ptext(stderr, msgtext);
		if(ADM(partial, "quit"))
				quit(4);
		if(nointeract)
			quit(5);
		msgtext = NULL;

		ckquit = 0;
		if(n = ckyorn(ans, NULL, NULL, HLP_PARTIAL, ASK_CONT))
			quit(n);
		if (!ckyorn_match(ans, nl_langinfo(YESEXPR)))
			quit(3);
		ckquit = 1;
	}

	if(access(rlockfile, 0) == 0) {
		msgtext = MSG_PARTREM;
		ptext(stderr, msgtext);
		if(ADM(partial, "quit"))
			quit(4);
		if(nointeract)
			quit(5);
		msgtext = NULL;

		ckquit = 0;
		if(n = ckyorn(ans, NULL, NULL, HLP_PARTIAL, ASK_CONT))
			quit(n);
		if (!ckyorn_match(ans, nl_langinfo(YESEXPR)))
			quit(3);
		ckquit = 1;
	}
}

void
ckrunlevel()
{
	struct utmp utmp;
	struct utmp *putmp;
	char	ans[INPBUF], *pt, *istates, *pstate;
	int	n;
	char	*uxstate;

	if(ADM(runlevel, "nocheck"))
		return;

	pt = getenv("ISTATES");
	if(pt == NULL)
		return;

	utmp.ut_type = RUN_LVL;
	putmp = getutid(&utmp);
	if(putmp == NULL) {
		progerr(":300:unable to determine current run-state");
		quit(99);
	}
	uxstate = strtok(&putmp->ut_line[10], " \t\n");
	
	istates = qstrdup(pt);
	if((pt = strtok(pt, " \t\n,")) == NULL)
		return; /* no list is no list */
	pstate = pt;
	do {
		if(!strcmp(pt, uxstate)) {
			free(istates);
			return;
		}
	} while(pt = strtok(NULL, " \t\n,"));

	msgtext = MSG_RUNLEVEL;
	ptext(stderr, msgtext, uxstate);
	pt = strtok(istates, " \t\n,");
	do {
		ptext(stderr, "\\t%s", pt);
	} while(pt = strtok(NULL, " \t\n,"));
	free(istates);
	if(ADM(runlevel, "quit"))
		quit(4);
	if(nointeract)
		quit(5);
	msgtext = NULL;

	ckquit = 0;
	if(n = ckyorn(ans, NULL, NULL, HLP_RUNLEVEL, ASK_CONT))
		quit(n);
	ckquit = 1;

	if (ckyorn_match(ans, nl_langinfo(NOEXPR))) {
		ptext(stderr, MSG_STATECHG, pstate);
		quit(3);
	}
}

void
ckdepend()
{
	int	n, i;
	char	ans[INPBUF];
	char	path[PATH_MAX];
	int	ftype;	

	for(i=0; eptlist[i]; i++) {
		/*
		 * The following section was added to rectify inconsistencies
		 * between pre-4.1 and 4.1 semantics of a '?' located in the
		 * mode, owner and group fields of the pkgmap file.  In 4.1
		 * this was used to force taking on the attributes of an existing
		 * file or directory.  The package installation would abort if
		 * this file or directory did not already exist.  In 4.0, the
		 * behavior differred in that if the file or directory did not
		 * exist, it would get created with default values.  In order
		 * to enable backward compatibility (allowing installation of
		 * pre-4.1 packages on a 4.1 system), the following section checks
		 * to see if the object does not exist and creates it with default
		 * values (mode: 644 for file, 755 for directory; owner: root and
		 * group: other).
		 */
		ftype=0;
		if(strchr("dxcbpfve", eptlist[i]->ftype)) {
			if(isdir(eptlist[i]->path) != 0 && isfile(eptlist[i]->path) != 0) {
				if('f' == eptlist[i]->ftype)
					ftype++;
				if(eptlist[i]->ainfo.mode == BADMODE) {
					if(ftype) {
						eptlist[i]->ainfo.mode = DEF_FILE_MOD;
						ptext(stderr, gettxt(":301", "## WARNING: No attributes defined for file <%s>."), eptlist[i]->path);
						ptext(stderr, gettxt(":302", "## WARNING: Setting mode: 644, owner: root, group: other"));
					}
					else {
						eptlist[i]->ainfo.mode = DEF_DIR_MOD;
						ptext(stderr, gettxt(":303", "## WARNING: No attributes defined for directory <%s>."), eptlist[i]->path);
						ptext(stderr, gettxt(":304", "## WARNING: Setting mode: 755, owner: root, group: other"));
					}
				}
				if(!strcmp(eptlist[i]->ainfo.owner, BADOWNER))
					(void) strcpy(eptlist[i]->ainfo.owner, "root");
				if(!strcmp(eptlist[i]->ainfo.group, BADGROUP))
					(void) strcpy(eptlist[i]->ainfo.group, "other");
			}

			if((eptlist[i]->ainfo.mode == BADMODE) || 
			(!strcmp(eptlist[i]->ainfo.owner, BADOWNER)) ||
			(!strcmp(eptlist[i]->ainfo.group, BADGROUP)) ||
			(eptlist[i]->ainfo.macid == BADMAC) ||
			(!strcmp(eptlist[i]->ainfo.priv_fix, BADPRIV)) ||
			(!strcmp(eptlist[i]->ainfo.priv_inh, BADPRIV))) 
				if(access(eptlist[i]->path, F_OK) < 0) {
					progerr(ERR_EXIST, eptlist[i]->path);
					quit(1);
				}
		}
	}
		
	if(ADM(idepend, "nocheck"))
		return;

	(void) sprintf(path, "%s/install/depend", instdir);
	if(access(path, 0))
		return; /* no dependency file provided by package */

	echo(":305:## Verifying package dependencies.");
	if(dockdeps(path, 0)) {
		msgtext = gettxt(":306", "Dependency checking failed.");
		if(ADM(idepend, "quit"))
			quit(4);
		if(nointeract)
			quit(5);
		msgtext = NULL;

		ckquit = 0;
		if(n = ckyorn(ans, NULL, NULL, HLP_DEPEND, ASK_CONT))
			quit(n);
		if (!ckyorn_match(ans, nl_langinfo(YESEXPR)))
			quit(3);
		ckquit = 1;
	}
}

#include <pkginfo.h>

static struct pkginfo info;

void
ckspace()
{
	int	n;
	char	ans[INPBUF];
	char	path[PATH_MAX];

	if(ADM(space, "nocheck"))
		return;


	if(ADM(instance, "overwrite")) {
		/*
		 * For overlay installation, check if the package is 
		 * already completely installed.  If so, we can skip
		 * space checking since we're simply overlaying the
		 * package in place.
		 */
		(void) pkginfo(&info, NULL);
		(void) pkginfo(&info, pkginst, NULL, NULL); 
		if(info.status == PI_INSTALLED) {
			(void) pkginfo(&info, NULL);
			return;
		}
		(void) pkginfo(&info, NULL);
	}

	/*
	 * For set installation, the space file is created from the SIP's
	 * request script by the setsizecvt utility.  If the directory
	 * 'instdir' is located on a read-only file system, then setsizecvt 
	 * would not be able to write to that directory.  For this reason,
	 * if the current pkginst is a SIP, let's look for the space file 
	 * where setsizecvt should have placed it.
	 */
	echo(":307:## Verifying disk space requirements.");
	if(!strcmp(category, "set"))
		(void) sprintf(path, "/tmp/%s.space", pkginst);
	else
		(void) sprintf(path, "%s/install/space", instdir);
	(void) pkginfo(&info, NULL);
	if(access(path, 0) == 0)
		n = dockspace(path);
	else
		n = dockspace(NULL);

	if(n) {
		msgtext = gettxt(":308", "Space checking failed.");
		if(ADM(space, "quit"))
			quit(4);
		if(nointeract)
			quit(5);
		msgtext = NULL;

		ckquit = 0;
		if(n = ckyorn(ans, NULL, NULL, HLP_SPACE, ASK_CONT))
			quit(n);
		if (!ckyorn_match(ans, nl_langinfo(YESEXPR)))
			quit(3);
		ckquit = 1;
	}
}

void
ckdirs()
{
	char	path[PATH_MAX];

 	if(mkpath(PKGADM)) {
		progerr(":309:unable to make packaging directory <%s>", PKGADM);
		quit(99);
	}
	(void) sprintf(path, "%s/admin", PKGADM);
 	if(mkpath(path)) {
		progerr(":309:unable to make packaging directory <%s>", path);
		quit(99);
	}
	(void) sprintf(path, "%s/logs", PKGADM);
 	if(mkpath(path)) {
		progerr(":309:unable to make packaging directory <%s>", path);
		quit(99);
	}
 	if(mkpath(PKGSCR)) {
		progerr(":309:unable to make packaging directory <%s>", PKGSCR);
		quit(99);
	}
	if(mkpath(PKGLOC)) {
		progerr(":309:unable to make packaging directory <%s>", PKGLOC);
		quit(99);
	}
}

void
ckpkgdirs()
{
 	if(mkpath(pkgloc)) {
		progerr(":309:unable to make packaging directory <%s>", pkgloc);
		quit(99);
	}
 	if(mkpath(pkgbin)) {
		progerr(":309:unable to make packaging directory <%s>", pkgbin);
		quit(99);
	}
 	if(mkpath(pkgsav)) {
		progerr(":309:unable to make packaging directory <%s>", pkgsav);
		quit(99);
	}
}

static int
mkpath(p)
char *p;
{
	char	*pt;

	pt = (*p == '/') ? p+1 : p;
	do {
		if(pt = strchr(pt, '/'))
			*pt = '\0';
		if(access(p, 0) && mkdir(p, 0755))
			return(-1);
		if(pt)
			*pt++ = '/';
	} while(pt);
	return(0);
}

void
ckconflct()
{
	int	i, n, count;
	char	ans[INPBUF];

	if(ADM(conflict, "nochange")) {
		nocnflct++;
		return;
	} else if(ADM(conflict, "nocheck"))
		return;

	echo(":310:## Checking for conflicts with packages already installed.");
	count = 0;
	for(i=0; eptlist[i]; i++) {
		if(!mstat[i].shared)
			continue;
		if(mstat[i].contchg) {
			if(!count++) 
				ptext(stderr, MSG_CONFLICT);
			else if(!nointeract && ((count % DISPSIZ) == 0)) {
				echo(MSG_HITRET);
				(void) getc(stdin);
			}
			echo("\t%s", eptlist[i]->path);
		} else if(mstat[i].attrchg) {
			if(!count++) 
				ptext(stderr, MSG_CONFLICT);
			else if(!nointeract && ((count % DISPSIZ) == 0)) {
				echo(MSG_HITRET);
				(void) getc(stdin);
			}
			echo(gettxt(":311", "\t%s <attribute change only>"), eptlist[i]->path);
		}
	}

	if(count) {
		msgtext = gettxt(":312", "Conflict checking failed.");
		if(ADM(conflict, "quit"))
			quit(4);
		if(nointeract)
			quit(5);
		msgtext = NULL;

		if(n = ckyorn(ans, NULL, NULL, HLP_CONFLICT, ASK_CONFLICT))
			quit(n);
		if (ckyorn_match(ans, nl_langinfo(NOEXPR))) {
			ckquit = 0;
			if(n = ckyorn(ans, NULL, NULL, HLP_CONT, ASK_CONT))
				quit(n);
			if (!ckyorn_match(ans, nl_langinfo(YESEXPR)))
				quit(3);
			ckquit = 1;
			nocnflct++;
			rprcflag++;
		}
	}
}

void
cksetuid()
{
	int	i, n, count;
	char	ans[INPBUF];

	if(ADM(setuid, "nocheck"))
		return;

	if(ADM(setuid, "nochange")) {
		nosetuid++;
		return;
	}

	echo(gettxt(":313", "## Checking for setuid/setgid programs."));
	count = 0;
	for(i=0; eptlist[i]; i++) {
		if(mstat[i].setuid) {
			if(!count++) 
				ptext(stderr, MSG_SETUID);
			else if(!nointeract && ((count % DISPSIZ) == 0)) {
				echo(MSG_HITRET);
				(void) getc(stdin);
			}
			if(eptlist[i]->ainfo.mode & S_ISUID)	
				echo(":314:\t%s <setuid %s>", eptlist[i]->path, 
					eptlist[i]->ainfo.owner);
			else
				echo(":315:\t%s <setuid will be removed>", 
					eptlist[i]->path);
		}
		if(mstat[i].setgid) {
			if(!count++) 
				ptext(stderr, MSG_SETUID);
			else if(!nointeract && ((count % DISPSIZ) == 0)) {
				echo(MSG_HITRET);
				(void) getc(stdin);
			}
			if((eptlist[i]->ainfo.mode & S_ISGID) && 
				(eptlist[i]->ainfo.mode & S_IXGRP))
				echo(":316:\t%s <setgid %s>", eptlist[i]->path, 
					eptlist[i]->ainfo.group);
			else
				echo(":317:\t%s <setgid will be removed>", 
					eptlist[i]->path);
		}
	}

	if(count) {
		msgtext = gettxt(":318", "Setuid/setgid processes detected.");
		if(ADM(setuid, "quit"))
			quit(4);
		if(nointeract)
			quit(5);
		msgtext = NULL;

		if(n = ckyorn(ans, NULL, NULL, HLP_SETUID, ASK_SETUID))
			quit(n);
		if (ckyorn_match(ans, nl_langinfo(NOEXPR))) {
			ckquit = 0;
			if(n = ckyorn(ans, NULL, NULL, HLP_CONT, ASK_CONT))
				quit(n);
			if (!ckyorn_match(ans, nl_langinfo(YESEXPR)))
				quit(3);
			ckquit = 1;
			nosetuid++;
			rprcflag++;
		}
	}
}

void
ckpriv()
{
	struct dirent *dp;
	DIR	*dirfp;
	int	n, found;
	char	ans[INPBUF], path[PATH_MAX];

	if(ADM(action, "nocheck"))
		return;
	
	(void) sprintf(path, "%s/install", instdir);
	if((dirfp = opendir(path)) == NULL) 
		return;

	found = 0;
	while((dp = readdir(dirfp)) != NULL) {
		if(!strcmp(dp->d_name, "preinstall") ||
		   !strcmp(dp->d_name, "postinstall") ||
		   !strncmp(dp->d_name, "i.", 2)) {
			found++;
			break;
		}
	}
	(void) closedir(dirfp);

	if(found) {
		ptext(stderr, MSG_PRIV);
		msgtext = gettxt(":319", "Package scripts were found.");
		if(ADM(action, "quit"))
			quit(4);
		if(nointeract)
			quit(5);
		msgtext = NULL;

		ckquit = 0;
		if(n = ckyorn(ans, NULL, NULL, HLP_PRIV, ASK_CONT))
			quit(n);
		if (!ckyorn_match(ans, nl_langinfo(YESEXPR)))
			quit(3);
		ckquit = 1;
	}
}

void
ckpkgfiles()
{
	register int i;
	struct cfent	*ept;
	int	errflg;
	char	source[PATH_MAX];

	errflg = 0;
	for(i=0; eptlist[i]; i++) {
		ept = eptlist[i];
		if(ept->ftype != 'i')
			continue;

		if(ept->ainfo.local) {
			(void) sprintf(source, "%s/%s", instdir, 
				ept->ainfo.local);
		} else 
			/*
			 * If this is a set installation package install setinfo 
			 * file in the same directory as where the pkginfo file.
			 */
			if(!strcmp(ept->path, PKGINFO) || !strcmp(ept->path, SETINFO)) {
			(void) sprintf(source, "%s/%s", instdir, ept->path);
		} else {
			(void) sprintf(source, "%s/install/%s", instdir, 
				ept->path);
		}
		if(cverify(0, &ept->ftype, source, &ept->cinfo)) {
			errflg++;
			progerr(":320:packaging file <%s> is corrupt", source);
			logerr(errbuf);
		}
	}
	if(errflg)
		quit(99);
}

#define	PDLEN		3
#define	CHECK_SEQ	"4210863579"	/* Optimation used in guessing the
					   the number of processors */

int pkg_validate(k_skey_t *, char *, int);

ckvalidate()
{
	int n;
	int err = 0;
	register char *chk_seq = CHECK_SEQ;
	register char *snum;
	register char *skey;
	k_skey_t skeys;

	echo(":807:## Performing product validation for package.");
	if ((snum = getenv("SERIALNUM")) == NULL || *snum == '\0' ||
	    (skey = getenv("SERIALKEY")) == NULL || *skey == '\0') {
		if (nointeract) {
			quit(5);
		}
		if (n = pkgserid(prodname, pkgname, pkginst, &skeys)) {
			quit(n);
		}
		/*
		 * Since we prompted the user for the serial number,
		 * we need to guess the number of processors by iterating
		 * over 0-9.
		 */
		for (n = 0; chk_seq[n] != '\0'; n++) {
			skeys.sernum[PDLEN] = chk_seq[n];
			if ((err = pkg_validate(&skeys, prodname, 1)) == 0) {
				break;
			}
		}
	} else {
		(void)strncpy((char *)skeys.sernum + PDLEN, snum,
				STRLEN - PDLEN);
		(void)strncpy((char *)skeys.serkey, skey, STRLEN);
		err = pkg_validate(&skeys, prodname, 1);
	}
	if (err) {
		progerr(MSG_KEYCTL, pkginst, errno);
		quit(99);
	}
	putparam("SERIALID", skeys.sernum);
	putparam("SERIALKEY", skeys.serkey);
}
