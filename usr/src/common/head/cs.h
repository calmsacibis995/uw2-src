/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef CS_NO_ERROR
#ident	"@(#)head.usr:cs.h	1.1.2.7"

#ifdef __cplusplus
extern "C" {
#endif

struct csopts {
      struct netconfig *nc_p;
      int nd_opt;
      struct netbuf *nb_p;
};

int
cs_connect(char *, char *, struct csopts *, int *);

void
cs_perror(char *, int);

/*
 *	Error conditions of cs_connect().
 */

#define	CS_NO_ERROR	0
#define	CS_SYS_ERROR	1
#define CS_DIAL_ERROR	2
#define	CS_MALLOC	3
#define	CS_AUTH		4
#define CS_LIDAUTH	5
#define	CS_CONNECT	6
#define	CS_INVOKE	7
#define	CS_SCHEME	8
#define	CS_TRANSPORT	9
#define	CS_PIPE		10
#define	CS_FATTACH	11
#define	CS_CONNLD	12
#define	CS_FORK		13
#define CS_CHDIR	14
#define	CS_SETNETPATH	15
#define	CS_TOPEN	16
#define	CS_TBIND	17
#define	CS_TCONNECT	18
#define	CS_TALLOC	19
#define	CS_MAC		20
#define CS_DAC		21
#define	CS_TIMEDOUT	22
#define CS_NETPRIV	23
#define CS_NETOPTION	24
#define CS_NOTFOUND	25
#define CS_INTERRUPT	26
#define CS_NOERRMEM	27



/* PRIVATE DEFINITIONS */

/*
 * Please note: the following definitions are used internally by the
 * Connection Server client and server.  They are placed in this file
 * as the sole repository.  However, these definitions are not meant
 * to be an external interface and are not guaranteed to be maintained
 * in future releases.
 */

#define MSGSIZE		512	/* size of scratch buffer */
#define CS_STRSIZE	128
#define LRGBUF		5120
#define HOSTSIZE 	256
#define ALARMTIME 	60
#define NOTNULLPTR 	1
#define NULLPTR 	0

struct errmsg{
	char *e_str;	/* error string */
	int e_exitcode; /* and associated exit status */
};

struct	status{
	int cs_error; 	
	int sys_error; 	
	int dial_error;
	int tli_error;
	int unused[10];
};

struct con_request{
	char 	netpath[CS_STRSIZE];
	char 	host[HOSTSIZE];
	char 	service[CS_STRSIZE];
	int	option;
	int	nb_set;
	int	maxlen;
	int	len;
	char 	buf[CS_STRSIZE];
	int	nc_set;
	char	netid[CS_STRSIZE];
	unsigned long	semantics;
	unsigned long	flag;
	char	protofmly[CS_STRSIZE];
	char	proto[CS_STRSIZE];
};

struct schemelist {
	char 	*i_host;
	char	*i_service;
	char	*i_netid;
	char	*i_scheme;
	char	*i_role;
	struct schemelist *i_next;
};

struct dial_request{
	char netpath[CS_STRSIZE];
	int termioptr;
	unsigned short	c_iflag;	/* start TERMIO structure fields */
	unsigned short	c_oflag;
	unsigned short	c_cflag;
	unsigned short	c_lflag;
	char c_line;
	int c_ccptr;
	char c_cc[CS_STRSIZE];		/* end TERMIO structure fields */
	int	baud;
	int	speed;
	int 	lineptr;
	char	line[CS_STRSIZE];
	int 	telnoptr;
	char	telno[CS_STRSIZE];
	int	modem;
	int 	deviceptr;
	int 	serviceptr;
	char	service[CS_STRSIZE];	/* start CALL_EXT structure fields */
	int 	classptr;
#ifdef __cplusplus
	char	classnm[CS_STRSIZE];
#else
	char	class[CS_STRSIZE];
#endif
	int 	protocolptr;
	char	protocol[CS_STRSIZE];
	int 	reservedptr;
	char	reserved1[CS_STRSIZE];
	int	version;
	int	dev_len;		/* unused */
	int	pid;
};

struct schemeinfo {
	char *s_name;
	int   s_flag;
};

/*
 *	Files accessed
 */

#define CSPIPE		"/etc/.cs_pipe"
#define ROOT		"/"
#define LOGFILE		"/var/adm/log/cs.log"
#define DBGFILE		"/var/adm/log/cs.debug"	
#define LOGFILE_OLD	"/var/adm/log/cs.log_old"
#define DBGFILE_OLD	"/var/adm/log/cs.debug_old"	
#define AUTHFILE	"/etc/cs/auth"	
#define CACHEFILE	"/var/tmp/.cscache"
#define SERVEALLOW 	"/etc/iaf/serve.allow"
#define SERVEALIAS 	"/etc/iaf/serve.alias"
#define LIDFILE		"/etc/idmap/attrmap/LIDAUTH.map"

/*
 *	Log and debug log default sizes
 */

#define CS_LOG_FILE_SIZE	((off_t)160000)
#define CS_DEBUG_FILE_SIZE	((off_t)400000)

/*
 * miscellaneous defines
 */

#define AU_IMPOSER	0
#define AU_RESPONDER	1
#define NO_EXIT		0
#define CS_EXIT		1
#define DIAL_REQUEST	0	
#define TLI_REQUEST	1	
#define CS_READ_AUTH	2
	
#ifdef __cplusplus
}
#endif

#endif /* CS_NO_ERROR */
