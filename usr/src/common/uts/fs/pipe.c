/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/pipe.c	1.5"

#include <acc/audit/audit.h>
#include <acc/audit/auditrec.h>
#include <fs/fifofs/fifonode.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#endif

/*
 * int pipe(char *uap, rval_t *rvp)
 *
 * 	pipe(2) system call.
 *
 * Calling/Exit State:
 *	No Locking assumption.
 *
 * Description:
 * 	Create a pipe by connecting two streams together. Associate
 * 	each end of the pipe with a vnode, a file descriptor and 
 * 	one of the streams.
 */
/*ARGSUSED*/
int
pipe(char *uap, rval_t *rvp)
{
	struct vnode *vp1, *vp2;
	struct file *fp1, *fp2;
	register int error = 0;
	int fd1, fd2;

	/*
	 * Make pipe ends.
	 */
	if (error = fifo_mkpipe(&vp1, &vp2, CRED()))
		return (error);
	/*
	 * Allocate and initialize two file table entries and two
	 * file pointers. Each file pointer is open for read and write.
	 * Call falloc with a null vnode pointer, so the fd-entry is not
	 * prematurely activated.
	 */
	if (error = falloc((vnode_t *)NULL, FWRITE|FREAD, &fp1, &fd1)) {
		fifo_rmpipe(vp1, vp2, CRED());
		return (error);
	}
	fp1->f_vnode = vp1;


	if (error = falloc((vnode_t *)NULL, FWRITE|FREAD, &fp2, &fd2)) {
		unfalloc(fp1);
		setf(fd1, NULLFP);
		fifo_rmpipe(vp1, vp2, CRED());
		return (error);
	}
	fp2->f_vnode = vp2;

	/*
	 * install the fd-entries.
	 */
	setf(fd1, fp1);
	setf(fd2, fp2);

	/*
	 * Return the file descriptors to the user. They now
	 * point to two different vnodes which have different
	 * stream heads.
	 */
	rvp->r_val1 = fd1;
	rvp->r_val2 = fd2;
	ADT_GETF(vp1);
	return (0);
}
