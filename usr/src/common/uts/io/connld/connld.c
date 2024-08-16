/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/connld/connld.c	1.7"
#ident	"$Header: $"

/*
 * This module establishes a unique connection on
 * a STREAMS-based pipe.
 */
#include <util/types.h>
#include <util/param.h>
#include <util/debug.h>
#include <proc/user.h>
#include <io/stropts.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <fs/fifofs/fifohier.h>
#include <fs/fifofs/fifonode.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <acc/priv/privilege.h>

/*
 * Define local and external routines.
 */
STATIC int connopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int connclose(queue_t *, int, cred_t *);
extern int fifo_close(vnode_t *, int, boolean_t, off_t, cred_t *);
extern int strioctl(vnode_t *, int, int, int, int, cred_t *, int *);

/*
 * Define STREAMS header information.
 */
STATIC struct module_info conn_info = {
	1003, 
	"conn", 
	0, 
	INFPSZ, 
	STRHIGH, 
	STRLOW 
};
STATIC struct qinit connrinit = { 
	putnext, 
	NULL, 
	connopen, 
	connclose, 
	NULL, 
	&conn_info, 
	NULL 
};
STATIC struct qinit connwinit = { 
	putnext, 
	NULL, 
	NULL, 
	NULL, 
	NULL, 
	&conn_info, 
	NULL
};
struct streamtab conninfo = { 
	&connrinit, 
	&connwinit 
};

int conndevflag = D_MP;

/*
 * STATIC  int 
 * connopen(queue_t *rqp, dev_t *dev, int flag, int sflag, cred_t *crp)
 *
 *	connld's open routine.
 *
 * Calling/Exit State:
 *
 * Description:
 *
 *	On the first invokation of connopen(), a flag is set and 
 *	the routine returns 0, since the first open corresponds to 
 *	the pushing of the module.
 *
 *	For each subsequent invokation of connopen(), create a new pipe. 
 *	One end of the pipe is sent to the process on the other end 
 *	of this STREAM. The vnode for the other end is returned to 
 *	the open() system call as the vnode for the opened object.
 *
 */
/*ARGSUSED*/
STATIC	int
connopen(queue_t *rqp, dev_t *dev, int flag, int sflag, cred_t *crp)
{
	int error = 0;
	struct vnode *vp1;
	struct vnode *vp2;
	struct vnode *streamvp; 
	struct file *filep;
	struct fifonode *streamfnp = NULL;
	struct fifonode *matefnp = NULL;
	pl_t pl;
	pl_t savepl;
	int fd, rvalp;

	savepl = spl0();
	streamvp = rqp->q_str->sd_vnode;
	/*
	 * CONNLD is only allowed to be pushed onto a "pipe" that has both 
	 * of its ends open.
	 */
	if (streamvp->v_type != VFIFO) {
		splx(savepl);
		return(EINVAL);
	}
	streamfnp = VTOF(streamvp);
	pl = PIPE_LOCK(streamfnp);
	if (!(streamfnp->fn_flag & ISPIPE) || 
		(streamfnp->fn_mate == NULL)) {
			PIPE_UNLOCK(streamfnp, pl);
			splx(savepl);
			return(EPIPE);
	}


	/*
	 * If this is the first time CONNLD was opened while on this stream,
	 * it is being pushed. Therefore, set a flag and return 0.
	 */
	if ((int)rqp->q_ptr == 0) {
		if (streamfnp->fn_flag & CONNLDPUSHED) {
			/*
			 * CONNLD can only be pushed on a stream once,
			 * because it sets fn_unique in fifonode.
			 */
			PIPE_UNLOCK(streamfnp, pl);
			splx(savepl);
			return(EEXIST);
		}
		rqp->q_ptr = (caddr_t)1;
		streamfnp->fn_flag |= CONNLDPUSHED;
		/*
		 * if process has MAC_WRITE priv then set FIFOMACPRIV,
		 * which will bypass the MAC check in fifo_open.
		 */
		if (pm_denied(crp, P_MACWRITE)) 
			streamfnp->fn_flag &= ~FIFOMACPRIV;
		else
			streamfnp->fn_flag |= FIFOMACPRIV;
		PIPE_UNLOCK(streamfnp, pl);
		qprocson(rqp);
		splx(savepl);
		return(0);
	}
	PIPE_UNLOCK(streamfnp, pl);

	/*
	 * Make pipe ends.
	 */
	if (error = fifo_mkpipe(&vp1, &vp2, crp)) {
		splx(savepl);
		return(error);
	}

	/*
	 * Allocate a file descriptor and file pointer for one of the pipe 
	 * ends. The file descriptor will be used to send that pipe end to 
	 * the process on the other end of this stream.
	 * Call falloc with a NULL vp to prevent premature activation 
	 * of the fd-entry.
	 */
	if (error = falloc((vnode_t *)NULL, FWRITE|FREAD, &filep, &fd)) {
		fifo_rmpipe(vp1, vp2, crp);
		splx(savepl);
		return(error);
	}
	filep->f_vnode = vp1;

	setf(fd, filep);

	/*
	 * Send one end of the new pipe to the process on the other 
	 * end of this pipe and block until the other process received it.
	 * If the other process exits without receiving it, fail this open
	 * request.
	 * Note that fifo_rmpipe() cannot be called on failure after
	 * setf(fd, filep) has been called.
	 */
	(void)PIPE_LOCK(streamfnp);
	if (streamfnp->fn_mate == NULL) {
		error = EPIPE;
		goto out;
	}
	matefnp = VTOF(streamfnp->fn_mate);
	matefnp->fn_flag |= FIFOSEND;
	PIPE_UNLOCK(streamfnp, pl);

	error = strioctl(streamvp, I_SENDFD, fd, flag, K_TO_K, filep->f_cred,
	    &rvalp);
	(void)PIPE_LOCK(streamfnp);
	if (error != 0)
		goto out;

	if (streamfnp->fn_mate == NULL) {
		error = EPIPE;
		goto out;
	}
	while (matefnp->fn_flag & FIFOSEND) {
		/* drop pipe lock here */
		if (FIFO_FDWAIT(matefnp) == B_FALSE) {
			(void)PIPE_LOCK(streamfnp);
			error = EINTR;
			goto out;
		}
		(void)PIPE_LOCK(streamfnp);
		if (streamfnp->fn_mate == NULL) {
			error = EPIPE;
			goto out;
		}
	}
	/*
	 * all is okay...return new pipe end to user
	 */
	streamfnp->fn_unique = vp2;
	streamfnp->fn_flag |= FIFOPASS;
	PIPE_UNLOCK(streamfnp, pl);
	closefd(fd);
	splx(savepl);
	return(0);
out:
	streamfnp->fn_unique = NULL;
	streamfnp->fn_flag &= ~FIFOPASS;
	if (streamfnp->fn_mate != NULL) {
		matefnp->fn_flag &= ~FIFOSEND;
	}
	PIPE_UNLOCK(streamfnp, pl);
	(void)fifo_close(vp2, 0, B_TRUE, 0, crp);
	VN_RELE(vp2);
	closefd(fd);
	splx(savepl);
	return(error);
}

/*
 * STATIC int
 * connclose(queue_t *qp, int cflag, cred_t *crp)
 * 	close procedure
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
/*ARGSUSED*/
STATIC	int
connclose(queue_t *qp, int cflag, cred_t *crp)
{
	qprocsoff(qp);
	return (0);
}
