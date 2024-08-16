/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/_utility.c	1.11.8.5"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <stropts.h>
#include <sys/stream.h>
#include <sys/tihdr.h>
#include "nsl_mt.h"
#include <sys/timod.h>
#include <sys/xti.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <ulimit.h>
#include <stdlib.h>
#include <string.h>
#include "_import.h"

#define DEFSIZE 2048

extern struct _ti_user **fdp;
extern long openfiles;

/*
 * Checkfd - checks validity of file descriptor
 * If successful, the function returns with a write lock on the
 * _ti_user structure for the file descriptor.
 */

struct _ti_user *
_t_checkfd(fd)
int fd;
{
	int _t_restore_state();

	 /*
         * is this a valid file descriptor "and" a transport provider?
         * One way to verify transport provider is to check to see if
         * it's a stream and timod is pushed.
         */
	if (!openfiles) {
		openfiles = OPENFILES;
	}
        if ((fd < 0)
	 || (fd > (openfiles - 1))
	 || (!isastream(fd))
	 || (ioctl(fd, I_FIND, "timod") != 1)) {
                set_t_errno(TBADF);
                return(NULL);
        }
	/* 
	 * Check with read locks for better concurrency, then lock for
	 * writing in t_restore_state() if necessary.
	 *
	 * IMPORTANT: always acquire _nsl_lock first, then fdp[fd]->lock.
	 * Otherwise, a deadlock could result.
	 */
	RW_RDLOCK(&_nsl_lock);
	if ((fdp == NULL) || (fdp[fd] == NULL)) {
		RW_UNLOCK(&_nsl_lock);
		/*
		 * try to get state back.
		 * _t_restore_state sets t_errno.
		 */
		if (_t_restore_state(fd) < 0) 
			return(NULL);
	} else {
		MUTEX_LOCK(&fdp[fd]->lock);
		RW_UNLOCK(&_nsl_lock);
		if (!(fdp[fd]->ti_flags & USED)) {
			MUTEX_UNLOCK(&fdp[fd]->lock);
			/*
		 	* try to get state back.
		 	* _t_restore_state sets t_errno.
		 	*/
			if (_t_restore_state(fd) < 0) 
				return(NULL);
		}
	}
	/*
	 * From now on, fdp, fdp[fd], and *fdp[fd] will not change.
	 * However, some other thread may not know this and may
	 * be acquiring locks to check them.  Such threads
	 * will determine that state has already been restored,
	 * so they will not try to update this data.  Therefore,
	 * we need not acquire _nsl_lock to read safely.
	 *
	 * NOTE: We keep the lock on the _ti_user structure *fdp[fd].
	 */
	return(fdp[fd]);
}

/* 
 * copy data to output buffer and align it as in input buffer
 * This is to ensure that if the user wants to align a network
 * addr on a non-word boundry then it will happen.
 */
void
_t_aligned_copy(buf, len, init_offset, datap, rtn_offset)
char *buf;
char *datap;
long *rtn_offset;
{
		*rtn_offset = ROUNDUP(init_offset) + ((unsigned int)datap&0x03);
		memcpy((char *)(buf + *rtn_offset), datap, (int)len);
}


/*
 * Max - return max between two ints
 */
_t_max(x, y)
int x;
int y;
{
	if (x > y)
		return(x);
	else 
		return(y);
}

/* 
 * put data and control info in look buffer
 * 
 * The only thing that can be in look buffer is a T_discon_ind,
 * T_ordrel_ind or a T_uderr_ind.
 * This routine must be called with tiptr->lock held for writing.
 */
void
_t_putback(tiptr, dptr, dsize, cptr, csize)
struct _ti_user *tiptr;
caddr_t dptr;
int dsize;
caddr_t cptr;
int csize;
{
	memcpy(tiptr->ti_lookdbuf, dptr, dsize);
	memcpy(tiptr->ti_lookcbuf, cptr, csize);
	tiptr->ti_lookdsize = dsize;
	tiptr->ti_lookcsize = csize;
	tiptr->ti_lookflg++;

}

/*
 * Is there something that needs attention?
 */

_t_is_event(fd, tiptr)
int fd;
struct _ti_user *tiptr;
 {
	int size, retval;

	if ((retval = ioctl(fd, I_NREAD, &size)) < 0) {
		set_t_errno(TSYSERR);
		return(1);
	}

	if (retval || tiptr->ti_lookflg) {
		set_t_errno(TLOOK);
		return(1);
	}

	return(0);
}

/* 
 * wait for T_OK_ACK, T_ERROR_ACK, or T_DISCON_IND
 */
_t_is_ok(fd, tiptr, type)
int fd;
register struct _ti_user *tiptr;
long type;
{

	struct strbuf ctlbuf;
	struct strbuf rcvbuf;
	register union T_primitives *pptr;
	int flags;
	int retval, cntlflag;
	int size;

	cntlflag = fcntl(fd,F_GETFL,0);
	fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0) & ~(O_NDELAY | O_NONBLOCK));

	ctlbuf.len = 0;
	ctlbuf.buf = tiptr->ti_ctlbuf;
	ctlbuf.maxlen = tiptr->ti_ctlsize;
	rcvbuf.maxlen = tiptr->ti_rcvsize;
	rcvbuf.len = 0;
	rcvbuf.buf = tiptr->ti_rcvbuf;
	/*
	 * For T_CONN_REQ/RES we'll take messages at any priority
	 * level.  Otherwise, only HIPRI.
	 */
	if (type == T_CONN_REQ || type == T_CONN_RES) {
		flags = 0;
	} else {
		flags = RS_HIPRI;
	}

	while ((retval = getmsg(fd, &ctlbuf, &rcvbuf, &flags)) < 0) {
		if (errno == EINTR)
			continue;
		set_t_errno(TSYSERR);
		return(0);
	}
	if (rcvbuf.len == -1) { 
		/* To prevent segmentation fault in memcpy() */
		rcvbuf.len = 0;
	}

	/* did I get entire message */
	if (retval) {
		set_t_errno(TSYSERR);
		errno = EIO;
		return(0);
	}

	/* 
	 * is ctl part large enough to determine type?
	 */
	if (ctlbuf.len < sizeof(long)) {
		set_t_errno(TSYSERR);
		errno = EPROTO;
		return(0);
	}

	fcntl(fd,F_SETFL,cntlflag);

	pptr = (union T_primitives *)ctlbuf.buf;

	switch(pptr->type) {
		case T_OK_ACK:
			if ((ctlbuf.len < sizeof(struct T_ok_ack)) ||
			    (pptr->ok_ack.CORRECT_prim != type)) {
				set_t_errno(TSYSERR);
				errno = EPROTO;
				return(0);
			}
			return(1);

		case T_ERROR_ACK:
			if ((ctlbuf.len < sizeof(struct T_error_ack)) ||
			    (pptr->error_ack.ERROR_prim != type)) {
				set_t_errno(TSYSERR);
				errno = EPROTO;
				return(0);
			}
			/*
			 * if error is out of state and there is something
			 * on read queue, then indicate to user that
			 * there is something that needs attention
			 */
			if (pptr->error_ack.TLI_error == TOUTSTATE) {
				if ((retval = ioctl(fd, I_NREAD, &size)) < 0) {
					set_t_errno(TSYSERR);
					return(0);
				}
				if (retval)
					set_t_errno(TLOOK);
				else
					set_t_errno(TOUTSTATE);
			} else {
				set_t_errno(pptr->error_ack.TLI_error);
				if (get_t_errno() == TSYSERR)
					errno = pptr->error_ack.UNIX_error;
			}
			return(0);

		case T_DISCON_IND: /* For call from _snd_conn_req() only */
			if ((ctlbuf.len < sizeof(struct T_discon_ind))
			 || ((type != T_CONN_REQ) 
			  && (type != T_CONN_RES))) {
				set_t_errno(TSYSERR);
				errno = EPROTO;
				return(0);
			}
			/*
			 * put disconnect indication into look buf
			 */
			_t_putback(tiptr, rcvbuf.buf, rcvbuf.len, ctlbuf.buf,
				   ctlbuf.len);
			set_t_errno(TLOOK);  /* For check in t_connect() */
			return(0);

		default:
			set_t_errno(TSYSERR);
			errno = EPROTO;
			return(0);
	}
}

/*
 * timod ioctl
 */
_t_do_ioctl(fd, buf, size, cmd, retlen)
char *buf;
int *retlen;
{
	int retval;
	struct strioctl strioc;

	strioc.ic_cmd = cmd;
	strioc.ic_timout = -1;
	strioc.ic_len = size;
	strioc.ic_dp = buf;

	if ((retval = ioctl(fd, I_STR, &strioc)) < 0) {
		set_t_errno(TSYSERR);
		return(0);
	}

	if (retval) {
		set_t_errno(retval&0xff);
		if (get_t_errno() == TSYSERR)
			errno = (retval >>  8)&0xff;
		return(0);
	}
	if (retlen)
		*retlen = strioc.ic_len;
	return(1);
}

/*
 * Allocate scratch buffers and look buffers.
 * Assume we have acquired tiptr->lock for writing.
 */

/* ARGSUSED */
_t_alloc_bufs(fd, tiptr, info)
int fd;
register struct _ti_user *tiptr;
struct T_info_ack info;
{
	unsigned size1, size2;
	unsigned csize, dsize, asize, osize;
	char *ctlbuf, *rcvbuf;
	char *lookdbuf, *lookcbuf;

	csize = _t_setsize(info.CDATA_size);
	dsize = _t_setsize(info.DDATA_size);

	size1 = _t_max(csize,dsize);

	if (size1 > 0) {
		if ((rcvbuf = calloc(1, size1)) == NULL)
			return(-1);
		if ((lookdbuf = calloc(1, size1)) == NULL) {
			(void)free(rcvbuf);
			return(-1);
		}
	} else {
		rcvbuf = NULL;
		lookdbuf = NULL;
	}

	asize = _t_setsize(info.ADDR_size);
	osize = _t_setsize(info.OPT_size);

	size2 = sizeof(union T_primitives) + asize + sizeof(long) + osize + sizeof(long);

	if ((ctlbuf = calloc(1, size2)) == NULL) {
		if (size1 > 0) {
			(void)free(rcvbuf);
			(void)free(lookdbuf);
		}
		return(-1);
	}

	if ((lookcbuf = calloc(1, size2)) == NULL) {
		if (size1 > 0) {
			(void)free(rcvbuf);
			(void)free(lookdbuf);
		}
		(void)free(ctlbuf);
		return(-1);
	}


	tiptr->ti_rcvsize = size1;
	tiptr->ti_rcvbuf = rcvbuf;
	tiptr->ti_ctlsize = size2;
	tiptr->ti_ctlbuf = ctlbuf;
	tiptr->ti_lookcbuf = lookcbuf;
	tiptr->ti_lookdbuf = lookdbuf;
	tiptr->ti_lookcsize = 0;
	tiptr->ti_lookdsize = 0;
	tiptr->ti_lookflg = 0;
	tiptr->ti_flags = USED;
	if (info.PROVIDER_flag & SENDZERO)
                tiptr->ti_provider_flgs |= T_SENDZERO;
        if (info.PROVIDER_flag & EXPINLINE)
                tiptr->ti_provider_flgs |= T_EXPINLINE;
	tiptr->ti_maxpsz = info.TIDU_size;
        tiptr->ti_tsdu = info.TSDU_size;
        tiptr->ti_servtype = info.SERV_type;
        tiptr->ti_state = T_UNINIT;
        tiptr->ti_ocnt = 0;
        tiptr->ti_qlen = 0;

	return(0);
}

/*
 * set sizes of buffers
 */

static int
_t_setsize(infosize)
long infosize;
{
	switch(infosize)
	{
		case -1: return(DEFSIZE);
		case -2: return(0);
		default: return(infosize);
	}
}

/*
 * Re-initialize the _ti_user structure members (except for lock).
 * Assume that tiptr->lock is held for writing.
 */

void
_null_tiptr(tiptr)
struct _ti_user *tiptr;
{
	tiptr->ti_flags = 0;
	tiptr->ti_rcvsize = 0;
	tiptr->ti_rcvbuf = NULL;
	tiptr->ti_ctlsize = 0;
	tiptr->ti_ctlbuf = NULL;
	tiptr->ti_lookdbuf = NULL;
	tiptr->ti_lookcbuf = NULL;
	tiptr->ti_lookdsize = 0;
	tiptr->ti_lookcsize = 0;
	tiptr->ti_maxpsz = 0;
        tiptr->ti_tsdu = 0;
        tiptr->ti_servtype = 0;
        tiptr->ti_lookflg = 0;
        tiptr->ti_state = 0;
        tiptr->ti_ocnt = 0;
        tiptr->ti_qlen = 0;
	tiptr->ti_provider_flgs = 0;
}

/*
 * _t_restore_state - try to restore the state of the library.
 * (After an exec, file descriptors remain but user data is gone.
 * This function prevents users from having to know to do a t_sync.)
 * The code was adapted from t_sync.c.
 * If successful, the function will return with the file descriptor's
 * _ti_user structure locked for writing.
 */
static int
_t_restore_state(fd)
int fd;
{
	struct T_info_ack info;
	register struct _ti_user *tiptr;
	int retlen;
	sigset_t		oldmask;
	sigset_t		newmask;
	int arg,rval;
	int didalloc_1 = 0;	/* if 1, array fdp was allocated here */
	int didalloc_2 = 0;	/* if 1, struct _ti_user was allocated here */
	int didalloc = 0;	/* if 1, fdp or _ti_user was allocated here */

	/*
	 * Acquire _nsl_lock for writing in order
	 *	(1) to protect fdp and fdp[fd] while updating and
	 *	(2) to make it safe to remove the _ti_user structure.
	 * Otherwise, a thread could be waiting for fdp[fd]->lock
	 * while we are freeing the whole structure. 
	 * We release _nsl_lock after we have locked the _ti_user structure.
	 */
	RW_WRLOCK(&_nsl_lock);
	if (!fdp) {
		openfiles = OPENFILES;
		if ((fdp = (struct _ti_user **)
	 	     calloc((unsigned)openfiles,
			    sizeof(struct _ti_user *))) == NULL) {
			RW_UNLOCK(&_nsl_lock);
			(void) set_t_errno(TSYSERR);
			return(-1);
		}
		didalloc_1 = 1;
	}
	/*
	 * If we have just allocated any storage, we should not let
	 * any other threads use it until we are sure that we will
	 * not need to free it.
	 * Even if we have not allocated storage, we must continue to hold 
	 * _nsl_lock for writing, since we must prevent other threads from
	 * trying to acquire (and we must not acquire) fdp[fd]->lock,
	 * which is in the _ti_user structure that we may have to free.
	 * 
	 */

	/*
	 * Assume that fd is within range, since a check is made
	 * by _t_checkfd(), its only caller.
	 */

	if (fdp[fd] == NULL) {
		if ((fdp[fd] = (struct _ti_user *)
				calloc(1, sizeof(struct _ti_user)))
		    == NULL) {
			if (didalloc_1) {
				(void)free(fdp);
				fdp = NULL;
			}
			RW_UNLOCK(&_nsl_lock);
			set_t_errno(TSYSERR);
			return(-1);
		}
		didalloc_2 = 1;
		MUTEX_INIT(&fdp[fd]->lock, USYNC_THREAD, NULL);
	}
	didalloc = didalloc_1 || didalloc_2;
	tiptr = fdp[fd];

	/*
	 * If no storage has been allocated during the current 
	 * invocation of _t_restore_state(), we can obtain the
	 * per-fd lock and release _nsl_lock, thus giving better
	 * better concurrency.
	 * If storage has just been allocated, then we wait until
	 * just before the switch statement to do exchange locks.
	 * If we are holding _nsl_lock for writing, no other thread
	 * will be able to acquire it for reading, so we do not have to
	 * acquire the per-fd lock and risk holding a lock in storage that
	 * we must free.
	 */
	if(!didalloc) {
		MUTEX_LOCK(&tiptr->lock);
		RW_UNLOCK(&_nsl_lock);
	}
	/*
	 *  (didalloc) implies (holding _nsl_lock)
	 * (!didalloc) implies (holding tiptr->lock)
	 */

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);

	if (ioctl(fd, I_FIND, "timod") <= 0) {
		if (didalloc_2) {
			MUTEX_DESTROY(&tiptr->lock);
			(void)free(tiptr);
			fdp[fd] = NULL;
		}
		if (didalloc_1) {
			(void)free(fdp);
			fdp = NULL;
		}
		if(didalloc) 
			RW_UNLOCK(&_nsl_lock);
		else
			MUTEX_UNLOCK(&tiptr->lock);
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		set_t_errno(TBADF);
		return(-1);
	}

	info.PRIM_type = T_INFO_REQ;
	if (!_t_do_ioctl(fd, (caddr_t)&info, sizeof(struct T_info_req),
			 TI_GETINFO, &retlen)) {
		if (didalloc_2) {
			MUTEX_DESTROY(&tiptr->lock);
			(void)free(tiptr);
			fdp[fd] = NULL;
		}
		if (didalloc_1) {
			(void)free(fdp);
			fdp = NULL;
		}
		if(didalloc) 
			RW_UNLOCK(&_nsl_lock);
		else
			MUTEX_UNLOCK(&tiptr->lock);
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		return(-1);
	}
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
	
	if (_t_alloc_bufs(fd, tiptr, info) < 0) {
		_null_tiptr(tiptr);
		if (didalloc_2) {
			MUTEX_DESTROY(&tiptr->lock);
			(void)free(tiptr);
			fdp[fd] = NULL;
		}
		if (didalloc_1) {
			(void)free(fdp);
			fdp = NULL;
		}
		if(didalloc) 
			RW_UNLOCK(&_nsl_lock);
		else
			MUTEX_UNLOCK(&tiptr->lock);
		set_t_errno(TSYSERR);
		return(-1);
	}
	/*
	 * Now the _ti_user structure exists for file descriptor fd.  It has
	 * an initialized lock, and its data buffers have been allocated.
	 */

	if (retlen != sizeof(struct T_info_ack)) {
		if (didalloc_2) {
			MUTEX_DESTROY(&tiptr->lock);
			(void)free(tiptr);
			fdp[fd] = NULL;
		}
		if (didalloc_1) {
			(void)free(fdp);
			fdp = NULL;
		}
		if(didalloc) 
			RW_UNLOCK(&_nsl_lock);
		else
			MUTEX_UNLOCK(&tiptr->lock);
		errno = EIO;
		set_t_errno(TSYSERR);
		return(-1);
	}

	/*
	 * The array of pointers fdp[fd] is well-established, so we can,
	 * if necessary, release _nsl_lock for writing, thus allowing
	 * other threads also to access the pointer array.
	 * We do not need to acquire _nsl_lock for reading,
	 * since no other thread will be changing fdp or fdp[fd].  
	 * Before releasing _nsl_lock, we acquire tiptr->lock, since we want
	 * to update the _ti_user structure and we must exclude other threads
	 * that want to read or write this structure.
	 */
	if(didalloc) {
		MUTEX_LOCK(&tiptr->lock);
		RW_UNLOCK(&_nsl_lock);
	}

	/*
	 * From now on, the fdp array must not be freed
	 * and fdp[fd]->lock must not be destroyed.
	 */

	switch (info.CURRENT_state) {

	case TS_UNBND:
		tiptr->ti_state = T_UNBND;
		break;
	case TS_IDLE:
		if((rval = ioctl(fd,I_NREAD,&arg)) < 0) {
			/*
			 * We must give some state and T_FAKE
			 * seems appropriate. Don't bother to free
			 * _ti_user, since it may come in handy for
			 * subsequent operations.
			 */
			tiptr->ti_state = T_FAKE;
			MUTEX_UNLOCK(&tiptr->lock);
			set_t_errno(TSYSERR);
			return(-1);
		}
		if(rval == 0 )
			tiptr->ti_state = T_IDLE;
		else if (tiptr->ti_servtype == T_COTS
		      || tiptr->ti_servtype == T_COTS_ORD) {
			/*
			 * To handle data or DISCONNECT indications that
			 * might be at the stream head waiting to be read
			 * if the transport provider is connection-oriented .
			 */
			tiptr->ti_state = T_DATAXFER;
		}
		else
			tiptr->ti_state = T_IDLE;
		break;
	case TS_WRES_CIND:
		tiptr->ti_state = T_INCON;
		break;
	case TS_WCON_CREQ:
		tiptr->ti_state = T_OUTCON;
		break;
	case TS_DATA_XFER:
		tiptr->ti_state = T_DATAXFER;
		break;
	case TS_WIND_ORDREL:
		tiptr->ti_state = T_OUTREL;
		break;
	case TS_WREQ_ORDREL:
		if((rval = ioctl(fd,I_NREAD,&arg)) < 0) {
			/*
			 * See comments in TS_IDLE case.
			 */
			tiptr->ti_state = T_FAKE;
			MUTEX_UNLOCK(&tiptr->lock);
			set_t_errno(TSYSERR);
			return(-1);
		}
		if(rval == 0 )
			tiptr->ti_state = T_INREL;
		else {
			/*
			 * To handle T_ORDREL_IND indication that
			 * might be at the stream head waiting to be read.
			 */
			tiptr->ti_state = T_DATAXFER;
		}
		break;
	default:
		tiptr->ti_state = T_FAKE;
		MUTEX_UNLOCK(&tiptr->lock);
		set_t_errno(TSTATECHNG);
		return(-1);
	}
	return(0);
}



