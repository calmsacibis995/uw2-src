/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/cs/csi.c	1.2.13.10"

#include <stdlib.h>
#include <xti.h>
#include <fcntl.h>
#include <netconfig.h>
#include <netdir.h>
#include <stdio.h>
#include <stropts.h>
#include <termios.h>
#include <sys/types.h>
#include <pfmt.h>
#include <locale.h>
#include <dial.h>
#include <poll.h>
#include <string.h> 
#include <cs.h>
#include <errno.h>
#include <unistd.h>

#ifdef	DEBUG_ON
#define DEBUG(x) fprintf x
#else
#define DEBUG(x) 
#endif

static	int 	read_status();
static 	int	write_request();
static	int	write_dialrequest();
static 	int	read_dialrequest();

/*
 *	cs_connect() attempts to establish an authenticated connection
 *	to the service on the specified host.  cs_connect passes a 
 *	host-service structure over a mounted streams pipe to the
 *	CS daemon.  The CS daemon services the request and passes
 *	a status/error structure, and an fd, if the connection was
 *	sucessful, back to the waiting cs_connect().
 */

int
cs_connect(host, service, cs_opt, error)
char 	*host;
char 	*service;
struct 	csopts *cs_opt;
int	*error;
{
	int 	circuitfd;		/* fd into service connect  */
	struct 	con_request	conrequest;
 	struct 	strrecvfd 	recvfd;
	char	*netpath;
	int 	i;				/* counter */
	int	requestype = TLI_REQUEST;	/* request type */
	struct 	netbuf    *nbp;			/* temp pointer */
	struct 	netconfig *ncp;			/* temp pointer */
        char    *where=(char *)&requestype;
        int     reqsize = sizeof(int);
	
	*error = CS_NO_ERROR;

	/* Copy arguments to con_request structure */
	memset(&conrequest, NULL, sizeof(struct con_request));
	if ((netpath = getenv(NETPATH)) != NULL) 
		strcpy(conrequest.netpath,netpath); 
	strcpy(conrequest.host,host);
	strcpy(conrequest.service,service);
	if (cs_opt != NULL) {
		conrequest.option = cs_opt->nd_opt;
		nbp = cs_opt->nb_p;
		if (nbp != NULL) {
			conrequest.nb_set = TRUE;
			conrequest.maxlen = nbp->maxlen;
			conrequest.len = nbp->len;
			(void) memcpy(conrequest.buf,
				     nbp->buf,
				     nbp->len); 
		}
		ncp = cs_opt->nc_p;
		if (ncp != NULL) {
			conrequest.nc_set = TRUE;
			if (ncp->nc_netid) 
				strncpy(conrequest.netid,
					ncp->nc_netid,CS_STRSIZE);
			conrequest.semantics = ncp->nc_semantics;
			conrequest.flag = ncp->nc_flag;
			if (ncp->nc_protofmly) 
				strncpy(conrequest.protofmly,
					ncp->nc_protofmly,CS_STRSIZE);
				conrequest.nc_set = TRUE;
			if (ncp->nc_proto) 
				strncpy(conrequest.proto,
					ncp->nc_proto,CS_STRSIZE);
		}
	}

	/* Open named stream to create a unique connection between the
	 * CS library interface and the CS daemon.
	 */

again1:	/* temporary fix until phantom EINTR signal symptom is fixed
	   in kernel.  Sorry for the goto.
	 */
	if ((circuitfd = open(CSPIPE, O_RDWR)) == -1) {
		if (errno == EINTR) { 	/*temp fix*/
			sleep(2); 	/*temp fix*/
			goto again1; 	/*temp fix*/
		} 			/*temp fix*/
		*error = CS_SYS_ERROR;	
		return(-1);
	}

	/* Has stream to cs daemon been setup? */
	if ((i=isastream(circuitfd)) == 0){
		*error = CS_SYS_ERROR;	
		close(circuitfd);
		return(-1);
	}

	/* Inform daemon a TLI type connection is requested */
        while ((i=write(circuitfd, where, reqsize)) != reqsize) {
                where = where + i;
                reqsize = reqsize -i;
                if (i == -1) {
			*error = CS_SYS_ERROR;	
			close(circuitfd);
			return(-1);
		}
        }


	/*	Write connection request to daemon	*/	
	if ((*error = write_request(circuitfd, &conrequest)) != CS_NO_ERROR) {
		close(circuitfd);
		return(-1);
	}


	/*	Read result of connection attempt from daemon	*/
	if ((*error = read_status(circuitfd, &recvfd)) != CS_NO_ERROR) {
	  	DEBUG((stderr,"csi:read_status return <%d>\n",*error));
		close(circuitfd);
		return(-1);
	}

	/*
	*	Receive file descriptor from daemon if no error
	* 	has been made while making the connection.
	*/

	if (ioctl(circuitfd, I_RECVFD, &recvfd) == 0) {
		close(circuitfd);
		return(recvfd.fd);
	} else {
	  	DEBUG((stderr,"csi: ioctl error:errno <%d>\n",errno));
		*error = CS_SYS_ERROR;	
		close(circuitfd);
		return(-1);
	}
}





/*
 *	dial() attempts to establish an authenticated connection
 *	to the service on the specified host.  dial() passes a 
 *	dial request structure over a mounted streams pipe to the
 *	CS daemon. The dialrequest structure contains all dial data
 *	from the old dial interface plus host and service information in
 *	the CALL_EXT structure. The cs daemon processes the request
 *	and passes a status/error structure, and an fd, if the connection
 *	was sucessful, back to the waiting dial().  dial() uses it's
 *	own old interface for error handling and debugging.
*/

int
dial(i_call)
CALL	i_call;
{
	int 	circuitfd;		/* fd into service connect  */
 	struct 	strrecvfd 	recvfd;
 	CALL	Call;
	struct	termio Call_termio;
	CALL_EXT	Call_ext;
	int 	i;
	int	requestype = DIAL_REQUEST;	/* request type */
	int	rval;
        char    *where=(char *) &requestype;
        int     reqsize = sizeof(int);
	
	Call.attr = &Call_termio;
	Call.device = (char *) &Call_ext;

	/* Open named stream to create a unique connection between the
	*  CS library interface and the CS daemon.
	*/

again2:	/* temporary fix until phantom EINTR signal symptom is fixed
	   in kernel.  Sorry for the goto.
	 */
	if ((circuitfd= open(CSPIPE, O_RDWR)) == -1) {
		if (errno == EINTR) { 	/*temp fix*/
			sleep(2); 	/*temp fix*/
			goto again2; 	/*temp fix*/
		}			/*temp fix*/
		return(CS_PROB);
	}
	
	if ((i=isastream(circuitfd)) == 0) {
		close(circuitfd);
		return(CS_PROB);
	}

	/* Inform daemon a dial type connection is requested */

        while ((i=write(circuitfd, where, reqsize)) != reqsize) {
                where = where + i;
                reqsize = reqsize -i;
                if (i == -1) {
			close(circuitfd);
			return(CS_PROB);
		}
        }

	if ((i=write_dialrequest(circuitfd, i_call)) == -1) {
		close(circuitfd);
		return(CS_PROB);
	}
	
	if ((rval = read_status(circuitfd, &recvfd)) == CS_DIAL_ERROR) {
		close(circuitfd);
		/* dial error codes are held in the fd */
		return(recvfd.fd);
	}
	else if (rval != CS_NO_ERROR) {
		close(circuitfd);
		return(rval);
	}
	else {
		if (ioctl(circuitfd, I_RECVFD, &recvfd) != 0) {
			close(circuitfd);
			return(CS_PROB);
		}
		rval = read_dialrequest(circuitfd, &Call);
		if (rval != 0 && i_call.device != NULL)
			((CALL_EXT *)i_call.device)->protocol =
				((CALL_EXT *)Call.device)->protocol;
		close(circuitfd);
                /* a small window with cs child which syncs on above close.
                 * note: a blocking lock
                 */
                lockf(recvfd.fd, F_LOCK, 0L);
		return(recvfd.fd);
	}
}

void
undial(int fd)
{
	struct termios ttybuf;

	if(fd >= 0) {
		if (ioctl(fd, TCGETA, &ttybuf) == 0) {
			if (!(ttybuf.c_cflag & HUPCL)) {
				ttybuf.c_cflag |= HUPCL;
				(void) ioctl(fd, TCSETAW, &ttybuf);
			}
		}
		(void) close(fd);
	}
	return;
}



/*
*	Writes a con_request structure over the streams pipe to the 
*	CS daemon
*/	

static	int
write_request(circuitfd, conrequestp)
	int 	circuitfd;		/* fd into service connect  */
	struct 	con_request	*conrequestp;
{
	int	i=0;
        char    *where=(char *)conrequestp;
        int     reqsize = sizeof(struct con_request);

        while ((i=write(circuitfd, where, reqsize)) != reqsize) {
                where = where + i;
                reqsize = reqsize -i;
                if (i == -1)
                        return(CS_SYS_ERROR);
        }

	return(CS_NO_ERROR);
}


/*	Reads a status structure generated by the CS daemon
*	If the cs daemon exits before writing back a status
*	structure read_status will detect the POLLHUP and
*	timeout.  It is not recommended for read_status(),
*	cs_connect(), or dial()  to timeout (via alarm) since 
*	a connection request, especially a dial request,  can 
*	take a very long time to process.  There is no correct 
*	timeout value.
*/

static	int
read_status(circuitfd, recvfdp)
	int 	circuitfd;		/* fd into service connect  */
 	struct 	strrecvfd 	*recvfdp;
{
	struct 	pollfd pinfo;	/* poll only 1 stream to CS daemon */
	struct  status	cs_stat;
        int     reqsize = sizeof(struct status);
	int	i=0;

	DEBUG((stderr, "csi:read_status: Waiting to read status\n"));
		
	pinfo.fd = circuitfd;
	pinfo.events = POLLIN | POLLHUP;
	pinfo.revents = 0;

	if (poll(&pinfo, 1, -1) < 0) {
		/* poll error */
		return(CS_TIMEDOUT);
	} else if (pinfo.revents & POLLIN) {
		/* incoming request or message */
		if ((i=read(circuitfd,&cs_stat,reqsize)) != reqsize) {
			DEBUG((stderr,"read in read_status failed\n"));
	                if (i == -1)
				return(CS_SYS_ERROR);
			return(CS_TIMEDOUT);
		}
	  	DEBUG((stderr,"csi: cs_error<%d>\n",cs_stat.cs_error));
	  	DEBUG((stderr,"csi: sys_error<%d>\n",cs_stat.sys_error));
	  	DEBUG((stderr,"csi: dial_error<%d>\n",cs_stat.dial_error));
	  	DEBUG((stderr,"csi: tli_error<%d>\n",cs_stat.tli_error));
		errno = cs_stat.sys_error;
		/*
		set_t_errno(cs_stat.tli_error); 
		*/
		if (cs_stat.cs_error == CS_DIAL_ERROR) 
			/* dial returns its error code in the fd */
			recvfdp->fd = cs_stat.dial_error;
		return(cs_stat.cs_error);

	} else {
		DEBUG((stderr, "POLL revents=0x%x\n", pinfo.revents));
	  	DEBUG((stderr,"csi:read_status FAILED: nothing read\n"));
		return(CS_TIMEDOUT);
	}
}



/*
 *	Print out returned error value from cs_connect() or cs_dial()
 */
void
cs_perror(str, err)
char *str;
int err;
{
	switch (err) {
	case CS_NO_ERROR:
		(void)pfmt(stderr, MM_ERROR, "uxnsu:134:%s: No Error\n",str);
		break;
	case CS_SYS_ERROR:
		(void)pfmt(stderr, MM_ERROR, "uxnsu:135:%s: System Error: %s\n",str, strerror(errno));
		break;
	case CS_DIAL_ERROR:
		(void)pfmt(stderr, MM_ERROR, "uxnsu:136:%s: Dial error\n",str);
		break;
	case CS_MALLOC:
		(void)pfmt(stderr, MM_ERROR, "uxnsu:137:%s: No Memory\n",str);
		break;
	case CS_AUTH:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:139:%s: Authentication scheme specified by server is not acceptable\n",str);
		break;
	case CS_CONNECT:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:140:%s: Connection to service failed\n",str);
		break;
	case CS_INVOKE:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:141:%s: Error in invoking authentication scheme\n",str);
		break;
	case CS_SCHEME:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:142:%s: Authentication scheme unsucessful\n",str);
		break;
	case CS_TRANSPORT:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:143:%s: Could not obtain address of service over any transport\n",str);
		break;
	case CS_PIPE:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:144:%s: Could not create CS pipe\n",str);
		break;
	case CS_FATTACH:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:145:%s: Could not mount remote stream to CS pipe\n",str);
		break;
	case CS_CONNLD:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:146:%s: Could not push CONNLD\n",str);
		break;
	case CS_FORK:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:147:%s: Could not fork CS child request\n",str);
		break;
	case CS_CHDIR:
		(void)pfmt(stderr, MM_ERROR, "uxnsu:138:%s: Could not chdir\n",str);
		break;
	case CS_SETNETPATH:
		{
		char *path;
		path = getenv(NETPATH);
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:148:%s: host/service not found over available transport %s\n",
		      str, path==NULL? "": path);
		}
		break;
	case CS_TOPEN:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:149:%s: TLI failure: t_open failed\n",str);
		break;
	case CS_TBIND:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:150:%s: TLI failure: t_bind failed\n",str);
		break;
	case CS_TCONNECT:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:151:%s: TLI failure: t_connect failed\n",str);
		break;
	case CS_TALLOC:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:152:%s: TLI failure: t_alloc failed\n",str);
		break;
	case CS_MAC:
		(void)pfmt(stderr, MM_ERROR, 
	              "uxnsu:153:%s: MAC check failure or Secure Device access denied\n",str);
		break;
	case CS_DAC:
		(void)pfmt(stderr, MM_ERROR, 
	              "uxnsu:154:%s: DAC check failure or Secure Device access denied\n",str);
		break;
	case CS_TIMEDOUT:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:155:%s: Connection attempt timed out\n",str);
		break;
	case CS_NETPRIV:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:156:%s: Privileges not correct for requested network options\n",str);
		break;
	case CS_NETOPTION:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:157:%s: Netdir option incorrectly set in csopts struct\n",str);
		break;
	case CS_NOTFOUND:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:158:%s: Service not found in server's _pmtab\n",str);
		break;
	case CS_LIDAUTH:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:159:%s: Connection not permitted by LIDAUTH.map\n",str);
		break;
	default:
		(void)pfmt(stderr, MM_ERROR, 
		      "uxnsu:160:%s:  cs_perror error in message reporting\n",str);
		break;
	}
}

/*
 *	Write a dialrequest structure over streams pipe to cs daemon.
 *	Account for any NULL pointers that may be in CALL structure
 *	and NETPATH environment variable.
*/
static	int
write_dialrequest(fd, Call)
int	fd;
CALL	Call;
{
	struct dial_request	dialrequest;
	struct dial_request	*dialrequestp=&dialrequest;
	int	i;

	if (Call.attr != NULL){
		dialrequestp->termioptr=NOTNULLPTR;
		dialrequestp->c_iflag=Call.attr->c_iflag;	
		dialrequestp->c_oflag=Call.attr->c_oflag;	
		dialrequestp->c_cflag=Call.attr->c_cflag;	
		dialrequestp->c_lflag=Call.attr->c_lflag;	
		dialrequestp->c_line=Call.attr->c_line;	
		memcpy(dialrequestp->c_cc,Call.attr->c_cc,NCC); 
	}else
		dialrequestp->termioptr=NULLPTR;

	if (Call.line != NULL){
		dialrequestp->lineptr=NOTNULLPTR;
		strcpy(dialrequestp->line,Call.line); 
	}else
		dialrequestp->lineptr=NULLPTR;


	if (Call.telno != NULL){
		dialrequestp->telnoptr=NOTNULLPTR;
		strcpy(dialrequestp->telno,Call.telno); 
	}else
		dialrequestp->telnoptr=NULLPTR;

	if (Call.device != NULL){
		dialrequestp->deviceptr=NOTNULLPTR;

		if (((CALL_EXT	*)Call.device)->service != NULL){ 
			strcpy(dialrequestp->service,((CALL_EXT	*)Call.device)->service); 
			dialrequestp->serviceptr=NOTNULLPTR;
		}else
			dialrequestp->serviceptr=NULLPTR;

		if (((CALL_EXT	*)Call.device)->class != NULL){ 
			dialrequestp->classptr=NOTNULLPTR;
			strcpy(dialrequestp->class,((CALL_EXT	*)Call.device)->class); 
		}else
			dialrequestp->classptr=NULLPTR;

		if (((CALL_EXT	*)Call.device)->protocol != NULL){ 
			dialrequestp->protocolptr=NOTNULLPTR;
			strcpy(dialrequestp->protocol,((CALL_EXT	*)Call.device)->protocol); 
		}else
			dialrequestp->protocolptr=NULLPTR;

	}else
		dialrequestp->deviceptr=NULLPTR;

	dialrequestp->version=1;	
	dialrequestp->baud=Call.baud;	
	dialrequestp->speed=Call.speed;	
	dialrequestp->modem=Call.modem;	
	dialrequestp->dev_len=Call.dev_len;	
 	if(getenv(NETPATH) != NULL)
 		strcpy(dialrequestp->netpath, getenv(NETPATH));
 	else
 		strcpy(dialrequestp->netpath,"");
	dialrequestp->pid=getpid();
		
#ifdef DEBUG_ON
	fprintf(stderr, "write_dialrequest:writing request to fd:<%d>\n",fd);
	fprintf(stderr, "dialrequestp->baud<%d>\n",dialrequestp->baud);
	fprintf(stderr, "dialrequestp->speed<%d>\n",dialrequestp->speed);
	fprintf(stderr, "dialrequestp->modem<%d>\n",dialrequestp->modem);
	fprintf(stderr, "dialrequestp->dev_len<%d>\n",dialrequestp->dev_len);
	fprintf(stderr, "dialrequestp->pid<%d>\n",dialrequestp->pid);

	if (dialrequestp->termioptr != NULLPTR) {	
	fprintf(stderr, "dialrequestp->c_iflag<%d>\n",dialrequestp->c_iflag);
	fprintf(stderr, "dialrequestp->c_oflag<%d>\n",dialrequestp->c_oflag);
	fprintf(stderr, "dialrequestp->c_cflag<%d>\n",dialrequestp->c_cflag);
	fprintf(stderr, "dialrequestp->c_lflag<%d>\n",dialrequestp->c_lflag);
	fprintf(stderr, "dialrequestp->c_line<%c>\n",dialrequestp->c_line);
	/* c_cc is not a string: cannot printf with %s - leave out for now
	fprintf(stderr, "dialrequestp->c_cc<%s>\n",dialrequestp->c_cc);
	*/
	} else
		fprintf(stderr, "dialrequestp->termio = NULL\n");

	if (dialrequestp->lineptr != NULLPTR) {	
	fprintf(stderr, "dialrequestp->line<%s>\n",dialrequestp->line);
	} else
		fprintf(stderr, "dialrequestp->lineptr = NULL\n");

	if (dialrequestp->telnoptr != NULLPTR) {	
		fprintf(stderr, "dialrequestp->telno<%s>\n",dialrequestp->telno);
	}else
		fprintf(stderr, "dialrequestp->telnoptr = NULL\n");

	if (dialrequestp->deviceptr != NULLPTR) {	

		if (dialrequestp->serviceptr != NULLPTR) {	
			fprintf(stderr, "dialrequestp->service<%s>\n",dialrequestp->service);
		}else
			fprintf(stderr, "dialrequestp->serviceptr = NULL\n");

		if (dialrequestp->classptr != NULLPTR) {	
			fprintf(stderr, "dialrequestp->class<%s>\n",dialrequestp->class);
		}else
			fprintf(stderr, "dialrequestp->classptr = NULL\n");


		if (dialrequestp->protocolptr != NULLPTR) {	
			fprintf(stderr, "dialrequestp->protocol<%s>\n",dialrequestp->protocol);
		}else
			fprintf(stderr, "dialrequestp->protocolptr = NULL\n");

		if (dialrequestp->reserved1 != NULLPTR) {	
			fprintf(stderr, "dialrequestp->reserved1<%s>\n",dialrequestp->reserved1);
		}else
			fprintf(stderr, "dialrequestp->reserved1 = NULL\n");
	
	}else
		fprintf(stderr, "dialrequestp->deviceptr = NULL\n");

#endif

	while ((i=write(fd, dialrequestp, (sizeof(dialrequest)))) != (sizeof(dialrequest))){
		if ( i == -1)
		return(-1);
		}

	return(0);
}

/*
 *	Read a dialrequest structure over streams pipe from cs daemon.
*/
static	int
read_dialrequest(fd, Callp)
int	fd;
CALL	*Callp;
{
	char	 buff[LRGBUF];
	char	*where=&buff[0];
	int 	 dialsize;
	struct 	 dial_request	*dialrequestp;
	int	 i;
	dialsize = sizeof( struct dial_request );

	while ((i=read(fd, where, dialsize )) != dialsize) {
		if ( i == -1)
			return(0);
		where = where + i;
		dialsize = dialsize -i;
	}

	dialrequestp = (struct dial_request *)buff;
	
	Callp->baud = dialrequestp->baud;
	Callp->speed = dialrequestp->speed;
	Callp->modem = dialrequestp->modem;
	Callp->dev_len = dialrequestp->dev_len;


	if (dialrequestp->termioptr != NULLPTR) {	
		Callp->attr->c_iflag = dialrequestp->c_iflag;
		Callp->attr->c_oflag = dialrequestp->c_oflag;
		Callp->attr->c_cflag = dialrequestp->c_cflag;
		Callp->attr->c_lflag = dialrequestp->c_lflag;
		Callp->attr->c_line = (dialrequestp->c_line);
		memcpy((char *)Callp->attr->c_cc, dialrequestp->c_cc,NCC); 
	} else
		Callp->attr = NULL;

	if (dialrequestp->lineptr != NULLPTR) {	
		Callp->line = strdup(dialrequestp->line); 
	} else
		Callp->line = NULL;

	if (dialrequestp->telnoptr != NULLPTR) {	
		Callp->telno = strdup(dialrequestp->telno); 
	} else
		Callp->telno = NULL;

	if (dialrequestp->deviceptr != NULLPTR) {	

		if (dialrequestp->serviceptr != NULLPTR) 
			((CALL_EXT *)Callp->device)->service = 
			      strdup(dialrequestp->service); 
		else
			((CALL_EXT *)Callp->device)->service = NULL;

		if (dialrequestp->classptr != NULLPTR) 
			((CALL_EXT *)Callp->device)->class = 
			      strdup(dialrequestp->class); 
		else
			((CALL_EXT *)Callp->device)->class = NULL;


		if (dialrequestp->protocolptr != NULLPTR) 
			((CALL_EXT *)Callp->device)->protocol = 
			      strdup(dialrequestp->protocol); 
		else
			((CALL_EXT *)Callp->device)->protocol = NULL;

		if (dialrequestp->reserved1 != NULLPTR)
			((CALL_EXT *)Callp->device)->reserved1 = 
			      strdup(dialrequestp->reserved1); 
		else
			((CALL_EXT *)Callp->device)->reserved1 = NULL;

	} else
		Callp->device = NULL;
		
#ifdef DEBUG_ON
	fprintf(stderr, "Callp->baud<%d>\n", Callp->baud);
	fprintf(stderr, "Callp->speed<%d>\n", Callp->speed);
	fprintf(stderr, "Callp->modem<%d>\n", Callp->modem);
	fprintf(stderr, "Callp->dev_len<%d>\n", Callp->dev_len);

	if (Callp->attr != NULL){
		fprintf(stderr, "Callp->attr->c_iflag<%d>\n",
			Callp->attr->c_iflag);
		fprintf(stderr, "Callp->attr->c_oflag<%d>\n",
			Callp->attr->c_oflag);
		fprintf(stderr, "Callp->attr->c_cflag<%d>\n",
			Callp->attr->c_cflag);
		fprintf(stderr, "Callp->attr->c_lflag<%d>\n",
			Callp->attr->c_lflag);
		fprintf(stderr, "Callp->attr->c_line<%c>\n",
			Callp->attr->c_line);
	/* c_cc is not a string: cannot printf with %s - leave out for now
		fprintf(stderr, "Callp->attr->c_cc<%s>\n", Callp->attr->c_cc);
	*/
	} else
		fprintf(stderr, "Callp->attr =  NULL\n");

	fprintf(stderr, "Callp->line<%s>\n", Callp->line?Callp->line:"NULL");
	fprintf(stderr, "Callp->telno<%s>\n", Callp->telno?Callp->telno:"NULL");
	if (Callp->device != NULL){
		dialrequestp->deviceptr = NOTNULLPTR;
		fprintf(stderr, 
		   "((CALL_EXT *)Callp->device)->service<%s>\n",
		    ((CALL_EXT *)Callp->device)->service ? 
		    ((CALL_EXT *)Callp->device)->service : "NULL");

		fprintf(stderr, 
		   "((CALL_EXT *)Callp->device)->class<%s>\n",
		    ((CALL_EXT *)Callp->device)->class ?
		    ((CALL_EXT *)Callp->device)->class : "NULL");

		fprintf(stderr, 
		   "((CALL_EXT *)Callp->device)->protocol<%s>\n",
		    ((CALL_EXT *)Callp->device)->protocol ?
		    ((CALL_EXT *)Callp->device)->protocol : "NULL");

		fprintf(stderr, 
		   "((CALL_EXT *)Callp->device)->reserved1<%s>\n",
		    ((CALL_EXT *)Callp->device)->reserved1 ?
		    ((CALL_EXT *)Callp->device)->reserved1 : "NULL");
	} else
		fprintf(stderr, "((CALL_EXT *)Callp->device = NULL\n");
#endif	
	return(1);
}
