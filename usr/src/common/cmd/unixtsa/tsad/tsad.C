#ident	"@(#)unixtsa:common/cmd/unixtsa/tsad/tsad.C	1.13"
/***
 *
 *  name	tsad.C - driver of Unix TSA daemon process
 *		@(#)unixtsa:common/cmd/unixtsa/tsad/tsad.C	1.13 10/31/94
 *
 ***/

#include <tsad.h>
#include	<stdlib.h>
#include	"config_file.h"

#ifndef SYSV
#include <sys/file.h>
#endif

#ifdef SYSV
#include	<locale.h>
#include	<sys/systeminfo.h>
#include	<netdb.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/fcntl.h>
#include	<time.h>
#include	<i18n.h>
#include	<limits.h>
#endif

#ifdef SVR4
#include	<unistd.h>
#include	<tsad_msgs.h>
#include	<nl_types.h>
#include	<pfmt.h>
#include        <rpcsvc/ypclnt.h>
#include        <rpcsvc/yp_prot.h>
#endif

/* Extern variables */
#ifdef TLI
#include	<tiuser.h>

extern int t_errno;
extern char *t_errlist[];
extern int t_nerr;
#endif
extern int CloseTransportEndpoint(int registerFD);
#ifdef SINGLE_THREAD
extern void	respond(void) ;
#endif

#include	<netconfig.h>
#include	<netdir.h>
#include	<tiuser.h>

/* Function Prototypes */
#ifdef DEBUG
void show_buf(char *buf, int len) ;
#endif
void daemonize(void);
int lookupAndRegister(struct requestPacket *request) ;
void OpenListeningSocket() ;
void processRequests() ;
void updateConfigFile() ;
void processConnectionRequest(int listenFD, struct netconfig *netEntry);
int registerWithServers( struct serverData *list, int force ) ;
int ReadDataBlock( int Fd, int Count, char *Buffer ) ;
int WriteDataBlock( int Fd, int Count, char *Buffer ) ;
void TruncateFile(char *logFile, int maxSize) ;

/* Global variables */
static struct serverData	*serverList = NULL ;
static struct serverData	*defaultServer = NULL ;
FILE 			*logfp ;
static int 	spxListenFD = -1, tcpListenFD  = -1;	// listen endpoints
static struct netconfig	*spxNetEntry = NULL, *tcpNetEntry = NULL ;
struct t_bind		*spxBound = NULL, *tcpBound = NULL ;
nl_catd			mCat ;	// i18n message catalog
static int		signalReceived = 0 ;
static struct hostent	*host_data = NULL ;

/* save argv so we can re-exec ourselves if we need to */
static char		CommPath[PATH_MAX] ;
static char		**Argv ;

#ifdef SVR4
void sig_intr(int sig,...);
#else
static int logfd ;
void sig_child(int sig,...);
#endif


/* globals for arguments */
static char		*optString = "TSl:c:p:t:f:v" ;
static char		*tsaunixPath = TSA_NAME ;
static unsigned int	registerTimeout = REGISTRATION_TIMEOUT ;
static unsigned int	connectTimeout = CONNECT_TIMEOUT ;
static char		*configFile = CONFIG_FILE ;
static char		*logFile = LOG_FILE ;
static char		errorBuffer[64] ;

void
main(int argc, char **argv)
{
	int 			c, errflg = 0 ;
	int			tcpOnly = 0, spxOnly = 0 ;
	int			verbose = 0 ;
	time_t			start_time ;
#ifdef SVR4
	struct sigaction	act ;
#endif

	setlocale(LC_ALL, "") ;		// set up for i18n
	mCat = catopen(MCAT, 0) ;
	setlabel("UX:tsad") ;

	Argv = argv ;
	if ( *argv[0] == '/' )
	    {
	    // run with an absolute path
	    (void) strcpy(CommPath, *argv) ;
	    }
	else
	    {
	    (void) getcwd(CommPath, PATH_MAX) ;
	    (void) strcat(CommPath, "/") ;
	    (void) strcat(CommPath, *argv) ;
	    }

	while ( (c = getopt(argc, argv, optString)) != EOF )
	    switch ( c ) {
		case 'v' :
		    verbose++ ;
		    break ;

		case 'S' :
		    spxOnly++ ;
		    break ;

		case 'T' :
		    tcpOnly++ ;
		    break ;

		case 'l' :
		    logFile = optarg ;
		    break ;

		case 'p' :
		    tsaunixPath = optarg ;
		    break ;

		case 'f' :
		    configFile = optarg ;
		    break ;

		case 'c' :
		    connectTimeout = atoi(optarg) ;
		    if ( connectTimeout < CONNECT_TIMEOUT )
			connectTimeout = CONNECT_TIMEOUT ; 
		    break ;

		case 't' :
		    registerTimeout = atoi(optarg) ;
		    if ( registerTimeout < MIN_TIMEOUT )
			registerTimeout = MIN_TIMEOUT ; 
		    break ;

		case '?' :
		    errflg++ ;
		}

	if ( errflg > 0 )
	    {
	    sprintf(errorBuffer, "%s:%d:%s\n", MCAT, C_USAGE, M_USAGE) ;
	    pfmt(stderr, MM_ACTION, errorBuffer, argv[0]) ;
	    exit(1) ;
	    }

	// open these here, so if they are not found we can complain right away
	if ( tcpOnly == 0 )
	    spxNetEntry = getnetconfigent(SPX_NAME) ;

	if ( spxOnly == 0 )
	    tcpNetEntry = getnetconfigent(TCP_NAME) ;

	TruncateFile(logFile, LOG_SIZE) ;

#ifndef DEBUG
	daemonize() ;
#endif

	if ( (logfp = fopen(logFile,LOG_MODE)) == NULL ) {
	    if ( (logfp = fopen(CONSLOG,"w")) == NULL )
		logfp = stderr ;

	    logerror(PRIORITYERROR, catgets(mCat, MSG_SET, C_NOLOG, M_NOLOG),
		logFile, CONSLOG) ;
	}
	start_time = time(NULL) ;
	logerror(PRIORITYERROR, "\n") ;
	logerror(PRIORITYERROR, catgets(mCat, MSG_SET, C_STARTUP, M_STARTUP),
	    ctime(&start_time)) ;

	if ( spxNetEntry == NULL && tcpNetEntry == NULL )
	    // this shouldn't happen, install should verify
	    {
	    sprintf(errorBuffer, "%s:%d:%s\n", MCAT, C_NOPROTO, M_NOPROTO) ;
	    pfmt(logfp, MM_ERROR, errorBuffer) ;
	    exit(C_NOPROTO) ;
	    }

	if ( access(tsaunixPath, X_OK | EX_OK) != 0 )
	    {
	    logerror(PRIORITYERROR, catgets(mCat, MSG_SET, C_NOTSA, M_NOTSA),
		tsaunixPath) ;
	    exit(C_NOTSA) ;
	    }

	fflush(logfp) ;

#ifdef SVR4
	sigemptyset(&(act.sa_mask)) ;
	act.sa_flags = SA_NOCLDWAIT ;
	act.sa_handler = (void (*)()) SIG_IGN ;
	sigaction(SIGCLD, &act, NULL) ;

	act.sa_handler = (void (*)()) sig_intr ;
	act.sa_flags = 0 ;
	sigaction(SIGHUP, &act, NULL) ;
	sigaction(SIGALRM, &act, NULL) ;
#else
	signal(SIGCLD,sig_child) ;
#endif

	OpenListeningSocket() ;

	processRequests();
}

/***
 *
 *  name	registerWithServers - register with each server on the list.
 *		If force is is zero, then only do this if timeout has really
 *		taken place.
 *
 ***/

int
registerWithServers( struct serverData *list, int force )
{
    int			successCount = 0 ;
    static time_t	lastRegisterTime = 0 ;
    int			registerFD ;
    static struct t_call	*callData = NULL ;
    int			sndRc, rcvRc ;
    int			rc ;
    int			tlook_value ;
    char		replyBuffer[REPLY_PACKET_SIZE] ;
    struct replyPacket	*replyData = (struct replyPacket *)replyBuffer ;
    struct serverData	*scan, *head = list ;

    if ( (force == 0) && (time(NULL) - lastRegisterTime < registerTimeout) )
	return(1) ;		// don't register again yet

    // set all success flags to zero
    scan = list ;
    while ( scan != NULL ) {
	scan->success = 0 ;
	scan = scan->next ;
	}

#ifdef DEBUG
    logerror(PRIORITYWARN, "Entering registration loop.\n") ;
    fflush(PRIORITYWARN) ;
#endif

    while ( list != NULL ) {
	if ( list->netData == NULL ) {
	    // skipping incomplete or invalid entry
	    list = list->next ;
	    continue ;
	    }

	if ( strcmp(list->serverName, DEFAULT_ENTRY) == 0 ) {
	    // don't register with the default entry
	    list = list->next ;
	    continue ;
	    }

	// check if we've already registered with this server
	scan = head ;
	while ( scan != NULL ) {
	    if ( scan == list )
		scan = NULL ;	// reached current entry, so drop out of loop
	    else if ( (strcmp(scan->serverName, list->serverName) == 0)
			&& (scan->success > 0) )
		break ;		// yep, we're already registered
	    else
		scan = scan->next ;
	    }

	if ( scan != NULL ) {
	    // we must have hit the break above
	    list = list->next ;
	    continue ;
	    }

	registerFD = t_open(list->netData->nc_device, O_RDWR,(struct t_info *)NULL) ;
	if ( registerFD < 0 ) {
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_TOPEN, M_TOPEN),
		list->netData->nc_device, t_errno, errno) ;
	    fflush(logfp) ;
	    list->lastFailure = C_TOPEN ;
	    list = list->next ;
	    continue ;
	    }

	if ( t_bind(registerFD, (struct t_bind *)NULL, (struct t_bind *)NULL) < 0 ) {
	    logerror(PRIORITYERROR, 
			catgets(mCat, MSG_SET, C_EBIND, M_EBIND),
			list->netData->nc_netid, t_errno) ;
	    fflush(logfp) ;
	    t_close(registerFD) ;
	    list->lastFailure = C_EBIND ;
	    list = list->next ;
	    continue ;
	    }

	if ( callData != NULL )
	    t_free( (char *) callData, T_CALL ) ;

	if ( (callData = (struct t_call *) t_alloc(registerFD, T_CALL, T_ADDR)) == NULL ) {
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_EALLOC, M_EALLOC),
		"t_call", t_errno) ;
	    fflush(PRIORITYERROR) ;
	    t_close(registerFD) ;
	    list->lastFailure = C_EALLOC ;
	    list = list->next ;
	    continue ;
	    }

	// NOTE: just using first address in nd_addrlist
	callData->addr.len = list->serverAddress->n_addrs->len ;
	(void) memcpy(callData->addr.buf, list->serverAddress->n_addrs->buf,
			callData->addr.len) ;

#ifdef DEBUG
	logerror(PRIORITYWARN,"Attempting connection to: %s\n",
			taddr2uaddr(list->netData, &(callData->addr))) ;
	fflush(PRIORITYWARN) ;
#endif

	signalReceived = 0 ;
	alarm(connectTimeout) ;
	rc = t_connect(registerFD, callData, (struct t_call *)NULL) ;
	alarm(0) ;

	if ( rc < 0 ) {
#ifdef DEBUG
	    logerror(PRIORITYWARN, "tconnect failure: signalReceived = %d errno = %d t_errno = %d\n", signalReceived, errno, t_errno) ;
#endif
	    if ( errno == EINTR && signalReceived == SIGALRM )
		{
		/* only print this error once per server */
		if ( list->lastFailure != C_TIMEOUT )
		    logerror(PRIORITYERROR, 
			catgets(mCat, MSG_SET, C_TIMEOUT, M_TIMEOUT),
			list->serverName) ;
		list->lastFailure = C_TIMEOUT ;
		}
	    else
		{
		logerror(PRIORITYERROR, 
			catgets(mCat, MSG_SET, C_TCONNECT, M_TCONNECT),
			list->serverName, list->netData->nc_netid, t_errno) ;
		list->lastFailure = C_TCONNECT ;
		}

	    if ( t_errno == TLOOK ) {
		tlook_value = t_look(registerFD) ;
#ifdef DEBUG
		logerror(PRIORITYERROR, "\t(t_look = 0x%4.4x)\n", tlook_value) ;
#endif
		if ( tlook_value == T_DISCONNECT )
		    logerror(PRIORITYERROR, catgets(mCat, MSG_SET, C_NONLM, M_NONLM), list->serverName) ;
		}

	    fflush(PRIORITYERROR) ;
	    t_close(registerFD) ;
	    list = list->next ;
	    continue ;
	    }

/***
#ifdef DEBUG
	printf("Sending the request packet :\n");
	show_buf((char *)list->requestData, sizeof(struct requestPacket));
	printf ("Packet being sent to: %s\n", taddr2uaddr(list->netData, list->serverAddress->n_addrs));
#endif
***/

	sndRc = WriteDataBlock(registerFD, sizeof(requestPacket), (char *)list->requestData) ;
	if ( sndRc == C_SDIED ) {
	    // server has died on us
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_SDIED, M_SDIED), list->serverName);
	    fflush(PRIORITYERROR) ;
	    }

	if ( sndRc != 0 ) {
	    // some send error including the C_SDIED
	    t_close(registerFD) ;
	    list->lastFailure = C_SDIED ;
	    list = list->next ;
	    continue ;
	    }

	rcvRc = ReadDataBlock(registerFD, sizeof(replyPacket), (char *)replyBuffer) ;

	if ( rcvRc == C_SDIED ) {
	    // server has died on us
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_SDIED, M_SDIED), list->serverName);
	    fflush(PRIORITYERROR) ;
	    }

	if ( rcvRc != 0 ) {
	    // some send error including the C_SDIED
	    t_close(registerFD) ;
	    list->lastFailure = C_SDIED ;
	    list = list->next ;
	    continue ;
	    }

/***
#ifdef DEBUG
        printf("reply packet :\n");
        show_buf((char *)replyBuffer, sizeof(struct replyPacket));
#endif
***/

	if ( SwapUINT32(replyData->packetType) == REPLY_PACKET ) {
	    if ( SwapUINT32(replyData->returnCode) == 0 ) {
		successCount++ ;
		list->lastFailure = 0 ;
		list->success++ ;
#ifdef DEBUG
	logerror(PRIORITYWARN,"Registration to %s was successful.\n",
			taddr2uaddr(list->netData, &(callData->addr))) ;
	fflush(PRIORITYWARN) ;
#endif
	    } else {
		/*
		 * error message from Renea goes here
		 */
		 ;
	    }
	} else {
	    list->lastFailure = C_BADREPLY ;
	    logerror(PRIORITYWARN,
		catgets(mCat, MSG_SET, C_BADREPLY, M_BADREPLY),
		replyData->packetType) ;
	    fflush(PRIORITYWARN) ;
	}

        CloseTransportEndpoint(registerFD);
	list = list->next ;
    }

#ifdef DEBUG
    logerror(PRIORITYWARN, "Exiting registration loop.\n") ;
    fflush(PRIORITYWARN) ;
#endif

    if ( successCount > 0 )
	lastRegisterTime = time(NULL) ;

    return(successCount) ;
}
	
void
processRequests()
{
    int 		nfds = 0, nd ;
    struct pollfd	fds[2] ;
    int  		timeout = registerTimeout * 1000;
    int			warnCount = 0 ;
    int			i ;
    int			forceRegister = 0 ;
#ifdef SVR4
    struct sigaction	act ;
#endif

    while ( 1 ) {
	updateConfigFile() ;	// update configuration file

	if ( registerWithServers(serverList, forceRegister) <= 0 ) {
	    logerror(PRIORITYWARN,
		catgets(mCat, MSG_SET, C_NOSERVERS, M_NOSERVERS)) ;
	    fflush(PRIORITYWARN);
	    if ( warnCount++ > 32 )
		exit(C_NOSERVERS) ;
	    sleep(warnCount*15) ;
	    continue ;
	    }
	else if ( warnCount > 0 )
	    {
	    // regitration failed before, but just worked
	    warnCount = 0 ;
	    }

	forceRegister = 0 ;

#ifdef SVR4
	sigemptyset(&(act.sa_mask)) ;
	act.sa_handler = (void (*)()) sig_intr ;
	act.sa_flags = 0 ;
	sigaction(SIGHUP, &act, NULL) ;
	sigaction(SIGALRM, &act, NULL) ;
#endif

	nfds = 0 ;
	if ( tcpListenFD >= 0 ) {
	    fds[nfds].fd = tcpListenFD ;
	    fds[nfds].events = POLLIN ;
	    fds[nfds].revents = 0 ;
	    nfds++ ;
	}
	if ( spxListenFD >= 0 ) {
	    fds[nfds].fd = spxListenFD ;
	    fds[nfds].events = POLLIN ;
	    fds[nfds].revents = 0 ;
	    nfds++ ;
	}

#ifdef DEBUG
	logerror(PRIORITYERROR, "t_errno = %d, t_look = %d\n", t_errno, t_look(spxListenFD)) ;
	fflush(PRIORITYERROR);
	logerror(PRIORITYWARN, "Entering poll.\n") ;
	fflush(PRIORITYWARN) ;
#endif

	nd = poll(fds,nfds,timeout);

#ifdef DEBUG
	logerror(PRIORITYWARN, "Return from poll = %d\n", nd) ;
	fflush(PRIORITYWARN) ;
#endif

	if ( nd < 0 ) {
	    if ( errno == EINTR && signalReceived == SIGHUP ) {
		// SIGHUP means to re-read config file
		signalReceived = 0 ;
		forceRegister = 1 ;
		continue ;
	    }
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_EPOLL, M_EPOLL), errno) ;
	    logerror(PRIORITYERROR, "t_errno = %d, t_look = %d\n", t_errno, t_look(spxListenFD)) ;
	    fflush(PRIORITYERROR);
	    if ( errno != EINTR )
		exit(C_EPOLL);
	}
	if ( nd == 0 ) {
	    // poll timed out, loop around again
	    continue ;
	}

	for ( i = 0; i < nfds ; i++ ) {
		if ( fds[i].revents & POLLERR || 
			fds[i].revents & POLLHUP ||
			fds[i].revents & POLLNVAL) {
	    		logerror(PRIORITYERROR,
				catgets(mCat, MSG_SET, C_PEVENT, M_PEVENT),
				(fds[i].fd == tcpListenFD) ?
				tcpNetEntry->nc_netid :spxNetEntry->nc_netid,
				fds[i].revents);

				/* re-exec myself */

			execvp(CommPath, Argv) ;
		} 
	}

	// note that connection requests could arrive on both ports
 	for ( i = 0; i < nfds ; i++ ) {
		if ( fds[i].revents & POLLIN ) {
			if(fds[i].fd == tcpListenFD) {
	    			// tcp connection
	    			processConnectionRequest(fds[i].fd, 
								tcpNetEntry);
			}
			else {
	    			// spx connection
	    			processConnectionRequest(fds[i].fd, 
	    							spxNetEntry);
			}
		}
		fds[i].revents = 0 ;
	}
    }
}

/***
 *
 *  name	OpenListeningSocket - open spx and tcp incoming ports
 *
 *		As a result of running this routine, the (spx|tcp)ListenFD
 *		and (spx|tcp)Bound globals will be initialized.
 *
 ***/

void
OpenListeningSocket()
{
    int			gotOne = 0 ;
    struct t_bind	*tbind ;
    struct sockaddr_in	*addrPtr ;
    int			rc ;

    if ( spxNetEntry ) {
	spxListenFD = t_open(spxNetEntry->nc_device, O_RDWR, (struct t_info *)NULL);
	if ( spxListenFD < 0 ) {
		logerror(PRIORITYERROR,
		    catgets(mCat, MSG_SET, C_TOPEN, M_TOPEN),
		    spxNetEntry->nc_device, t_errno, errno) ;
		fflush(PRIORITYERROR);
	    }

	if ( (tbind = (struct t_bind *)t_alloc(spxListenFD, T_BIND, T_ALL)) == NULL ) {
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_EALLOC, M_EALLOC),
		"t_bind", t_errno) ;
	    fflush(PRIORITYERROR) ;
	} else {
	    tbind->qlen = 5 ;
	    tbind->addr.len = 0 ;
	}

	if ( (spxBound = (struct t_bind *)t_alloc(spxListenFD, T_BIND, T_ALL)) == NULL ) {
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_EALLOC, M_EALLOC),
		"spxBound", t_errno) ;
	    fflush(PRIORITYERROR) ;
	}
		    
	rc = t_bind(spxListenFD, tbind, spxBound);
	if ( rc == 0 && spxBound->qlen == 0 )
	    {
	    /* this is occasional behaviour in SPX, and here's the workaround */
	    tbind->addr.len = spxBound->addr.len ;
	    memcpy(tbind->addr.buf, spxBound->addr.buf, spxBound->addr.len) ;
	    tbind->qlen = 5 ;

	    t_unbind(spxListenFD) ;
	    rc = t_bind(spxListenFD, tbind, spxBound);
	    }

	if ( rc == -1 ) {
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_EBIND, M_EBIND),
		"spxListenFD", t_errno) ;
	    fflush(PRIORITYERROR) ;
	    spxNetEntry = NULL ;	// forget about using spx
	} else {
		gotOne++ ;
#ifdef DEBUG
	logerror(PRIORITYWARN, "bound to spx listen address: %s\n",
		taddr2uaddr(spxNetEntry, &(spxBound->addr))) ;
	logerror(PRIORITYWARN, "\tqlen = %d\n", spxBound->qlen) ;
	fflush(PRIORITYWARN) ;
#endif
	}
	t_free((char *)tbind, T_BIND) ;
    }

    if ( tcpNetEntry ) {
	tcpListenFD = t_open(tcpNetEntry->nc_device, O_RDWR, (struct t_info *)NULL);
	if ( tcpListenFD < 0 ) {
		logerror(PRIORITYERROR,
		    catgets(mCat, MSG_SET, C_TOPEN, M_TOPEN),
		    tcpNetEntry->nc_device, t_errno, errno) ;
		fflush(PRIORITYERROR);
	    }

	if ( (tbind = (struct t_bind *)t_alloc(tcpListenFD, T_BIND, T_ALL)) == NULL ) {
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_EALLOC, M_EALLOC),
		"t_bind", t_errno) ;
	    fflush(PRIORITYERROR) ;
	} else {
	    tbind->qlen = 5 ;
	    tbind->addr.len = 0 ;
	}

	if ( (tcpBound = (struct t_bind *)t_alloc(tcpListenFD, T_BIND, T_ALL)) == NULL ) {
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_EALLOC, M_EALLOC),
		    "tcpBound", t_errno) ;
		fflush(PRIORITYERROR) ;
	}
		
	tbind->qlen = 5 ;
	rc = t_bind(tcpListenFD, tbind, tcpBound);
	if ( rc == -1 ) {
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_EBIND, M_EBIND),
		"tcpListenFD", t_errno) ;
	    fflush(PRIORITYERROR) ;
	    tcpNetEntry = NULL ;	// forget about using spx
	} else {
		gotOne++ ;

		// adjust sin.family for smdr 
		addrPtr = (struct sockaddr_in *)tcpBound->addr.buf ;
		addrPtr->sin_family = SwapUINT16(addrPtr->sin_family) ;
#ifdef DEBUG
	logerror(PRIORITYWARN, "bound to tcp listen address: %s\n",
		taddr2uaddr(tcpNetEntry, &(tcpBound->addr))) ;
	logerror(PRIORITYWARN, "\tqlen = %d\n", tcpBound->qlen) ;
	fflush(PRIORITYWARN) ;
#endif
	}

	t_free((char *)tbind, T_BIND) ;
    }

    if ( gotOne == 0 ) {
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_NOLISTEN, M_NOLISTEN)) ;
	    fflush(PRIORITYERROR) ;
	    exit(C_NOLISTEN) ;
    }
}

#ifdef SVR4
/***
 *
 *  name	sig_intr - allow signal to interrupt processing
 *
 ***/

void
sig_intr(int sig,...)
{
#ifdef DEBUG
    logerror(PRIORITYWARN, "<<INTERRUPT %d>>\n", sig) ;
    fflush(PRIORITYWARN) ;
#endif
    signalReceived = sig ;
}
#else
void sig_child(int sig,...)
{
	int pid ;
	union wait status ;

	sig = sig ; // Keep the compiler happy
	while ( (pid = wait3(&status,WNOHANG,(struct rusage *)0)) > 0 )
		;
}
#endif

/*
 * Detach from login session context
 */
void
daemonize(void)
{
	register int	fd ;
	pid_t			childpid ;

#ifdef SVR4
	struct sigaction	act ;
	int			open_max ;
#endif

	/*
	 * If we were started by init (process 1) from the /etc/inittab file
	 * there is no need to detach.
	 */
	if ( getppid() == 1 ) {
		goto  out ;
	}

#ifdef SVR4
	sigemptyset(&(act.sa_mask)) ;
	act.sa_handler = (void (*)()) SIG_IGN ;
	act.sa_flags = 0 ;
	sigaction(SIGTTOU, &act, NULL) ;
	sigaction(SIGTTIN, &act, NULL) ;
	sigaction(SIGTSTP, &act, NULL) ;
#else

#ifdef SIGTTOU
	signal(SIGTTOU,SIG_IGN);
#endif
#ifdef SIGTTIN
	signal(SIGTTIN,SIG_IGN);
#endif
#ifdef SIGTSTP
	signal(SIGTSTP,SIG_IGN);
#endif

#endif // #ifdef SVR4

	/*
	 * Allow parent shell to continue. Ensure the process is not a process
	 * group leader.
	 */
	if ( (childpid = fork()) < 0 ) {
		logerror(PRIORITYERROR,
		    catgets(mCat, MSG_SET, C_EFORK, M_EFORK), 1, errno) ;
		fflush(PRIORITYERROR);
		exit(C_EFORK);
	}
	else if ( childpid > 0 ) {
		exit(0);
	} /* parent exits */

	/* child */
	/* Diassociate from controlling terminal and process group
	 * Ensure the process can't acquire a new controlling terminal.
	 * This is done differently on BSD vs System V.
	 *
	 * BSD won't assign a new controlling terminal if process group is 
	 * non-zero.
	 * System V won't assign a controlling terminal if process is not a 
	 * process group leader.
	 */
#ifndef SYSV
	if ( setpgrp(0,getpid()) == -1 ) { /* change process group */
		logerror(PRIORITYERROR,
		    catgets(mCat, MSG_SET, C_EPGRP, M_EPGRP), errno) ;
		fflush(PRIORITYERROR);
		exit(C_EPGRP);
	}
	if ( (fd = open("/dev/tty",O_RDWR)) >= 0 ) {
		ioctl(fd,TIOCNOTTY,(char *)NULL); /* lose controlling TTY */
		close(fd);
	}
#else
	if ( setpgrp() == -1 ) { /*change process group & lose controlling TTY*/
		logerror(PRIORITYERROR,
		    catgets(mCat, MSG_SET, C_EPGRP, M_EPGRP), errno) ;
		fflush(PRIORITYERROR);
		exit(C_EPGRP);
	}
#endif

#ifdef SVR4
	sigemptyset(&(act.sa_mask)) ;
	act.sa_handler = (void (*)()) SIG_IGN ;
	act.sa_flags = 0 ;
	sigaction(SIGHUP, &act, NULL) ;
#else
	signal(SIGHUP,SIG_IGN); /* immune from pgrp leader death */
#endif

	if ((childpid = fork()) < 0 ) {
		logerror(PRIORITYERROR,
		    catgets(mCat, MSG_SET, C_EFORK, M_EFORK), 2, errno) ;
		fflush(PRIORITYERROR);
		exit(C_EFORK);
	}
	if ( childpid > 0 ) {  /* become non pgrp leader */
		exit(0);
	}
	
	/* Second Child */

out:
	/*
	 * Close all file descriptors
	 */
#ifdef SVR4
	open_max = (int) sysconf(_SC_OPEN_MAX) ;
	for ( fd = 0; fd < open_max; fd++ ) {
#else
	for ( fd = 0; fd < NOFILE; fd++ ) {
#endif
		close(fd);
	}
	errno = 0 ; /* might be set to EBADF from a close */

	/* Change current directory to root to make sure we are not on a 
	 * mounted file system
	 */
	chdir("/");

	/* clear any inherited file mode creation mask */
	umask(0);

	/* take care of death of child processes - make sure they won't
         * haunt by becoming zombies
	 * (note: this is the default in SVR4)
	 */
#ifdef SVR4
	sigemptyset(&(act.sa_mask)) ;
	act.sa_flags = SA_NOCLDWAIT ;
	act.sa_handler = (void (*)()) SIG_IGN ;
	sigaction(SIGCLD, &act, NULL) ;

	act.sa_handler = (void (*)()) sig_intr ;
	act.sa_flags = 0 ;
	sigaction(SIGHUP, &act, NULL) ;
#else
	signal(SIGCLD,SIG_IGN) ;
#endif
}

#ifdef SINGLE_THREAD
int acceptFD ;
#endif

void
processConnectionRequest(int listenFD, struct netconfig *netEntry)
{
	int			rc ;
	pid_t			pid ;
	int			fd ;
	static struct t_call	*callData = NULL ;
	int			acceptFD ;
	int			openMax ;
	struct serverData	*serverPtr ;
	struct nd_hostservlist	*hostData ;
	struct t_bind		*acceptData ;
	char			*argVector[16] ; //no more than 15 args
	int			argCount = 0 ;

#ifdef DEBUG
	logerror(PRIORITYWARN,"tsad: accepting connection request via %s\n", netEntry->nc_netid);
	fflush(PRIORITYWARN);
#endif

	if ( callData != NULL )
	    t_free( (char *)callData, T_CALL ) ;

	if ( (callData = (struct t_call *) t_alloc(listenFD, T_CALL, T_ALL)) == NULL ) {
	    logerror(PRIORITYERROR, 
		catgets(mCat, MSG_SET, C_EALLOC, M_EALLOC), "callData", t_errno) ;
	    fflush(PRIORITYERROR) ;
	    exit(C_EALLOC) ;
	    }

        rc = t_listen(listenFD,callData);
        if ( rc == -1 ) {
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_ELISTEN, M_ELISTEN), t_errno) ;
	    fflush(PRIORITYERROR);
	    t_close(listenFD);
	    exit(C_ELISTEN);
        }
 
        acceptFD = t_open(netEntry->nc_device, O_RDWR, (struct t_info *)NULL);
        if ( acceptFD == -1 ) {
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_TOPEN, M_TOPEN), "acceptFD", t_errno, errno) ;
	    fflush(PRIORITYERROR);
	    return ;
        }
 
	acceptData = (struct t_bind *) t_alloc(acceptFD, T_BIND, T_ALL) ;
        rc = t_bind(acceptFD, (struct t_bind *)NULL, acceptData);
 
        if ( rc == -1 ) {
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_EBIND, M_EBIND), "acceptFD", t_errno) ;
	    fflush(PRIORITYERROR);
	    t_close(acceptFD);
	    return ;
        }

        rc = t_accept(listenFD,acceptFD,callData);
        if ( rc == -1 ) {
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_EACCEPT, M_EACCEPT), t_errno) ;
	    fflush(PRIORITYERROR);
	    if ( t_errno == TLOOK )
		{
		// listenFD might be salvageable
		switch ( t_look(listenFD) ) {
		    case T_ORDREL :
		    case T_DISCONNECT :
			// client went away
			t_close(acceptFD);
			return ;

		    case T_LISTEN :
		    case T_CONNECT :
		    case T_DATA :
		    default :
			// consider listenFD to be corrupt
			;
		    }
		}

	    // if we get here, restart the process
	    rc = execvp(CommPath, Argv) ;
        }


	/*
	 * look up that machine in our list
	 */
	rc = netdir_getbyaddr(netEntry, &hostData, &(callData->addr)) ;
	serverPtr = serverList ;
	while ( serverPtr != NULL )
	    {
	    /* if ( strncmp(hostData->h_hostservs->h_host, serverPtr->serverName, strlen(serverPtr->serverName)) == 0 ) { */
	    if ( strcasecmp(hostData->h_hostservs->h_host, serverPtr->serverName) == 0 ) {
#ifdef DEBUG
		logerror(PRIORITYWARN,"tsad: requester identified as %s\n", hostData->h_hostservs->h_host) ;
		fflush(logfp);
#endif

		break ;
		}
	    else
		serverPtr = serverPtr->next ;
	    }

	// if server not in list, use the default
	if ( serverPtr == NULL )
	    serverPtr = defaultServer ;

	// log valid connection
	logerror(PRIORITYWARN, catgets(mCat, MSG_SET, C_CONNECT, M_CONNECT),
		hostData->h_hostservs->h_host, netEntry->nc_netid) ;
	fflush(PRIORITYWARN) ;

	pid = fork();
	if ( pid < 0 ) {
	    /*
	     * Can't fork for some reason, pause briefly and return
	     * Operator (on NW server) will have to try again
	     */
	    logerror(PRIORITYERROR,
		catgets(mCat, MSG_SET, C_EFORK, M_EFORK), errno) ;
	    fflush(PRIORITYERROR);
	    t_close(acceptFD);
	    sleep(2) ;
	    return ;
	}

#ifdef DEBUG
        logerror(PRIORITYWARN,"forked!\n");
	fflush(PRIORITYWARN);
#endif

	if ( pid == 0 ) {			// child
#ifdef SVR4
	    openMax = (int) sysconf(_SC_OPEN_MAX) ;
	    for ( fd = 0; fd < openMax; fd++ ) {
#else
	    for ( fd = 0; fd < NOFILE; fd++ ) {
#endif
		if ( fd != acceptFD )
		    close(fd);
	    }
	    errno = 0 ; /* might be set to EBADF from a close */
	    if ( acceptFD != 0 ) {
		rc = dup2(acceptFD,0);
		t_close(acceptFD);
	    }
	    rc = dup2(0,1);
	    rc = dup2(0,2);

/*
 * build arguments for tsaunix and exec
 */
	    // required arguments
	    argVector[argCount++] = tsaunixPath ;
	    argVector[argCount++] = "-l" ;
	    argVector[argCount++] = logFile ;

	    // optional arguments
	    if ( (serverPtr->tsaCodeset != NULL) && (serverPtr->tsaCodeset[0] != '\0') )
		{
		argVector[argCount++] = "-t" ;
		argVector[argCount++] = serverPtr->tsaCodeset ;
		}
	    if ( (serverPtr->engineCodeset != NULL) && (serverPtr->engineCodeset[0] != '\0') )
		{
		argVector[argCount++] = "-e" ;
		argVector[argCount++] = serverPtr->engineCodeset ;
		}
	    if ( (serverPtr->engineLocale != NULL) && (serverPtr->engineLocale[0] != '\0') )
		{
		argVector[argCount++] = "-L" ;
		argVector[argCount++] = serverPtr->engineLocale ;
		}

	    // null terminate argument list
	    argVector[argCount] = (char *)0 ;
	    execv(tsaunixPath,argVector) ;
	    exit(C_EXEC);
	}

#ifdef DEBUG
	logerror(PRIORITYWARN,
	     "Close the accepted socket and return to listen\n");
	fflush(PRIORITYWARN);
#endif

	t_close(acceptFD);
	sleep(30);   /* sleep for 30 secs. so that the child is all set up */
	return ;
}

/***
 *
 *  name	updateConfigFile
 *
 *  description	Reads configuration file information and stores it in
 *		the linked list pointed to by the global serverList.
 *		Maintains static information about the config file so
 *		that this is only done when the file has changed.
 *
 ***/

void
updateConfigFile()
{
    static int			found_something = 0 ;
    struct stat			fileStatus ;
    static time_t		lastUpdate = 0 ;
    struct serverData		*ptr, *scan ;
    struct tsaconfigent		*entry ;
    char			short_name[MAXHOSTNAMELEN] ;
    struct nd_hostserv		hostService ;
    char			*cptr ;

#ifdef DEBUG
    logerror(PRIORITYWARN, "in updateConfigFile\n") ;
    fflush(PRIORITYWARN) ;
#endif

    if ( (stat(configFile, &fileStatus) == 0) && (fileStatus.st_mtime > lastUpdate) ) {
	// configuration file found, read it in first
	lastUpdate = fileStatus.st_mtime ;

	/* erase old list */
	while ( serverList != NULL ) {
	    ptr = serverList->next ;
	    if ( serverList->myRequestData == 1 && serverList->requestData != NULL )
		free( serverList->requestData ) ;
	    free( serverList->serverName ) ;
	    free( serverList->tsaCodeset ) ;
	    free( serverList->engineCodeset ) ;
	    free( serverList->engineLocale ) ;
	    free( serverList ) ;
	    if ( serverList->serverAddress != NULL )
		netdir_free( serverList->serverAddress, ND_ADDRLIST ) ;
	    serverList = ptr ;
	}

	/* read in and initialize new list */
	settsaconfigent(configFile) ;
	hostService.h_serv = SERVICE_NAME ;	// needed later on
	while ( entry = gettsaconfigent() ) {
	    // see if we already have an entry for this machine
	    scan = serverList ;
	    while ( scan != NULL && (strcmp(scan->serverName, entry->hostName) != 0) && scan->protocol != 0 )
		scan = scan->next ;

	    // allocate a new entry in the list
	    if ( serverList == NULL ) {
		ptr = (struct serverData *) malloc(sizeof(struct serverData)) ;
		serverList = ptr ;
	    } else {
		ptr->next = (struct serverData *) malloc(sizeof(struct serverData)) ;
		ptr = ptr->next ;
	    }

	    if ( strcmp(entry->hostName, DEFAULT_ENTRY) == 0 )
		defaultServer = ptr ;

	    if ( ptr == NULL ) {
		logerror(PRIORITYERROR,
		    catgets(mCat, MSG_SET, C_MALLOC, M_MALLOC), errno) ;
		fflush(PRIORITYERROR);
		exit(C_MALLOC) ;
	    }

	    // make sure everything is nulled out to start with
	    (void) memset(ptr, '\0', sizeof(struct serverData)) ;

	    ptr->next = NULL ;
	    ptr->protocol = 0 ;
	    ptr->lastFailure = 0 ;
	    ptr->serverName = strdup(entry->hostName) ;
	    ptr->tsaCodeset = strdup(entry->tsaCodeset) ;
	    ptr->engineCodeset = strdup(entry->engineCodeset) ;
	    ptr->engineLocale = strdup(entry->engineLocale) ;

	    if ( ptr->serverName == NULL ||
			ptr->tsaCodeset == NULL ||
			ptr->engineCodeset == NULL ||
			ptr->engineLocale == NULL ) {
		logerror(PRIORITYERROR,
		    catgets(mCat, MSG_SET, C_MALLOC, M_MALLOC), errno) ;
		fflush(PRIORITYERROR);
		exit(C_MALLOC) ;
	    }

	    if ( spxNetEntry && strcmp(entry->protocol, spxNetEntry->nc_netid) == 0 ) {
		ptr->protocol |= SPX ;
		ptr->netData = spxNetEntry ;
	    } else if ( tcpNetEntry && strcmp(entry->protocol, tcpNetEntry->nc_netid) == 0 ) {
		ptr->protocol |= TCPIP ;
		ptr->netData = tcpNetEntry ;
	    } else {
		// entry identifies unsupported protocol, ignore it
		logerror(PRIORITYERROR,
		    catgets(mCat, MSG_SET, C_NOPROTOCOL, M_NOPROTOCOL),
		    entry->protocol, ptr->serverName) ;
		fflush(PRIORITYERROR) ;
		ptr->protocol = 0 ;
		ptr->netData = NULL ;
		ptr->requestData = NULL ;
		ptr->serverAddress = NULL ;
		continue ;
	    }

	    // get address information for this server in the correct format
	    hostService.h_host = ptr->serverName ;
	    if ( netdir_getbyname(ptr->netData, &hostService, &(ptr->serverAddress)) != 0 ) {
		ptr->serverAddress = NULL ;
		ptr->netData = NULL ;
		logerror(PRIORITYERROR,
		    catgets(mCat, MSG_SET, C_NOADDRESS, M_NOADDRESS),
		    SERVICE_NAME, ptr->serverName, entry->protocol) ;
		logerror(PRIORITYERROR, netdir_sperror()) ;
		logerror(PRIORITYERROR, "\n") ;
		fflush(PRIORITYERROR) ;
		continue ;
	    }
#ifdef DEBUG
	    logerror(PRIORITYWARN, "%s %s address: %s\n", ptr->serverName, ptr->netData->nc_netid, taddr2uaddr(ptr->netData, ptr->serverAddress->n_addrs)) ;
	    fflush(PRIORITYWARN) ;
#endif

	    // prepare for codeset translations
	    ConvertInitialize(entry->tsaCodeset, entry->engineCodeset) ;

	    // now initialize the request packet for this server
	    if ( scan != NULL && scan->requestData != NULL ) {
		// we've already got one for this server
		ptr->myRequestData = 0 ;
		ptr->requestData = scan->requestData ;
	    } else {
		ptr->myRequestData = 1 ;
		ptr->requestData = (struct requestPacket *)malloc(sizeof(struct requestPacket));
		if ( ptr->requestData == NULL ) {
		    logerror(PRIORITYERROR,
			catgets(mCat, MSG_SET, C_MALLOC, M_MALLOC), errno) ;
		    fflush(PRIORITYERROR);
		    exit(C_MALLOC) ;
		}
		memset((char *)ptr->requestData,'\0',sizeof(struct requestPacket));

#ifdef SYSV
		// this logic works both with and without DNS
		if ( host_data == NULL ) {
		    // hostname doesn't change so only get it once
		    (void) sysinfo(SI_HOSTNAME, short_name, MAXHOSTNAMELEN) ;
		    host_data = gethostbyname(short_name) ;
		}
		(void) strncpy(ptr->requestData->targetInfo.TSname, ConvertTsaToEngine(host_data->h_name, NULL), TSNAMELEN-1) ;

		if ( (cptr = strchr(ptr->requestData->targetInfo.TSname, '.')) != NULL )
		    *cptr = '\0' ;

#else
		gethostname(ptr->requestData->targetInfo.TSname,HOSTNAMELEN);
#endif

		strcpy(ptr->requestData->targetInfo.TStype,ConvertTsaToEngine(WORKSTATION_TYPE, NULL));
		strcpy(ptr->requestData->targetInfo.TSversion,ConvertTsaToEngine("1.13", NULL));

		// copy in our address data, both protocols
		if ( tcpNetEntry != NULL ) {
		    ptr->requestData->targetInfo.TCPaddressSize =
					    SwapUINT32(tcpBound->addr.len);
		    memcpy((char *)ptr->requestData->targetInfo.TCPaddress,
			    (char *)tcpBound->addr.buf, tcpBound->addr.len);
		    }

		if ( spxNetEntry != NULL ) {
		    ptr->requestData->targetInfo.SPXaddressSize =
					    SwapUINT32(spxBound->addr.len);
		    memcpy((char *)ptr->requestData->targetInfo.SPXaddress,
			    (char *)spxBound->addr.buf, spxBound->addr.len);
		    }
	    }

	    // have to switch here on protocol type requested -- rats!
	    if ( ptr->netData == tcpNetEntry ) {
		ptr->requestData->targetInfo.protocol |= SwapUINT32(TCPIP) ;
		ptr->requestData->targetInfo.preferredProtocol = SwapUINT32(TCPIP) ;
	    } else if ( ptr->netData == spxNetEntry ) {
		ptr->requestData->targetInfo.protocol |= SwapUINT32(SPX) ;
		if ( ptr->requestData->targetInfo.preferredProtocol == 0 )
		    ptr->requestData->targetInfo.preferredProtocol = SwapUINT32(SPX) ;
	    }

	found_something = 1 ;
	}	// while ( get tsa config file entry )
	endtsaent() ;
    }	// if ( config file changed )

    if ( found_something == 0 )
	{
	// no config entries or yp map, give up.
	logerror(PRIORITYERROR,
	    catgets(mCat, MSG_SET, C_NOCONFIG, M_NOCONFIG)) ;
	fflush(PRIORITYERROR);
	exit(C_NOCONFIG) ;
	}
    else if ( defaultServer == NULL )
	{
	// no default entry found, use first valid entry
	defaultServer = serverList ;
	}
}

/***
 *
 *  name	ReadDataBlock - read from a Fd using t_rcv
 *			calls t_rcv multiple times to get Count bytes if needed
 *			polls Fd for "connectTimeout" seconds to avoid blocking
 *
 ***/
int
ReadDataBlock( int Fd, int Count, char *Buffer )
    {
    int		byteCount, bytesRead ;
    int		rcode = 0 ;
    int		rcvFlags ;

    for (byteCount=0; byteCount<Count && rcode == 0; byteCount += bytesRead) {
	if ( PollTransportEndPoint(Fd, POLLIN, connectTimeout, 1) == -1 ) {
	    // server has died on us
	    rcode = C_SDIED ;
	    break ;
	    }

	bytesRead = t_rcv (Fd, &(Buffer[byteCount]), Count-byteCount, &rcvFlags) ;

	if ( bytesRead < 0 ) {
	    logerror(PRIORITYWARN,
		catgets(mCat, MSG_SET, C_TRCV, M_TRCV), t_errno, t_look(Fd)) ;
	    fflush(PRIORITYERROR) ;
	    rcode = C_TRCV ;
	    break ;
	    }
	}

    return(rcode) ;
    }

/***
 *
 *  name	WriteDataBlock - Write to a Fd using t_snd
 *			calls t_snd multiple times to write Count bytes if needed
 *			polls Fd for "connectTimeout" seconds to avoid blocking
 *
 ***/
int
WriteDataBlock( int Fd, int Count, char *Buffer )
    {
    int		byteCount, bytesWritten ;
    int		rcode = 0 ;

    for (byteCount=0; byteCount<Count && rcode == 0; byteCount += bytesWritten) {
	if ( PollTransportEndPoint(Fd, POLLOUT, connectTimeout, 1) == -1 ) {
	    // server has died on us
	    rcode = C_SDIED ;
	    break ;
	    }

	bytesWritten = t_snd(Fd, &(Buffer[byteCount]), Count-byteCount, 0) ;

	if ( bytesWritten < 0 ) {
	    logerror(PRIORITYWARN,
		catgets(mCat, MSG_SET, C_TSND, M_TSND), t_errno, t_look(Fd)) ;
	    fflush(PRIORITYERROR) ;
	    rcode = C_TSND ;
	    break ;
	    }
	}

    return(rcode) ;
    }

/***
 *
 *  name	TruncateFile - put limit on log file size
 *
 ***/

#define	TRUNC_BUFSIZE	128

void
TruncateFile(char *logFile, int maxSize)

    {
    struct stat			fileStatus ;
    FILE			*fpIn, *fpOut ;
    char			tmpFile[L_tmpnam] ;
    char			buffer[TRUNC_BUFSIZE] ;

    if ( (stat(logFile, &fileStatus) == 0) &&
	    (fileStatus.st_size > maxSize) &&
	    ((fpIn = fopen(logFile, "r")) != NULL) ) {

	sprintf(tmpFile, "/var/adm/tsa%d", getpid()) ;
	if ( (fpOut = fopen(tmpFile, "w")) == NULL )
	    return ;

	fseek(fpIn, fileStatus.st_size - maxSize, SEEK_SET) ;
	(void) fgets(buffer, TRUNC_BUFSIZE, fpIn) ; // skip portion of line

	// copy remainder of file one line at a time
	while ( fgets(buffer, TRUNC_BUFSIZE, fpIn) != NULL )
	    fputs(buffer, fpOut) ;

	// close files
	fclose(fpIn) ;
	fclose(fpOut) ;

	// shuffle things around
	chmod(tmpFile, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH) ;
	unlink(logFile) ;
	link(tmpFile, logFile) ;
	unlink(tmpFile) ;
	}
    }
