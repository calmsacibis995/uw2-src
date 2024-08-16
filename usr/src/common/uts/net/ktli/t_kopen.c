/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/ktli/t_kopen.c	1.12"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *
 *	t_kopen.c, kernel TLI function to initialize a transport
 *	endpoint using the protocol specified.
 *
 */

#include <fs/file.h>
#include <fs/specfs/snode.h>
#include <fs/vnode.h>
#include <io/ioctl.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strsubr.h>
#include <mem/kmem.h>
#include <net/ktli/t_kuser.h>
#include <net/tihdr.h>
#include <net/timod.h>
#include <net/tiuser.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/types.h>

extern	int	strioctl(struct vnode *, int, int, int, int, cred_t *, int *);
extern	int	tlitosyserr(int);
extern	void	delay(long);

static		_t_setsize();

/*
 * t_kopen(fp, rdev, flags, tiptr, cr)
 *	Initialize a transport endpoint.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success or a positive error value.
 *	On success, tiptr is set to a valid transport
 *	pointer.
 *
 * Description:
 *	This routine initializes a transport endpoint
 *	for the given transport.
 *
 * Parameters:
 *
 *	fp			# file pointer for device
 *	rdev			# transport device
 *	flags			# flags to open device with
 *	tiptr			# to return TLI pointer in
 *	fields			# fields to allocate within struct
 *	
 */
int
t_kopen(struct file *fp, dev_t rdev, int flags, TIUSER **tiptr, struct cred *cr)
{
	int			madefp = 0;
	struct T_info_ack	inforeq;
	int			retval;
	struct vnode		*vp;
	struct strioctl		strioc;
	int			error;
	TIUSER			*ntiptr;
	int			str_len;
	int			T_info_ack_len;

	KTLILOG(0x80, "t_kopen: fp %x, ", fp);
	KTLILOG(0x80, "rdev %x, ", rdev);
	KTLILOG(0x80, "flags %x\n", flags);

	error = 0;
	retval = 0;

	if (fp == NULL) {
		int		fd;

		if (rdev == 0) {

			KTLILOG(0x80, "t_kopen: null device\n", 0);

			return EINVAL;
		}

		/*
		 * make a vnode.
		 */
		vp = makespecvp(rdev, VCHR);

		/*
		 * this will call the streams open for us.
		 */
		if ((error = VOP_OPEN(&vp, flags, cr)) != 0) {

			KTLILOG(0x80, "t_kopen: VOP_OPEN: %d\n", error);

			return error;
		}

		/*
		 * allocate a file pointer
		 */
		while ((error = falloc(vp, flags, &fp, &fd)) != 0) {

			KTLILOG(0x80, "t_kopen: falloc: %d\n", error);

			if (error == EMFILE) {
				VOP_CLOSE(vp, flags, 0, 0, cr);
				return error;
			}

			(void)delay(HZ);
		}

		setf(fd, NULLFP);

		madefp = 1;
	} else {
		vp = (struct vnode *)fp->f_vnode;
	}

	/*
	 * allocate a new transport structure
	 */
	ntiptr = (TIUSER *)kmem_alloc((u_int)TIUSERSZ, KM_SLEEP);
	ntiptr->tp_fp = fp;

	KTLILOG(0x80, "t_kopen: vp %x, ", vp);
	KTLILOG(0x80, "stp %x\n", vp->v_stream);

	/*
	 * see if TIMOD is already pushed
	 */
	error = strioctl(vp, I_FIND, (int)"timod", 0, K_TO_K, cr, &retval);
	if (error) {
		kmem_free((caddr_t)ntiptr, (u_int)TIUSERSZ);
		if (madefp)
			(void)closef(fp);

		KTLILOG(0x80, "t_kopen: strioctl(I_FIND, timod): %d\n", error);

		return error;
	}

	if (retval == 0) {
tryagain:
		error = strioctl(vp, I_PUSH, (int)"timod", 0, K_TO_K, cr,
							 &retval);
		if (error) {
			switch(error) {
			case ENOSPC:
			case EAGAIN:
			case ENOSR:
				/*
				 *+ Timod could not be pushed.
				 *+ Warn and try again.
				 */
				cmn_err(CE_WARN,
			"t_kopen: I_PUSH of timod failed, error %d\n", error);

				(void)delay(HZ);
				error = 0;
				goto tryagain;

			default:
				kmem_free((caddr_t)ntiptr, (u_int)TIUSERSZ);
				if (madefp)
					(void)closef(fp);

				KTLILOG(0x80,
					"t_kopen: I_PUSH (timod): %d", error);

				return error;
			}
		}
	}

	inforeq.PRIM_type = T_INFO_REQ;
	strioc.ic_cmd = TI_GETINFO;
	strioc.ic_timout = 0;
	strioc.ic_dp = (char *)&inforeq;
	strioc.ic_len = sizeof(struct T_info_req);

	/*
	 * get transport info
	 */
	error = strioctl(vp, I_STR, (int)&strioc, 0, K_TO_K, cr, &retval);
	if (error) {
		kmem_free((caddr_t)ntiptr, (u_int)TIUSERSZ);
		if (madefp)
			(void)closef(fp);

		KTLILOG(0x80, "t_kopen: strioctl(T_INFO_REQ): %d\n", error);

		return error;
	}

	if (retval) {
		if ((retval & 0xff) == TSYSERR)
			error = (retval >> 8) & 0xff;
		else
			error = tlitosyserr(retval & 0xff);
		kmem_free((caddr_t)ntiptr, (u_int)TIUSERSZ);
		if (madefp)
			(void)closef(fp);

		KTLILOG(0x80,
	"t_kopen: strioctl(T_INFO_REQ): retval: 0x%x\n", retval);

		return error;
	}

	/*
	 * size of info returned should be either the new T_info_ack
	 * struct size, or the old one (pre-XTI).
	 */
	str_len = strioc.ic_len;
	T_info_ack_len = sizeof(struct T_info_ack);
	if ((str_len != T_info_ack_len) &&
			(str_len != T_info_ack_len - sizeof(long))) {

		kmem_free((caddr_t)ntiptr, (u_int)TIUSERSZ);
		if (madefp)
			(void)closef(fp);

		KTLILOG(0x80,
	"t_kopen: strioc.ic_len != sizeof (struct T_info_ack): %d\n",
			strioc.ic_len);

		return EPROTO;
	}

	ntiptr->tp_info.addr = _t_setsize(inforeq.ADDR_size);
	ntiptr->tp_info.options = _t_setsize(inforeq.OPT_size);
	ntiptr->tp_info.tsdu = _t_setsize(inforeq.TSDU_size);
	ntiptr->tp_info.etsdu = _t_setsize(inforeq.ETSDU_size);
	ntiptr->tp_info.connect = _t_setsize(inforeq.CDATA_size);
	ntiptr->tp_info.discon = _t_setsize(inforeq.DDATA_size);
	ntiptr->tp_info.servtype = inforeq.SERV_type;

	*tiptr = ntiptr;

	return (0);
}

/*
 * default returned by _t_setsize()
 */
#define	DEFSIZE	128

/*
 * _t_setsize(infosize)
 *	Return appropriate size.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	Return appropriate size.
 *
 * Parameters:
 *
 *	infosize		# 
 */
static int
_t_setsize(long infosize)
{
	switch(infosize) {
		case -1L: return(DEFSIZE);

		case -2L: return(0);

		default: return(infosize);
	}
}
