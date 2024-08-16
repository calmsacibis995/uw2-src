/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/asyncio/asyncio.c	1.2.1.21"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <siginfo.h>
#define _KERNEL
#include <sys/list.h>
#undef _KERNEL
#include <libthread.h>
#include <synch.h>
#include <aio.h>
#include "aiosys.h"

#if !defined(ABI) && defined(__STDC__)
#pragma weak close = _close
#endif /*  !defined(ABI) && defined(__STDC__) */

/*
 * Asynchronous I/O functions:
 *	new: aio_read(), aio_write(), lio_listio(), aio_error(), aio_return(),
 *	aio_cancel(), aio_suspend(), aio_fsync()
 *	existing: close()
 */

#ifndef STATIC
#ifdef DEBUG
#define STATIC
#else
#define STATIC	static
#endif
#endif
#if defined(AIODEBUG)

#define MUTEX_INIT(x,y,z)	(_THR_MUTEX_INIT(x,y,z) && ASSERT(0))
#define MUTEX_LOCK(x)		(_THR_MUTEX_LOCK(x) && ASSERT(0))
#define MUTEX_UNLOCK(x)		(_THR_MUTEX_UNLOCK(x) && ASSERT(0))
#define COND_INIT(x,y,z)	(_THR_COND_INIT(x,y,z) && ASSERT(0))
#define COND_WAIT(x,y)		(_THR_COND_WAIT(x,y) && ASSERT(0))
#define COND_SIGNAL(x)		(_THR_COND_SIGNAL(x) && ASSERT(0))
#define COND_TIMEDWAIT(x,y,z)	(_THR_COND_TIMEDWAIT(x,y,z) && ASSERT(0))

#else /* AIODEBUG */

#define MUTEX_INIT(x,y,z)	(_THR_MUTEX_INIT(x,y,z))
#define MUTEX_LOCK(x)		(_THR_MUTEX_LOCK(x))
#define MUTEX_UNLOCK(x)		(_THR_MUTEX_UNLOCK(x))
#define COND_INIT(x,y,z)	(_THR_COND_INIT(x,y,z))
#define COND_WAIT(x,y)		(_THR_COND_WAIT(x,y))
#define COND_SIGNAL(x)		(_THR_COND_SIGNAL(x))
#define COND_TIMEDWAIT(x,y,z)	(_THR_COND_TIMEDWAIT(x,y,z))

#endif /* AIODEBUG */
STATIC int	aioinitialized = 0;
STATIC mutex_t	aioinit_lock = DEFAULTMUTEX;
STATIC mutex_t	aiorequestq_lock = DEFAULTMUTEX;/* for per-fd and ready lists */

STATIC fd_table_t	fdtable;		/* one table per application */
STATIC aioop_t		aioreadyq;		/* head of ready-job list */

STATIC aioop_t		aioactiveq;		/* head of active-job list */
STATIC mutex_t		workjobq_lock;		/* for active-job list */

STATIC list_t		*aioop_freeq;		/* free list of aioop_t */
STATIC svrtcb_t		*svrtcb_freeq;		/* free list of svrtcb_t */
STATIC mutex_t		freelist_lock;		/* for the two free lists */

STATIC svrtcb_t		*svrtcb_tbl;		/* start of malloc'ed svrtcb_t array */

STATIC pollreq_t	*pollslpq;		/* aio_suspend() lio_listio() sleep queue */

/*
 * there is a server creator thread responsible for thr_create()'ing new
 * asynchronous I/O (AIO) server threads upon request.
 * it executes createaioserver().
 */
STATIC thread_t		creatortid;		/* its thread id */
STATIC cond_t		createsvr_cond;		/* for cond_wait/cond_signal */

STATIC int		createsvrcnt;		/* how many needed */

STATIC int		totalsvrcnt;		/* total number of AIO servers */
STATIC int		totalidlesvrcnt;	/* number of idle AIO servers */
STATIC cond_t		svr_cond;		/* for waking up idle servers */

STATIC sigset_t		ioserver_sigmask;	/* aio server signal mask */
STATIC boolean_t 	need_hndlr = B_TRUE;	/* need to establish SIGIO handler */

/*
 * the min and max of various structures
 */
STATIC int		maxaioop = MAXAIOOP;
STATIC int		maxsvrtcb = MAXSVRTCB;
STATIC int		maxsvrcnt = MAXSVRCNT;
STATIC int		minsvrcnt = MINSVRCNT;
STATIC int		maxfdq = MAXFDQ;
STATIC int		maxfdtincr = FDT_INCREMENT;

extern int	pread(int, char *, int, off_t);
extern int	pwrite(int, char *, int, off_t);
STATIC int	aiostart(void);
STATIC int	aioinit(void);
STATIC int	aiorw(uchar_t, aiocb_t *);
STATIC int	aiocontrol(aioop_t *);
STATIC int	apoll(pollreq_t *);
STATIC void	*createaioserver(void);
STATIC aioop_t	*aioop_alloc(void);
STATIC void	aioop_free(aioop_t *);
STATIC svrtcb_t	*svrtcb_alloc(void);
STATIC void	svrtcb_free(svrtcb_t *);
STATIC void	*aioserver(svrtcb_t *);
STATIC aioop_t	*get_aioop(svrtcb_t *, thread_t);
STATIC int	cancel(int, struct aiocb *, int);
STATIC int	cancelfd(fd_entry_t *, struct aiocb *);
STATIC void	canceljob(aioop_t *, int);
STATIC void	poll_wakeup(aiocb_t *);
STATIC void	aio_callback(aioop_t *);

STATIC void	sigaiohand(int);
STATIC void	setconfig();
#ifdef AIODEBUG
STATIC int	aio_assfail(char *, char *, int);
#endif /* AIODEBUG */

/*
 * int
 * aio_read(struct aiocb *aiocbp)
 *	asynchronous I/O read.
 *
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 */

int
aio_read(struct aiocb *aiocbp)
{
	if (!aiocbp) {
		errno = EFAULT;
		return(-1);
	}
	if (aiocbp->aio_flags & AIO_RAW) {
		/* 4.2 request */
		return(raio_read(aiocbp));
	}
	/*
	 * else it's esmp request.
	 */
	if (aiocbp->aio_reqprio != 0 ||
		(aiocbp->aio_sigevent.sigev_notify != SIGEV_NONE && 
		aiocbp->aio_sigevent.sigev_notify != SIGEV_CALLBACK)) {
		/*
		 * we don't check for SIGEV_SIGNAL as the signal notification 
		 * is not supported by this implementation.
		 */

		errno = EINVAL;
		return(-1);
	}
	/* esmp request */
	return(aiorw(AIO_READ, aiocbp));
}

/*
 * int
 * aio_write(struct aiocb *aiocbp)
 *	asynchronous I/O write.
 *
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 */

int
aio_write(struct aiocb *aiocbp)
{
        if (!aiocbp) {
                errno = EFAULT;
                return(-1);
        }
	if (aiocbp->aio_flags & AIO_RAW) {
		/* 4.2 request */
		return(raio_write(aiocbp));
	}
        /*
         * else it's esmp request.
         */
	if (aiocbp->aio_reqprio != 0 || 
		(aiocbp->aio_sigevent.sigev_notify != SIGEV_NONE &&
		aiocbp->aio_sigevent.sigev_notify != SIGEV_CALLBACK)) {
		/*
		 * we don't check for SIGEV_SIGNAL as the signal notification
		 * is not supported by this implementation.
		 */

		errno = EINVAL;
		return(-1);
	}
	/* esmp request */
	return(aiorw(AIO_WRITE, aiocbp));
}


/*
 * STATIC int
 * aiorw(uchar_t rwflag, aiocb_t *ap)
 *	Common routine for aioread/aiowrite calls. It performs initialization
 *	on very first call of aioread/aiowrite. It then validates the input
 *	argument, ap, and allocates an aioop_t and links it with ap, and
 *	then calls aiocontrol() to queue the request.
 *
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 *    Return value:
 *	0 on success,
 *	-1 and errno = EINVAL for invalid input values
 *	       errno = EAGAIN for insufficient resources
 *	   or when aioinit() or aiocontrol() returns failure.
 *		(these functions also set errno)
 * Remarks:
 *    aioinit() creates a server creator thread and allocates a table of
 *    fd_entry_t's. if it fails the allocation, the creator thread is killed here.
 */

STATIC int
aiorw(uchar_t rwflag, aiocb_t *ap)
{
	aioop_t	*aioop;
	thread_desc_t *curtp = curthread;

	/*
	 * get an aioop_t structure, fill in cmd and aiocbp
	 * call aiocontrol() to further process it,
	 */
	 _thr_sigoff(curtp);
	if ((aioop = aioop_alloc()) == NULLAIOOP) {
		errno = EAGAIN;
		_thr_sigon(curtp);
		return (-1);
	}
	if (!aioinitialized) {
		if (aiostart() < 0) {
			/* aiostart() calls aioinit(), which sets errno */
			aioop_free(aioop);
			_thr_sigon(curtp);
			return (-1);
		}
	}
	aioop->a_aiocbp = ap;
	aioop->a_cmd = rwflag;
	aioop->a_flag = 0;

	/* aiocontrol() sets errno to either error code or EINPROGRESS */
	if (aiocontrol(aioop) < 0) {
		aioop_free(aioop);
		_thr_sigon(curtp);
		return (-1);
	}
	_thr_sigon(curtp);	
	return (0);
}

/*
 * int
 * aio_suspend(const struct aiocb *list[] int nent, const struct timespec *timeout)
 *	suspend the calling thread until at least one of the selected I/O requests
 *	completes or a timeout occurs.
 *
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 *    Return value:
 *	function returns 0 after one or more asynchronous I/O operations have
 *	completed. Otherwise, the function returns -1 and set errno to indicate
 *	the error. The function sets errno to:
 *	       EAGAIN if timedout expired,
 *	       EINTR if signal interrupted aio_suspend(),
 *	       ENOSYS if aio_suspend is called for 4.2 application.
 *
 */

int
aio_suspend(const struct aiocb *list[], int nent, const struct timespec *timeout)
{
	aioop_t		*ap;
	struct aiocb	**iopp; 
	int		nreqs, elcount;
	/*
	 * allocate a structure for this request since
	 * it might block waiting for 1+ completions
	 */
	pollreq_t	preq;

	if (!aioinitialized) {
		if (aiostart() < 0)
			return (-1);
	}

	/*
	 * validate data.
	 */
	nreqs = 0;

	/* 
	 * since list is "const" cast (struct aiocb **) below is needed 
	 * to satisfy compiler complains .
	 */

	for (iopp = (struct aiocb **)list, elcount = 0; elcount < nent; elcount++, iopp++) {
		if (*iopp) {
			if ((*iopp)->aio_flags & AIO_RAW) {
				/* 4.2 request */
				errno = ENOSYS;
				return (-1);
			}
			nreqs++;
		}
	}
	preq.p_list = (aiocb_t **)list;
	preq.p_size = nent;
	preq.p_nent = nreqs;
	preq.p_flag = AIO_SUSPEND;
	if (timeout) {
		preq.p_timeout = *timeout;
	} else {
		preq.p_timeout.tv_sec = -1; /* wait forever */
	}
	COND_INIT(&preq.p_cond, USYNC_THREAD, NULLV);
	return(apoll(&preq));
}

/*
 * int
 * aio_fsync(int op, struct aiocb *aiocbp)
 *	asynchronously force all outstanding I/O operations to the synchronized
 *	I/O completion state. Not supported in 4.2MP except for POSIX compliance.
 *	Function returns -1 and sets errno to ENOSYS.
 */

int
aio_fsync(int op, struct aiocb *aiocbp)
{
	errno = ENOSYS;
	return(-1);
}

/*
 * int
 * aio_error(const struct aiocb *aiocbp)
 *	retrieves the error status associated with asynchronous I/O control
 *	block.
 * Calling/Exit state:
 *      no locks are held upon entry or on exit.
 *    Return value:
 *	If the asynchronous I/O operation has completed successfully, the 
 *	function returns 0. If it has failed, then the corresponding read(),
 *	write(), or fsync() error status is returned.
 *	EINPROGRESS is returned if the operation is still in progress.
 *	
 */

int
aio_error(const struct aiocb *aiocbp)
{
	if (aiocbp->aio_flags & AIO_RAW) {
		/* 4.2 request */
		return(raio_error(aiocbp));
	} else {
		/* esmp request */
		return(aiocbp->aio__error);
	}
}

/*
 * int
 * aio_return(struct aiocb *aiocbp)
 *	retrieves the status associated with asynchronous I/O control
 *	block.
 * Calling/Exit state:
 *      no locks are held upon entry or on exit.
 *    Return value:
 *	If the asynchronous I/O operation has completed the corresponding
 *	read(),	write(), or fsync() status is returned.
 *	
 */

int
aio_return(struct aiocb *aiocbp)
{
	if (aiocbp->aio_flags & AIO_RAW) {
		/* 4.2 request */
		return(raio_return(aiocbp));
	} else {
		/* esmp request */
		return(aiocbp->aio__return);
	}
}

/*
 * int
 * aio_cancel(int fd, struct aiocb *aiocbp)
 *	cancel asynchronous I/O operations
 *
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 *    Return value:
 *	AIO_CANCELED if the requested operation(s) were canceled.
 *	AIO_NOTCANCELED if at least one of the requested operation(s) cannot
 *	be canceled because it is in progress.
 *	AIO_ALLDONE is returned if all of the operations have already completed.
 *	
 *	Otherwise, the function returns -1 and sets errno to:
 *		EBADF if fd is invalid
 *		ENOSYS aio_cancel was requested for raw slice.
 *		EINVAL both fd and aiocbp are passed and they do not refer to 
 *		the same fd.
 *
 */

int
aio_cancel(int fd, struct aiocb *aiocbp)
{
	if (!aioinitialized) { 
		errno = EINVAL;
		return (-1);
	}
	if (aiocbp && (aiocbp->aio_flags & AIO_RAW)) {
		/* requested 4.2 aio_cancel */
		errno = ENOSYS;
		return (-1);
	}
	if (fd < 0 || (aiocbp && (aiocbp->aio_fildes != fd))) {
		errno = EINVAL;
		return (-1);
	}
	return(cancel(fd, aiocbp, AIO_CANCEL));
}
/*
 * int
 * close(int fd)
 *	close a file descriptor
 *
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 *
 * Remarks:
 *	we intercept all close() calls, so those close calls that are not
 *	associated with any AIO will just return (_close(fd))
 */
int
_close(int fd)
{
	fd_entry_t	*fdqp;
	int		rtn;
	thread_desc_t *tp = curthread;

	_thr_sigoff(tp);
	MUTEX_LOCK(&aiorequestq_lock);
	if ((fd >= 0 && fd < fdtable.fdt_size) &&
		(fdtable.fdt_array[fd].fd_flag & F_INUSE)) {
		fdqp = &fdtable.fdt_array[fd];
		/*
		 * if it is being closed, we simply return EBADF
		 */
		if (fdqp->fd_flag & F_CLOSING) {
			errno = EBADF;	
			MUTEX_UNLOCK(&aiorequestq_lock);
			_thr_sigon(tp);
			return (-1);
		}
	} else {
		MUTEX_UNLOCK(&aiorequestq_lock);
		_thr_sigon(tp);
		return ((*_sys_close)(fd));
	}
	MUTEX_UNLOCK(&aiorequestq_lock);
	/*
	 * cancel request
	 */
	rtn =  cancel(fd, 0, AIO_CLOSE);

	if (errno == EINTR) {
		MUTEX_LOCK(&aiorequestq_lock);
		fdqp = &fdtable.fdt_array[fd];
		fdqp->fd_flag &= ~F_CLOSING;
		MUTEX_UNLOCK(&aiorequestq_lock);
		_thr_sigon(tp);
		return (rtn);
	}
	/*
	 * now all outstanding AIOs have been cancelled, _close() it
	 */

	if ((rtn = (*_sys_close)(fd)) < 0) {
		MUTEX_LOCK(&aiorequestq_lock);
		fdqp = &fdtable.fdt_array[fd];
		fdqp->fd_flag &= ~F_CLOSING;
		MUTEX_UNLOCK(&aiorequestq_lock);
		_thr_sigon(tp);
		return (rtn);
	}
	MUTEX_LOCK(&aiorequestq_lock);
	fdqp = &fdtable.fdt_array[fd];
	ASSERT(EMPTYQUE(&fdqp->fd_aiolist));
	fdqp->fd_flag = 0;
	MUTEX_UNLOCK(&aiorequestq_lock);
	ASSERT(rtn == 0);
	_thr_sigon(tp);
	return (rtn);
}

/*
 * int
 * aiostart()
 *	start the asynchonous I/O subsystem
 *	if not initialized, call aioinit() to initialize the subsystem.
 *	since aioinitialized is set to 1 only once and never changed,
 *	we use test, lock, test such that once initialized, it does not
 *	have to lock on every aio call.
 *
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 *    Return value:
 *	0 - initialization completed successfully and server-creator thread is
 *	    up and running
 *	-1 - if initialization failed 
 */
STATIC int
aiostart()
{
	char	*ep;

        /*
         * Note that a snapshot of aioinitialized is taken
         * before this function is called.
         */

	MUTEX_LOCK(&aioinit_lock);
	if (!aioinitialized) {
		if (aioinit()) {	/* failed */
			MUTEX_UNLOCK(&aioinit_lock);
			if (creatortid != NULLTID) {
				thr_kill(creatortid, SIGTERM);
			}
			return (-1);
		}
		aioinitialized = 1; 
	}
	MUTEX_UNLOCK(&aioinit_lock);
	return (0);
}

/*
 * STATIC int
 * aioinit(void)
 *	asynchronous I/O initialization: server-creator thread creation,
 *	free list allocation, lists initialization, lock initialization, 
 *	called exactly once.
 *
 * Calling/Exit state:
 *	aioinit_lock is held upon entry and on exit.
 *    Return value:
 *	0 - resource allocation completed successfully
 *	-1 - insufficient resource, sets errno to EAGAIN
 */

STATIC int
aioinit()
{
	aioop_t		*aioop, *tmp;
	svrtcb_t	*svrtcbp;
	sigset_t	oset;
	/*
	 * thr_create() a server-creator thread first since it's easy
	 * to clean up if it fails,
	 * initialize the cond var, the ready-job list and its lock needed
	 * by this thread.
	 */
	INITQUE(&aioreadyq.a_flist);
	COND_INIT(&createsvr_cond, USYNC_THREAD, NULLV);
	creatortid = NULLTID;
	/*
	 * set server-creator signal mask that will be inherited by each
	 * async I/O server. _thr_sig_allmask is initialized in libthread.
	 */
	ioserver_sigmask = _thr_sig_allmask;
	sigdelset(&ioserver_sigmask, SIGIO);

	/*
	 * we have to use actual sys call rather than thread library wrapper
	 * to avoid unmasking SIGWAITING and SIGLWP (except for SIGIO, async I/O
	 * servers should not process any signals).
	 */

	(void ) (*_sys_sigprocmask)(SIG_SETMASK, &ioserver_sigmask, &oset);
	if (thr_create(NULLV, 0, (void *(*) (void *))createaioserver, NULL, 
		THR_BOUND|THR_DETACHED|THR_DAEMON, &creatortid)) {
		(void ) (*_sys_sigprocmask)(SIG_SETMASK, &oset, NULL);
		errno = EAGAIN;
		return (-1);
	}
	/*
	 * restore caller's signal mask
	 */
	(void ) (*_sys_sigprocmask)(SIG_SETMASK, &oset, NULL);
	/*
	 * allocate storage for free lists: aioop_t and svrtcb_t.
	 * one malloc for total and divide into two pools,
	 * first determine the max sizes if __AIO_TRACE is on
	 */
	setconfig();
	MUTEX_INIT(&freelist_lock, USYNC_THREAD, NULLV);
	if ((tmp = (aioop_t *)malloc(
			  (maxaioop * sizeof(aioop_t))
			+ (maxsvrtcb * sizeof(svrtcb_t)))) == NULLAIOOP) {
		errno = EAGAIN;
		return (-1);
	}
	/* build a pool of aioop_t's */
	for (aioop = tmp; (aioop-tmp) < maxaioop; aioop++) {
		aioop_t *ap = aioop + 1;
		aioop->a_flink = &ap->a_flist;
	}
	(aioop-1)->a_flink = NULLLIST;
	aioop_freeq = (list_t *)tmp;
	/* 
	 * next build a pool of svrtcb_t's, one per aio server thread,
	 * remember start of table for signal handling
	 */
	svrtcb_tbl = svrtcb_freeq = (svrtcb_t *) aioop;
	for (svrtcbp = svrtcb_freeq; (svrtcbp-svrtcb_freeq) < maxsvrtcb; svrtcbp++) {
		svrtcbp->st_svrtcbp = svrtcbp + 1;
		svrtcbp->st_fd = INVALIDFD;
	}
	(svrtcbp-1)->st_svrtcbp = NULLSVRTCB;

	/*
	 * intialize file descriptor table, must do separate malloc since on
	 * growth, must free the old one.
	 */
	fdtable.fdt_size = maxfdq;
	fdtable.fdt_array = (fd_entry_t *)malloc(maxfdq * sizeof(fd_entry_t));
	if (fdtable.fdt_array == NULLFDQ) {
		errno = EAGAIN;
		return (-1);
	}
	(void) memset((void *)fdtable.fdt_array, 0, maxfdq * sizeof(fd_entry_t));

	/* initialize active-job list, and its lock */
	INITQUE(&aioactiveq.a_flist);
	MUTEX_INIT(&workjobq_lock, USYNC_THREAD, NULLV);

	/* initialize the common cond variable for waking up the servers */
	COND_INIT(&svr_cond, USYNC_THREAD, NULLV);

	/* initialize list for blocked aio_suspend() calls */
	pollslpq = NULLPOLLREQ;

	return (0);
}

/*
 * STATIC void
 * setconfig(void)
 *	determines the max number of various control structures,
 *	may be set with private environment variables,
 *	use value from environment variable if set, or else use default #define,
 *	for testing and measurement purpose,
 */
STATIC void
setconfig(void)
{
	char	*ep;

	ep = getenv("AIO_MAX");
	if (ep == NULL)
		maxaioop = MAXAIOOP;
	else {
		maxaioop = atoi(ep);
		if (maxaioop == 0)
			maxaioop = MAXAIOOP;
	}
	ep = getenv("__AIO_MAXSVRTCB");
	if (ep == NULL)
		maxsvrtcb = MAXSVRTCB;
	else {
		maxsvrtcb = atoi(ep);
		if (maxsvrtcb == 0)
			maxsvrtcb = MAXSVRTCB;
	}
	ep = getenv("__AIO_MAXSVRCNT");
	if (ep == NULL)
		maxsvrcnt = MAXSVRCNT;
	else {
		maxsvrcnt = atoi(ep);
		if (maxsvrcnt == 0)
			maxsvrcnt = MAXSVRCNT;
	}
	ep = getenv("__AIO_MINSVRCNT");
	if (ep == NULL)
		minsvrcnt = MINSVRCNT;
	else {
		minsvrcnt = atoi(ep);
		if (minsvrcnt == 0)
			minsvrcnt = MINSVRCNT;
	}
	if (minsvrcnt > maxsvrcnt)
		minsvrcnt = maxsvrcnt;
	ep = getenv("__AIO_MAXFDQ");
	if (ep == NULL)
		maxfdq = MAXFDQ;
	else {
		maxfdq = atoi(ep);
		if (maxfdq == 0)
			maxfdq = MAXFDQ;
	}
	ep = getenv("__AIO_FDT_INCREMENT");
	if (ep == NULL)
		maxfdtincr = FDT_INCREMENT;
	else {
		maxfdtincr = atoi(ep);
		if (maxfdtincr == 0)
			maxfdtincr = FDT_INCREMENT;
	}
#ifdef AIODEBUG
	(void) printf("AIO configuration:\n\tmaxaioop: %d\tmaxsvrtcb: %d\n\tmaxsvrcnt: %d\tminsvrcnt: %d\n\tmaxfdq: %d\tfdt_increment: %d\n", 
		maxaioop, maxsvrtcb, maxsvrcnt, minsvrcnt, maxfdq, maxfdtincr);
#endif
}

/*
 * STATIC int
 * aiocontrol(aioop_t *aioop)
 *	put an asynchronous I/O request on either the per-fd request list
 *	or the ready-job list
 *
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 *	locks/unlocks aiorequestq_lock.
 *    Return value:
 *	0 - request successfully queued up
 *	-1 - insufficient resource, sets errno to EAGAIN
 *	     aioop->a_aiocbp->aio_fildes invalid, sets errno to EBADF
 */

STATIC int
aiocontrol(aioop_t *aioop)
{
	int		fd, fx;
	char		qempty;
	fd_entry_t	*fdqp;
	boolean_t	fd_notvalid = B_TRUE;
	int		mode;

	fd = aioop->a_aiocbp->aio_fildes;
	/*
	 * grow fdtable if fd higher than size of fdtable.fdt_array
	 */
	MUTEX_LOCK(&aiorequestq_lock);
	if (fd >= fdtable.fdt_size) {
		fd_entry_t	*newfdqp;
		int		newsize;

		if ((mode = fcntl(fd, F_GETFL)) < 0) {
			MUTEX_UNLOCK(&aiorequestq_lock);
			errno = EBADF;
			return (-1);
		} else {
			fd_notvalid = B_FALSE;
		}
		/* allocate a new fdtable and copy the old over */
		newsize = fdtable.fdt_size + FDT_INCREMENT;
		newfdqp = (fd_entry_t *)malloc(newsize * sizeof(fd_entry_t));
		if (newfdqp == NULLFDQ) {
			MUTEX_UNLOCK(&aiorequestq_lock);
			errno = EAGAIN;
			return (-1);
		}
		fdqp = fdtable.fdt_array;
		/*
		 * first copy the old into the corresponding element in the new:
		 * 	- not in use: clear fd_flag,
		 * 	- in use but empty: init to empty,
		 * 	- in use but not empty: copy all pointers
		 */
		for (fx = 0; fx < fdtable.fdt_size; fx++) {
			if ((fdqp[fx].fd_flag & F_INUSE) == 0) {
				newfdqp[fx].fd_flag = 0;
				continue;
			}
			newfdqp[fx].fd_flag = fdqp[fx].fd_flag;
			if (EMPTYQUE(&fdqp[fx].fd_aiolist)) {
				INITQUE(&newfdqp[fx].fd_aiolist);
			} else {
				newfdqp[fx].fd_aioflink = fdqp[fx].fd_aioflink;
				newfdqp[fx].fd_aioblink = fdqp[fx].fd_aioblink;
				newfdqp[fx].fd_aioflink->rlink = &newfdqp[fx].fd_aiolist;
				newfdqp[fx].fd_aioblink->flink = &newfdqp[fx].fd_aiolist;
			}
		}
		/*
		 * now continue and initialize the enlarged portion
		 */
		fdtable.fdt_array = newfdqp;
		(void) memset((void *)&fdtable.fdt_array[fx], 0, 
			FDT_INCREMENT * sizeof(fd_entry_t));
		fdtable.fdt_size = newsize;
	}
	fdqp = &fdtable.fdt_array[fd];
	if ((fdqp->fd_flag & F_INUSE) == 0) {
		/*
		 * a request to a fdqp the first time, return error if file
		 * descriptor is bad, this is done only once til closed.
		 */
		if (fd_notvalid && (mode = fcntl(fd, F_GETFL)) < 0) {
			MUTEX_UNLOCK(&aiorequestq_lock);
			errno = EBADF;
			return (-1);
		}
		if (mode & O_APPEND) {
			fdqp->fd_flag = F_INUSE | F_SEQUENTIAL;
		} else {
			fdqp->fd_flag = F_INUSE;
		}
		INITQUE(&fdqp->fd_aiolist);
		qempty = 1;
	} else {
		/*
		 * if this fd is being closed by another application thread,
		 * then we return EBADF; otherwise, check if it's empty.
		 */
		if (fdqp->fd_flag & F_CLOSING) {
			MUTEX_UNLOCK(&aiorequestq_lock);
			errno = EBADF;
			return (-1);
		}
		qempty = (EMPTYQUE(&fdqp->fd_aiolist) ? 1 : 0);
	}
	/*
	 * all possible errors have been tested and passed, set EINPROGRESS now
	 */
	aioop->a_aiocbp->aio__error = EINPROGRESS;

	/*
	 * a request for fd opened with O_APPEND (F_SEQUENTIAL is set),
	 * it will be queued on the per-fd list to ensure FIFOness, 
	 * this will be the case if:
	 *	i) fdqp was not empty , as indicated by qempty == 0,
	 *	ii) there is a request for this fd on ready-job list, as
	 *		indicated by the F_ONREADYQ flag,
	 *	iii) a server is serving this per-fd list, as indicated by
	 *		the F_INSVC flag,
	 */
	if (fdqp->fd_flag & F_SEQUENTIAL) {
		if (!qempty || (fdqp->fd_flag & (F_ONREADYQ | F_INSVC))) {
			aioop->a_flink = &fdqp->fd_aiolist;
			aioop->a_blink = fdqp->fd_aioblink;
			fdqp->fd_aioblink->flink = &aioop->a_flist;
			fdqp->fd_aioblink = &aioop->a_flist;
			MUTEX_UNLOCK(&aiorequestq_lock);
			return (0);
		}
		/*
	 	 * this request must go onto the ready-job list, set flag
	 	 * F_ONREADYQ so later requests may only need to be queued
	 	 * on the per-fd list.
	 	 */
		fdqp->fd_flag |= F_ONREADYQ;
	}
	/*
	 * come here for either a ready-to-be-executed sequential I/O request
	 * or a random I/O request, just insert it at tail of the ready-job list
	 */
	aioop->a_flink = &aioreadyq.a_flist;
	aioop->a_blink = aioreadyq.a_blink;
	aioreadyq.a_blink->flink = &aioop->a_flist;
	aioreadyq.a_blink = &aioop->a_flist;

	/*
	 * have work to do, arrange for a server to be awakened, if there are
	 * idle servers,  or created, if needed and upper limit of the pool
	 * has not been reached, to serve this request.
	 * for a new server, we simply wake up the server creator thread to 
	 * do it and return, instead of waiting for thr_create() to complete.
	 */
	if (totalidlesvrcnt) {
		COND_SIGNAL(&svr_cond);
	} else {
		if (totalsvrcnt < maxsvrcnt) {
			totalsvrcnt++;
			createsvrcnt++;
			COND_SIGNAL(&createsvr_cond);
		}
	}
	MUTEX_UNLOCK(&aiorequestq_lock);

	return (0);
}

/*
 * STATIC void
 * createaioserver(void)
 *	this thread is created exactly once during aiostart(). it is
 *	a forever loop responsible for creating aioserver threads when
 *	requested by aioread/aiowrite.
 *
 * Calling/Exit state:
 *	never exit once called, may acquire and release aiorequestq_lock
 *
 * Remarks:
 *	since aiocontrol() had incremented totalsvrcnt before waking us up,
 *	we have to update totalsvrcnt if any thr_create() fails. also we
 *	use local var and release the aiorequestq_lock before calling 
 *	thr_create since thr_create could take a while
 */

STATIC void *
createaioserver()
{
	svrtcb_t	*svrp;
	int		needcnt;
	int		failcnt = 0;

	creatortid = thr_self();
	for ( ; ; )
	{
		MUTEX_LOCK(&aiorequestq_lock);
		if (failcnt)
			totalsvrcnt -= failcnt;
		while (createsvrcnt == 0) {
			COND_WAIT(&createsvr_cond, &aiorequestq_lock);
		}
		needcnt = createsvrcnt;
		createsvrcnt = 0;
		MUTEX_UNLOCK(&aiorequestq_lock);
		failcnt = 0;
		while (needcnt--) {
			if ((svrp = svrtcb_alloc()) == NULLSVRTCB) {
				failcnt++;
				continue;
			}
			if (thr_create(NULLV, 0, (void *(*) (void *))aioserver, 
				(void *)svrp, THR_BOUND|THR_DETACHED|THR_DAEMON,
				 &svrp->st_tid)) {
				svrtcb_free(svrp);
				failcnt++;
			}
		}
		/*
		 * if failcnt !=0, must adjust totalsvrcnt; but do it at 
		 * the beginning of loop to save MUTEX_LOCK/MUTEX_UNLOCK calls
		 */
	}
}

/*
 * STATIC void
 * aioserver(svrtcb_t *svrtp)
 *	asynchronous I/O server thread executes this routine.
 *	throughout its lifetime, it fetches the next asynchronous I/O request
 *	from either the per-fd list or the ready-job list, puts it on active-job
 *	list, starts the I/O using [p]read/[p]write.
 *	On completion, it removes the request from the active-job list and 
 *	performs requested action (e.g. notification, aio_suspend) as appropriate.
 *
 * 	Each aioserver() thread, has an associated aio-server-thread-control-block,
 *	svrtcb_t *svrtp, used for storing info about tid, working per-fd list,
 *	etc. svrtp is passed in by the caller of thr_create() who should also 
 *	have initialized the mutex & cond_var.
 * 	aioserver() terminates itself on timeout/reaching server pool size limit
 *
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 */
STATIC void *
aioserver(svrtcb_t *svrtp)
{
	aioop_t		*ap;
	fd_entry_t	*fdqp;
	int		workitem, rtn;
	struct sigaction sact;
	struct timeval	ct;
	timestruc_t	aiosvrtimer = {0, 0};
	time_t		aiosleep;	/* how long it should sleep */
	pollreq_t	*pp, *tpp;
	int		longjmped = 0;
	int		aio_seq = 0;
	thread_desc_t *tp = curthread;
	boolean_t	aio_found;
	thread_t	mytid = thr_self();
	/*
	 * Signal mask of I/O server is inherited from the server creator.
	 * SIGAIO will be enabled only for the [p]read/[p]write system calls.
	 */

	/*
	 * let the first server establish SIGAIO handler.
	 */
	_thr_sigoff(tp);

	if (need_hndlr) {
		sact.sa_flags = 0;
		sact.sa_handler = sigaiohand;
		sact.sa_mask = ioserver_sigmask;
		(void) sigaddset(&sact.sa_mask, SIGAIO);
		(void) sigaction(SIGAIO, &sact, NULL);
		need_hndlr = B_FALSE;
	}
	/*
	 * prepare for signal SIGAIO from aiocancel(). 
	 * sigsetjmp() here using a jmpbuf in svrtp, always returns 0 the first time.
	 * sigaiohand(), when entered, finds the right jmpbuf and siglongjmp() back.
	 * On return from siglongjmp(), sigsetjmp() returns 1.
	 */
	longjmped = 0;
	if (sigsetjmp(svrtp->st_jmpbuf, 1) != 0) {
		_thr_sigoff(tp);
		longjmped = 1;	/* we don't jump INTO any while loop */
	}
	aiosleep = AIOSLEEP;
	/*
	 * loop until it terminates itself on timeout.
	 */
	for ( ; ; )
	{
		if (longjmped) {
			/*
			 * pick up the ap we were working on before signal
			 * interrupt, set request as canceled
			 * Note that _thr_sigoff() is in effect in this point
			 * (see sigsetjmp() above).
			 */
			ap = svrtp->st_ap;
			longjmped = 0;
			MUTEX_LOCK(&workjobq_lock);
			ap->a_flink->rlink = ap->a_blink;
			ap->a_blink->flink = ap->a_flink;
			ap->a_aiocbp->aio__error = ECANCELED;
			aio_callback(ap);
                        /*
                         * Note that workjobq_lock is released by aio_callback()
                         */
                        continue;
		}
		MUTEX_LOCK(&aiorequestq_lock);
		/*
		 * if we were working on an per-fd queue, then fetch next job
		 * from that queue if the associated fd is not being closed and 
		 * the queue is not empty.
		 *
		 * if there isn't any workitem from the per-fd queue, or we
		 * were working on a random request, then fetch the next job
		 * from the ready-job list.
		 */
		workitem = 0;
		if (svrtp->st_fd != INVALIDFD) { /* we were working on a per-fd q */
			fdqp = &fdtable.fdt_array[svrtp->st_fd];
			if (fdqp->fd_flag & F_INUSE) {
				ASSERT(fdqp->fd_flag & F_INSVC);
				ASSERT(!(fdqp->fd_flag & F_ONREADYQ));
				if ((fdqp->fd_flag & F_CLOSING)
				     || (EMPTYQUE(&fdqp->fd_aiolist))) {
					fdqp->fd_flag &= ~F_INSVC;
					svrtp->st_fd = INVALIDFD;
				} else {
					ap = (aioop_t *)fdqp->fd_aioflink;
					ap->a_flink->rlink = ap->a_blink;
					ap->a_blink->flink = ap->a_flink;
					ap->a_tid = thr_self();
					workitem = 1;
				}
			}
		}
		if (!workitem) {
			while ((ap = get_aioop(svrtp, mytid)) == NULLAIOOP) {
				totalidlesvrcnt++;
				(void) _sys_gettimeofday(&ct);
				aiosvrtimer.tv_sec = ct.tv_sec + aiosleep;
				aiosvrtimer.tv_nsec = 0;

				rtn = COND_TIMEDWAIT(&svr_cond,
					&aiorequestq_lock, &aiosvrtimer);
				totalidlesvrcnt--;	/* ready to fetch job */
				if (rtn == ETIME) {
					if (totalsvrcnt > minsvrcnt) {
						svrtcb_free(svrtp);
						totalsvrcnt--;
						MUTEX_UNLOCK(&aiorequestq_lock);
						_thr_sigon(tp);
						thr_exit((void *) 0);
					}
					/*
					 * min pool: "long" sleep til work in
					 */
					aiosleep = AIOLONGSLEEP;
				} else
					aiosleep = AIOSLEEP;
			} /* while get_aioop() */
		}

		aio_seq = (fdtable.fdt_array[ap->a_aiocbp->aio_fildes].fd_flag & F_SEQUENTIAL) ?
		 F_SEQUENTIAL : ~F_SEQUENTIAL;
		MUTEX_UNLOCK(&aiorequestq_lock);
		/*
		 * now put the request on the active-job list
		 * also remember it in svrtp->st_ap in case we're signal'ed
		 */
		MUTEX_LOCK(&workjobq_lock);
		ap->a_flink = aioactiveq.a_flink;
		ap->a_blink = &aioactiveq.a_flist;
		aioactiveq.a_flink->rlink = &ap->a_flist;
		aioactiveq.a_flink = &ap->a_flist;
		svrtp->st_ap = ap;

		MUTEX_UNLOCK(&workjobq_lock);
		_thr_sigon(tp);
		/*
		 *  all locks should be released.
		 *  get info from ap->aiocbp and call [p]read/[p]write to 
		 *  start I/O. two cases for both: AIO_SEQ or non-AIO_SEQ
		 */
		switch(ap->a_cmd) {
		case AIO_READ:
			if (aio_seq == F_SEQUENTIAL) {
				rtn = read(ap->a_aiocbp->aio_fildes,
						(void *) ap->a_aiocbp->aio_buf,
						ap->a_aiocbp->aio_nbytes);
			} else {
				rtn = pread(ap->a_aiocbp->aio_fildes,
						(void *) ap->a_aiocbp->aio_buf,
						ap->a_aiocbp->aio_nbytes,
						ap->a_aiocbp->aio_offset);
			}
			break;
		case AIO_WRITE:
			if (aio_seq == F_SEQUENTIAL) {
				rtn = write(ap->a_aiocbp->aio_fildes,
						(void *) ap->a_aiocbp->aio_buf,
						ap->a_aiocbp->aio_nbytes);
			} else {
				rtn = pwrite(ap->a_aiocbp->aio_fildes,
						(void *) ap->a_aiocbp->aio_buf,
						ap->a_aiocbp->aio_nbytes,
						ap->a_aiocbp->aio_offset);
			}
			break;
		default:
			fprintf(stderr,"Unknown aio command\n");
			abort();
			break;
		}
		/*
		 * done, mask off SIGAIO, process completed job
		 */
		_thr_sigoff(tp); /* SIGAIO off */
		MUTEX_LOCK(&workjobq_lock);
		ap->a_flink->rlink = ap->a_blink;
		ap->a_blink->flink = ap->a_flink;
		/*
		 * I/O completed, either successfully or in error
		 * if in error, could be EINTR due to aiocancel, or real error
		 * order of assignments here is important, since user may be
		 * spinning on testing if aio_errno is changed from EINPROGRESS
		 */
		if (rtn == -1) {
			if (errno == EINTR) {
				ap->a_aiocbp->aio__error = ECANCELED;
			} else {
				ap->a_aiocbp->aio__error = errno;
			}
		} else {
			ap->a_aiocbp->aio__return = rtn;
			ap->a_aiocbp->aio__error = 0;
		}
		aio_callback(ap);
	} /* for ( ; ; ) */
}

/*
 * STATIC void
 * aio_callback(aioop_t *ap)
 *	check in order blocked aio_suspend, or notify.
 * Calling/Exit state:
 *	workjobq_lock is held upon entry and it is released before exit.
 */

STATIC void 
aio_callback(aioop_t *ap)
{
	poll_wakeup(ap->a_aiocbp);
	MUTEX_UNLOCK(&workjobq_lock);
	if (ap->a_aiocbp->aio_sigevent.sigev_notify == SIGEV_CALLBACK) {
		(ap->a_aiocbp->aio_sigevent.sigev_func)(ap->a_aiocbp->aio_sigevent.sigev_value);
	}
	aioop_free(ap);
}

/*
 * STATIC aioop_t *
 * get_aioop(svrtcb_t *sp)
 *	get the next request from the ready-job list
 *	if request is not for AIO_SEQ I/O, then st_fd is set to INVALIDFD such
 *	that aioserver will not attempt to fetch next request from the per-fd
 *	list. it will instead fetch next request from the ready-job list.
 *
 * Calling/Exit state:
 *	aiorequestq_lock is held upon entry and not released
 *
 *    Return values:
 *	an aioop_t * to next AIO request to be processed
 *	NULL - if ready-job list is empty, 
 */

STATIC aioop_t *
get_aioop(svrtcb_t *sp, thread_t tid)
{
        aioop_t         *aiop;
        fd_entry_t	*fp;

	if (EMPTYQUE(&aioreadyq.a_flist))
		return (NULLAIOOP);
	aiop = (aioop_t *)aioreadyq.a_flink;
	/*
	 * pick up the first request on ready-job list,
	 * if fd was opened for sequential I/O, the corresponding per-fd q 
	 * must have F_ONSVRQ set, clear it. also set F_INSVC.
	 * if ^AIO_SEQ, then set st_fd to indicate a invalid fd
	 * so server will not attempt to continue processing those
	 * on the corresponding per-fd list.
	 */
	aiop->a_flink->rlink = aiop->a_blink;
	aiop->a_blink->flink = aiop->a_flink;
	aiop->a_tid = tid;

	fp = &fdtable.fdt_array[aiop->a_aiocbp->aio_fildes];
	if (fp->fd_flag & F_SEQUENTIAL) {
		sp->st_fd = aiop->a_aiocbp->aio_fildes;
		ASSERT(fp->fd_flag & F_ONREADYQ);
		fp->fd_flag &= ~F_ONREADYQ;
		fp->fd_flag |= F_INSVC;
	} else
		sp->st_fd = INVALIDFD;
	return(aiop);
}

/*
 * STATIC int
 * apoll(pollreq_t *pp)
 *	search for matching completed operations.
 *	sleep p_timeout if no matching requests are found,
 *	using p_cond and workjobq_lock.
 *
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 *    Return values:
 *	0 - if one or more completed requests were found
 *	-1 - if timedout, sets errno to EAGAIN
 *           if COND_TIMEDWAIT() is EINTR'ed, sets errno to EINTR
 */

STATIC int
apoll(pollreq_t *pp)
{
	aiocb_t		**iopp;
	int		rtn;
	int		nent, elcount;
	struct	timeval	pt;
	timestruc_t	polltimer;

	MUTEX_LOCK(&workjobq_lock);
	elcount = 0;
	nent = 0;	
	for (iopp = pp->p_list, elcount = 0; elcount < pp->p_size && nent < pp->p_nent;
	elcount++, iopp++) {
		if (*iopp) {
			if ((*iopp)->aio__error != EINPROGRESS) {
				MUTEX_UNLOCK(&workjobq_lock);
				return(0);
			}
			nent++;
		}
	}
	/*
	 * must block until either time-out or one completion,
	 * first add it to the queue of blocked aiopoll calls, 
	 * figure out the timeout interval, then sleep
	 */
	pp->p_next = pollslpq;
	pollslpq = pp;
	(void) gettimeofday(&pt, (struct timezone *) NULL);
	if (pp->p_timeout.tv_sec != -1) {	/* finite sleep */
		time_t	tsec;
		long	tnsec;
		/*
		 * calculate the sec and nsec in p_timeout,
		 */
		tsec = pp->p_timeout.tv_sec + pt.tv_sec;
		tnsec = pp->p_timeout.tv_nsec + (pt.tv_usec * (NANOSEC/MICROSEC));
		if (tnsec >= NANOSEC) {
			tnsec -= NANOSEC;
			tsec += 1;
		}
		polltimer.tv_sec = tsec;
		polltimer.tv_nsec = tnsec;
	} else { /* indefinite sleep via repeated AIOLONGSLEEP */
		polltimer.tv_sec = pt.tv_sec + AIOLONGSLEEP;
		polltimer.tv_nsec = pt.tv_usec * (NANOSEC/MICROSEC);
	}
apollagain:
	rtn = COND_TIMEDWAIT(&pp->p_cond, &workjobq_lock, &polltimer);
	if (rtn != 0) {
		pollreq_t	*tpp;
		/*
		 * if infinite timer and errno == ETIME, then sleep again
		 */
		if (pp->p_timeout.tv_sec == -1 && errno == ETIME)
			goto apollagain;
		/*
		 * rtn is either ETIME or EINTR, just set to errno
		 * remove pp from pollslpq first
		 */
		if (pollslpq == pp)
			pollslpq = pp->p_next;
		else {
			for (tpp = pollslpq; tpp; tpp = tpp->p_next) {
				if (tpp->p_next == pp) {
					tpp->p_next = pp->p_next;
					break;
				}
			}
		}
		if (rtn == ETIME) {
			errno = EAGAIN;
		} else {
			errno = rtn;
		}
		ASSERT(!pp->p_runcnt);
		MUTEX_UNLOCK(&workjobq_lock);
		return (-1);
	}
	/*
	 * Otherwise, aioserver() will remove pp from pollslpq.
	 */
	ASSERT(pp->p_runcnt);
	MUTEX_UNLOCK(&workjobq_lock);
	return (0);
}

/*
 * STATIC int
 * cancel(int fd, struct aiocb *aiocbp, int cmd)
 *	search all lists and cancel all matching requests.
 *	
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 *    Return values:
 *	AIO_CANCELED if the requested operation(s) were canceled.
 *	AIO_NOTCANCELED if at least one of the requested operation(s) cannot
 *	be canceled because it is in progress.
 *	AIO_ALLDONE is returned if all of the operations have already completed.
 *	
 */

STATIC int
cancel(int fd, struct aiocb *aiocbp, int cmd)
{
	fd_entry_t	*fdqp;
	aioop_t		*ap, *tmp;
	int		rval;
	int		rtn = AIO_ALLDONE; /* assume here that's all done */
					   /* then adjust appropriately */

	/*
	 * we cancel only those that are already in our system, so lock the
	 * aiorequestq_lock to block the new aioread/aiowrite from coming in
	 */
	MUTEX_LOCK(&aiorequestq_lock);
	if (fd > fdtable.fdt_size) {
		goto cancel_badf;
	}
	fdqp = &fdtable.fdt_array[fd];
	if (!(fdqp->fd_flag & F_INUSE) || (fdqp->fd_flag & F_CLOSING)) {
		goto cancel_badf;
	}
	if (aiocbp && (aiocbp->aio__error != EINPROGRESS)) {
		MUTEX_UNLOCK(&aiorequestq_lock);
		return(rtn);
	}		
	if (cmd == AIO_CLOSE) {
	/*
	 * set F_CLOSING flag so future close()/aioread()/aiowrite() to this
	 * fd will get EBADF.
	 */

		fdqp->fd_flag |= F_CLOSING;
	}
	if (!EMPTYQUE(&fdqp->fd_aiolist)) {
		rtn = cancelfd(fdqp, aiocbp);
	}

	/*
	 * if cancel of a single aiocb was requested and the aiocb was found
	 * we are done.
	 */

	if (aiocbp && rtn == AIO_CANCELED) {
		
		MUTEX_UNLOCK(&aiorequestq_lock);
		return(rtn);
	}
	/*
	 * check ready-job list 
	 */	
	for (ap = (aioop_t *)aioreadyq.a_flink; ap != &aioreadyq;) {
		if (aiocbp && (ap->a_aiocbp != aiocbp)) {
			ap = (aioop_t *)ap->a_flink;
			continue;
		} else if (ap->a_aiocbp->aio_fildes != fd) {
			ap = (aioop_t *)ap->a_flink;
			continue;
		}
		canceljob(ap, ECANCELED);
		 poll_wakeup(ap->a_aiocbp);
		if (ap->a_aiocbp->aio_sigevent.sigev_notify == SIGEV_CALLBACK) {
			MUTEX_UNLOCK(&aiorequestq_lock);
			(ap->a_aiocbp->aio_sigevent.sigev_func)(ap->a_aiocbp->aio_sigevent.sigev_value);
			MUTEX_LOCK(&aiorequestq_lock);
		}
		tmp = ap;
		ap = (aioop_t *)ap->a_flink;
		aioop_free(tmp);
		rtn = AIO_CANCELED;
		if (aiocbp) {
			break;
		}
	}
		
	if (aiocbp && rtn == AIO_CANCELED) {
		MUTEX_UNLOCK(&aiorequestq_lock);
		return(rtn);
	}
	/*
	 * next send signal to interrupt active jobs
	 */
	MUTEX_LOCK(&workjobq_lock);
	for (ap = (aioop_t *)aioactiveq.a_flink; ap != &aioactiveq; ap = (aioop_t *)ap->a_flink) {
		if (aiocbp && (ap->a_aiocbp != aiocbp)) {
			continue;
		}
		if (ap->a_aiocbp->aio_fildes != fd) {
			continue;
		}
		rval = thr_kill(ap->a_tid, SIGAIO);
		ASSERT(!rval);
		rtn = AIO_NOTCANCELED; 
		if (aiocbp) { /* first found and we are done */
			goto cancel_done;	
			break;
		}
	}
cancel_done:
	MUTEX_UNLOCK(&aiorequestq_lock);
	MUTEX_UNLOCK(&workjobq_lock);
	return (rtn);
cancel_badf:
	MUTEX_UNLOCK(&aiorequestq_lock);
	errno = EBADF;
	return (-1);
}

/*
 * STATIC int
 * cancelfd(fd_entry_t *fp, struct aiocb *aiocbp)
 * 	if aiocbp == NULL, cancel every request on this per-file-descriptor
 *	list, otherwise, cancel request that match aiocbp,
 * 	called only if fp queue is not empty
 *
 * Calling/Exit state:
 *	aiorequestq_lock is held upon entry and is not released
 */

STATIC int
cancelfd(fd_entry_t *fp, struct aiocb *aiocbp)
{
	aioop_t		*ap;
	aioop_t		*tmp;
	int		rtn = AIO_ALLDONE ;

	/* scan per-fd queue */

	for (ap = (aioop_t *)fp->fd_aioflink; ap != (aioop_t *)fp; ) {
		if (aiocbp && (ap->a_aiocbp != aiocbp)) {
			ap = (aioop_t *)ap->a_flink;
			continue;
		}
		canceljob(ap, ECANCELED);
		poll_wakeup(ap->a_aiocbp);
		if (ap->a_aiocbp->aio_sigevent.sigev_notify == SIGEV_CALLBACK) {
			MUTEX_UNLOCK(&aiorequestq_lock);
			(ap->a_aiocbp->aio_sigevent.sigev_func)(ap->a_aiocbp->aio_sigevent.sigev_value);
			MUTEX_LOCK(&aiorequestq_lock);
		}
		tmp = ap;
		ap = (aioop_t *)ap->a_flink;
		aioop_free(tmp);
		rtn = AIO_CANCELED;
		if (aiocbp) {
			break;
		}
	}
	return(rtn);
}

/*
 * STATIC void
 * canceljob(aioop_t *ap, int ecancel)
 * 	cancel an asynchronous I/O request
 * 	1. for those on per-fd list and ready-job list, ecancel = ECANCELED,
 * 	   and its aio_errno is set to it.
 * 	2. for those on done-job list, ecancel = 0 since they may have been
 *	   completed successfully, and aio_errno is unchanged.
 *
 * Calling/Exit state:
 *	aiorequestq_lock is always held upon entry and is not releases on exit
 *	workjobq_lock may be held upon entry and if so, is not released on exit.
 *	(workjobq_lock is held if ap is on done-job list and not held if on
 *	per-fd or ready-job lists.)
 */
STATIC void
canceljob(aioop_t *ap, int ecancel)
{
	ap->a_blink->flink = ap->a_flink;
	ap->a_flink->rlink = ap->a_blink;
	if (ecancel)
		ap->a_aiocbp->aio__error = ecancel;
}

/*
 * STATIC void
 * poll_wakeup(aiocb_t *aiop, int flag)
 *      find if there is anybody blocked for this aiocb
 *	and if needed wake it up.
 * Calling/Exit state:
 *	workjobq_lock is held upon entry and on exit.
 */

STATIC void
poll_wakeup(aiocb_t *aiop)
{
pollreq_t	*pp, *tpp;
boolean_t	aio_found;

	for (tpp = pp = pollslpq, aio_found = B_FALSE; 
		aio_found == B_FALSE && pp != NULLPOLLREQ; 
		tpp = pp, pp = pp->p_next) {
		register aiocb_t **iopp;
		register int elcount;
		for(iopp = pp->p_list, elcount = 0; elcount < pp->p_size;elcount++, iopp++) {
			if (*iopp && *iopp == aiop) { /* found it */
				pp->p_runcnt++;
				if (pp->p_runcnt == pp->p_nent || 
					pp->p_flag == AIO_SUSPEND) {
					/* wake up this matching aiopoll requestor */
					if (tpp == pp) {
						pollslpq = pp->p_next;
					} else {
						tpp->p_next = pp->p_next;
					}
					COND_SIGNAL(&pp->p_cond);
				}
				aio_found = B_TRUE;
				return;
			}
		}
	}
}

/*
 * STATIC aioop_t *
 * aioop_alloc(void)
 *	allocate an aioop_t structure
 *
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 *    Return value:
 *	NULLAIOOP, or a pointer to an aioop_t structure
 *
 * Remarks:
 *	aioop_t nodes are one-way chained through the a_flink pointer field.
 */

STATIC aioop_t *
aioop_alloc()
{
	aioop_t	*ap, *tmp;

	MUTEX_LOCK(&freelist_lock);
	if (aioop_freeq == NULLLIST) {
		/* ran out, allocate another maxaioop */
		if ((tmp = (aioop_t *)malloc(maxaioop * sizeof(aioop_t))) == NULLAIOOP) {
			ap = NULLAIOOP;
			goto a_alloc_out;
		}
		for (ap = tmp; (ap-tmp) < maxaioop; ap++) {
			aioop_t *tap = ap + 1;
			ap->a_flink = &tap->a_flist;
		}
		(ap-1)->a_flink = NULLLIST;
		aioop_freeq = (list_t *)tmp;
	}
	ap = (aioop_t *)aioop_freeq;
	aioop_freeq = ap->a_flink;
a_alloc_out:
	MUTEX_UNLOCK(&freelist_lock);
	return(ap);
}

/*
 * STATIC void
 * aioop_free(aioop_t *ap)
 *	deallocate an aioop_t structure
 *
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 *
 * Remarks:
 *	aioop_t nodes are chained through the a_flink pointer field.
 */

STATIC void
aioop_free(aioop_t *ap)
{
	MUTEX_LOCK(&freelist_lock);
	ap->a_flink = aioop_freeq;
	aioop_freeq = &ap->a_flist;
	MUTEX_UNLOCK(&freelist_lock);
}

/*
 * STATIC svrtcb_t *
 * svrtcb_alloc(void)
 *	allocate a svrtcb_t structure
 *
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 *    Return value:
 *	NULLSVRTCB, or a pointer to a svrtcb_t structure
 *
 * Remarks:
 *	svrtcb_t nodes are one-way chained through the st_svrtcbp pointer field.
 */

STATIC svrtcb_t *
svrtcb_alloc()
{
	svrtcb_t	*sp;

	MUTEX_LOCK(&freelist_lock);
	if ((sp = svrtcb_freeq) != NULLSVRTCB)
		svrtcb_freeq = sp->st_svrtcbp;
	MUTEX_UNLOCK(&freelist_lock);
	return(sp);
}

/*
 * STATIC void
 * svrtcb_free(svrtcb_t *sp)
 *	deallocate a svrtcb_t structure
 *
 * Calling/Exit state:
 *	aiorequestq_lock may be locked upon entry and on exit
 *
 * Remarks:
 *	svrtcb_t nodes are one-way chained through the st_svrtcbp pointer field.
 */

STATIC void
svrtcb_free(svrtcb_t *sp)
{
	MUTEX_LOCK(&freelist_lock);
	sp->st_tid = NULLTID;
	sp->st_svrtcbp = svrtcb_freeq;
	svrtcb_freeq = sp;
	MUTEX_UNLOCK(&freelist_lock);
}

/*
 * STATIC void
 * sigaiohand(int signo)
 *	signal handler for SIGAIO
 *	an aioserver thread executes this handler when interrupted by a SIGAIO
 *	sent by aiocancel(),
 *	it finds the sigjmp_buf for calling thread and siglongjmp()s back.
 *
 * Calling/Exit state:
 *	no locks are held upon entry or on exit.
 */

/* ARGSUSED */
STATIC void
sigaiohand(int signo)
{
	thread_t	tid;
	int		sx;

	tid = thr_self();
	/*
	 * must find the sigjmp_buf to siglongjmp() back to
	 * where the sigsetjmp() was done
	 */
	for (sx = 0; sx < maxsvrtcb; sx++) {
		if (svrtcb_tbl[sx].st_tid == tid)
			break;
	}
	ASSERT(sx != maxsvrtcb);
	siglongjmp(svrtcb_tbl[sx].st_jmpbuf, 1); /* no return */
}

#ifdef AIODEBUG
/*
 * void
 * aio_assfail(char *a, char *f, int l)
 *	prints out that an assertion failed and calls abort();
 *
 * Parameter/Calling State:
 *	first argument is a string identifying the assertion that failed
 *	second argument is a string identifying the file where the
 *	 failure occurred
 *	third argument is an int identifying the line where the failure
 *	 occurred
 *
 * Return Values/Exit State:
 *	has no return value
 */

STATIC void
aio_assfail(char *a, char *f, int l)
{	
	(void) fprintf(stderr, "assertion failed: %s, file: %s, line:%d\n", a, f, l);
	abort();
}
#endif /* AIODEBUG */

/*
 * Asynchronous I/O functions for raw slices of hard disk:
 *	raio_read(), raio_write(), raio_error(), raio_return(),
 *	and lio_listio()
 */

#define ASYNCDEV	"/dev/async"
STATIC int		asyncfd = -1;		/* file desc for aio module */
STATIC int		aio_listio_max;		/* max array for lio_listio */
STATIC mutex_t		aiolistio_lock = DEFAULTMUTEX;

STATIC int	raiorw(uchar_t, aiocb_t *);
STATIC int	raioinit();
STATIC void	sigemthandler(int);
STATIC void	setalleagain(struct aiocb **, int);
STATIC void	setsigemt();
STATIC int	validate(aiocb_t *, uchar_t, aiojob_t *);
STATIC void	proc_donejobs(struct aiostatus *, int);

/*
 * int
 * raio_read(aiocb_t *ap)
 *	asynchronous I/O read.
 */

int
raio_read(aiocb_t *ap)
{
	return(raiorw(AIOREAD, ap));
}

/*
 * int
 * raio_write(aiocb_t *ap)
 *	asynchronous I/O write.
 */

int
raio_write(aiocb_t *ap)
{
	return(raiorw(AIOWRITE, ap));
}

/*
 * int
 * raio_error(aiocb_t *ap)
 *	retrieve error status of an asynchronous I/O operation
 *	if ap's error status is not EINPROGRESS, just return it;
 *	otherwise must call ioctl(AIOPOLL) to retrieve the status.
 *
 * Return value:
 *	-1 - if ap is NULL,
 *	EINPROGRESS - if the asynchronous I/O operation is not done yet,
 *	error code for the completed asynchronous I/O operation,
 *
 */

int
raio_error(aiocb_t *ap)
{
	struct aioresult aiop, *aiopp = &aiop;
	struct aiostatus *aiosp = &aiopp->ar_stat[0];
	int		rtn;
	thread_desc_t *curtp = curthread;

	/*
	 * reject if bad ap, or aio not yet initialized (if you have not called
	 * any of aio_read/aio_write/lio_listio, how can you call me?
	 */
	if (asyncfd == -1 || !ap) {
		errno = EINVAL;
		return (-1);
	}
	if (ap->aio__error != EINPROGRESS)
		return (ap->aio__error);
	/*
	 * if still EINPROGRESS, must call ioctl(AIOPOLL) to retrieve results
	 * of completed jobs. we always use MAXSTATUS struct aiostatus's.
	 * (the type of the timeout member is tempoarily set to ulong since
	 * aio_suspend() is not yet supported for raw slice/hard disks.)
	 */
	do {
		aiopp->ar_total = MAXSTATUS;
		aiopp->ar_timeout = 0;
		memset(aiosp, 0, MAXSTATUS * sizeof(aiostatus_t));

		rtn = ioctl(asyncfd, AIOPOLL, aiopp);
		ASSERT(rtn >= 0);
		/*
		 * proc_donejobs() manipulates shared resources such as
		 * the listio control block, lioreq_t. so disable signal
		 * during critical section.
		 */
		if (aiopp->ar_total) {
			_thr_sigoff(curtp);
			proc_donejobs(aiosp, aiopp->ar_total);
			_thr_sigon(curtp);
		}
	} while (aiopp->ar_total == MAXSTATUS);
	return (ap->aio__error);
	
}

/*
 * ssize_t
 * raio_return(aiocb_t *ap)
 *	retrieve return value of an asynchronous I/O operation
 *
 * Return value:
 *	the return status of the corresponding synchronous I/O operation
 *
 * Remarks:
 *	the result is undefined if
 *		- the I/O has not been completed, or
 *		- no async I/O has been called using the argument, ap.
 */

ssize_t
raio_return(aiocb_t *ap)
{
	if (asyncfd == -1 || !ap || ap->aio__error == EINPROGRESS) {
		errno = EINVAL;
		return (-1);
	}
	return (ap->aio__return);
}

/*
 * int
 * lio_listio(int mode, aiocb_t *list[], int nent, struct sigevent *sig)
 *	initiate a list of I/O requests
 * 
 *	mode has to be either LIO_WAIT or LIO_NOWAIT
 *	list[] is an array of nent aiocb_t pointers
 *	sig may be NULL indicating no notification; otherwise it has same
 *	meaning as aio_sigevent.
 *
 *	valid entries will be processed for I/O and at the end a notification
 *	function may be called depending on mode and sig.
 *
 * Return:
 *	0 - if all started for LIO_NOWAIT, or all succeeded for LIO_WAIT.
 *	-1 - if one or more has failed, either invalid or I/O error
 */

int
lio_listio(int mode, struct aiocb *list[], int nent, struct sigevent *sig)
{
	aiojob_t	*aiojobp;
	lioreq_t	*lioreqp = NULLLIOREQ;
	int err, rtn, cmd, i, j;
	int notify, lib_total;
	aiolistio_t	*aiolistp = NULL;

	if (asyncfd == -1) {
		/*
		 * if ASYNCDEV never opened, then aio_memlock() hasn't
		 * been called, reject the request
		 */
		errno = EINVAL;
		return (-1);
	}
	if ((mode != LIO_WAIT && mode != LIO_NOWAIT) || nent > aio_listio_max) {
		errno = EINVAL;
		return (-1);
	}
	/*
	 * if one or more requests are valid, ioctl(AIOLISTIO) is called to
	 * process them. for simplicity, kernel never sends a signal for any 
	 * entry that specifies a callback. furthermore, upon list completion, 
	 * kernel just wakes up and returns with statuses for LIO_WAIT, and for
	 * LIO_NOWAIT, w/ or w/o callback, it always sends a signal; if w/ 
	 * callback, we'll call it, otherwise we don't. the reason for sending 
	 * a signal even when there is not a callback is so kernel does not
	 * have to send a signal for each entry that specifies a callback, again
	 * for simplicity, and this signal causes these entries to be updated
	 * including calling their notification, if any. (The downside of this
	 * simplification is the individual callbacks may be called later than
	 * they should be. For hard disk/raw slice this should be ok.
	 *
	 * if mode == LIO_NOWAIT and sigev_notify == SIGEV_CALLBACK, then
	 * we need to remember the notification information by allocating
	 * a control block for it.
	 */
	notify = 0;
	if (mode == LIO_NOWAIT && sig) {
		if ((sig->sigev_notify == SIGEV_CALLBACK)
			|| (sig->sigev_notify == SIGEV_NONE)) {
			notify = A_ASYNCNOTIFY;
			lioreqp = (lioreq_t *) malloc(sizeof(lioreq_t));
			if (lioreqp == NULLLIOREQ) {
				setalleagain(list, nent);
				errno = EAGAIN;
				return (-1);
			}
		} else {
			errno = EINVAL;
			return (-1);
		}
	}
	/*
	 * create an aiojob[] for list[] entries that are not to be ignored
	 * and set aiojob[].aj_errno and list[].aio__errno to EINPROGRESS
	 */
	aiolistp = (aiolistio_t *) malloc(sizeof(aiolistio_t) + 
			sizeof(aiojob_t)*aio_listio_max);
	if (aiolistp == (aiolistio_t *) NULL) {
		setalleagain(list, nent);
		free(lioreqp);
		errno = EAGAIN;
		return (-1);
	}
	aiojobp = &aiolistp->al_jobs[0];
	i = lib_total = err = 0; /* lib_total is no. of jobs accepted by lib */
	for (; i < nent; i++) {
		if (list[i] == NULL || list[i]->aio_lio_opcode == LIO_NOP)
			continue;
		/*
		 * we don't call notification on individual errors
		 */
		cmd = list[i]->aio_lio_opcode;
		if (cmd != LIO_READ && cmd != LIO_WRITE) {
			list[i]->aio__error = EINVAL;
			if (!err)
				err = EIO;
			continue;
		}
		cmd = (cmd == LIO_READ ? AIOREAD : AIOWRITE);
		if (rtn = validate(list[i], cmd, &aiojobp[lib_total])) {
			err = list[i]->aio__error = rtn;
			continue;
		}
		list[i]->aio__next = (void *) lioreqp;
		list[i]->aio__error = EINPROGRESS;
		lib_total++;
	}
	/*
	 * if none of the requests was accepted, don't call ioctl, but free all
	 * malloc'ed areas;
	 * return -1 even if all were ignored (ie none was bad, or err == 0)
	 */
	if (lib_total == 0) {
		free(lioreqp);	/* free() checks if lioreqp is 0 */
		free(aiolistp);
		errno = err;
		return(err ? -1 : 0);
	}
	if (lioreqp) {
		lioreqp->lio_total = lib_total;
		lioreqp->lio_cnt = 0;
		lioreqp->lio_notify = *sig;
	}
	aiolistp->al_nent = lib_total;
	aiolistp->al_mode = mode;
	aiolistp->al_flag = notify;
	
	/*
	 * at this point, the requests in the list[] array submitted by
	 * the user may have been split into two parts, one for those that
	 * have failed the validation check and will thus not be queued, and
	 * the other that will be given to kernel for real I/O processing. 
	 *
	 * This is determined by err. 
	 *
	 * all in one piece if err == 0. Otherwise, user lio_listio() call is 
	 * considered failed even if the following ioctl() call succeeds.
	 *
	 * if mode is LIO_WAIT and ioctl() is good, then process every entry
	 * in aiolistp since we know they have completed, the assumption is
	 * for LIO_WAIT, kernel copyout()s the status results back, in aiolistp.
	 * (If kernel did not copy them out, as in LIO_NOWAIT, then they would
	 * have to be kept in kernel til someone called ioctl(AIOPOLL))
	 * [ reason:
	 *   if kernel does not, then what we see here will be just
	 *   EINPROGRESS but won't know the real status, thus we won't know
	 *   if we should call their notification function, if any, or not.
	 *   This could results in: lio_listio() returned success but the
	 *   notification has not occurred yet. this is bad.
	 *   An alternative is we call ioctl(AIOPOLL) to get the results. But
	 *   if a SIGEMT comes in after ioctl(AIOLISTIO) returns (after all 
	 *   jobs on the list have completed) but before the next statement 
	 *   (eg timesliced out), when a job NOT on the list completes, then 
	 *   the handler will call AIOPOLL to get the results that may include
	 *   some from our list. Then we won't get all here. In this case
	 *   do we just assume their notification, if any, have all been called,
	 *   what if some of them failed, do we just return 0? (Handler always
	 *   has to call ioctl(AIOPOLL) because it has no way of knowing what
	 *   the signal is for, ie, for listio or non-listio.)
	 *
	 *   Note that when ioctl(AIOLISTIO) returns, those jobs whose aj_errno
	 *   equals EINPROGRESS may have actually completed in error or 
	 *   successfully, but we cannot tell.
	 * ]
	 * So kernel copyout() the aiolistio_t/aiojobp[] with statuses, and
	 * no one else would get it but us here. For each aiojobp[i], update 
	 * status, callback if so requested, remember if any is in error
	 * which, along with err, is used to set return value.
	 *
	 * if mode is LIO_NOWAIT and ioctl() is good, then just return 
	 * (return value depends on err). When all are done later, sig handler 
	 * will be called and it will ioctl(AIOPOLL) to get as many as it can, 
	 * update status, call individual callback if any. if an lioreq_t has
	 * been allocated, then it will also call its callback when all 
	 * lio_total have completed. In this case, it will then free lioreqp. 
	 * so we don't need to bother with lioreqp here.
	 *
	 * If LIO_WAIT and ioctl() failed, then all jobs have completed but
	 * some may be in error. Since kernel copyout() the status, processing 
	 * is very similar to that when ioctl() returns 0.
	 *
	 * If LIO_NOWAIT and ioctl() failed, then some jobs may have started 
	 * and others not; in this case kernel only copyout() the aj_errno
	 * in aiojob_t. So must check aiojobp->aj_errno to determine. 
	 * Since we set aiojobp->aj_cbp->aio__error to EINPROGRESS
	 * before calling ioctl(), we ignore those aiojobp whose aj_errno is
	 * still EINPROGRESS. For all others, we know they've failed, so move
	 * aiojobp->aj_errno to aiojobp->aj_cbp->aio__error and
	 * call the callback necessary here??.
	 *
	 * Now if LIO_NOWAIT w/ callback for the list, let poll call
	 * notification and afree lioreqp.
	 *
	 * for LIO_WAIT, if some but not all requests are bad, for example,
	 * non-aligned offset, lio_listio() will block. P1003.4 is not
	 * clear on this.
	 *
	 * The assumption is that even if there are individual reqeusts in
	 * the list[] that specify a callback function, kernel will NOT
	 * send a signal upon individual completion until all are completed.
	 * For LIO_WAIT, when all are completed, kernel simply wakes up and
	 * returns from the ioctl(). Also kernel will have it sleep at PZERO,
	 * so no EINTR is possible. This is fine for hard disk/raw slice.
	 * For LIO_NOWAIT, when all are queued, kernel simply returns;
	 * and w/ or w/o callback, kernel will send a signal when all
	 * are completed. Again, it sends no siganl on individual completion.
	 * Also, it's possible that kernel is not able to allocate space, or
	 * not able to copyin() the aiolistio_t. In that case, kernel sets
	 * EAGAIN and returns -1. In general kernel returns -1 only if it is
	 * not able to set errno in individual aiojob_t entries in aiolistio_t.
	 * Also, it returns 0 even if all entries are processed and all are bad.
	 */
	rtn = ioctl(asyncfd, AIOLISTIO, aiolistp);
	if (mode == LIO_WAIT) {
		aiocb_t *aiocbp;
		/*
		 * ioctl(AIOLISTIO) returns -1/EAGAIN only if all requests failed
		 */ 
		if (errno == EAGAIN) {
			setalleagain(list, nent);
			free(aiolistp);
			return (-1);
		}
		for (j = 0; j < lib_total; j++) {
			aiocbp = aiojobp[j].aj_cbp;
			aiocbp->aio__return = aiojobp[j].aj_cnt;
			aiocbp->aio__error = aiojobp[j].aj_errno;
			ASSERT(aiocbp->aio__error != EINPROGRESS);
			if (aiocbp->aio__error != 0)
				err = EIO;
			/*
			 * call notification if so specified regardless of
			 * the error?
			 */
			if (aiocbp->aio_sigevent.sigev_notify == SIGEV_CALLBACK)
				(aiocbp->aio_sigevent.sigev_func)
					(aiocbp->aio_sigevent.sigev_value);
		}
		if (err) {
			errno = err;
			rtn = -1;
		}
		free(aiolistp);
		return (rtn);
	}
	{ /* mode == LIO_NOWAIT */
		aiocb_t *aiocbp;
		int seterr = 0;
		if (rtn == 0) {
			if ((errno = err) != 0)
				rtn = -1;
			free(aiolistp);
			return(rtn);
		}
		if (errno == EAGAIN) {
			setalleagain(list, nent);
			free(lioreqp);
			free(aiolistp);
			return (-1);
		}
		/*
		 * update status for those not accepted/queued by kernel
		 */
		for (j = 0; j < lib_total; j++) {
			if (aiojobp[j].aj_errno == EINPROGRESS)
				continue; /* accepted/queued by kernel */
			aiocbp = aiojobp[j].aj_cbp;
			aiocbp->aio__return = aiojobp[j].aj_cnt;
			aiocbp->aio__error = aiojobp[j].aj_errno;
			seterr++;
		}
		ASSERT(seterr);
		if (err)
			errno = err;
		else
			errno = EIO;
		free(aiolistp);
		return (-1);
	}
}

/*
 *
 * int
 * aio_memlock(void * avaddr, size_t asize)
 *	lock asize bytes of memory at address avaddr
 * Return value:
 *	0 on success,
 *	-1 and errno = EINVAL if attempting to lock more than once.
 *	   or when aioinit() returns failure (these functions also set errno).
 */
int
aio_memlock(void * avaddr, size_t asize)
{
	asyncmlock_t alock;

	if (asyncfd == -1) {
		if (raioinit() < 0)
			return (-1);
	}
	alock.am_vaddr = (vaddr_t)avaddr;
	alock.am_size = asize;
	if (ioctl(asyncfd, AIOMEMLOCK, &alock) < 0) {
		printf("ioctl(AIOMEMLOCK) failed, errno = %d\n", errno);
		return (-1);
	}
	return (0);
}

/*
 * STATIC int
 * raiorw(uchar_t rwflag, struct aiocb *ap)
 *	Common routine for raio_read/raio_write calls, performs initialization
 *	on very first call of raio_read/raio_write. It then validates the input
 *	argument, ap, and calls ioctl() to start the asynchronous I/O.
 *
 * Return value:
 *	0 on success,
 *	-1 and errno = EINVAL for invalid input values
 *	       errno = EAGAIN for insufficient resources
 *	   or when raioinit() returns failure (these functions also set errno).
 */

STATIC int
raiorw(uchar_t rwflag, aiocb_t *ap)
{
	struct aiojob aiojob;
	int rtn;

	if (asyncfd == -1) {
		/*
		 * if ASYNCDEV never opened, then aio_memlock() hasn't
		 * been called, reject the request
		 */
		errno = EINVAL;
		return (-1);
	}
	if (rtn = validate(ap, rwflag, &aiojob)) {
		errno = rtn;
		return (-1);
	}
	ap->aio__next = NULLLIOREQ;
	/*
	 * set EINPROGRESS before starting I/O to avoid a race between
	 * setting (if set after ioctl() returns) and I/O completion
	 */
	ap->aio__error = EINPROGRESS;

	if (ioctl(asyncfd, AIORW, &aiojob) < 0) {
		ap->aio__error = errno;
		return (-1);
	}
	return (0);
}

/*
 * STATIC int
 * validate(aiocb_t *ap, uchar_t rwflag, aiojob_t *aiojobp)
 *	Routine for validation of an I/O request described in an aiocb_t that
 *	are common to aio_read()/aio_write() and lio_listio().
 *	It validates the input argument, ap, and sets up an aiojob_t for 
 *	ioctl(). (checks of non-common fields are done by each function.)
 *
 * Return value:
 *	0 on success,
 *	EINVAL: for invalid input values
 *	EAGAIN: for insufficient resources
 */

STATIC int
validate(aiocb_t *ap, uchar_t rwflag, aiojob_t *aiojobp)
{
	int notify;
	/*
	 * only callback notification is supported, reject if signal.
	 * clear aio__return field.
	 */
	switch (ap->aio_sigevent.sigev_notify) {
		case SIGEV_NONE:
			notify = 0;
			break;
		case SIGEV_CALLBACK:
			notify = A_ASYNCNOTIFY;
#ifdef AIODEBUG
			if (ap->aio_sigevent.sigev_func == NULL) {
				fprintf(stderr, "bad user aiocb, SIGEV_CALLBACK but sigev_func is NULL\n");
				return(EINVAL);
			}
#endif /* AIODEBUG */
			break;
		case SIGEV_SIGNAL:
		default:
			return (EINVAL);
	}
	if (!(ap->aio_flags & AIO_RAW) || ap->aio_reqprio)
		return (EINVAL);
	ap->aio__return = 0;

	/*
	 * now send the request down via ioctl(fd, AIORW, arg);
	 * first set up info in aiojob_t;
	 */
	aiojobp->aj_fd = ap->aio_fildes;
	aiojobp->aj_buf = (vaddr_t)ap->aio_buf;
	aiojobp->aj_cnt = ap->aio_nbytes;
	aiojobp->aj_offset = ap->aio_offset;
	aiojobp->aj_cmd = rwflag;
	aiojobp->aj_cbp = (void *)ap;
	aiojobp->aj_errno = EINPROGRESS;
	aiojobp->aj_flag = 0;
	if (notify)
		aiojobp->aj_flag |= notify;
	return (0);
}

/*
 * STATIC int
 * raioinit(void)
 *	asynchronous I/O initialization: open ASYNCDEV and get AIO_LISTIO_MAX
 *	called exactly once.
 *
 * Return value:
 *	0 - open succeeded
 *	-1 - open failed
 */

STATIC int
raioinit()
{
	aio_tune_t aioparms;
	/*
	 * open ASYNCDEV for access to AIO kernel support module,
	 * set the close-on-exec flag
	 */
	asyncfd = open(ASYNCDEV, O_RDWR);
	if (asyncfd == -1) {
		perror("open of async device failed\n");
		errno = ENOSYS;
		return (-1);
	}
	if (fcntl(asyncfd, F_SETFD, FD_CLOEXEC) < 0) {
		perror("fcntl of async device for FD_CLOEXEC failed\n");
		errno = ENOSYS;
		return (-1);
	}
	if (ioctl(asyncfd, AIOGETTUNE, &aioparms) < 0) {
		perror("AIOGETTUNE of async device failed\n");
		errno = ENOSYS;
		return (-1);
	}
	aio_listio_max = aioparms.at_listio_max;
	/*
	 * establisth handler for SIGEMT that kernel sends to us on I/O
	 * completions.
	 */
	setsigemt();
	return (0);
}

/*
 * void
 * proc_donejobs(struct aiostatus *aiosp, int total)
 *	process the completed jobs retrieved from kernel with ioctl(AIOPOLL)
 *	aiosp is the array of aiostatus_t with status updated by kernel
 *	total is the size of the array
 *
 *	proc_donejobs() examines every completed job, transfers the erro
 *	status and the return status to the user's aiocb_t. if notification
 *	is specified for the individual job (aio_read or aio_write) or a 
 *	list of jobs (lio_listio()), then call the notification function.
 */
STATIC void
proc_donejobs(struct aiostatus *aiosp, int total)
{
	int i;
	aiocb_t	*notifyp;
	aiocb_t	*uap;
	lioreq_t	*lionotifyp, *liop;

	/*
	 * given aiosp, we find user's job in uap = aiosp->aio_cbp, and
	 * update the statues.
	 * if callback is needed, either for the job itself, or for the 
	 * completion of a lio_listio(), set up lists for their notification 
	 * functions to be called with an argument specified by the user
	 */
	notifyp = NULLAIOCB;	/* head of list of aiocb_t to be notified */
	lionotifyp = NULLLIOREQ;/* head of list of lioreq_t to be notified */
	MUTEX_LOCK(&aiolistio_lock);
	for (i = 0; aiosp->ast_cbp && i < total; i++, aiosp++) {
		uap = aiosp->ast_cbp;
		uap->aio__return = aiosp->ast_count;
		uap->aio__error = aiosp->ast_errno;
		if ((liop = uap->aio__next) != NULLLIOREQ) {
			/*
			 * this job is part of a lio_listio() request,
			 * increment liop->lio_cnt, and if it now equals
			 * liop->lio_total, collect its notification, if any
			 */
			liop->lio_cnt++;
			if ((liop->lio_cnt == liop->lio_total) &&
				(liop->lio_notify.sigev_notify == SIGEV_CALLBACK)) {
				liop->lio_next = lionotifyp;
				lionotifyp = liop;
			}
		}
		if (uap->aio_sigevent.sigev_notify == SIGEV_CALLBACK){
			/*
			 * collect and call later, in case the notification
			 * function calls aio_error(), to avoid a race
			 */
			uap->aio__next = (void *)notifyp;
			notifyp = uap;
			continue;
		}
	}
	MUTEX_UNLOCK(&aiolistio_lock);
	/*
	 * those individual jobs that require a callback 
	 */
	for (uap = notifyp; uap; uap = (aiocb_t *)uap->aio__next) {
		(uap->aio_sigevent.sigev_func)(uap->aio_sigevent.sigev_value);
	}
	/*
	 * those lio_listio() jobs that require a callback 
	 */
	for (liop = lionotifyp; liop; liop = (lioreq_t *)liop->lio_next) {
		(liop->lio_notify.sigev_func)(liop->lio_notify.sigev_value);
		free(liop);
	}
}

/*
 * STATIC void 
 * setsigemt()
 *	set disposition for signal SIGEMT, once only.
 *	SA_RESTART is set so applications' syscalls are not affected.
 *	Assumptions: users do not use SIGEMT at all.
 */

STATIC void
setsigemt()
{
	struct sigaction sa;

	sa.sa_handler = sigemthandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
        sigaction(SIGEMT, &sa, NULL);
}

/*
 * STATIC void
 * sigemthandler(int)
 *	handles the notification of asynchronous I/O completion.
 *
 *	Since signal handler does not get an arbitrary value as its argument,
 *	it must then call ioctl(AIOPOLL) to receive results of jobs. In this
 *	case, it might get one or more or nothing; in any case, it just process
 *	all the returned results, including calling the notification for both
 *	the individual jobs and lio_listio() requests.
 *	We use a separate buffer for job status retrieval, this is necessary
 *	because we are executed out of a signal handler. For example, when
 *	the user calls aio_error() and we are just returning from ioctl(AIOPOLL)
 *	when SIGEMT is fired. If we use a global buffer, then the data in it 
 *	from the main line ioctl(AIOPOLL) would be lost.
 *	Also, repeat ioctl(AIOPOLL) if we get max entries back.
 */

STATIC void
sigemthandler(int signo)
{
	struct aioresult aiop, *aiopp = &aiop;
	struct aiostatus *aiosp = &aiopp->ar_stat[0];
	int rtn;

	do {
		aiopp->ar_total = MAXSTATUS;
		memset(&aiopp->ar_timeout, 0, sizeof(ulong));
		memset(aiosp, 0, MAXSTATUS * sizeof(aiostatus_t));

		rtn = ioctl(asyncfd, AIOPOLL, aiopp);
		ASSERT(rtn >= 0);
		/*
		 * since the signal may be sent just when
		 * the user is doing aio_suspend/ioctl(AIOPOLL) and picks up the
		 * just-completed job and gets signal-interrupted on its way 
		 * back to the user mode. in this case, our ioctl() may not 
		 * get anything back, just return.
		 */
		if (aiopp->ar_total) {
			proc_donejobs(aiosp, aiopp->ar_total);
		}
	} while (aiopp->ar_total == MAXSTATUS);
}

/*
 * STATIC void
 * setalleagain(struct aiocb **list, int n)
 *	sets every entry in the list to EAGAIN. this is called by lio_listio()
 *	when there is a lack of resources.
 */
STATIC void
setalleagain(struct aiocb **list, int n)
{
	int cmd, i = 0;

	for (; i < n; i++) {
		if (list[i] == NULL || list[i]->aio_lio_opcode == LIO_NOP)
			continue;
		if ((cmd = list[i]->aio_lio_opcode) != LIO_READ
		     && cmd != LIO_WRITE) {
			list[i]->aio__error = EINVAL;
			continue;
		}
		list[i]->aio__error = EAGAIN;
	}
}
