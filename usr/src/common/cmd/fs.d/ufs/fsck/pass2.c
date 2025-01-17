/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

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

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/fsck/pass2.c	1.3.8.10"
/*  "errexit()" and "pfatal()" have been internationalized.
 *  The string to be output must at least include the message number
 *  and optionally a catalog name.
 *  The string is output using <MM_ERROR>.
 *
 *  "pwarn()" and "direrr()" have been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_WARNING>.
 */

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
#include <pfmt.h>

extern int traverse();

pass2()
{
	register DINODE *dp;
	struct inodesc inod, rootdesc;
	ino_t itmp;
	struct dirmap *tdirmapp;

	memset((char *)&rootdesc, 0, sizeof(struct inodesc));
        memset((char *)&inod, 0, sizeof(struct inodesc));
        rootdesc.id_type = inod.id_type = DATA;
	inod.id_func = rootdesc.id_func = NULL;
	inod.id_fix = rootdesc.id_fix = DONTKNOW;

        rootdesc.id_number = SFSROOTINO;
	pathp = pathname;
	switch (nstatemap(SFSROOTINO).flag) {
	case USTATE:
		dp = sginode(SFSROOTINO);
		WYPFLG(wflg, yflag, preen);
		getsem();
		pfatal(":254:ROOT INODE UNALLOCATED\n");
		if (reply(1, gettxt(":255","ALLOCATE")) == 0) {
			relsem();
			errexit("");
		}
		relsem();
		if (allocdir(SFSROOTINO, SFSROOTINO) != SFSROOTINO) {
			errexit(":256:CANNOT ALLOCATE ROOT INODE\n");
		}
		dp = sginode(SFSROOTINO);
		ckinode(dp, &rootdesc, 1, 1, 0, 0); 
		nstatemap(SFSROOTINO).flag = DSTATE;
		break;

	case DCLEAR:
		WYPFLG(wflg, yflag, preen);
		getsem();
		pfatal(":257:DUPS/BAD IN ROOT INODE\n");
		if (reply(1, gettxt(":192","REALLOCATE"))) {
			if (!bflg)
				Pprintf(":258:%s: DCLEAR\n", "pass2");
			relsem();
			freeino(SFSROOTINO);
			if (allocdir(SFSROOTINO, SFSROOTINO) != SFSROOTINO) {
				errexit(":256:CANNOT ALLOCATE ROOT INODE\n");
			}
			dp = sginode(SFSROOTINO);
			ckinode(dp, &rootdesc, 1, 1, 0, 0); 
			nstatemap(SFSROOTINO).flag = DSTATE;
			break;
		}
		if (reply(1, gettxt(":35","CONTINUE")) == 0) {
			relsem();
			errexit("");
		}
		relsem();
		nstatemap(SFSROOTINO).flag = DSTATE;
		break;

	case FSTATE:
	case FCLEAR:
		WYPFLG(wflg, yflag, preen);
		getsem();
		pfatal(":259:ROOT INODE NOT DIRECTORY\n");
		if (reply(1, gettxt(":192","REALLOCATE"))) {
			if (!bflg)
				Pprintf(":260:%s: FSTATE/FCLEAR\n", "pass2");
			relsem();
			freeino(SFSROOTINO);
			if (allocdir(SFSROOTINO, SFSROOTINO) != SFSROOTINO) {
				errexit(":256:CANNOT ALLOCATE ROOT INODE\n");
			}
			dp = sginode(SFSROOTINO);
			ckinode(dp, &rootdesc, 1, 1, 0, 0); 
			nstatemap(SFSROOTINO).flag = DSTATE;
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
		nstatemap(SFSROOTINO).flag = DSTATE;
		break;
	case DSTATE:
		chkdirsiz_descend(NULL, SFSROOTINO);
		break;
	default:
		errexit(":261:BAD STATE %d FOR ROOT INODE\n", nstatemap(SFSROOTINO).flag);
	}
	/* read all directory data blocks sequentially, check all entries
	 * and construct inolist, set dot and dotdot.
	 */
	for (itmp = SFSROOTINO; itmp < imax; itmp++) 
	{
		switch (nstatemap(itmp).flag) {
		case USTATE:
		case DCLEAR:
		case FCLEAR:
		case FSTATE:
       			break;
		case DSTATE:
			tdirmapp = nstatemap(itmp).dir_p;
			inod.id_number = itmp;
			inod.id_fix = DONTKNOW;
			inod.id_filesize = tdirmapp->filesize;	
			inod.id_loc = inod.id_entryno = 0;
			inmem_readdir(tdirmapp, 0, &inod, 0, 0, 0, 0);
			/* fall through */
		case DFOUND:
			break;
		default:
                       	errexit(":276:BAD STATE %d FOR INODE I=%d\n",
                           	nstatemap(itmp).flag, itmp);
		}
			
	}
	/* traverse the memory tree to check link count, ".", and  ".." */
	traverse(SFSROOTINO, SFSROOTINO, SFSROOTINO);
}
