/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/strcalls.c	1.6"

#include <util/types.h>
#include <util/param.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <proc/user.h>
#include <proc/cred.h>
#include <fs/vnode.h>
#include <util/debug.h>
#include <fs/file.h>
#include <io/stropts.h>
#include <io/stream.h>
#include <util/plocal.h>

/*
 * STREAMS system calls.
 */

struct msgnp {		/* non-priority version retained for compatibility */
	int fdes;
	struct strbuf *ctl;
	struct strbuf *data;
	int flags;
};

struct msgp {		/* priority version */
	int fdes;
	struct strbuf *ctl;
	struct strbuf *data;
	int pri;
	int flags;
};

STATIC int	msgio(struct msgp *, rval_t *, int, unsigned char *, int *);

/*
 * int
 * getmsg(struct msgnp *uap, rval_t *rvp)
 *	Get next msg off a stream.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A non-negative return value indicates successful completion.
 *	A return value of 0 indicates that a full message was read
 * 	successfully. A return value of:
 *		MORECTL indicates more control info is waiting 
 *		MOREDATA indicates more data are waiting for retreival
 *		MORECTL|MOREDATA indicates more of both types are waiting.
 */

int
getmsg(struct msgnp *uap, rval_t *rvp)
{
	struct msgp ua;
	int error;
	int localflags;
	int realflags = 0;
	unsigned char pri = 0;

	ASSERT(KS_HOLD0LOCKS());
	/*
	 * Convert between old flags (localflags) and new flags (realflags).
	 */
	if (copyin((caddr_t)uap->flags, (caddr_t)&localflags, sizeof(int)))
		return (EFAULT);
	switch (localflags) {
	case 0:
		realflags = MSG_ANY;
		break;

	case RS_HIPRI:
		realflags = MSG_HIPRI;
		break;

	default:
		return (EINVAL);
	}

	ua.fdes = uap->fdes;
	ua.ctl = uap->ctl;
	ua.data = uap->data;
	if ((error = msgio(&ua, rvp, FREAD, &pri, &realflags)) == 0) {
		/*
		 * massage realflags based on localflags.
		 */
		if (realflags == MSG_HIPRI)
			localflags = RS_HIPRI;
		else
			localflags = 0;
		if (copyout((caddr_t)&localflags, (caddr_t)uap->flags,
		  sizeof(int)))
			error = EFAULT;
	}
	return (error);
}

/*
 * int
 * putmsg(struct msgnp *uap, rval_t *rvp)
 *	Send a message on a stream.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A zero is returned on successful completion.
 */
 
int
putmsg(struct msgnp *uap, rval_t *rvp)
{
	unsigned char pri = 0;

	ASSERT(KS_HOLD0LOCKS());

	switch (uap->flags) {
	case RS_HIPRI:
		uap->flags = MSG_HIPRI;
		break;

	case 0:
		uap->flags = MSG_BAND;
		break;

	default:
		return (EINVAL);
	}
	return (msgio((struct msgp *)uap, rvp, FWRITE, &pri, &uap->flags));
}

/*
 * int
 * getpmsg(struct msgp *uap, rval_t *rvp)
 *	Get a priority message off a stream.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

int
getpmsg(struct msgp *uap, rval_t *rvp)
{
	int error;
	int flags;
	int intpri;
	unsigned char pri;

	ASSERT(KS_HOLD0LOCKS());
	if (copyin((caddr_t)uap->flags, (caddr_t)&flags, sizeof(int)))
		return (EFAULT);
	if (copyin((caddr_t)uap->pri, (caddr_t)&intpri, sizeof(int)))
		return (EFAULT);
	if ((intpri > 255) || (intpri < 0))
		return (EINVAL);
	pri = (unsigned char)intpri;
	if ((error = msgio(uap, rvp, FREAD, &pri, &flags)) == 0) {
		if (copyout((caddr_t)&flags, (caddr_t)uap->flags, sizeof(int)))
			return (EFAULT);
		intpri = (int)pri;
		if (copyout((caddr_t)&intpri, (caddr_t)uap->pri, sizeof(int)))
			error = EFAULT;
	}
	return (error);
}

/*
 * int
 * putpmsg(struct msgp *uap, rval_t *rvp)
 *	Put a priority message  on a stream.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

int
putpmsg(struct msgp *uap, rval_t *rvp)
{
	unsigned char pri;

	ASSERT(KS_HOLD0LOCKS());

	if ((uap->pri > 255) || (uap->pri < 0))
		return (EINVAL);
	pri = (unsigned char)uap->pri;
	return (msgio(uap, rvp, FWRITE, &pri, &uap->flags));
}

/*
 * int
 * msgio(register struct msgp *uap, rval_t *rvp, register int mode,
 *	 unsigned char *pri, int *flagsp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * 	Common code for getmsg and putmsg calls: check permissions,
 * 	copy in args, do preliminary setup, and call VOP_MSGIO()
 * 	and let it switch to the appropriate routine.
 */
STATIC int
msgio(register struct msgp *uap, rval_t *rvp, register int mode,
	unsigned char *prip, int *flagsp)
{
	file_t *fp;
	register vnode_t *vp;
	struct strbuf msgctl, msgdata;
	register int error;

	if (error = getf(uap->fdes, &fp))
		return (error);
	if ((fp->f_flag & mode) == 0) {
		error = EBADF;
		goto exit_msgio;
	}
	vp = fp->f_vnode;

	/* Let specfs make a decision for non-streams char devices */
	if (vp->v_type != VFIFO && vp->v_type != VCHR) {
		error = ENOSTR;
		goto exit_msgio;
	}
	if (uap->ctl && copyin((caddr_t)uap->ctl, (caddr_t)&msgctl,
	    sizeof(struct strbuf))) {
		error = EFAULT;
		goto exit_msgio;
	}
	if (uap->data && copyin((caddr_t)uap->data, (caddr_t)&msgdata,
	    sizeof(struct strbuf))) {
		error = EFAULT;
		goto exit_msgio;
	}

	if (mode == FREAD) {
		if (!uap->ctl)
			msgctl.maxlen = -1;
		if (!uap->data)
			msgdata.maxlen = -1;
	} else {  /* FWRITE */
		if (!uap->ctl)
			msgctl.len = -1;
		if (!uap->data)
			msgdata.len = -1;
	}
	
	error = VOP_MSGIO(vp, &msgctl, &msgdata, mode, prip, 
			(int) fp->f_flag, flagsp, rvp, fp->f_cred);
	if(error)
		goto exit_msgio;
	if (mode == FREAD) {
		if ((uap->ctl && copyout((caddr_t)&msgctl, (caddr_t)uap->ctl,
		    sizeof(struct strbuf))) || (uap->data &&
		    copyout((caddr_t)&msgdata, (caddr_t)uap->data,
		    sizeof(struct strbuf))))
			error = EFAULT;
	}

exit_msgio:
	FTE_RELE(fp);
	return(error);
}
