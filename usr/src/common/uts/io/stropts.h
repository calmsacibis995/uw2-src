/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_STROPTS_H	/* wrapper symbol for kernel use */
#define _IO_STROPTS_H	/* subject to change without notice */
#define _SYS_STROPTS_H	/* SVR4.0COMPAT */

#ident	"@(#)kern:io/stropts.h	1.15"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <io/conf.h>	/* REQUIRED */
#include <svc/secsys.h>	/* REQUIRED */
#include <io/stropts_f.h> /* SVR4.0COMPAT */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/conf.h>	/* REQUIRED */
#include <sys/secsys.h>	/* REQUIRED */
#include <sys/stropts_f.h> /* SVR4.0COMPAT */

#else

#include <sys/types.h>	/* SVR4.0COMPAT */
/* For FMNAMESZ define */
#include <sys/conf.h> 	/* SVR4.0COMPAT */
/* For struct sub_attr */
#include <sys/secsys.h>	/* SVR4.0COMPAT */
#include <sys/stropts_f.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * Write options.
 */
#define SNDZERO		0x001		/* send a zero length message */
#define SNDPIPE		0x002		/* send SIGPIPE on write and */
					/* putmsg if sd_werror is set */

/*
 * Read options
 */

#define RNORM		0x000		/* read msg norm */
#define RMSGD		0x001		/* read msg discard */
#define RMSGN		0x002		/* read msg no discard */

#define RMODEMASK	0x003		/* all above bits */

/*
 * These next three read options are added for the sake of
 * user-level transparency.  RPROTDAT will cause the stream head
 * to treat the contents of M_PROTO and M_PCPROTO message blocks
 * as data.  RPROTDIS will prevent the stream head from failing
 * a read with EBADMSG if an M_PROTO or M_PCPROTO message is on
 * the front of the stream head read queue.  Rather, the protocol
 * blocks will be silently discarded and the data associated with
 * the message (in linked M_DATA blocks), if any, will be delivered
 * to the user.  RPROTNORM sets the default behavior, where read
 * will fail with EBADMSG if an M_PROTO or M_PCPROTO are at the
 * stream head.
 */
#define RPROTDAT	0x004		/* read protocol messages as data */
#define RPROTDIS	0x008		/* discard protocol messages, but */
					/* read data portion */
#define RPROTNORM	0x010

#define RPROTMASK	0x01c		/* all RPROT bits */

/*
 * Flush options
 */

#define FLUSHR		0x01		/* flush read queue */
#define FLUSHW		0x02		/* flush write queue */
#define FLUSHRW		0x03		/* flush both queues */
#define FLUSHBAND	0x04		/* flush only band specified */
					/* in next byte */

/*
 * Events for which to be sent SIGPOLL signal and for which events
 * can be posted by the I_SETEV ioctl.
 */
#define S_INPUT		0x0001		/* any msg but hipri on read Q */
#define S_HIPRI		0x0002		/* high priority msg on read Q */
#define S_OUTPUT	0x0004		/* write Q no longer full */
#define S_MSG		0x0008		/* signal msg at front of read Q */
#define	S_ERROR		0x0010		/* error msg arrived at stream head */
#define	S_HANGUP	0x0020		/* hangup msg arrived at stream head */
#define S_RDNORM	0x0040		/* normal msg on read Q */
#define S_WRNORM	S_OUTPUT
#define	S_RDBAND	0x0080		/* out of band msg on read Q */
#define S_WRBAND	0x0100		/* can write out of band */
#define S_BANDURG	0x0200		/* modifier to S_RDBAND, to generate */
					/* SIGURG instead of SIGPOLL */

/*
 * Flags for getmsg() and putmsg() syscall arguments.
 * "RS" stands for recv/send.  The system calls were originally called
 * recv() and send(), but were renamed to avoid confusion with the BSD
 * calls of the same name.  A value of zero will cause getmsg() to return
 * the first message on the stream head read queue and putmsg() to send
 * a normal priority message.
 */
#define RS_HIPRI	0x01		/* send/recv high priority message */

/*
 * Flags for getpmsg() and putpmsg() syscall arguments.
 */

/*
 * These are settable by the user and will be set on return
 * to indicate the priority of message received.
 */
#define MSG_HIPRI	0x01		/* send/recv high priority message */
#define MSG_ANY		0x02		/* recv any messages */
#define MSG_BAND	0x04		/* recv messages from specified band */

/*
 * Flags returned as value of getmsg() and getpmsg() syscall.
 */
#define MORECTL		1		/* more ctl info is left in message */
#define MOREDATA	2		/* more data is left in message */

/*
 * Define to indicate that all multiplexors beneath a stream should
 * be unlinked.
 */
#define MUXID_ALL	(-1)

/*
 * Flag definitions for the I_ATMARK ioctl.
 */
#define ANYMARK		0x01
#define LASTMARK	0x02

/*
 *  Stream Ioctl defines
 */
#define	STR		('S'<<8)
/* (STR|000) in use */
#define I_NREAD		(STR|01)
#define I_PUSH		(STR|02)
#define I_POP		(STR|03)
#define I_LOOK		(STR|04)
#define I_FLUSH		(STR|05)
#define I_SRDOPT	(STR|06)
#define I_GRDOPT	(STR|07)
#define I_STR		(STR|010)
#define I_SETSIG	(STR|011)
#define I_GETSIG	(STR|012)
#define I_FIND		(STR|013)
#define I_LINK		(STR|014)
#define I_UNLINK	(STR|015)
/* (STR|016) in use */
#define I_PEEK		(STR|017)
#define I_FDINSERT	(STR|020)
#define I_SENDFD	(STR|021)
#if defined(_KERNEL)

#define I_RECVFD	(STR|022)
#define I_E_RECVFD	(STR|016)

#elif !defined(_STYPES)	/* user level definition */

#define I_RECVFD	(STR|016)	/* maps to kernel I_E_RECVFD */

#else

#define I_RECVFD	(STR|022)	/* non-EFT definition */

#endif /* defined(_KERNEL) */

#define I_SWROPT	(STR|023)
#define I_GWROPT	(STR|024)
#define I_LIST		(STR|025)
#define I_PLINK		(STR|026)
#define I_PUNLINK	(STR|027)
#define I_FLUSHBAND	(STR|034)
#define I_CKBAND	(STR|035)
#define I_GETBAND	(STR|036)
#define I_ATMARK	(STR|037)
#define I_SETCLTIME	(STR|040)
#define I_GETCLTIME	(STR|041)
#define I_CANPUT	(STR|042)
#define	I_S_RECVFD	(STR|043)
/*#define STRPERF	1 */
#ifdef STRPERF
#define I_STATS		(STR|044)
#endif
/*
 * Warning: this ioctl should only be used if the application is
 * willing to sacrifice normal pipe semantics on write sizes
 */
#define I_BIGPIPE	(STR|045)

/*
 * User level ioctl format for ioctls that go downstream (I_STR)
 */
struct strioctl {
	int 	ic_cmd;			/* command */
	int	ic_timout;		/* timeout value */
	int	ic_len;			/* length of data */
	char	*ic_dp;			/* pointer to data */
};

/*
 * Value for timeouts (ioctl, select) that denotes infinity
 */
#define INFTIM		-1

/*
 * Stream buffer structure for putmsg and getmsg system calls
 */
struct strbuf {
	int	maxlen;			/* no. of bytes in buffer */
	int	len;			/* no. of bytes returned */
	char	*buf;			/* pointer to data */
};

/* 
 * Stream I_PEEK ioctl format
 */
struct strpeek {
	struct strbuf ctlbuf;
	struct strbuf databuf;
	long	      flags;
};

/*
 * Stream I_FDINSERT ioctl format
 */
struct strfdinsert {
	struct strbuf ctlbuf;
	struct strbuf databuf;
	long	      flags;
	int	      fildes;
	int	      offset;
};

/*
 * Receive file descriptor structure
 */

#if defined(_KERNEL)

struct o_strrecvfd {	/* SVR3 syscall structure */
	union {
		struct file *fp;
		int fd;
	} f;
	o_uid_t uid;		/* always ushort */
	o_gid_t gid;
	char fill[8];
};

/* Although EFT is enabled in the kernel we kept the following definition
** to support an EFT application on a 4.0 non-EFT system.
*/

struct e_strrecvfd {	/* SVR4 expanded syscall interface structure */
	union {
		struct file *fp;
		int fd;
	} f;
	uid_t uid;		/* always long */
	gid_t gid;
	char fill[8];
};

struct strrecvfd {	/* Kernel structure dependent on EFT definition */
	union {
		struct file *fp;
		int fd;
	} f;
	uid_t uid;
	gid_t gid;
	char fill[8];
};

/* Information that needs to be audited for Receive File Descriptor */
struct adtrecvfd {
	pid_t	adtrfd_sendpid;		/* Sender's process id */  
	lwpid_t	adtrfd_sendlwpid;	/* Sender's lwp id */
};
/* Auditing version of the Receive File Descriptor structure */
struct adt_strrecvfd {
	struct	strrecvfd	adt_strrecvfd;
	struct	adtrecvfd	adt_adtrecvfd;
};

#elif !defined(_STYPES) 	/* EFT user definition */

struct strrecvfd {
	int fd;
	uid_t uid;
	gid_t gid;
	char fill[8];
};

#else

/*
 * User compatibility mode. EOVERFLOW returned 
 * when uid/gid exceeds ushort limit.
 */
struct strrecvfd {
	int fd;
	o_uid_t uid;
	o_gid_t gid;
	char fill[8];
};

#endif	/* defined(_KERNEL) */

struct s_strrecvfd {
	int fd;
	uid_t uid;
	gid_t gid;
	struct sub_attr s_attrs;
};

/*
 * For I_LIST ioctl.
 */
struct str_mlist {
	char l_name[FMNAMESZ+1];
};

struct str_list {
	int sl_nmods;
	struct str_mlist *sl_modlist;
};

/*
 * For I_FLUSHBAND ioctl.  Describes the priority
 * band for which the operation applies.
 */
struct bandinfo {
	unsigned char	bi_pri;
	int		bi_flag;
};

#if !defined(_KERNEL)
#if defined(__STDC__)
int isastream(int);
#else
int isastream();
#endif
#endif /* !defined(_KERNEL) */

#if defined(__cplusplus)
        }
#endif
#endif	/* _IO_STROPTS_H */
