/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head.usr:rx.h	1.1.6.5"
#ident  "$Header: $"

#ifndef _RX_H
#define _RX_H

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * rexec client program interface
 *
 */

#if defined(__STDC__)

extern int rexecve(char *, char *, char **, char **, long);
extern int rx_proc_msg(int, long *, long *);
extern int rx_write(int, char *, long);
extern int rx_signal(int, int);
extern int rx_ack_exit(int, char *, long);
extern int rx_set_ioctl_hand(int, int (*) (int, int, ...));
extern int rx_set_write_hand(int, ssize_t (*) (int, const void *, size_t));
extern int rx_fd(int);
extern int rx_free_conn(int);
extern long get_Rx_errno(void);
extern int set_Rx_errno(long);
extern const long *_rx_errno();
extern int get_Rx_cserrno(void);
extern int set_Rx_cserrno(int);
extern const int *_rx_cserrno();

#else

extern int rexecve();
extern int rx_proc_msg();
extern int rx_write();
extern int rx_signal();
extern int rx_ack_exit();
extern int rx_set_ioctl_hand();
extern int rx_set_write_hand();
extern int rx_fd();
extern int rx_free_conn();
extern long get_Rx_errno();
extern int set_Rx_errno();
extern const long *_rx_errno();
extern int get_Rx_cserrno();
extern int set_Rx_cserrno();
extern const int *_rx_cserrno();

#endif

extern char	Rxenvfile[]; /* defined in library, but it's not set there */

#ifdef _REENTRANT

#define Rx_errno	(*_rx_errno())
#define Rx_cserrno	(*_rx_cserrno())

#else /* ! _REENTRANT */

extern long	Rx_errno;
extern int	Rx_cserrno;

#endif /* _REENTRANT */

/*
 * rexec flags
 *
 */

#define	RXF_SEPERR	0001	/* Separate stderr from stdout */
#define	RXF_STDINPIPE	0002	/* Standard input is redirected from a pipe/file */
#define	RXF_STDOUTTERM	0004	/* Standard output is going to a terminal */
#define	RXF_DEBUG	0100	/* enable debug mode on client side */

/*
 * all the flags together
 *
 */

#define	RXF_ALL		(RXF_SEPERR | RXF_STDINPIPE | RXF_STDOUTTERM | RXF_DEBUG)


/*
 * message type codes returned in msg_type parameter to rx_proc_msg()
 *
 */

#define	RX_INCOMPLETE	1	/* incomplete message */
#define	RX_PROTOCOL	2	/* protocol mesage (open, close, etc) */
#define	RX_SERVICE_DEAD	3	/* service termination message */
#define	RX_TYPEAHEAD	4	/* typeahead message */
#define	RX_DATA		5	/* data message */
#define	RX_IOCTL	6	/* ioctl message */
#define	RX_EOF		7	/* 0-length message */


/*
 * various rexec constants
 *
 */

#define	RX_SVCNAME	"listen:rexec"
#define	RX_LOGFILE	"/var/adm/log/rexec.log"
#define	RX_MODULEID	"rexec"

#define	RX_MAXRXCONN	5	/* maximum number of open rexec client connections */
#define	RX_MAXSVCLINE	1024	/* maximum service entry line size */
#define	RX_MAXSVCSZ	14	/* maximum service name size */
#define	RX_MAXSVCDESCR	256	/* maximum service description */
#define	RX_MAXSVCDEF	256	/* maximum service definition */
#define	RX_MAXUTMP	1	/* maximum utmp flag size */
#define	RX_MAXMSGSZ	5120	/* maximum rx message size */
#define	RX_MAXARGSZ	4096	/* maximum argument string size */
#define	RX_MAXENVSZ	4096	/* maximum environment string size */
#define	RX_MAXTASZ	1024	/* maximum typeahead buffer size */
#define	RX_MAXDATASZ	1024	/* maximum data buffer size */
#define	RX_MAXIOCARGSZ	1024	/* maximum ioctl argument buffer size */
#define	RX_MAXENVFNAME	256	/* maximum environment file name */
#define	RX_MAXARGS	64	/* maximum number of arguments to service */
#define	RX_MAXENVS	128	/* maximum number of environment variables */
#define	RX_WRITEWAIT	1	/* seconds to wait in case of RXE_AGAIN */


/*
 * rexec error numbers
 *
 */

#define	RXE_OK		0	/* no error */
#define	RXE_2MANYRX	1	/* too many open client rexec connections */
#define	RXE_BADFLAGS	2	/* bad options/flags specified */
#define	RXE_BADARGS	3	/* too many arguments */
#define	RXE_BADENV	4	/* bad environment specified */
#define	RXE_BADMACH	5	/* unknown host */
#define	RXE_CONNPROB	6	/* connection problem */
#define	RXE_NORXSERVER	7	/* host is not running rxserver */
#define	RXE_BADVERSION	8	/* unsupported version */
#define	RXE_NOSVCFILE	9	/* could not open services file */
#define	RXE_NOSVC	10	/* no such service */
#define	RXE_NOTAUTH	11	/* not authorized to execute service */
#define	RXE_NOPTS	12	/* no pseudo terminals available */
#define	RXE_PIPE	13	/* cannot make pipe for stderr */
#define	RXE_BADSTART	14	/* error in starting server side */
#define	RXE_NOSPACE	15	/* server side memory allocation problems */
#define	RXE_BADCNUM	16	/* bad rexec connection number */
#define	RXE_AGAIN	17	/* write would cause server to block, try later */
#define	RXE_BADSIG	18	/* bad signal number */
#define	RXE_BADSTATE	19	/* conn. is in wrong state to perform operation */
#define	RXE_TIRDWR	20	/* could not push module "tirdwr" at client */
#define	RXE_WRITE	21	/* write handler failure at client */
#define	RXE_IOCTL	22	/* ioctl handler failure at client */
#define	RXE_PROTOCOL	23	/* protocol failure - unexpected message */
#define	RXE_NOERRMEM	24	/* could not allocate memory for error code */
#define	RXE_UNKNOWN	99	/* unknown error code */

/*
 * WARNING:
 * The following definitions are used internally by libnsl and rexec.
 * Since they are not part of the interface, they may be changed
 * without notice in future releases.
 */

/* debugging macros */
#ifdef _REENTRANT
#define	Printf0(format)		if (_rx_get_Dflag()) { (void) printf(format); }
#define	Printf1(format,x)	if (_rx_get_Dflag()) { (void) printf(format,x); }
#define	Printf2(format,x,y)	if (_rx_get_Dflag()) { (void) printf(format,x,y); }
#define	Printf3(format,x,y,z)	if (_rx_get_Dflag()) { (void) printf(format,x,y,z); }

#else /* !_REENTRANT */

#define	Printf0(format)		if (Dflag) { (void) printf(format); }
#define	Printf1(format,x)	if (Dflag) { (void) printf(format,x); }
#define	Printf2(format,x,y)	if (Dflag) { (void) printf(format,x,y); }
#define	Printf3(format,x,y,z)	if (Dflag) { (void) printf(format,x,y,z); }

#endif /* _REENTRANT */

/* types of messages which flow between rexec client and server */

#define	RXM_OPEN_REQ	1	/* open request message */
#define	RXM_OPEN_ARGS	2	/* message containing service arguments */
#define	RXM_OPEN_ENVF	3	/* message containing environment file */
#define	RXM_OPEN_ENV	4	/* message containing user environment */
#define	RXM_OPEN_DONE	5	/* message indicating end of open messages */
#define	RXM_OPEN_REPLY	6	/* open reply message */
#define	RXM_CLOSE_REQ	7	/* close request message */
#define	RXM_CLOSE_REPLY	8	/* close reply message */
#define	RXM_CLOSE_TA	9	/* close typeahead message */
#define	RXM_DATA	10	/* message containing data */
#define	RXM_WRITEACK	11	/* write acknowledgement message */
#define	RXM_SIGNAL	12	/* message containing a signal */
#define	RXM_SIGNALACK	13	/* signal acknowledgement message */
#define	RXM_IOCTL	14	/* message containing an ioctl */


/* rexec message header */

struct rx_msg_head {
	long	msg_type;	/* type of rx message */
	long	msg_len;	/* length of rx message */
};


/* message structures */


/* service opening protocol messages */

/* RXM_OPEN_REQ */

struct open_req {
	long	version;		/* rexec client version */
	char	service[RX_MAXSVCSZ];	/* service to start */
	long	flags;			/* open options */
};

#define	RX_VERSION	1


/* RXM_OPEN_ARGS */

struct open_args {
	char	argstr[RX_MAXARGSZ];	/* argument string */
};

#define	RX_OPEN_ARGS_SZ(argv_sz) (argv_sz)


/* RXM_ENVF */

struct open_envf {
	char	envfile[RX_MAXENVFNAME];/* environment file name */
};


/* RXM_OPEN_ENV */

struct open_env {
	char	envstr[RX_MAXENVSZ];	/* environment string */
};

#define	RX_OPEN_ENV_SZ(envp_sz)	(envp_sz)


/* RXM_OPEN_REPLY */

struct open_reply {
	long	version;	/* rxserver version */
	long	ret_code;	/* return code for open operation */
	long	credit;		/* initial write credit */
};

/* server will only buffer 1 data message worth of data */

#define	RX_INITCREDIT	1


/* service closing protocol messages */

/* RXM_CLOSE_REQ */

struct close_req {
	long	ret_code;	/* dying process' return code */
	long	tasize;		/* amount of unused typeahead at server */
};


/* RXM_CLOSE_REPLY */

struct close_reply {
	long	tasize;		/* amount of typeahead to return */
};


/* RXM_CLOSE_TA */

struct close_ta {
	long	tasize;			/* returned typeahead size */
	char	tabuf[RX_MAXTASZ];	/* returned typeahead buffer */
};

#define	RX_CLOSE_TA_SZ(tabuf_sz) (sizeof(struct close_ta) - RX_MAXTASZ + tabuf_sz)


/* data and data acknowledgement messages */

/* RXM_DATA */

struct data_msg {
	long	fd;			/* orig / dest fd */
	long	len;			/* len of data */
	char	buf[RX_MAXDATASZ];	/* data */
};

#define	RX_DATA_MSG_SZ(buf_sz)	(sizeof(struct data_msg) - RX_MAXDATASZ + buf_sz)


/* RXM_WRITE_ACK */

struct writeack_msg {
	long	credit;	/* acknowledgement flag */
};


/* signal and signal acknowledgement messages */

/* RXM_SIGNAL */

struct signal_msg {
	long	sig;		/* signal number */
};


/* RXM_SIGNALACK */

struct signalack_msg {
	long	sig;		/* signal number */
};


/* ioctl message */

/* RXM_IOCTL */

struct ioctl_msg {
	long	fd;			/* destination fd */
	long	ioc;			/* ioctl number */
	long	arglen;			/* ioctl argument length */
	char	arg[RX_MAXIOCARGSZ];	/* ioctl argument buffer */
};

#define	RX_IOCTL_MSG_SZ(arg_sz)	(sizeof(struct ioctl_msg) - RX_MAXIOCARGSZ + arg_sz)


/* client/server states */

#define	RXS_OPENING	1
#define	RXS_OPEN	2
#define	RXS_CLOSING	3
#define	RXS_CLOSED	4


#if defined(__cplusplus)
}
#endif

#endif /* _RX_H */
