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
#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/fsck/pass3.c	1.2.8.7"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fs.d/ufs/fsck/pass3.c,v 1.1 91/02/28 17:28:36 ccs Exp $"

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
#include "fsck.h"

int	pass2check();

pass3()
{
	struct inodesc idesc;
	ino_t inumber, orphan;
	struct dirmap *dirp;
	char state;

	memset((char *)&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = DATA;
	for (inumber = SFSROOTINO; inumber < imax; inumber++) {
		state = get_state(inumber);
		if (state == DSTATE) {
		    if (MEM) {
			WYPFLG(wflag, yflag, preen);
			orphan = inumber;
			dirp = nstatemap(orphan).dir_p;
			idesc.id_parent = lfdir;
			if (lncntp[inumber] <= 0) {
				continue;
			}
			if (dirp->filesize == 0) {
				nstatemap(inumber).flag = DCLEAR;
				continue;
			}

			do {
				if (nstatemap(dirp->dotdot).flag != DSTATE)
					break;
				orphan = dirp->dotdot;
				dirp = nstatemap(orphan).dir_p;
			} while (nstatemap(dirp->dotdot).flag == DSTATE);
			if (linkup(orphan, lfdir) == 1) {
				chkdirsiz_descend(NULL, orphan);
				if (nstatemap(orphan).flag == DSTATE) {
					idesc.id_number = orphan;
					idesc.id_fix = DONTKNOW;
					idesc.id_func = 0;
					idesc.id_filesize = dirp->filesize;	
					idesc.id_loc = idesc.id_entryno = 0;
					inmem_readdir(dirp, 1, &idesc, 0, 0, 0, 1);
					traverse(orphan, lfdir, lfdir);
				}
			}
	        } else {
		int loopcnt;

			pathp = pathname;
			*pathp++ = '?';
			*pathp = '\0';
			idesc.id_func = findino;
			idesc.id_name = "..";
			idesc.id_parent = inumber;
			loopcnt = 0;
			do {
				orphan = idesc.id_parent;
				if (orphan < SFSROOTINO || orphan > imax)
					break;
				idesc.id_parent = 0;
				idesc.id_number = orphan;
				(void)ckinode(sginode(orphan), &idesc, 0,0,0,0);
				if (idesc.id_parent == 0)
					break;
				if (loopcnt >= sblock.fs_cstotal.cs_ndir)
					break;
				loopcnt++;
			} while (ostatemap(idesc.id_parent) == DSTATE);
			if (linkup(orphan, idesc.id_parent) == 1) {
				idesc.id_func = pass2check;
				idesc.id_number = lfdir;
				chkdirsiz_descend(&idesc, orphan);
			}
		}
 	    }
	}
}
