/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rexec/rxlib.c	1.2.8.7"
#ident  "$Header: $"

/*
 *
 * MESSAGES:
 *
 * sent by:	CLIENT			SERVER
 *		--------------------------------------
 *		RXM_OPEN_REQ
 *		RXM_OPEN_ARGS
 *		RXM_OPEN_ENVF
 *		RXM_OPEN_ENV
 *		RXM_IOCTL
 *		RXM_OPEN_DONE
 *					RXM_OPEN_REPLY
 *		--------------------------------------
 *		RXM_DATA		RXM_DATA
 *					RXM_WRITEACK
 *		--------------------------------------
 *		RXM_SIGNAL
 *					RXM_SIGNALACK
 *		--------------------------------------
 *					RXM_IOCTL
 *		--------------------------------------
 *					RXM_CLOSE_REQ
 *		RXM_CLOSE_REPLY
 *					RXM_CLOSE_TA
 *		--------------------------------------
 *
 */


/* LINTLIBRARY */

#include <sys/byteorder.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <termio.h>
#include <stdio.h>
#include <stropts.h>
#include <rx.h>
#include <cs.h>
#include "rx_mt.h"

#undef Rx_errno
#undef Rx_cserrno

/* externally defined routines */

extern	char  *argvtostr(char **);   /* changes argv type list to cmd string */

/* private (static) routine declarations */

static	int	rxcsendopenreq(int, long, char *, long);
static	int	rxcsendopenargs(int, char *, int);
static	int	rxcsendopenenvf(int, char *);
static	int	rxcsendopenenv(int, char *, int);
static	int	rxcsendtcsetsioctl(int, struct termios *);
static	int	rxcsendopendone(int);
static	int	rxcsendclosereply(int, long);
static	int	rxcsenddata(int, long, long, char *);
static	int	rxcsendsignal(int, int);
static	int	rxputm(int, long, long, char *);
static	int	_rx_set_ioctl_hand(int, int (*)(int, int, ...));
static	int	_rx_set_write_hand(int, ssize_t (*)(int, const void *, size_t));
static	int	_rx_free_conn(int);
static	int	_rx_set_Dflag(int);
static	int	_rx_get_Dflag(void);


/*
 * global public variables
 *
 * Thread-specific variables will be used for threads other than the first.
 * Access functions will provide a uniform means of
 * setting and getting the values of each thread's variables.
 */

char	Rxenvfile[RX_MAXENVFNAME];	/* name of the environment file */
long	Rx_errno;			/* rexec errno */
int	Rx_cserrno;			/* cs errno for rexec's call */


/*
 * global private variables
 *
 * These variables must be protected by global locks.
 * The Connp pointer is used to access a dynamically-allocated array
 * of pointers to rx_conn structures; the statically-allocated array 
 * Connp[RX_MAXRXCONN] has been retired;
 */

static	int	nextnull; /* Index of next NULL pointer to struct rx_conn */
static	struct	rx_conn {
	int	state;			/* active connection flag */
	int	netfd;			/* network file descriptor */
	long	flags;			/* rexec options */
	int	credit;			/* write msg credit */
	int	flush;		/* flush output flag/outstanding sig count */
	long	msg_type;	/* message type being read, 0 -> header */
	long	bytes_expected;		/* total number of bytes in message */
	long	bytes_received;		/* number of bytes received */
	int	(*ioctl_hand)(int, int, ...);	/* ioctl handler routine */
	int	(*write_hand)(int, const void *, size_t);/* write handler routine */
	struct rx_msg_head	rx_msg_head;	/* msg header */
	char	msg[RX_MAXMSGSZ];	/* holds a generic rexec message */
	char	*ta_buf;		/* pointer to user-supplied ta buffer */
	int	cnum;			/* index of pointer to this struct */
	struct rx_conn		*next;	/* pointer to next free rx_conn */
#ifdef _REENTRANT
        MUTEX_T lock;        		/* lock for this structure */
#endif /* _REENTRANT */
}	**Connp,	/* Pointer to array of pointers to rx_conn structures */
	*freep; 	/* Pointer to list of free rx_conn structures */
static	int	Dflag;	/* Indicates whether the RXF_DEBUG flag is set */


/*
 *	rexecve() is the program level access to rexec remote execution.
 *
 *	This routine sends the opening messages (OPEN_REQ, OPEN_ARGS,
 *	OPEN_ENVF, and OPEN_ENV, followed by OPEN_DONE)  to the rexec server.
 *
 *	Upon success, a connection handle is allocated and returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an
 *	appropriate error number.
 */

int
rexecve
(
	char	*machine,	/* destination host name */
	char	*rx_service,	/* requested rx service */
	char	*argv[],	/* service arguments in array of ptrs format */
	char	*envp[],	/* user environment in array of ptrs format */
	long	flags 		/* rexec options */
)
{
	int	cnum;		/* connection number */
	int	netfd;		/* network fd */
	char	*argstr;	/* service argument string */
	char	*envstr;	/* environment string */
	char	fullenvstr[RX_MAXENVSZ]; /* full env string */
	struct	termios	tp;	/* termios structure to be sent over */
	int	cserrno;	/* local copy of cserrno */


	/* need to set Dflag before the first Printf* because
	 * Printf? uses _rx_get_Dflag for REENTRANT code
	 */
	if (flags & RXF_DEBUG)
		(void) _rx_set_Dflag(1);
	else
		(void) _rx_set_Dflag(0);

	Printf0("rexecve: starting\n");

	(void) set_Rx_errno(RXE_OK);

	/* check flags */

	if (flags & ~RXF_ALL) {
		(void) set_Rx_errno(RXE_BADFLAGS);
		return(-1);
	}

	/* process arguments */

	if (argv == (char **) NULL) {
		(void) set_Rx_errno(RXE_BADARGS);
		return(-1);
	}

	/* convert into one string */

	argstr = argvtostr(argv);

	/* process environment */

	if (envp == (char **) NULL) {
		(void) set_Rx_errno(RXE_BADENV);
		return(-1);
	}

	/* convert into one string */

	envstr = argvtostr(envp);

	/* 
	 * We maintain a free list of rx_conn structures that have been
	 * allocated but are not in use (state == RXS_CLOSED).
	 * We also maintain an array of pointers to rx_conn structures
	 * that have been allocated.	
	 *
	 * First, we lock and check the free list.
	 * If we find a free rx_conn, we lock it, remove it from the free
	 * list, and unlock the free list.
	 *
	 * If no rx_conn is free, then we try to allocate one.
	 * FIRST, we lock the connection list.
	 * SECOND, we unlock the free list.
	 * (Imagine a child going from one bar to the next on a jungle gym.)
	 * We may have to allocate the connection list if it doesn't exist.
	 * Next we allocate a struct rx_conn and save its address in the
	 * next NULL slot in the connection list.
	 */

	/* Several threads may be racing through this code! */
	MUTEX_LOCK(&_rx_free_lock);
	if (freep) { /* A free struct rx_conn exists */
		/*
		 * Set cnum to the connection number stored in the free rx_conn.
		 * Remove the rx_conn from the free list.
		 * Set the free list pointer in the rx_conn to NULL.
		 * Lock the rx_conn.
		 * Unlock the free list.
		 */
		cnum = freep->cnum;
		freep = freep->next;
		MUTEX_LOCK(&Connp[cnum]->lock);
		MUTEX_UNLOCK(&_rx_free_lock);
		Connp[cnum]->next = NULL;
	}
	else { /* No free struct rx_conn exists */
		/*
		 * Lock the connection list.
		 * Unlock the free list.
		 * Make sure the array of pointers has been allocated.
		 * Make sure the maximum number of connections is
		 * not exceeded.
		 * Allocate an rx_conn.
		 * Initialize the rx_conn.
		 * Lock the rx_conn.
		 * Unlock the connection list.
		 */
		RW_WRLOCK(&_rx_conn_lock);
		MUTEX_UNLOCK(&_rx_free_lock);
		if (!Connp) { /* No array of pointers has been allocated yet */
			/* Allocate the array of pointers */
			if ((Connp = (struct rx_conn **) calloc(RX_MAXRXCONN,
				                      sizeof(struct rx_conn *)))
			    == NULL) {
				RW_UNLOCK(&_rx_conn_lock);
				set_Rx_errno(RXE_NOSPACE); /* ??? right code? */
				return(-1);
			}
			/* Initialize the array of pointers */
			for (cnum=0; cnum<RX_MAXRXCONN; cnum++)
				Connp[cnum] = NULL;
		}
		/* Check that another connection is OK */
		if (nextnull == RX_MAXRXCONN) {
			/* No more connections are allowed */
			RW_UNLOCK(&_rx_conn_lock);
			(void)set_Rx_errno(RXE_2MANYRX);
			return(-1);
		}
		/* Allocate a struct rx_conn */
		cnum = nextnull;
		if ((Connp[cnum]
		     = (struct rx_conn *) calloc(1, sizeof(struct rx_conn)))
	    	    == NULL) {
			RW_UNLOCK(&_rx_conn_lock);
			/* ??? right code? */
			set_Rx_errno(RXE_NOSPACE);
			return(-1);
		}
		nextnull++;	/* incremented only after alloc. succeeds */

		/* Initialize lock in rx_conn */
		MUTEX_INIT(&Connp[cnum]->lock, USYNC_THREAD, NULL);

		/* Lock the rx_conn and unlock the connection list */
		MUTEX_LOCK(&Connp[cnum]->lock);
		RW_UNLOCK(&_rx_conn_lock);

		/* Initialize cnum, next pointer, and state in rx_conn */
		Connp[cnum]->cnum = cnum;
		Connp[cnum]->next = NULL;
		Connp[cnum]->state = RXS_CLOSED;
	}

	/*
	 * Now we have an assigned and locked connection.
	 */

	/* open a connection to rxserver */

	Printf0("rexecve: about to call cs_connect()\n");

	if ((netfd = cs_connect(machine, RX_SVCNAME, NULL, &cserrno)) < 0) {
		(void) _rx_free_conn(cnum);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		(void) set_Rx_cserrno(cserrno);
		(void) set_Rx_errno(RXE_CONNPROB);
		return(-1);
	}

	Printf1("rexecve: tli connection established, netfd = %d\n", netfd);

	/* push tirdwr */

	if (ioctl(netfd, I_PUSH, "tirdwr") < 0) {
		(void) _rx_free_conn(cnum);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		(void) set_Rx_errno(RXE_TIRDWR);
		(void) close(netfd);
		return(-1);
	}

	/* send OPEN_REQ */

	Printf0("rexecve: sending OPEN_REQ\n");

	if (rxcsendopenreq(netfd, RX_VERSION, rx_service, flags) < 0) {
		(void) _rx_free_conn(cnum);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		(void) set_Rx_errno(RXE_CONNPROB);
		(void) close(netfd);
		return(-1);
	}

	/* send OPEN_ARGS, if needed */

	if (strlen(argstr) > 0) {
		Printf0("rexecve: sending OPEN_ARGS\n");

		if (rxcsendopenargs(netfd, argstr, strlen(argstr) + 1) < 0) {
			(void) _rx_free_conn(cnum);
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			(void) set_Rx_errno(RXE_CONNPROB);
			(void) close(netfd);
			return(-1);
		}
	}

	/* send OPEN_ENVF, if set */

	if (Rxenvfile[0] != '\0') {

		Printf0("rexecve: sending OPEN_ENVF\n");

		if (rxcsendopenenvf(netfd, Rxenvfile) < 0) {
			(void) _rx_free_conn(cnum);
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			(void) set_Rx_errno(RXE_CONNPROB);
			(void) close(netfd);
			return(-1);
		}
	}

	/* send OPEN_ENV, if needed */

	if (strlen(envstr) > 0) {
		Printf0("rexecve: sending OPEN_ENV\n");

		if (rxcsendopenenv(netfd, envstr, strlen(envstr) + 1) < 0) {
			(void) _rx_free_conn(cnum);
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			(void) set_Rx_errno(RXE_CONNPROB);
			(void) close(netfd);
			return(-1);
		}
	}

	/* send TCSETS IOCTL */

	/* the termios structure should be a parameter to rexecve */

	if (ioctl(0, TCGETS, &tp) < 0)
		if (ioctl(1, TCGETS, &tp) < 0)
			if (ioctl(2, TCGETS, &tp) < 0) {

				/*
				 *	cannot get terminal parameters,
				 *	so use defaults.
				 */

				tp.c_iflag = IMAXBEL | IXANY  | IXON   |
					     ICRNL   | ISTRIP | IGNPAR |
					     BRKINT;
				tp.c_oflag = TABDLY  | ONLCR  | OPOST;
				tp.c_cflag = HUPCL   | PARENB | CREAD  |
					     CS7     | CBAUD;
				tp.c_lflag = IEXTEN  | ECHOK  | ECHO   |
					     ISIG;

				tp.c_cc[0]  = '\177';
				tp.c_cc[1]  =  '\34';
				tp.c_cc[2]  =  '\10';
				tp.c_cc[3]  = '\100';
				tp.c_cc[4]  =   '\4';
				tp.c_cc[5]  =   '\0';
				tp.c_cc[6]  =   '\0';
				tp.c_cc[7]  =   '\0';
				tp.c_cc[8]  =  '\21';
				tp.c_cc[9]  =  '\23';
				tp.c_cc[10] =  '\32';
				tp.c_cc[11] =   '\0';
				tp.c_cc[12] =  '\22';
				tp.c_cc[13] =  '\17';
				tp.c_cc[14] =  '\27';
				tp.c_cc[15] =  '\26';
				tp.c_cc[16] =   '\0';
				tp.c_cc[17] =   '\0';
				tp.c_cc[18] =   '\0';
			}

	if (rxcsendtcsetsioctl(netfd, &tp) < 0) {
		(void) _rx_free_conn(cnum);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		(void) set_Rx_errno(RXE_CONNPROB);
		(void) close(netfd);
		return(-1);
	}

	/* send OPEN_DONE */

	Printf0("rexecve: sending OPEN_DONE\n");

	if (rxcsendopendone(netfd) < 0) {
		(void) _rx_free_conn(cnum);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		(void) set_Rx_errno(RXE_CONNPROB);
		(void) close(netfd);
		return(-1);
	}

	/* add the new connection to a list of open connections */

	Connp[cnum]->state = RXS_OPENING;
	Connp[cnum]->netfd = netfd;
	Connp[cnum]->flags = flags;
	Connp[cnum]->credit = -1;/* invalid value, fill in later from OPEN_REPLY*/
	Connp[cnum]->flush = 0;	/* no signal msgs to server outstanding */
	Connp[cnum]->msg_type = 0; /* waiting for header */
	Connp[cnum]->bytes_received = 0; /* none of it received yet */
	(void) _rx_set_ioctl_hand(cnum, ioctl);
	(void) _rx_set_write_hand(cnum, write);

	MUTEX_UNLOCK(&Connp[cnum]->lock);
	return(cnum);
}


/*
 *	rx_set_ioctl_hand() sets the ioctl handling routine for ioctls
 *	issued by the remote service and sent over by the rxserver.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 */

int
rx_set_ioctl_hand
	(
	int	cnum,
	int	(*ioctl_hand)(int, int, ...)
	)
{
	int retval;
	RW_RDLOCK(&_rx_conn_lock);
	MUTEX_LOCK(&Connp[cnum]->lock);
	RW_UNLOCK(&_rx_conn_lock);
	retval = _rx_set_ioctl_hand(cnum, ioctl_hand);
	MUTEX_UNLOCK(&Connp[cnum]->lock);
	return (retval);
}

/*
 * The following internal function assumes that Connp[cnum]->lock is held.
 */

static int
_rx_set_ioctl_hand
(
	int	cnum,
	int	(*ioctl_hand)(int, int, ...)
)
{
	Printf1("rx_set_ioctl_hand: cnum = %d\n", cnum);

	if (Connp[cnum]->state == RXS_CLOSED) {
		(void) set_Rx_errno(RXE_BADCNUM);
		return(-1);
	}

	Connp[cnum]->ioctl_hand = ioctl_hand;

	return(0);
}


/*
 *	rx_set_write_hand() sets the write handling routine for writes
 *	issued by the remote service and sent over by the rxserver.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 */

int
rx_set_write_hand
(
	int	cnum,
	int	(*write_hand)(int, const void *, size_t)
)
{
	int retval;

	RW_RDLOCK(&_rx_conn_lock);
	MUTEX_LOCK(&Connp[cnum]->lock);
	RW_UNLOCK(&_rx_conn_lock);
	retval = _rx_set_write_hand(cnum, write_hand);
	MUTEX_UNLOCK(&Connp[cnum]->lock);
	return (retval);
}

/*
 * The following internal function assumes that Connp[cnum]->lock is held.
 */

static int
_rx_set_write_hand
(
	int	cnum,
	ssize_t	(*write_hand)(int, const void *, size_t)
)
{
	Printf1("rx_set_write_hand: cnum = %d\n", cnum);

	if (Connp[cnum]->state == RXS_CLOSED) {
		(void) set_Rx_errno(RXE_BADCNUM);
		return(-1);
	}

	Connp[cnum]->write_hand = write_hand;

	return(0);
}


/*
 *	rx_proc_msg() process an incoming message from rxserver.  This
 *	message may be one of:
 *		RXM_OPEN_REPLY	- connection status
 *		RXM_DATA	- data from the service
 *		RXM_WRITEACK	- credit for sending more data to service
 *		RXM_IOCTL	- ioctl message
 *		RXM_SIGNALACK	- signal acknowledgement
 *		RXM_CLOSE_REQ	- service death indication
 *		RXM_CLOSE_TA	- returned typeahead data
 *
 *	The routine will read in the message, decode it, and process it.
 *	The msg_type parameter is set to the message type (if any).
 *	The ret_code parameter is set to the length of data (for RXM_DATA),
 *	ioctl *	return code (for RXM_IOCTL), or service process return code
 *	(for RXM_CLOSE_REQ).
 *
 *	Since with some protocols (tcp), messages may be broken up into smaller
 *	pieces which are then delivered separately (but in the same order), the
 *	newly arrived network data may not be a complete message (or message
 *	header), so more data may need to be received before the message can be
 *	processed.
 *
 *	Upon success, for RXM_CLOSE_REQ messages, the number of typeahead
 *	characters available at the rexec server (which may be 0) to be sent
 *	back is returned.  For other types of messages, 0 is returned if
 *	the action caused by the message succeeded.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 *	In case rx_proc_msg() does not receive a complete message, 0 is returned,
 *	unless an error has also occurred.
 *
 */

int
rx_proc_msg
(
	int	cnum,		/* connection number */
	long	*msg_type,	/* message type */
	long	*ret_code	/* return code */
)
{
	long	type;	/* message type */
	int	need;	/* number of bytes needed for a complete message */
	int	got;	/* number of bytes received */

	Printf1("rx_proc_msg: cnum = %d\n", cnum);

	RW_RDLOCK(&_rx_conn_lock);
	MUTEX_LOCK(&Connp[cnum]->lock);
	RW_UNLOCK(&_rx_conn_lock);
	if (Connp[cnum]->state == RXS_CLOSED) {
		(void) set_Rx_errno(RXE_BADCNUM);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(-1);
	}

	/* default value for returned msg_type */

	*msg_type = RX_INCOMPLETE;

	/* what message were we reading? */

	if (Connp[cnum]->msg_type == 0) {

		/* reading header */

		need = sizeof(struct rx_msg_head) - Connp[cnum]->bytes_received;
		if ((got = read(Connp[cnum]->netfd,
				(char *) &Connp[cnum]->rx_msg_head +
					Connp[cnum]->bytes_received,
				need)) < 0) {
			Printf2("rx_proc_msg: read returned %d, errno = %d\n",
			       got, errno);
			(void) set_Rx_errno(RXE_CONNPROB);
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			return(-1);
		}

		Printf1("rx_proc_msg: read %d bytes\n", got);

		if (got == 0) {
			Printf0("rx_proc_msg: got 0 byte message, exiting\n");
			*msg_type = RX_EOF;
			if (_rx_free_conn(cnum) < 0)
				Printf0("rx_proc_msg: _rx_free_conn() failed\n");
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			return(0);
		}

		if (got < need) {
			Connp[cnum]->bytes_received += got;
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			return(0);
		}

		/* header is complete */

		Connp[cnum]->bytes_received = 0;
		Connp[cnum]->bytes_expected =
			ntohl((int) Connp[cnum]->rx_msg_head.msg_len);
		Connp[cnum]->msg_type =
			ntohl((int) Connp[cnum]->rx_msg_head.msg_type);

		Printf2("rx_proc_msg: got header, msgtype = %d, len = %d\n",
		       Connp[cnum]->msg_type, Connp[cnum]->bytes_expected);

		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(0);
	}

	/* reading message */

	need = Connp[cnum]->bytes_expected - Connp[cnum]->bytes_received;
	if ((got = read(Connp[cnum]->netfd,
			Connp[cnum]->msg + Connp[cnum]->bytes_received, need))
	    < 0) {
		Printf1("rx_proc_msg: read returned %d\n", got);
		(void) set_Rx_errno(RXE_CONNPROB);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(-1);
	}

	Printf1("rx_proc_msg: read %d bytes\n", got);

	if (got == 0) {
		Printf0("rx_proc_msg: got 0 byte message, exiting\n");
		if (_rx_free_conn(cnum) < 0)
			Printf0("rx_proc_msg: _rx_free_conn() failed\n");
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(0);
	}

	if (got < need) {
		Connp[cnum]->bytes_received += got;
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(0);
	}

	type = Connp[cnum]->msg_type;

	/* reset message reading status */

	Connp[cnum]->msg_type = 0;	/* message header is next */
	Connp[cnum]->bytes_received = 0;	/* no bytes received */

	/* process message */

	switch(type) {

	case RXM_OPEN_REPLY: {
		struct open_reply	*open_reply;
		long	version;
		long	credit;

		if (Connp[cnum]->state != RXS_OPENING) {
			(void) set_Rx_errno(RXE_BADSTATE);
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			return(-1);
		}

		*msg_type = RX_PROTOCOL;

		open_reply = (struct open_reply *) Connp[cnum]->msg;

		version = ntohl(open_reply->version);
		*ret_code = ntohl(open_reply->ret_code);
		credit = ntohl(open_reply->credit);

		Printf0("rx_proc_msg: RXM_OPEN_REPLY message, ");
		Printf2("version = %d, ret_code = %d, ", version, *ret_code);
		Printf1("credit = %d\n", credit);

		/* process open_reply message - check ret_code */

		if (*ret_code != RXE_OK) {
			Connp[cnum]->state = RXS_CLOSED;
			(void) set_Rx_errno(*ret_code);
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			return(-1);
		}

		/* if more than one version, may want to check version here */

		Connp[cnum]->state = RXS_OPEN;
		Connp[cnum]->credit = credit;

		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(0);
	}

	case RXM_DATA: {
		struct data_msg		*data_msg;
		int	fd;
		long	len;
		char	*buf;

		if ((Connp[cnum]->state != RXS_OPEN) &&
		    (Connp[cnum]->state != RXS_CLOSING)) {
			(void) set_Rx_errno(RXE_BADSTATE);
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			return(-1);
		}

		*msg_type = RX_DATA;

		if (Connp[cnum]->flush > 0) {
			Printf0("rx_proc_msg: DATA flushed, not written\n");
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			return(0);
		}

		data_msg = (struct data_msg *) Connp[cnum]->msg;

		fd = ntohl(data_msg->fd);
		len = ntohl(data_msg->len);
		buf = data_msg->buf;

		*ret_code = len;

		Printf2("rx_proc_msg: RXM_DATA message, fd = %d, len = %d\n",
			fd, len);

		if (((Connp[cnum]->write_hand) (fd, buf, len)) != len) {
			
			Printf0
			   ("rx_proc_msg: problems in using write handler\n");

			(void) set_Rx_errno(RXE_WRITE);
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			return(-1);
		}

		Printf0("rx_proc_msg: data written successfully\n");

		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(0);
	}

	case RXM_WRITEACK: {
		struct writeack_msg	*writeack_msg;
		long	credit;

		if (Connp[cnum]->state != RXS_OPEN) {
			(void) set_Rx_errno(RXE_BADSTATE);
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			return(-1);
		}

		*msg_type = RX_PROTOCOL;

		writeack_msg = (struct writeack_msg *) Connp[cnum]->msg;

		credit = ntohl(writeack_msg->credit);

		Printf1
		   ("rx_proc_msg: RXM_WRITEACK message, credit = %d\n", credit);

		Connp[cnum]->credit += credit;

		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(0);
	}

	case RXM_IOCTL: {
		struct ioctl_msg	*ioctl_msg;
		int	fd;
		int	ioc;
		int	arglen;
		struct termio	*tio;
		struct termios	*tios;
		int	tcsbrkarg;
		struct winsize	*win;

		if (Connp[cnum]->state != RXS_OPEN) {
			(void) set_Rx_errno(RXE_BADSTATE);
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			return(-1);
		}

		*msg_type = RX_IOCTL;

		ioctl_msg = (struct ioctl_msg *) Connp[cnum]->msg;

		fd = ntohl(ioctl_msg->fd);
		ioc = ntohl(ioctl_msg->ioc);
		arglen = ntohl(ioctl_msg->arglen);

		Printf1("rx_proc_msg: RXM_IOCTL, fd = %d, ", fd);
		Printf2("ioc = %d, arglen = %d\n", ioc, arglen);

		switch(ioc) {

		case TCSETS:
		case TCSETSW:
		case TCSETSF:
			tios = (struct termios *) ioctl_msg->arg;
			tios->c_iflag = ntohl(tios->c_iflag);
			tios->c_oflag = ntohl(tios->c_oflag);
			tios->c_cflag = ntohl(tios->c_cflag);
			tios->c_lflag = ntohl(tios->c_lflag);

			if (((Connp[cnum]->ioctl_hand) (fd, ioc, tios)) < 0) {
				Printf1("rx_proc_msg: problems with ioctl handler, errno = %d\n", errno);
				(void) set_Rx_errno(RXE_IOCTL);
				MUTEX_UNLOCK(&Connp[cnum]->lock);
				return(-1);
			}
			break;

		case TCSETA:
		case TCSETAW:
		case TCSETAF:
			tio = (struct termio *) ioctl_msg->arg;
			tio->c_iflag = ntohs(tio->c_iflag);
			tio->c_oflag = ntohs(tio->c_oflag);
			tio->c_cflag = ntohs(tio->c_cflag);
			tio->c_lflag = ntohs(tio->c_lflag);

			if (((Connp[cnum]->ioctl_hand) (fd, ioc, tio)) != 0) {
				Printf1("rx_proc_msg: problems with ioctl handler, errno = %d\n", errno);
				(void) set_Rx_errno(RXE_IOCTL);
				MUTEX_UNLOCK(&Connp[cnum]->lock);
				return(-1);
			}
			break;

		case TCSBRK:
			tcsbrkarg = ntohl(*((int *) ioctl_msg->arg));
			if (((Connp[cnum]->ioctl_hand)
				(fd, TCSBRK, tcsbrkarg)) != 0) {
				Printf1("rx_proc_msg: problems with ioctl handler, errno = %d\n", errno);
				(void) set_Rx_errno(RXE_IOCTL);
				MUTEX_UNLOCK(&Connp[cnum]->lock);
				return(-1);
			}
			break;


		case TIOCSWINSZ:
			win = (struct winsize *) ioctl_msg->arg;
			win->ws_row = ntohs(win->ws_row);
			win->ws_col = ntohs(win->ws_col);
			win->ws_xpixel = ntohs(win->ws_xpixel);
			win->ws_ypixel = ntohs(win->ws_ypixel);

			if (((Connp[cnum]->ioctl_hand) (fd, ioc, win)) != 0) {
				Printf1("rx_proc_msg: problems with ioctl handler, errno = %d\n", errno);
				(void) set_Rx_errno(RXE_IOCTL);
				MUTEX_UNLOCK(&Connp[cnum]->lock);
				return(-1);
			}
			break;

		default:
			break;
		}

		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(0);
	}

	case RXM_SIGNALACK: {
		struct signalack_msg	*signalack_msg;
		long	sig;

		if (Connp[cnum]->state != RXS_OPEN) {
			(void) set_Rx_errno(RXE_BADSTATE);
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			return(-1);
		}

		*msg_type = RX_PROTOCOL;

		signalack_msg = (struct signalack_msg *) Connp[cnum]->msg;

		sig = ntohl(signalack_msg->sig);

		Printf1("rx_proc_msg: RXM_SIGNALACK, for signal %d\n", sig);

		/* restart output if no more outstanding signals */

		Connp[cnum]->flush--;

		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(0);
	}

	case RXM_CLOSE_REQ: {
		struct close_req	*close_req;

		if (Connp[cnum]->state != RXS_OPEN) {
			(void) set_Rx_errno(RXE_BADSTATE);
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			return(-1);
		}

		*msg_type = RX_SERVICE_DEAD;

		Printf0("rx_proc_msg: RXM_CLOSE_REQ\n");

		Connp[cnum]->state = RXS_CLOSING;

		close_req = (struct close_req *) Connp[cnum]->msg;

		*ret_code = ntohl(close_req->ret_code);

		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(ntohl(close_req->tasize));
	}

	case RXM_CLOSE_TA: {
		struct close_ta		*close_ta;
		int	tasize;

		if (Connp[cnum]->state != RXS_CLOSING) {
			(void) set_Rx_errno(RXE_BADSTATE);
			MUTEX_UNLOCK(&Connp[cnum]->lock);
			return(-1);
		}

		*msg_type = RX_TYPEAHEAD;

		Printf0("rx_proc_msg: RXM_CLOSE_TA\n");

		close_ta = (struct close_ta *) Connp[cnum]->msg;
		tasize = ntohl(close_ta->tasize);

		memcpy(Connp[cnum]->ta_buf, close_ta->tabuf, tasize);

		Connp[cnum]->state = RXS_CLOSED;

		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(0);
	}

	default:
		Printf1("rx_proc_msg: UNEXPECTED message type = %d\n", type);

		(void) set_Rx_errno(RXE_PROTOCOL);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(-1);
	}
}


/*
 *	rx_signal() sends a signal to the remote process.  Only four
 *	signals are allowed: SIGHUP, SIGPIPE, SIGINT, and SIGQUIT.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 */

int
rx_signal
(
	int	cnum,		/* connection number */
	int	signum		/* signal number */
)
{
	Printf2("rx_signal: cnum = %d, signum = %d\n", cnum, signum);

	RW_RDLOCK(&_rx_conn_lock);
	MUTEX_LOCK(&Connp[cnum]->lock);
	RW_UNLOCK(&_rx_conn_lock);

	if (Connp[cnum]->state != RXS_OPEN) {
		(void) set_Rx_errno(RXE_BADCNUM);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(-1);
	}

	if ((signum != SIGHUP) &&
	    (signum != SIGINT) &&
	    (signum != SIGPIPE) &&
	    (signum != SIGQUIT)) {
		(void) set_Rx_errno(RXE_BADSIG);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(-1);
	}

	if (rxcsendsignal(Connp[cnum]->netfd, signum) < 0) {
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(-1);
	}

	Connp[cnum]->flush++;

	MUTEX_UNLOCK(&Connp[cnum]->lock);
	return(0);
}


/*
 *	rx_write() is used to send input data to the remote service.
 *	Since rexec is not allowed to block on writes to the service,
 *	the routine will check for possible block condition when
 *	writing to the network and later again when writing to the
 *	master pty by rxserver.  If the write cannot complete right
 *	away, the rx_write() will drop the request and fail.
 *
 *	Determining whether a write will block at the server is done
 *	by keeping track of "write credit".  As long as the client
 *	has a write credit of 1 or more RXM_DATA messages, the
 *	server is guaranteed to accept them and not block.  For each
 *	RXM_DATA message which the server successfully writes, it will
 *	send back an RXM_WRITEACK message "containing" additional credit.
 *	Since it is OK for the client to block on a write to stdout,
 *	this procedure is not used for data flowing in the reverse
 *	(server to client) direction.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 */

int
rx_write
(
	int	cnum,	/* connection number */
	char	*buf,	/* data buffer */
	long	len	/* length of data to be sent */
)
{
	Printf2("rx_write: cnum = %d, len = %d\n", cnum, len);

	RW_RDLOCK(&_rx_conn_lock);
	MUTEX_LOCK(&Connp[cnum]->lock);
	RW_UNLOCK(&_rx_conn_lock);

	if ((Connp[cnum]->state == RXS_OPENING) ||
	    (Connp[cnum]->credit == 0)) {
		(void) set_Rx_errno(RXE_AGAIN);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(-1);
	}

	if (Connp[cnum]->state != RXS_OPEN) {
		(void) set_Rx_errno(RXE_BADCNUM);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(-1);
	}

	if (rxcsenddata(Connp[cnum]->netfd, 0, len, buf) < 0) {
		(void) set_Rx_errno(RXE_CONNPROB);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(-1);
	}

	Connp[cnum]->credit--;

	Printf0("rx_write: succeeded\n");

	MUTEX_UNLOCK(&Connp[cnum]->lock);
	return(0);
}


/*
 *	rx_ack_exit() is used by rexec to ACK the termination of the
 *	remote service and request the return of unused typeahead.
 *	The use of this routine is optional.  If the connection is
 *	dropped before calling this routine, rxserver assumes that
 *	no typeahead need be returned and exits.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 */

int
rx_ack_exit
(
	int	cnum,		/* connection number */
	char	*ta_buf,	/* buffer which will hold returned typeahead */
	long	ta_len		/* typeahead buffer length */
)
{
	Printf1("rx_ack_exit: cnum = %d\n", cnum);

	RW_RDLOCK(&_rx_conn_lock);
	MUTEX_LOCK(&Connp[cnum]->lock);
	RW_UNLOCK(&_rx_conn_lock);

	if (Connp[cnum]->state != RXS_CLOSING) {
		(void) set_Rx_errno(RXE_BADCNUM);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(-1);
	}

	if (rxcsendclosereply(Connp[cnum]->netfd, ta_len) < 0) {
		(void) set_Rx_errno(RXE_CONNPROB);
		MUTEX_UNLOCK(&Connp[cnum]->lock);
		return(-1);
	}

	Connp[cnum]->ta_buf = ta_buf;

	MUTEX_UNLOCK(&Connp[cnum]->lock);
	return(0);
}


/*
 *	rx_free_conn() releases the client state associated with an
 *	rxserver connection.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 */

int
rx_free_conn
(
	int	cnum		/* connection number */
)
{
	int retval;

	if (cnum < 0 ||
	    cnum >= nextnull ||
	    Connp[cnum]->state == RXS_CLOSED) {
		(void) set_Rx_errno(RXE_BADCNUM);
		return(-1);
	}
	RW_RDLOCK(&_rx_conn_lock);
	MUTEX_LOCK(&Connp[cnum]->lock);
	RW_UNLOCK(&_rx_conn_lock);
	retval = _rx_free_conn(cnum);
	MUTEX_UNLOCK(&Connp[cnum]->lock);
	return (retval);
}

/*
 * The following internal function assumes that Connp[cnum]->lock is held.
 */

static int
_rx_free_conn
(
	int	cnum		/* connection number */
)
{
	Printf1("_rx_free_conn: cnum = %d\n", cnum);

	if (cnum < 0 || cnum >= nextnull) {
		(void) set_Rx_errno(RXE_BADCNUM);
		return(-1);
	}

	Connp[cnum]->state = RXS_CLOSED;

	(void) close(Connp[cnum]->netfd);

	/* 
	 * Put rx_conn onto the free list
	 *
	 * We need not release the lock on the rx_conn before requesting
	 * a lock on the free list, since this rx_conn is not on the
	 * current free list.  A thread queued for a lock on the free list
	 * will either try to lock another rx_conn or it will find the
	 * free list empty and try to lock the connection list.
	 * If this other thread locks the connection list, it will try
	 * to allocate and then lock a completely new structure.
	 *
	 * Instead, we simply hold the lock on the rx_conn and lock the free
	 * list before adding the rx_conn to the free list.
	 */
	MUTEX_LOCK(&_rx_free_lock);
	Connp[cnum]->next = freep;
	freep = Connp[cnum];
	MUTEX_UNLOCK(&_rx_free_lock);

	return(0);
}


/*
 *	rx_fd() returns the file descriptor associated with an rexec
 *	connection.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 *	Thread-safe-ing note: this function is read-only and its use
 *	does not require that the array pointed to by Connp be locked.
 */

int
rx_fd
(
	int	cnum		/* connection number */
)
{

	if (Connp[cnum]->state == RXS_CLOSED) {
		(void) set_Rx_errno(RXE_BADCNUM);
		return(-1);
	}

	return(Connp[cnum]->netfd);
}

/*
 *	get_Rx_errno()
 *
 */

long
get_Rx_errno()
{
#ifdef _REENTRANT
	struct _rx_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) return (Rx_errno);

	key_tbl = (struct _rx_tsd *)
		  _mt_get_thr_specific_storage(_rx_key, RX_KEYTBL_SIZE);
	if (key_tbl != NULL && key_tbl->errno_p != NULL)
		return(*(long *)key_tbl->errno_p);
	return (RXE_NOERRMEM);
#else
	return (Rx_errno);
#endif /* _REENTRANT */
}

/*
 *	set_Rx_errno()
 *
 */

int
set_Rx_errno
(
	long errcode
)
{
#ifdef _REENTRANT
	struct _rx_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) {
		Rx_errno = errcode;
		return 0;
	}
	key_tbl = (struct _rx_tsd *)
		  _mt_get_thr_specific_storage(_rx_key, RX_KEYTBL_SIZE);
	if (key_tbl == NULL)
		return -1;
	if (key_tbl->errno_p == NULL) 
		key_tbl->errno_p = (void *)calloc(1, sizeof(long));
	if (key_tbl->errno_p == NULL)
		return -1;
	*(long *)key_tbl->errno_p = errcode;
#else
	Rx_errno = errcode;
#endif /* _REENTRANT */
	return 0;
}

/*
 *	_rx_errno()
 *
 */

const long *
_rx_errno()
{
#ifdef _REENTRANT
	struct _rx_tsd *key_tbl;
	const static long _Rx_errno = RXE_NOERRMEM;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD)
		return (&Rx_errno);

	key_tbl = (struct _rx_tsd *)
		  _mt_get_thr_specific_storage(_rx_key, RX_KEYTBL_SIZE);
	if (key_tbl != NULL && key_tbl->errno_p != NULL)
		return((long *)key_tbl->errno_p);
	return (&_Rx_errno);
#else
	return (&Rx_errno);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_rx_errno(void *p)
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

/*
 *	get_Rx_cserrno()
 *
 */

int
get_Rx_cserrno()
{
#ifdef _REENTRANT
	struct _rx_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD)
		return (Rx_cserrno);

	key_tbl = (struct _rx_tsd *)
		  _mt_get_thr_specific_storage(_rx_key, RX_KEYTBL_SIZE);
	if (key_tbl != NULL && key_tbl->cserrno_p != NULL)
		return(*(int *)key_tbl->cserrno_p);
	return (-1);
#else
	return (Rx_cserrno);
#endif /* _REENTRANT */
}

/*
 *	set_Rx_cserrno()
 *
 */

int
set_Rx_cserrno(int errcode)
{
#ifdef _REENTRANT
	struct _rx_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) {
		Rx_cserrno = errcode;
		return 0;
	}
	key_tbl = (struct _rx_tsd *)
		  _mt_get_thr_specific_storage(_rx_key, RX_KEYTBL_SIZE);
	if (key_tbl == NULL)
		return -1;
	if (key_tbl->cserrno_p == NULL) 
		key_tbl->cserrno_p = (void *)calloc(1, sizeof(int));
	if (key_tbl->cserrno_p == NULL) return -1;
	*(int *)key_tbl->cserrno_p = errcode;
#else
	Rx_cserrno = errcode;
#endif /* _REENTRANT */
	return 0;
}

/*
 *	_rx_cserrno()
 *
 */

const int *
_rx_cserrno()
{
#ifdef _REENTRANT
	struct _rx_tsd *key_tbl;
	const static int _Rx_cserrno = CS_NOERRMEM;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD)
		return (&Rx_cserrno);

	key_tbl = (struct _rx_tsd *)
		  _mt_get_thr_specific_storage(_rx_key, RX_KEYTBL_SIZE);
	if (key_tbl != NULL && key_tbl->cserrno_p != NULL)
		return((int *)key_tbl->cserrno_p);
	return (&_Rx_cserrno);
#else
	return (&Rx_cserrno);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_rx_cserrno(void *p)
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

/*
 *	_rx_set_Dflag()
 *
 */

static int
_rx_set_Dflag(int flag)
{
#ifdef _REENTRANT
	struct _rx_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) {
		Dflag = flag;
		return 0;
	}
	key_tbl = (struct _rx_tsd *)
		  _mt_get_thr_specific_storage(_rx_key, RX_KEYTBL_SIZE);
	if (key_tbl == NULL)
		return -1;
	if (key_tbl->dflag_p == NULL) 
		key_tbl->dflag_p = (void *)calloc(1, sizeof(int));
	if (key_tbl->dflag_p == NULL)
		return -1;
	*(int *)key_tbl->dflag_p = flag;
#else
	Dflag = flag;
#endif /* _REENTRANT */
	return 0;
}

/*
 *	_rx_get_Dflag()
 *
 */

static int
_rx_get_Dflag()
{
#ifdef _REENTRANT
	struct _rx_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD)
		return (Dflag);

	key_tbl = (struct _rx_tsd *)
		  _mt_get_thr_specific_storage(_rx_key, RX_KEYTBL_SIZE);
	if (key_tbl == NULL)
		return (-1);
	/*
	 * if not allocated: allocate now per-thread Dflag and set
	 * it to global Dflag.  This is for the case where _rx_get_Dflag
	 * is called before _rx_set_Dflag, in oder to return a value
	 * that makes sense (and not a -1).
	 */
	if (key_tbl->dflag_p == NULL) {
		key_tbl->dflag_p = (void *)calloc(1, sizeof(int));
		if (key_tbl->dflag_p == NULL)
			return (-1);
		*(int *)key_tbl->dflag_p = Dflag;
	}

	return(*(int *)key_tbl->dflag_p);
#else
	return (Dflag);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_rx_dflag(void *p)
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */


/*
 *	rxcsendopenreq()
 *
 */

static int
rxcsendopenreq
(
	int	netfd,			/* network file descriptor */
	long	version,		/* rexec version number */
	char	service[RX_MAXSVCSZ],	/* remote service name */
	long	flags			/* rexec options */
)
{
	struct open_req open_req;

	open_req.version = htonl(version);
	(void) strncpy(open_req.service, service, RX_MAXSVCSZ);
	open_req.flags = htonl(flags);

	if (rxputm(netfd, RXM_OPEN_REQ, sizeof(struct open_req),
		   (char *) &open_req) < 0) {
		Printf0("rxcsendopenreq: rxputm(RXM_OPEN_REQ) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsendopenargs()
 *
 */

static int
rxcsendopenargs
(
	int	netfd,			/* network file descriptor */
	char	*argstr,		/* argument string */
	int	len			/* string length */
)
{
	struct open_args open_args;

	if (len > RX_MAXARGSZ)
		len = RX_MAXARGSZ;

	(void) strncpy(open_args.argstr, argstr, len);

	if (rxputm(netfd, RXM_OPEN_ARGS, RX_OPEN_ARGS_SZ(len),
		   (char *) &open_args) < 0) {
		Printf0("rxcsendopenargs: rxputm(RXM_OPEN_ARGS) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsendopenenvf()
 *
 */

static int
rxcsendopenenvf
(
	int	netfd,			/* network file descriptor */
	char	*rxenvfile
)
{
	struct open_envf	open_envf;

	memcpy(open_envf.envfile, rxenvfile, RX_MAXENVFNAME);

	if (rxputm(netfd, RXM_OPEN_ENVF, sizeof(struct open_envf),
		   (char *) &open_envf) < 0) {
		Printf0("rxcsendopenenvf: rxputm(RXM_OPEN_ENVF) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsendopenenv()
 *
 */

static int
rxcsendopenenv
(
	int	netfd,			/* network file descriptor */
	char	*envstr,		/* environment string */
	int	len			/* string length */
)
{
	struct open_env open_env;

	if (len > RX_MAXENVSZ)
		len = RX_MAXENVSZ;

	(void) strncpy(open_env.envstr, envstr, len);

	if (rxputm(netfd, RXM_OPEN_ENV, RX_OPEN_ENV_SZ(len),
		   (char *) &open_env) < 0) {
		Printf0("rxcsendopenenv: rxputm(RXM_OPEN_ENV) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsendtcsetsioctl()
 *
 */

static int
rxcsendtcsetsioctl
(
	int	netfd,			/* network file descriptor */
	struct	termios	*tp		/* termios structure pointer */
)
{
	struct ioctl_msg	ioctl_msg;
	struct termios	*ntp = (struct termios *) ioctl_msg.arg;

	ioctl_msg.fd = htonl(0);
	ioctl_msg.ioc = htonl(TCSETS);
	ioctl_msg.arglen = htonl(sizeof(struct termios));
	ntp->c_iflag = htonl(tp->c_iflag);
	ntp->c_oflag = htonl(tp->c_oflag);
	ntp->c_cflag = htonl(tp->c_cflag);
	ntp->c_lflag = htonl(tp->c_lflag);
	(void) memcpy(ntp->c_cc, tp->c_cc, NCCS);

	if (rxputm(netfd, RXM_IOCTL, RX_IOCTL_MSG_SZ(sizeof(struct termios)),
		   (char *) &ioctl_msg) < 0) {
		Printf0("rxcsendtcsetsioctl: rxputm(RXM_IOCTL) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsendopendone()
 *
 */

static int
rxcsendopendone
(
	int	netfd			/* network file descriptor */
)
{
	if (rxputm(netfd, RXM_OPEN_DONE, 0, NULL) < 0) {
		Printf0("rxcsendopendone: rxputm(RXM_OPEN_DONE) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsendclosereply()
 *
 */

static int
rxcsendclosereply
(
	int	netfd,			/* network file descriptor */
	long	tasize			/* amount of typeahead to return */
)
{
	struct close_reply close_reply;

	close_reply.tasize = htonl(tasize);

	if (rxputm(netfd, RXM_CLOSE_REPLY, sizeof(struct close_reply),
		   (char *) &close_reply) < 0) {
		Printf0("rxcsendclosereply: rxputm(RXM_CLOSE_REPLY) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsenddata()
 *
 */

static int
rxcsenddata
(
	int	netfd,			/* network file descriptor */
	long	fd,			/* data's src fd */
	long	len,			/* data length */
	char	*buf			/* data buffer */
)
{
	struct data_msg data_msg;

	data_msg.fd = htonl(fd);
	data_msg.len = htonl(len);
	(void) memcpy(data_msg.buf, buf, len);

	if (rxputm(netfd, RXM_DATA, RX_DATA_MSG_SZ(len), (char *) &data_msg) < 0) {
		Printf0("rxcsenddata: rxputm(RXM_DATA) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsendsignal()
 *
 */

static int
rxcsendsignal
(
	int	netfd,			/* network file descriptor */
	int	signum			/* signal number */
)
{
	struct signal_msg signal_msg;

	signal_msg.sig = htonl(signum);

	if (rxputm(netfd, RXM_SIGNAL, sizeof(struct signal_msg),
		   (char *) &signal_msg) < 0) {
		Printf0("rxcsendsignal: rxputm(RXM_SIGNAL) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxputm() is an internal routine used to send rexec messages
 *	to rxserver.
 *	It assumes that netfd is an open file descriptor to rxserver.
 *	If the message cannot be written out to the network immediately,
 *	the routine fails and sets Rx_errno to RXE_WOULDBLOCK.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 */

static int
rxputm
(
	int	netfd,
	long	type,
	long	len,
	char	*msg
)
{
	struct rx_msg_head	head;	/* rexec message header */
	int	wr;			/* write return value */

	Printf0("rxputm: writing header\n");

	head.msg_type = htonl(type);
	head.msg_len = htonl(len);

	/* write the header */
	wr = write(netfd, (char *) &head, sizeof(struct rx_msg_head));

	if (wr < 0) {

		Printf2("rxputm: write returned %d, errno = %d\n", wr, errno);

		return(-1);
	}

	if (wr != sizeof(struct rx_msg_head)) {

		Printf2("rxputm: did not complete: wrote %d out of %d\n",
			wr, sizeof(struct rx_msg_head));

		return(-1);
	}

	/* write the message, if any */

	if (len == 0) {

		Printf0("rxputm: 0-length message, nothing sent\n");

		return(0);
	}

	Printf0("rxputm: writing message\n");

	wr = write(netfd, msg, (unsigned int) len);

	if (wr < 0) {

		Printf2("rxputm: write returned %d, errno = %d\n", wr, errno);

		return(-1);
	}

	if (wr != len) {

		Printf2("rxputm: did not complete: wrote %d out of %d\n",
		       wr, len);

		return(-1);
	}

	Printf0("rxputm: success\n");

	return(0);
}
