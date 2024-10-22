/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
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

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/fsck/Opass2.c	1.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fs.d/ufs/fsck/pass2.c,v 1.1 91/02/28 17:28:33 ccs Exp $"

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mntent.h>
#include <sys/fs/sfs_fs.h>
#include <sys/vnode.h>
#include <sys/acl.h>
#include <sys/fs/sfs_inode.h>
#define _KERNEL
#include <sys/fs/sfs_fsdir.h>
#undef _KERNEL
#include <sys/mnttab.h>
#include <string.h>
#include "fsck.h"

int	pass2check();

Opass2()
{
	register DINODE *dp;
	struct inodesc rootdesc;

	memset((char *)&rootdesc, 0, sizeof(struct inodesc));
	rootdesc.id_type = ADDR;
	rootdesc.id_func = pass2check;
	rootdesc.id_number = SFSROOTINO;
	pathp = pathname;
	switch (ostatemap(SFSROOTINO)) {
	case USTATE:
		WYPFLG(wflg, yflag, preen);
		getsem();
		pfatal(":254:ROOT INODE UNALLOCATED");
		if (reply(1, gettxt(":255", "ALLOCATE")) == 0) {
			relsem();
			errexit("");
		}
		relsem();
		if (allocdir(SFSROOTINO, SFSROOTINO) != SFSROOTINO)
			errexit(":256:CANNOT ALLOCATE ROOT INODE\n");
		chkdirsiz_descend(&rootdesc, SFSROOTINO);
		break;

	case DCLEAR:
		WYPFLG(wflg, yflag, preen);
		getsem();
		pfatal(":257:DUPS/BAD IN ROOT INODE");
		if (reply(1, gettxt( ":192", "REALLOCATE"))) {
			if (!bflag)
				Pprintf(":258:%s: DCLEAR\n","pass2");
			relsem();
			freeino(SFSROOTINO);
			if (allocdir(SFSROOTINO, SFSROOTINO) != SFSROOTINO)
				errexit(":256:CANNOT ALLOCATE ROOT INODE\n");
			chkdirsiz_descend(&rootdesc, SFSROOTINO);
			break;
		}
		if (reply(1, gettxt(":35","CONTINUE")) == 0) {
			relsem();
			errexit("");
		}
		relsem();
		ostatemap(SFSROOTINO) = DSTATE;
		chkdirsiz_descend(&rootdesc, SFSROOTINO);
		break;

	case FSTATE:
	case FCLEAR:
		WYPFLG(wflg, yflag, preen);
		getsem();
		pfatal(":259:ROOT INODE IS NOT DIRECTORY\n");
		if (reply(1, gettxt(":192","REALLOCATE"))) {
			if (!bflg)
				Pprintf(":260:%s: FSTATE/FCLEAR\n","pass2");
			relsem();
			freeino(SFSROOTINO);
			if (allocdir(SFSROOTINO, SFSROOTINO) != SFSROOTINO)
				errexit(":256:CANNOT ALLOCATE ROOT INODE\n");
			chkdirsiz_descend(&rootdesc, SFSROOTINO);
			break;
		}
		if (reply(1, gettxt(":42","FIX")) == 0) {
			relsem();
			errexit("");
		}
		relsem();
		dp = sginode(SFSROOTINO);
		dp->di_mode &= ~IFMT;
		dp->di_mode |= IFDIR;
		dp->di_smode = dp->di_mode;
		inodirty();
		ostatemap(SFSROOTINO) = DSTATE;
		/* fall into ... */

	case DSTATE:
		chkdirsiz_descend(&rootdesc, SFSROOTINO);
		break;

	default:
		errexit(":261:BAD STATE %d FOR ROOT INODE", ostatemap(SFSROOTINO));
	}
}

pass2check(idesc)
	struct inodesc *idesc;
{
	register DIRECT *dirp = idesc->id_dirp;
	char *curpathloc;
	int n, entrysize, ret = 0;
	DINODE *dp;
	DIRECT proto;
	char namebuf[BUFSIZ];

	/* 
	 * check for "."
	 */
	if (idesc->id_entryno != 0)
		goto chk1;
	getsem();
	if (dirp->d_ino != 0 && strcmp(dirp->d_name, ".") == 0) {
		if (dirp->d_ino != idesc->id_number) {
			direrr(idesc->id_number, ":262:BAD INODE NUMBER FOR '.'");
			dirp->d_ino = idesc->id_number;
			if (reply(1, gettxt(":373:","FIX")) == 1)
				ret |= ALTERED;
		}
		relsem();
		goto chk1;
	}
	direrr(idesc->id_number, ":263:MISSING '.'");
	relsem();
	proto.d_ino = idesc->id_number;
	proto.d_namlen = 1;
	(void)strcpy(proto.d_name, ".");
	entrysize = DIRSIZ(&proto);
	if (dirp->d_ino != 0 && strcmp(dirp->d_name, "..") != 0) {
		pfatal(":264:CANNOT FIX, FIRST ENTRY IN DIRECTORY CONTAINS %s\n",
			dirp->d_name);
	} else if ((int)dirp->d_reclen < entrysize) {
		pfatal(":265:CANNOT FIX, INSUFFICIENT SPACE TO ADD '.'\n");
	} else if ((int)dirp->d_reclen < 2 * entrysize) {
		proto.d_reclen = dirp->d_reclen;
		memcpy((char *)dirp, (char *)&proto, entrysize);
		getsem();
		if (reply(1, gettxt(":42","FIX")) == 1)
			ret |= ALTERED;
		relsem();
	} else {
		n = dirp->d_reclen - entrysize;
		proto.d_reclen = entrysize;
		memcpy((char *)dirp, (char *)&proto, entrysize);
		idesc->id_entryno++;
		lncntp[dirp->d_ino]--;
		dirp = (DIRECT *)((char *)(dirp) + entrysize);
		memset((char *)dirp, 0, n);
		dirp->d_reclen = n;
		getsem();
		if (reply(1,gettxt(":42","FIX")) == 1)
			ret |= ALTERED;
		relsem();
	}
chk1:
	if (idesc->id_entryno > 1)
		goto chk2;
	proto.d_ino = idesc->id_parent;
	proto.d_namlen = 2;
	(void)strcpy(proto.d_name, "..");
	entrysize = DIRSIZ(&proto);
	if (idesc->id_entryno == 0) {
		n = DIRSIZ(dirp);
		if ((int)dirp->d_reclen < n + entrysize)
			goto chk2;
		proto.d_reclen = dirp->d_reclen - n;
		dirp->d_reclen = n;
		idesc->id_entryno++;
		lncntp[dirp->d_ino]--;
		dirp = (DIRECT *)((char *)(dirp) + n);
		memset((char *)dirp, 0, n);
		dirp->d_reclen = n;
	}
	if (dirp->d_ino != 0 && strcmp(dirp->d_name, "..") == 0) {
		if (dirp->d_ino != idesc->id_parent) {
			getsem();
			direrr(idesc->id_number, ":266:BAD INODE NUMBER FOR '..'");
			dirp->d_ino = idesc->id_parent;
			if (reply(1, gettxt( ":42","FIX")) == 1)
				ret |= ALTERED;
			relsem();
		}
		goto chk2;
	}
	getsem();
	direrr(idesc->id_number, ":267:MISSING '..'");
	relsem();
	if (dirp->d_ino != 0 && strcmp(dirp->d_name, ".") != 0) {
		pfatal(":268:CANNOT FIX, SECOND ENTRY IN DIRECTORY CONTAINS %s\n",
			dirp->d_name);
	} else if ((int)dirp->d_reclen < entrysize) {
		pfatal(":269:CANNOT FIX, INSUFFICIENT SPACE TO ADD '..'\n");
	} else {
		proto.d_reclen = dirp->d_reclen;
		memcpy((char *)dirp, (char *)&proto, entrysize);
		getsem();
		if (reply(1, gettxt( ":42","FIX")) == 1)
			ret |= ALTERED;
		relsem();
	}
chk2:
	if (dirp->d_ino == 0)
		return (ret|KEEPON);
	if (dirp->d_namlen <= 2 &&
	    dirp->d_name[0] == '.' &&
	    idesc->id_entryno >= 2) {
		if (dirp->d_namlen == 1) {
			getsem();
			direrr(idesc->id_number, ":270:EXTRA '.' ENTRY");
			dirp->d_ino = 0;
			if (reply(1, gettxt(":42","FIX")) == 1)
				ret |= ALTERED;
			relsem();
			return (KEEPON | ret);
		}
		if (dirp->d_name[1] == '.') {
			getsem();
			direrr(idesc->id_number, ":271:EXTRA '..' ENTRY");
			dirp->d_ino = 0;
			if (reply(1, gettxt(":42","FIX")) == 1)
				ret |= ALTERED;
			relsem();
			return (KEEPON | ret);
		}
	}
	curpathloc = pathp;
	*pathp++ = '/';
	if (pathp + dirp->d_namlen >= endpathname) {
		*pathp = '\0';
		errexit(":372:NAME TOO LONG %s%s\n", pathname, dirp->d_name);
	}
	memcpy(pathp, dirp->d_name, dirp->d_namlen + 1);
	pathp += dirp->d_namlen;
	idesc->id_entryno++;
	n = 0;
	if (dirp->d_ino > imax || dirp->d_ino <= 0) {
		getsem();
		direrr(dirp->d_ino, ":374:I OUT OF RANGE");
		n = reply(1,gettxt(":86","REMOVE"));
		relsem();
	} else {
again:
		switch (ostatemap(dirp->d_ino)) {
		case USTATE:
			getsem();
			direrr(dirp->d_ino, ":81:UNALLOCATED");
			n = reply(1,gettxt(":86","REMOVE"));
			relsem();
			break;

		case DCLEAR:
		case FCLEAR:
			getsem();
			direrr(dirp->d_ino, ":82:DUP/BAD");
			if ((n = reply(1,gettxt(":86","REMOVE"))) == 1) {
				relsem();
				break;
			}
			relsem();
			dp = sginode(dirp->d_ino);
			ostatemap(dirp->d_ino) = DIRCT(dp) ? DSTATE : FSTATE;
			goto again;

		case DFOUND:
			if (idesc->id_entryno > 2) {
				getpathname(namebuf, dirp->d_ino, dirp->d_ino);
				pwarn("%s %s %s\n", pathname,
				    ":273:IS AN EXTRANEOUS HARD LINK TO DIRECTORY",
				    namebuf);
				getsem();
				if (preen)
					Pprintf(":274: (IGNORED)\n");
				else if ((n = reply(1,gettxt(":86","REMOVE"))) == 1) {
					relsem();
					break;
				}
				relsem();
			}
			/* fall through */

		case FSTATE:
			lncntp[dirp->d_ino]--;
			break;

		case DSTATE:
			chkdirsiz_descend(idesc, dirp->d_ino);
			if (ostatemap(dirp->d_ino) == DFOUND) {
				lncntp[dirp->d_ino]--;
			} else if (ostatemap(dirp->d_ino) == DCLEAR) {
				dirp->d_ino = 0;
				ret |= ALTERED;
			} else
				errexit(":373:BAD RETURN STATE %d FROM DESCEND",
				    ostatemap(dirp->d_ino));
			break;

		default:
			errexit(":276:BAD STATE %d FOR INODE I=%d",
			    ostatemap(dirp->d_ino), dirp->d_ino);
		}
	}
	pathp = curpathloc;
	*pathp = '\0';
	if (n == 0)
		return (ret|KEEPON);
	dirp->d_ino = 0;
	return (ret|KEEPON|ALTERED);
}
