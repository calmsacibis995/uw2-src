/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libserver:server.c	1.8"
#include	<stdio.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<fcntl.h>
#include	<netdb.h>
#include	<malloc.h>
#include	<stropts.h>
#include	<poll.h>
#include	<tiuser.h>
#include	<netdir.h>
#include	<netconfig.h>
#include	<fcntl.h>
#include	<sys/utsname.h>
#include	<sys/syslog.h>
#include	<sys/conf.h>
#include	<mail/link.h>
#include	<mail/table.h>

#if	defined(X_SERVER)
#include	<X11/Intrinsic.h>
#endif

#define	SERVER_OBJ

typedef struct outmsg_s
    {
    void
	*om_link;

    char
	*om_msg;

    int
	om_length;
    }	outmsg_t;

typedef struct accept_s
    {
    void
	(*acc_readFunc)(),
	(*acc_freeFunc)(),
	*(*acc_acceptFunc)(),
	*acc_data;
    }	accept_t;

typedef struct connection_s
    {
    int
	conn_fd,
	conn_xmitLength,
	conn_inBuffSize;
    
    unsigned
	conn_ok		:1,
	conn_freeing	:1,
	conn_connect	:1,
	conn_writeable	:1,
	conn_readable	:1,
	conn_terminate	:1,
	conn_tirdwr	:1;

    char
	*conn_xmitPtr,
	*conn_inBuff;

    int
	conn_pollIndex;

    void
	(*conn_readFunc)(),
	(*conn_freeDataFunc)(),
	*conn_link,
	*conn_terminateLink,
	*conn_outgoing,
	*conn_data;
    
#if	defined(X_SERVER)
    XtInputId
	conn_inputReadId,		/* this is used by X-clients only */
	conn_inputWriteId;		/* this is used by X-clients only */
#endif

    struct netconfig
	*conn_nconf;
    }	connection_t;

typedef struct clientConnSetup_s
    {
    void
	(*ccs_callback)(),
	*ccs_handle,
	*ccs_localData,
	(*ccs_readFunc)(),
	(*ccs_freeDataFunc)();
    
    int
	ccs_reservedPort,
	ccs_nextAddrIndex,
	ccs_fd;

    struct netconfig
	*ccs_netconfig;

    struct nd_addrlist
	*ccs_addrlist;
    
    struct t_call
	*ccs_call;
    
    struct nd_hostserv
	ccs_hostserv;

    connection_t
	*ccs_conn;
    }	clientConnSetup_t;

#include	<mail/server.h>

extern int
    t_errno;

#if	defined(X_SERVER)
static XtAppContext
    X_applicationContext;
#endif

static int
    Done = 0,
    ConnectionCount = 0;

static struct nd_hostserv
    *connGetHostByAddr(struct netbuf *, struct netconfig *);

static clientConnSetup_t
    *ccsNextAddress(clientConnSetup_t *),
    *ccsNextConfig(clientConnSetup_t *);

static struct pollfd
    *PollEventList = NULL;

static void
    connFree(connection_t *conn_p),
    connAccept();

static int
    N_PollEvents = 0;

static FILE
    *Ddt = stderr;

static int
    Debug = 0;

char
    *strtok();

static void
    *AllConnectionList,
    ccsReady(),
    ccsRcvConnect(),
#if	!defined(X_SERVER)
    *TerminateWaitList;
#else
    connXReadCallback(connection_t *, int, void *),
    connXWriteCallback(connection_t *, int, void *);
#endif

static void
    outmsgFree(outmsg_t *msg_p)
	{
	if(Debug > 4) (void) fprintf(stderr, "outmsgFree(0x%x) Exited.\n", (int)msg_p);

	if(msg_p != NULL)
	    {
	    if(msg_p->om_link != NULL) linkFree(msg_p->om_link);
	    if(msg_p->om_msg != NULL) free(msg_p->om_msg);
	    free(msg_p);
	    }

	if(Debug > 4) (void)fprintf(stderr, "outmsgFree() Exited.\n");
	}

static outmsg_t
    *outmsgNew(char *data, int length, void *list)
	{
	outmsg_t
	    *result;
	
	if(Debug > 4)
	    {
	    (void)fprintf
		(
		stderr,
		"outmsgNew(0x%x, %d, 0x%x) Entered.\n",
		(int)data,
		length,
		(int)list
		);
	    }

	if((result = (outmsg_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    }
	else if((result->om_msg = malloc(length)) == NULL)
	    {
	    outmsgFree(result);
	    result = NULL;
	    }
	else if((result->om_link = linkNew(result)) == NULL)
	    {
	    outmsgFree(result);
	    result = NULL;
	    }
	else
	    {
	    (void)memcpy(result->om_msg, data, length);
	    result->om_length = length;
	    if(list != NULL) (void)linkAppend(list, result->om_link);
	    }

	if(Debug > 4)
	    {
	    (void) fprintf(stderr, "outmsgNew() = 0x%x Exited.\n", (int)result);
	    }

	return(result);
	}

#if	!defined(X_SERVER)
static void
    printPollEventList(struct pollfd *list, int count)
	{
	while(count-- > 0)
	    {
	    if(list->fd >= 0)
		{
		(void) fprintf
		    (
		    stderr,
		    "fd = %d: events = 0x%x: revents = 0x%x\n",
		    list->fd,
		    (int)list->events,
		    (int)list->revents
		    );
		}
	    
	    list++;
	    }
	}
#endif

static int
    pollfdGet(int fd)
	{
	struct pollfd
	    *curPollEvent_p;

	int
	    result;
	
	for
	    (
	    curPollEvent_p = PollEventList,
		result = 0;
	    result < N_PollEvents && curPollEvent_p->fd >= 0;
	    curPollEvent_p++,
		result++
	    );

	if(result < N_PollEvents)
	    {
	    curPollEvent_p->fd = fd;
	    }
	else if
	    (
		!(curPollEvent_p = (struct pollfd*)realloc
		    (
		    PollEventList,
		    (N_PollEvents + 16)*sizeof(*PollEventList)
		    )
		)
	    )
	    {
	    result = -1;
	    }
	else
	    {
	    PollEventList = curPollEvent_p;
	    for
		(
		result = N_PollEvents,
		    curPollEvent_p = PollEventList + result,
		    N_PollEvents +=16;
		result < N_PollEvents - 1;
		result++,
		    curPollEvent_p++
		)
		{
		curPollEvent_p->fd = -1;
		curPollEvent_p->events = 0;
		curPollEvent_p->revents = 0;
		}

	    curPollEvent_p->fd = fd;
	    curPollEvent_p->events = 0;
	    curPollEvent_p->revents = 0;
	    }

	return(result);
	}

static void
    pollfdFree(int index)
	{
	struct pollfd
	    *curPollEvent_p = &PollEventList[index];

	curPollEvent_p->fd = -1;
	curPollEvent_p->events = 0;
	curPollEvent_p->revents = 0;
	}

connection_t
    *connNew
	(
	int fd,
	struct netconfig *nconf,
	void (*readFunc)(),
	void (*freeDataFunc)(),
	void *data
	)
	{
	connection_t
	    *result;
	
	char
	    modName[FMNAMESZ + 1];

	if(Debug)
	    {
	    (void)fprintf
		(
		stderr,
		"connNew(%d, 0x%x, 0x%x, 0x%x, 0x%x) entered.\n",
		fd,
		(int) nconf,
		(int) readFunc,
		(int) freeDataFunc,
		(int) data
		);
	    }

	ConnectionCount++;
	if((result = (connection_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    ConnectionCount--;
	    }
	else if((result->conn_terminateLink = linkNew(result)) == NULL)
	    {
	    result->conn_fd = -1;
	    connFree(result);
	    result = NULL;
	    }
	else if((result->conn_link = linkNew(result)) == NULL)
	    {
	    result->conn_fd = -1;
	    connFree(result);
	    result = NULL;
	    }
	else if((result->conn_outgoing = linkNew(NULL)) == NULL)
	    {
	    result->conn_fd = -1;
	    connFree(result);
	    result = NULL;
	    }
	else if((result->conn_pollIndex = pollfdGet(fd)) < 0)
	    {
	    result->conn_fd = -1;
	    connFree(result);
	    result = NULL;
	    }
	else
	    {
	    result->conn_fd = fd;
	    result->conn_freeDataFunc = freeDataFunc;
	    result->conn_readFunc = readFunc;
	    result->conn_data = data;
	    result->conn_terminate = 0;
	    result->conn_writeable = 0;
	    result->conn_connect = 0;
	    result->conn_freeing = 0;
	    result->conn_xmitPtr = NULL;
	    result->conn_inBuffSize = 0;
	    result->conn_inBuff = malloc(result->conn_inBuffSize);
	    result->conn_nconf = nconf;
	    if(ioctl(result->conn_fd, I_LOOK, modName) < 0)
		{
		result->conn_tirdwr = 0;
		if(Debug) (void)fprintf(stderr, "\tNo Modules pushed.\n");
		}
	    else if(!strcmp(modName, "ttcompat"))
		{
		result->conn_tirdwr = 1;
		if(Debug) (void)fprintf(stderr, "\t%s Pushed.\n", modName);
		}
	    else if(!strcmp(modName, "tirdwr"))
		{
		result->conn_tirdwr = 1;
		if(Debug) (void)fprintf(stderr, "\t%s Pushed.\n", modName);
		}
	    else
		{
		result->conn_tirdwr = 0;
		if(Debug) (void)fprintf(stderr, "\t%s Pushed.\n", modName);
		}

	    result->conn_ok = 1;
	    (void)linkAppend(AllConnectionList, result->conn_link);
	    PollEventList[result->conn_pollIndex].events |= POLLRDNORM | POLLRDBAND | POLLPRI | POLLIN;
#if	defined(X_SERVER)
	    result->conn_inputReadId = XtAppAddInput
		(
		X_applicationContext,
		result->conn_fd,
		XtInputReadMask,
		connXReadCallback,
		result
		);

	    result->conn_readable = 1;
	    result->conn_inputWriteId = 0;
#endif
	    }

	if(Debug)
	    {
	    (void)fprintf
		(
		stderr,
		"connNew() = 0x%x Exited.\n",
		(int) result
		);
	    }

	return(result);
	}

void
    connTerminate(connection_t *conn_p)
	{
	if(Debug)
	    {
	    (void)fprintf
		(
		stderr,
		"connTerminate(0x%x) entered.\n",
		(int)conn_p
		);
	    }

	if(conn_p->conn_writeable)
	    {
	    conn_p->conn_terminate = 1;
	    }
	else if(conn_p->conn_terminateLink != NULL)
	    {
#if	defined(X_SERVER)
	    connFree(conn_p);
#else
	    (void)linkRemove(conn_p->conn_terminateLink);
	    (void)linkAppend
		(
		TerminateWaitList,
		conn_p->conn_terminateLink
		);
#endif
	    }
	else
	    {
	    connFree(conn_p);
	    }

	if(Debug) (void)fprintf(stderr, "connTerminate() Exited.\n");
	}

static void
    connFree(connection_t *conn_p)
	{
	void
	    *link_p;

	if(Debug) (void)fprintf(stderr, "connFree(0x%x) entered.\n", (int)conn_p);
	if(conn_p == NULL)
	    {
	    }
	else if(!conn_p->conn_ok)
	    {
	    if(Debug)
		{
		(void)fprintf
		    (
		    stderr,
		    "\tERROR!!!!! FREEING BAD CONNECTION!!!\n"
		    );
		}
	    }
	else if(conn_p->conn_freeing)
	    {
	    }
	else
	    {
	    conn_p->conn_freeing = 1;
	    ConnectionCount--;
	    if(conn_p->conn_terminateLink != NULL)
		{
		linkFree(conn_p->conn_terminateLink);
		conn_p->conn_terminateLink = NULL;
		}

	    if(conn_p->conn_link != NULL)
		{
		linkFree(conn_p->conn_link);
		conn_p->conn_link = NULL;
		}

	    if(conn_p->conn_outgoing != NULL)
		{
		while
		    (
			(
			link_p = linkNext(conn_p->conn_outgoing)
			) != conn_p->conn_outgoing
		    )
		    {
		    if(Debug)
			{
			(void) fprintf
			    (
			    Ddt,
			    "link_p = 0x%x, conn_outgoing = 0x%x.\n",
			    (int)link_p,
			    (int)conn_p->conn_outgoing
			    );
			}

		    outmsgFree(linkOwner(link_p));
		    }

		linkFree(conn_p->conn_outgoing);
		conn_p->conn_outgoing = NULL;
		}

#if	1
#if	defined(X_SERVER)
	    if(conn_p->conn_readable)
		{
		XtRemoveInput(conn_p->conn_inputReadId);
		}

	    if(conn_p->conn_writeable)
		{
		XtRemoveInput(conn_p->conn_inputWriteId);
		}
#endif

	    t_close(conn_p->conn_fd);
#endif

	    if(conn_p->conn_freeDataFunc != NULL)
		{
		(conn_p->conn_freeDataFunc)(conn_p->conn_data);
		}

	    if(conn_p->conn_pollIndex >= 0) pollfdFree(conn_p->conn_pollIndex);
	    if(Debug)
		{
		(void)fprintf(stderr, "Closing fd %d.\n", conn_p->conn_fd);
		}

#if 0
#if	defined(X_SERVER)
	    if(conn_p->conn_readable)
		{
		XtRemoveInput(conn_p->conn_inputReadId);
		}

	    if(conn_p->conn_writeable)
		{
		XtRemoveInput(conn_p->conn_inputWriteId);
		}
#endif

	    t_close(conn_p->conn_fd);
#endif
	    conn_p->conn_freeing = 0;
	    conn_p->conn_ok = 0;
	    free(conn_p);
	    }

	if(Debug) (void)fprintf(stderr, "connFree(0x%x) done.\n", (int)conn_p);
	}

void
    connSendBinary(connection_t *conn_p, char *data, int length)
	{
	if(outmsgNew(data, length, conn_p->conn_outgoing) == NULL)
	    {
	    /* No Memory */
	    }
	else
	    {
	    PollEventList[conn_p->conn_pollIndex].events |= POLLWRNORM | POLLWRBAND;
#if	defined(X_SERVER)
	    if(!conn_p->conn_writeable)
		{
		conn_p->conn_inputWriteId = XtAppAddInput
		    (
		    X_applicationContext,
		    conn_p->conn_fd,
		    XtInputWriteMask,
		    connXWriteCallback,
		    conn_p
		    );

		}
#endif
	    conn_p->conn_writeable = 1;
	    }
	}

void
    connSend(connection_t *conn_p, char *string)
	{
	if(Debug)
	    {
	    (void)fprintf
		(
		stderr,
		"connSend(0x%x, %s) entered.\n",
		(int)conn_p,
		string
		);
	    }

	if(outmsgNew(string, strlen(string), conn_p->conn_outgoing) == NULL)
	    {
	    /* No Memory */
	    }
	else
	    {
	    PollEventList[conn_p->conn_pollIndex].events |= POLLWRNORM | POLLWRBAND;
#if	defined(X_SERVER)
	    if(!conn_p->conn_writeable)
		{
		conn_p->conn_inputWriteId = XtAppAddInput
		    (
		    X_applicationContext,
		    conn_p->conn_fd,
		    XtInputWriteMask,
		    connXWriteCallback,
		    conn_p
		    );

		}
#endif
	    conn_p->conn_writeable = 1;
	    }

	if(Debug) (void)fprintf(stderr, "connSend() done.\n");
	}

#if 0
static void
    freeListen(accept_t *accept_p)
	{
	void
	    (*freeFunc)();

	freeFunc = accept_p->acc_freeFunc;
	free(accept_p);

	if(freeFunc != NULL)
	    {
	    freeFunc(NULL);
	    }
	}
#endif
int
    connNewListener
	(
	char *service,
	int length,
	void *(*acceptFunc)(),
	void (*readFunc)(),
	void (*freeFunc)(),
	void *data
	)
	{
	int
	    result = -1,
	    i,
	    fd;
	
	accept_t
	    *accept_p;

	struct nd_addrlist
	    *addrs;

	struct nd_hostserv
	    hostserv;
	
	struct t_bind
	    *bind_in,
	    *bind_out;

	struct netbuf
	    *curAddr_p;

	struct netconfig
	    *nconf;

	void
	    *handle_p;

	hostserv.h_serv = service;
	hostserv.h_host = HOST_SELF;

	if((handle_p = setnetpath()) == NULL)
	    {
	    /*nc_perror("setnetpath");*/
	    exit(2);
	    }

	while((nconf = getnetpath(handle_p)) != NULL)
	    {
	    if(nconf->nc_semantics == NC_TPI_CLTS)
		{
		}
	    else if((netdir_getbyname(nconf, &hostserv, &addrs)))
		{
		if(Debug)
		    {
		    (void)fprintf
			(
			stderr,
			"Service %s not available for transport %s.\n",
			service,
			nconf->nc_netid
			);
		    }
		}
	    else if
		(
		    (
		    fd = t_open
			(
			nconf->nc_device,
			O_RDWR | O_NDELAY | O_NONBLOCK,
			NULL
			)
		    ) < 0
		)
		{
		t_error("t_open failed");
		netdir_free(addrs, ND_ADDRLIST);
		}
	    else if(fcntl(fd, F_SETFD, 1) == -1)
		{
		/*perror("fcntl failed.");*/
		netdir_free(addrs, ND_ADDRLIST);
		}
	    else if
		(
		    (
		    bind_in = (struct t_bind *)t_alloc(fd, T_BIND, T_ALL)
		    ) == NULL
		)
		{
		t_error("t_alloc of t_bind structure failed");
		t_close(fd);
		netdir_free(addrs, ND_ADDRLIST);
		}
	    else if
		(
		    (
		    bind_out = (struct t_bind *)t_alloc(fd, T_BIND, T_ALL)
		    ) == NULL
		)
		{
		t_error("t_alloc of t_bind structure failed");
		t_close(fd);
		netdir_free(addrs, ND_ADDRLIST);
		}
	    else for
		(
		curAddr_p = addrs->n_addrs,
		    i = addrs->n_cnt;
		i > 0;
		curAddr_p++,
		    i--
		)
		{
		bind_in->qlen = length;
		bind_in->addr.len = curAddr_p->len;
		(void)memcpy(bind_in->addr.buf, curAddr_p->buf, curAddr_p->len);
		if(t_bind(fd, bind_in, bind_out))
		    {
		    t_error("t_bind failed");
		    t_close(fd);
		    }
		else if
		    (
		    memcmp
			(
			bind_in->addr.buf,
			bind_out->addr.buf,
			bind_in->addr.len
			)
		    )
		    {
		    if(Debug)
			{
			(void)fprintf(stderr, "Address does not match.\n");
			}
		    }
		else if((accept_p = (accept_t *)malloc(sizeof(*accept_p))) == NULL)
		    {
		    if(Debug)
			{
			(void)fprintf(stderr, "No space for accept structure.\n");
			}

		    t_close(fd);
		    netdir_free(addrs, ND_ADDRLIST);
		    break;
		    }
		else
		    {
		    accept_p->acc_readFunc = readFunc;
		    accept_p->acc_acceptFunc = acceptFunc;
		    accept_p->acc_freeFunc = freeFunc;
		    accept_p->acc_data = data;
		    if(Debug > 5)
			{
			(void)fprintf
			    (
			    stderr,
			    "\tq_len returned = %d.\n",
			    bind_out->qlen
			    );
			}

		    if
			(
			connNew
			    (
			    fd,
			    nconf,
			    connAccept,
			    free,
			    accept_p
			    ) == NULL
			)
			{
			if(Debug) (void)fprintf(stderr, "connNew failed.\n");
			t_close(fd);
			free(accept_p);
			}
		    else
			{
			result = 0;
			}

		    netdir_free(addrs, ND_ADDRLIST);
		    break;
		    }
		}
	    }

	return(result);
	}

#if	defined(X_SERVER)
void
    connSetApplicationContext(XtAppContext appContext)
	{
	X_applicationContext = appContext;
	}
#endif

int
    connInit(int debugLevel)
	{
	static int
	    initialized = 0;

	struct pollfd
	    *curPollEvent_p;

	/*
	    Initialize the debug message level.
	*/
	Debug = debugLevel;

	if(!initialized)
	    {
	    /*
		Initialize the data structures.
	    */
#if	!defined(X_SERVER)
	    TerminateWaitList = linkNew(NULL);
#endif
	    AllConnectionList = linkNew(NULL);

	    PollEventList = (struct pollfd *)calloc(16, sizeof(*PollEventList));
	    N_PollEvents = 16;
	    for
		(
		curPollEvent_p = PollEventList + N_PollEvents;
		curPollEvent_p-- > PollEventList;
		)
		{
		curPollEvent_p->fd = -1;
		}

	    /*
		Data structures initialized.
	    */
	    }

	return(0);
	}

static void
    connReadConnection(connection_t *conn_p)
	{
	int
	    type,
	    nbytes = 0,
	    bytesToRcv,
	    flags;

	char
	    *curBuff_p;

	switch(type = t_look(conn_p->conn_fd))
	    {
	    default:
		{
		if(Debug)
		    {
		    (void)fprintf
			(
			Ddt,
			"t_look(%d) = other(%d).\n",
			conn_p->conn_fd,
			type
			);
		    }

		break;
		}

	    case	-1:
	    case	T_DATA:
	    case	T_EXDATA:
		{
		if(Debug) (void)fprintf(Ddt, "t_look(%d) = T_DATA.\n", conn_p->conn_fd);
		if(conn_p->conn_inBuffSize > 256)
		    {
		    }
		else if
		    (
			(
			conn_p->conn_inBuff = realloc
			    (
			    conn_p->conn_inBuff,
			    conn_p->conn_inBuffSize = 256
			    )
			) == NULL
		    )
		    {
		    /* ERROR No Memory */
		    conn_p->conn_inBuffSize = 0;
		    break;
		    }

		if(conn_p->conn_tirdwr)
		    {
		    if
			(
			    (
			    nbytes = read
				(
				conn_p->conn_fd,
				conn_p->conn_inBuff,
				conn_p->conn_inBuffSize
				)
			    )< 0
			)
			{
			}
		    else
			{
			}
		    }
		else
		    {
		    for
			(
			bytesToRcv = conn_p->conn_inBuffSize,
			    curBuff_p = conn_p->conn_inBuff,
			    nbytes = 0,
			    flags = T_MORE;
			flags & T_MORE;
			bytesToRcv = conn_p->conn_inBuffSize - nbytes,
			    curBuff_p = conn_p->conn_inBuff + nbytes
			)
			{
			nbytes += t_rcv
			    (
			    conn_p->conn_fd,
			    curBuff_p,
			    bytesToRcv - 1,
			    &flags
			    );

			if(!(flags & T_MORE))
			    {
			    }
			else if
			    (
				(
				conn_p->conn_inBuff = realloc
				    (
				    conn_p->conn_inBuff,
				    conn_p->conn_inBuffSize += 256
				    )
				) == NULL
			    )
			    {
			    conn_p->conn_inBuffSize = 0;
			    nbytes = 0;
			    flags = 0;
			    }
			}
		    }

		if(conn_p->conn_inBuff == NULL)
		    {
		    break;
		    }

		if(nbytes >= 0)
		    {
		    conn_p->conn_inBuff[nbytes] = '\0';
		    }

		if(Debug)
		    {
		    if(nbytes <= 0)
			{
			/*perror("No Data");*/
			if(Debug)
			    {
			    (void)fprintf
				(
				Ddt,
				"errno = %d.\n",
				errno
				);
			    }
			}
		    else if(Debug)
			{
			(void)fprintf
			    (
			    Ddt,
			    "%d bytes, fd(%d) = %s",
			    nbytes,
			    conn_p->conn_fd,
			    conn_p->conn_inBuff
			    );
			}
		    }

		/* Deliberate fall through */
		}
	    
	    case	T_LISTEN:
		{
		if(Debug) (void)fprintf(Ddt, "t_look(%d) = T_LISTEN.\n", conn_p->conn_fd);
		(conn_p->conn_readFunc)
		    (
		    conn_p,
		    conn_p->conn_data,
		    conn_p->conn_inBuff,
		    nbytes
		    );

		break;
		}

	    case	T_ORDREL:
		{
		if(Debug)
		    {
		    (void)fprintf
			(
			Ddt,
			"t_look(%d) = T_ORDREL.\n",
			conn_p->conn_fd
			);
		    }

		t_rcvrel(conn_p->conn_fd);
		connTerminate(conn_p);
		break;
		}

	    case	T_DISCONNECT:
		{
		if(Debug)
		    {
		    (void)fprintf
			(
			Ddt,
			"t_look(%d) = T_DISCONNECT.\n",
			conn_p->conn_fd
			);
		    }

		if(conn_p->conn_connect)
		    {
		    (void)ccsNextAddress(conn_p->conn_data);
		    }
		else
		    {
		    connTerminate(conn_p);
		    }

		break;
		}
	    
	    case	T_CONNECT:
		{
		if(Debug)
		    {
		    (void)fprintf
			(
			Ddt,
			"t_look(%d) = T_CONNECT.\n",
			conn_p->conn_fd
			);
		    }

		if(conn_p->conn_connect)
		    {
		    ccsRcvConnect(conn_p->conn_data);
		    ccsReady(conn_p->conn_data);
		    }

		break;
		}
	    }
	}

static void
    connWriteConnection(connection_t *conn_p)
	{
	void
	    *curLink_p;

	int
	    byteCount;

	outmsg_t
	    *msg_p;

	if(Debug > 1)
	    {
	    (void)fprintf
		(
		Ddt,
		"Descriptor %d is writeable.\n",
		conn_p->conn_fd
		);
	    }

	curLink_p = linkNext(conn_p->conn_outgoing);
	if(conn_p->conn_xmitPtr != NULL)
	    {
	    }
	else if((msg_p = (outmsg_t *)linkOwner(curLink_p)) == NULL)
	    {
	    }
	else if((conn_p->conn_xmitPtr = msg_p->om_msg) == NULL)
	    {
	    /* Should never happen. */
	    outmsgFree(msg_p);
	    }
	else
	    {
	    conn_p->conn_xmitLength = msg_p->om_length;
	    }

	if(conn_p->conn_xmitPtr == NULL)
	    {
	    conn_p->conn_writeable = 0;
	    PollEventList[conn_p->conn_pollIndex].events &= ~(POLLWRNORM | POLLWRBAND);

#if	defined(X_SERVER)
	    if(conn_p->conn_inputWriteId)
		{
		XtRemoveInput(conn_p->conn_inputWriteId);
		}
#endif

	    if(conn_p->conn_terminate)
		{
		connTerminate(conn_p);
		}
	    }
	else if
	    (
		(
		byteCount = write
		    (
		    conn_p->conn_fd,
		    conn_p->conn_xmitPtr,
		    conn_p->conn_xmitLength
		    )
		) < 0
	    )
	    {
	    }
	else if((conn_p->conn_xmitLength -= byteCount) > 0)
	    {
	    conn_p->conn_xmitPtr += byteCount;
	    if(Debug > 5)
		{
		(void)fprintf
		    (
		    Ddt,
		    "Wrote %d bytes %d bytes left.\n",
		    byteCount,
		    conn_p->conn_xmitLength
		    );
		}

	    }
	else if((msg_p = (outmsg_t *)linkOwner(curLink_p)) != NULL)
	    {
	    conn_p->conn_xmitPtr = NULL;
	    outmsgFree(msg_p);
	    if(Debug > 5)
		{
		(void)fprintf
		    (
		    Ddt,
		    "Wrote %d bytes %d bytes left. Done.\n",
		    byteCount,
		    conn_p->conn_xmitLength
		    );
		}
	    }
	}

#if	!defined(X_SERVER)
void
    connStopLoop()
	{
	Done = 1;
	}

void
    connMainLoop()
	{
	struct pollfd
	    *curPollEvent_p;

	connection_t
	    *conn_p;

	int
	    n;

	/*
	    Main Service Loop.
	*/
	while(!Done || ConnectionCount)
	    {
	    if
		(
		    (
		    conn_p = (connection_t *)linkOwner
			(
			linkNext(TerminateWaitList)
			)
		    ) != NULL
		)
		{
		connFree(conn_p);
		continue;
		}
	    

	    if(0)
		{
	        printPollEventList(PollEventList, N_PollEvents);
		}

	    n = poll(PollEventList, N_PollEvents, -1);

	    if(Debug)
		{
		(void)fprintf(stderr, "Poll returned %d.\n", n);
		}

	    if(n == 0)
		{
		}
	    else switch(errno)
		{
		case	EINTR:
		   {
/*
		   syslog(LOG_WARNING, "poll: %m\n");
*/
		   if(Debug) (void)fprintf(Ddt, "Interrupted.\n");
		   break;
		   }
		}

	    for
		(
		conn_p = (connection_t *)linkOwner(linkNext(AllConnectionList));
		n > 0 && conn_p != NULL;
		conn_p = (connection_t *)linkOwner(linkNext(conn_p->conn_link))
		)
		{
		if(Debug > 1)
		    {
		    (void)fprintf(Ddt, "checking fd %d.\n", conn_p->conn_fd);
		    }

		curPollEvent_p = &PollEventList[conn_p->conn_pollIndex];

		if(curPollEvent_p->revents & POLLWRNORM)
		    {
		    connWriteConnection(conn_p);
		    }

		if(curPollEvent_p->revents & (POLLRDNORM))
		    {
		    connReadConnection(conn_p);
		    n--;
		    }
		}
	    }

	Done = 0;
	}

#else
static void
    connXReadCallback(connection_t *conn_p, int fd, void *ignore)
	{
	connReadConnection(conn_p);
	}

static void
    connXWriteCallback(connection_t *conn_p, int fd, void *ignore)
	{
	connWriteConnection(conn_p);
	}
#endif

typedef struct inData_s
    {
    void
	*id_link;
    
    struct t_call
	*id_call;

    int
	id_fd;
    }	inData_t;

static void
    inDataFree(inData_t *inData_p)
	{
	if(inData_p != NULL)
	    {
	    if(inData_p->id_link != NULL) linkFree(inData_p->id_link);
	    if(inData_p->id_call != NULL) t_free((char *)inData_p->id_call, T_CALL);
	    if(inData_p->id_fd > 0) t_close(inData_p->id_fd);
	    free(inData_p);
	    }
	}

static inData_t
    *inDataNew(connection_t *conn_p, void *list)
	{
	inData_t
	    *result;

	if((result = calloc(sizeof(*result), 1)) == NULL)
	    {
	    }
	else if
	    (
		(
		result->id_call = (struct t_call *)t_alloc(conn_p->conn_fd, T_CALL, T_ALL)
		) == NULL
	    )
	    {
	    inDataFree(result);
	    result = NULL;
	    }
	else if(t_listen(conn_p->conn_fd, result->id_call) < 0)
	    {
	    inDataFree(result);
	    result = NULL;
	    }
	else if((result->id_link = linkNew(result)) == NULL)
	    {
	    inDataFree(result);
	    result = NULL;
	    }
	else if
	    (
		(
		result->id_fd = t_open
		    (
		    conn_p->conn_nconf->nc_device,
		    O_RDWR | O_NDELAY | O_NONBLOCK,
		    NULL
		    )
		) < 0
	    )
	    {
	    inDataFree(result);
	    result = NULL;
	    }
	else if(fcntl(result->id_fd, F_SETFD, 1) == -1)
	    {
	    inDataFree(result);
	    result = NULL;
	    }
	else if(t_bind(result->id_fd, NULL, NULL) < 0)
	    {
	    t_error("t_bind failed for responding fd.");
	    inDataFree(result);
	    result = NULL;
	    }
	else if(list != NULL)
	    {
	    linkAppend(list, result->id_link);
	    }

	return(result);
	}

static void
    connAccept(connection_t *conn, accept_t *accept_p, char *buffer, int nbytes)
	{
	connection_t
	    *newConn_p;

	inData_t
	    *inData_p;

	static void
	    *incomingConList = NULL;
	
	if(Debug)
	    {
	    (void)fprintf(stderr, "connAccept() Entered.\n");
	    }

	if(incomingConList == NULL) incomingConList = linkNew(NULL);

	do
	    {
	    while(inDataNew(conn, incomingConList) != NULL);

	    while
		(
		    (
		    inData_p = (inData_t *)linkOwner(linkNext(incomingConList))
		    ) != NULL
		)
		{
		if
		    (
		    t_accept
			(
			conn->conn_fd,
			inData_p->id_fd,
			inData_p->id_call
			) < 0
		    )
		    {
		    if(t_errno != TLOOK)
			{
			}
		    else
			{
			char
			    *stateStr;

			int
			    doBreak = 0;

			switch(t_getstate(conn->conn_fd))
			    {
			    case T_UNBND:
				{
				stateStr = "T_UNBND";
				break;
				}

			    case T_IDLE:
				{
				stateStr = "T_IDLE";
				break;
				}

			    case T_OUTCON:
				{
				stateStr = "T_OUTCON";
				break;
				}

			    case T_INCON:
				{
				stateStr = "T_INCON";
				doBreak = 1;
				break;
				}

			    case T_DATAXFER:
				{
				stateStr = "T_DATAXFER";
				break;
				}

			    case T_OUTREL:
				{
				stateStr = "T_OUTREL";
				break;
				}

			    case T_INREL:
				{
				stateStr = "T_INREL";
				t_rcvrel(conn->conn_fd);
				break;
				}
			    }
			
			if(doBreak) break;

			fprintf(stderr, "State = %s.\n", stateStr);
			}
		    }
		else if
		    (
			(
			newConn_p = connNew
			    (
			    inData_p->id_fd,
			    conn->conn_nconf,
			    accept_p->acc_readFunc,
			    accept_p->acc_freeFunc,
			    NULL
			    )
			) == NULL
		    )
		    {
		    inDataFree(inData_p);
		    }
		else
		    {
		    newConn_p->conn_data = (accept_p->acc_acceptFunc)
			(
			newConn_p,
			inData_p->id_call,
			connGetHostByAddr
			    (
			    &inData_p->id_call->addr,
			    conn->conn_nconf
			    ),
			accept_p->acc_data
			);

		    inData_p->id_fd = -1;
		    inData_p->id_call = NULL;
		    inDataFree(inData_p);
		    }
		}
	    }
	while(inData_p != NULL);
	}

static struct nd_hostserv
    *connGetHostByAddr(struct netbuf *addr, struct netconfig *config)
	{
	static struct nd_hostserv
	    result;
	
	static char
	    hostNameBuffer[128],
	    serviceNameBuffer[128];

	struct nd_hostservlist
	    *serviceList;
	
	struct nd_addrlist
	    *addrList;

	struct nd_hostserv
	    *service;

	struct netbuf
	    *curNetbuf;
	
	int
	    done,
	    i,
	    j;
	
	char
	    *uaddr1,
	    *uaddr2;

	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"connGetHostByAddr(0x%x, 0x%x) Entered.\n",
		(int)addr,
		(int)config
		);
	    }

	if((uaddr1 = taddr2uaddr(config, addr)) == NULL)
	    {
	    done = 0;
	    }
	else
	{
	if(Debug > 5) (void)fprintf(stderr, "\tdone taddr2uaddr.\n");
	if(netdir_getbyaddr(config, &serviceList, addr))
	    {
	    /* ERROR Cannot find address. */
	    if(Debug > 5) (void)fprintf(stderr, "\tuaddr1 = %s.\n", uaddr1);

	    free(uaddr1);
	    done = 0;
	    }
	else
	    {
	    if(Debug > 5) (void)fprintf(stderr, "\tdone netdir_getbyaddr.\n");
	    for
		(
		service =  serviceList->h_hostservs,
		    done = 0,
		    i = 0;
		i < serviceList->h_cnt && !done;
		service++,
		    i++
		)
		{
		if(netdir_getbyname(config, service, &addrList))
		    {
		    /* ERROR Cannot find name */
		    if(Debug > 5) (void)fprintf(stderr, "\tuaddr1 = %s.\n", uaddr1);
		    }
		else
		    {
		    for
			(
			curNetbuf = addrList->n_addrs,
			    j = 0;
			j < addrList->n_cnt && !done;
			curNetbuf++,
			    j++
			)
			{
			if((uaddr2 = taddr2uaddr(config, curNetbuf)) == NULL)
			    {
			    /* No Match */
			    }
			else if(strcmp(uaddr1, uaddr2))
			    {
			    /* No Match */
			    if(Debug > 5)
				{
				(void)fprintf(stderr, "\tuaddr2 = %s.\n", uaddr2);
				}

			    free(uaddr2);
			    }
#if	0
			if(curNetbuf->len != addr->len)
			    {
			    /* No Match */
			    }
			else if(memcmp(curNetbuf->buf, addr->buf, addr->len))
			    {
			    /* No Match */
			    }
#endif
			else
			    {
			    if(Debug > 5)
				{
				(void)fprintf(stderr, "\tuaddr2 = %s.\n", uaddr2);
				}

			    free(uaddr2);
			    /* Match */
			    done = 1;
			    result.h_host = strncpy
				(
				hostNameBuffer,
				service->h_host,
				sizeof(hostNameBuffer)
				);

			    result.h_serv= strncpy
				(
				serviceNameBuffer,
				service->h_serv,
				sizeof(serviceNameBuffer)
				);
			    }
			}

		    netdir_free(addrList, ND_ADDRLIST);
		    }
		}

	    netdir_free(serviceList, ND_HOSTSERVLIST);
	    if(!done && Debug > 5) (void)fprintf(stderr, "\tuaddr1 = %s.\n", uaddr1);
	    free(uaddr1);
	    }
	}

	if(Debug > 2)
	    {
	    (void)fprintf(stderr, "connGetHostByAddr() Exited.\n");
	    }

	return((done)? &result: NULL);
	}

static void
    ccsFree(clientConnSetup_t *ccs_p)
	{
	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"ccsFree(0x%x) Entered.\n",
		(int)ccs_p
		);
	    }

	if(ccs_p != NULL)
	    {
	    if(ccs_p->ccs_conn)
		{
		ccs_p->ccs_conn->conn_freeDataFunc = NULL;
		ccs_p->ccs_conn->conn_data = NULL;
		connTerminate(ccs_p->ccs_conn);
		}

	    if(ccs_p->ccs_handle)endnetconfig(ccs_p->ccs_handle);
	    if(ccs_p->ccs_call)t_free((char *)ccs_p->ccs_call, T_CALL);
	    if(ccs_p->ccs_addrlist)netdir_free(ccs_p->ccs_addrlist, ND_ADDRLIST);
	    if(ccs_p->ccs_hostserv.h_host)free(ccs_p->ccs_hostserv.h_host);
	    if(ccs_p->ccs_hostserv.h_serv)free(ccs_p->ccs_hostserv.h_serv);
	    if(ccs_p->ccs_callback)
		{
		ccs_p->ccs_callback(ccs_p->ccs_localData, NULL);
		}

	    if(ccs_p->ccs_freeDataFunc)
		{
		ccs_p->ccs_freeDataFunc(ccs_p->ccs_localData);
		}

	    free(ccs_p);
	    }

	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"ccsFree() Exited.\n"
		);
	    }
	}

static void
    ccsRcvConnect(clientConnSetup_t *ccs_p)
	{
	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"ccsRcvConnect(0x%x) Entered.\n",
		(int)ccs_p
		);
	    }

	if(t_rcvconnect(ccs_p->ccs_fd, ccs_p->ccs_call) < 0)
	    {
	    t_error("\t");
	    }

	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"ccsRcvConnect() Exited.\n"
		);
	    }
	}

static void
    ccsReady(clientConnSetup_t *ccs_p)
	{
	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"ccsReady(0x%x) Entered.\n",
		(int)ccs_p
		);
	    }

	ccs_p->ccs_conn->conn_data = ccs_p->ccs_localData;
	ccs_p->ccs_conn->conn_freeDataFunc = ccs_p->ccs_freeDataFunc;
	ccs_p->ccs_conn->conn_connect = 0;
	ccs_p->ccs_callback(ccs_p->ccs_localData, ccs_p->ccs_conn);
	ccs_p->ccs_callback = NULL;
	ccs_p->ccs_conn = NULL;
	ccs_p->ccs_freeDataFunc = NULL;
	ccsFree(ccs_p);
	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"ccsReady() Exited.\n"
		);
	    }
	}

static clientConnSetup_t
    *ccsNextAddress(clientConnSetup_t *ccs_p)
	{
	struct netbuf
	    *curAddr_p;
	
	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"ccsNextAddress(0x%x) Entered.\n",
		(int)ccs_p
		);
	    }

	if(Debug > 5)
	    {
	    (void)fprintf
		(
		stderr,
		"\tccs_nextAddrIndex = %d, n_cnt = %d\n",
		ccs_p->ccs_nextAddrIndex,
		ccs_p->ccs_addrlist->n_cnt
		);
	    }
	/*
	    First get rid of any remaining stuff from the last config entry.
	*/
	if(ccs_p->ccs_conn)
	    {
	    ccs_p->ccs_conn->conn_freeDataFunc = NULL;
	    ccs_p->ccs_conn->conn_data = NULL;
	    connTerminate(ccs_p->ccs_conn);
	    ccs_p->ccs_conn = NULL;
	    }

	if(ccs_p->ccs_call)
	    {
	    t_free((char *)ccs_p->ccs_call, T_CALL);
	    ccs_p->ccs_call = NULL;
	    }

	if
	    (
		(
		ccs_p->ccs_fd = t_open
		    (
		    ccs_p->ccs_netconfig->nc_device,
		    O_RDWR | O_NDELAY | O_NONBLOCK,
		    NULL
		    )
		) < 0
	    )
	    {
	    netdir_free(ccs_p->ccs_addrlist, ND_ADDRLIST);
	    ccs_p->ccs_addrlist = NULL;
	    }
	else if(fcntl(ccs_p->ccs_fd, F_SETFD, 1) == -1)
	    {
	    /*perror("fcntl failed.");*/
	    netdir_free(ccs_p->ccs_addrlist, ND_ADDRLIST);
	    ccs_p->ccs_addrlist = NULL;
	    }
	else if
	    (
		(
		ccs_p->ccs_reservedPort && netdir_options
		    (
		    ccs_p->ccs_netconfig,
		    ND_SET_RESERVEDPORT,
		    ccs_p->ccs_fd,
		    NULL
		    ) < 0
		) || (t_bind(ccs_p->ccs_fd, NULL, NULL) < 0)
	    )
	    {
	    netdir_free(ccs_p->ccs_addrlist, ND_ADDRLIST);
	    ccs_p->ccs_addrlist = NULL;
	    t_close(ccs_p->ccs_fd);
	    ccs_p->ccs_fd = -1;
	    }
	else if
	    (
		(
		ccs_p->ccs_call = (struct t_call *)t_alloc
		    (
		    ccs_p->ccs_fd,
		    T_CALL,
		    T_ALL
		    )
		) == NULL
	    )
	    {
	    netdir_free(ccs_p->ccs_addrlist, ND_ADDRLIST);
	    ccs_p->ccs_addrlist = NULL;
	    t_close(ccs_p->ccs_fd);
	    ccs_p->ccs_fd = -1;
	    }
	else if
	    (
		(
		ccs_p->ccs_conn = connNew
		    (
		    ccs_p->ccs_fd,
		    ccs_p->ccs_netconfig,
		    ccs_p->ccs_readFunc,
		    ccsFree,
		    ccs_p
		    )
		) == NULL
	    )
	    {
	    netdir_free(ccs_p->ccs_addrlist, ND_ADDRLIST);
	    ccs_p->ccs_addrlist = NULL;
	    t_close(ccs_p->ccs_fd);
	    ccs_p->ccs_fd = -1;
	    }
	else if(ccs_p->ccs_nextAddrIndex < ccs_p->ccs_addrlist->n_cnt)
	    {
	    ccs_p->ccs_conn->conn_connect = 1;
	    curAddr_p = &ccs_p->ccs_addrlist->n_addrs[ccs_p->ccs_nextAddrIndex++];
	    (void)memcpy
		(
		ccs_p->ccs_call->addr.buf,
		curAddr_p->buf,
		ccs_p->ccs_call->addr.maxlen
		);
	    
	    ccs_p->ccs_call->addr.len = curAddr_p->len;
	    
	    if(t_connect(ccs_p->ccs_fd, ccs_p->ccs_call, NULL) == 0)
		{
		ccsReady(ccs_p);
		ccs_p = NULL;
		}
	    else if(t_errno == TNODATA)
		{
		/* OK Wait for connect ind. */
		}
	    else
		{
		if(Debug > 5) t_error("\tt_connect");
		if(Debug > 5) (void)fprintf(stderr, "t_errno = %d.\n", t_errno);
		ccs_p = ccsNextAddress(ccs_p);
		}
	    }
	else
	    {
	    ccs_p = ccsNextConfig(ccs_p);
	    }

	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"ccsNextAddress() = 0x%x Exited.\n",
		(int)ccs_p
		);
	    }

	return(ccs_p);
	}

static clientConnSetup_t
    *ccsNextConfig(clientConnSetup_t *ccs_p)
	{
	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"ccsNextConfig(0x%x) Entered.\n",
		(int)ccs_p
		);
	    }

#if 0
	/*
	    First get rid of any remaining stuff from the last config entry.
	*/
	if(ccs_p->ccs_conn)
	    {
	    ccs_p->ccs_conn->conn_freeDataFunc = NULL;
	    ccs_p->ccs_conn->conn_data = NULL;
	    connTerminate(ccs_p->ccs_conn);
	    ccs_p->ccs_conn = NULL;
	    }

	if(ccs_p->ccs_call)
	    {
	    t_free((char *)ccs_p->ccs_call, T_CALL);
	    ccs_p->ccs_call = NULL;
	    }
#endif

	if(ccs_p->ccs_addrlist)
	    {
	    netdir_free(ccs_p->ccs_addrlist, ND_ADDRLIST);
	    ccs_p->ccs_addrlist = NULL;
	    }

	while((ccs_p->ccs_netconfig = getnetpath(ccs_p->ccs_handle)) != NULL)
	    {
	    if(Debug > 5)
		{
		(void)fprintf
		    (
		    stderr,
		    "\tnetconfig = %s.\n",
		    ccs_p->ccs_netconfig->nc_netid
		    );
		}

	    if(ccs_p->ccs_netconfig->nc_semantics == NC_TPI_CLTS)
		{
		}
	    else if(ccs_p->ccs_netconfig->nc_semantics == NC_TPI_RAW)
		{
		}
	    else if
		(
		netdir_getbyname
		    (
		    ccs_p->ccs_netconfig,
		    &ccs_p->ccs_hostserv,
		    &ccs_p->ccs_addrlist
		    ) != 0
		)
		{
		}
#if 0
	    else if
		(
		    (
		    ccs_p->ccs_fd = t_open
			(
			ccs_p->ccs_netconfig->nc_device,
			O_RDWR | O_NDELAY | O_NONBLOCK,
			NULL
			)
		    ) < 0
		)
		{
		netdir_free(ccs_p->ccs_addrlist, ND_ADDRLIST);
		ccs_p->ccs_addrlist = NULL;
		}
	    else if(fcntl(ccs_p->ccs_fd, F_SETFD, 1) == -1)
		{
		/*perror("fcntl failed.");*/
		netdir_free(ccs_p->ccs_addrlist, ND_ADDRLIST);
		ccs_p->ccs_addrlist = NULL;
		}
	    else if
		(
		    (
		    ccs_p->ccs_reservedPort && netdir_options
			(
			ccs_p->ccs_netconfig,
			ND_SET_RESERVEDPORT,
			ccs_p->ccs_fd,
			NULL
			) < 0
		    ) || (t_bind(ccs_p->ccs_fd, NULL, NULL) < 0)
		)
		{
		netdir_free(ccs_p->ccs_addrlist, ND_ADDRLIST);
		ccs_p->ccs_addrlist = NULL;
		t_close(ccs_p->ccs_fd);
		ccs_p->ccs_fd = -1;
		}
	    else if
		(
		    (
		    ccs_p->ccs_call = (struct t_call *)t_alloc
			(
			ccs_p->ccs_fd,
			T_CALL,
			T_ALL
			)
		    ) == NULL
		)
		{
		netdir_free(ccs_p->ccs_addrlist, ND_ADDRLIST);
		ccs_p->ccs_addrlist = NULL;
		t_close(ccs_p->ccs_fd);
		ccs_p->ccs_fd = -1;
		}
	    else if
		(
		    (
		    ccs_p->ccs_conn = connNew
			(
			ccs_p->ccs_fd,
			ccs_p->ccs_netconfig,
			ccs_p->ccs_readFunc,
			ccsFree,
			ccs_p
			)
		    ) == NULL
		)
		{
		netdir_free(ccs_p->ccs_addrlist, ND_ADDRLIST);
		ccs_p->ccs_addrlist = NULL;
		t_close(ccs_p->ccs_fd);
		ccs_p->ccs_fd = -1;
		}
#endif
	    else
		{
		ccs_p->ccs_nextAddrIndex = 0;
		ccs_p = ccsNextAddress(ccs_p);
		break;
		}
	    }

	if(ccs_p->ccs_netconfig == NULL)
	    {
	    ccsFree(ccs_p);
	    ccs_p = NULL;
	    }

	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"ccsNextConfig() = 0x%x Exited.\n",
		(int)ccs_p
		);
	    }

	return(ccs_p);
	}

static clientConnSetup_t
    *ccsNew
	(
	struct nd_hostserv *hostserv_p,
	void (*callback)(),
	void *localData,
	void (*freeDataFunc)(),
	void (*readFunc)(),
	int reservedPort
	)
        {
        clientConnSetup_t
	    *result;
	
	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"clientConnSetup(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, %s) Entered.\n",
		(int)hostserv_p,
		(int)callback,
		(int)localData,
		(int)freeDataFunc,
		(int)readFunc,
		reservedPort? "reserved": "normal"
		);
	    }

	if((result = (clientConnSetup_t *)calloc(1, sizeof(*result))) == NULL)
	    {
	    /* ERROR No Memory */
	    if(callback != NULL) callback(localData, NULL);
	    if(freeDataFunc != NULL) freeDataFunc(localData);
	    }
	else if
	    (
		(
		result->ccs_hostserv.h_host = strdup(hostserv_p->h_host)
		) == NULL
	    )
	    {
	    /* ERROR No Memory */
	    result->ccs_callback = callback;
	    result->ccs_localData = localData;
	    result->ccs_freeDataFunc = freeDataFunc;
	    ccsFree(result);
	    result = NULL;
	    }
	else if
	    (
		(
		result->ccs_hostserv.h_serv = strdup(hostserv_p->h_serv)
		) == NULL
	    )
	    {
	    /* ERROR No Memory */
	    result->ccs_callback = callback;
	    result->ccs_localData = localData;
	    result->ccs_freeDataFunc = freeDataFunc;
	    ccsFree(result);
	    result = NULL;
	    }
	else if((result->ccs_handle = setnetpath()) == NULL)
	    {
	    result->ccs_callback = callback;
	    result->ccs_localData = localData;
	    result->ccs_freeDataFunc = freeDataFunc;
	    ccsFree(result);
	    result = NULL;
	    }
	else
	    {
	    result->ccs_fd = -1;
	    result->ccs_reservedPort = reservedPort;
	    result->ccs_callback = callback;
	    result->ccs_readFunc = readFunc;
	    result->ccs_localData = localData;
	    result->ccs_freeDataFunc = freeDataFunc;
	    }

	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"clientConnSetup() = 0x%x Exited.\n",
		(int)result
		);
	    }

	return(result);
	}

void
    connNewClient
	(
	struct nd_hostserv *hostserv_p,
	void (*callback)(),
	void *localData,
	void (*freeDataFunc)(),
	void (*readFunc)(),
	int reservedPort
	)
	{
	clientConnSetup_t
	    *ccs_p;

	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"connNewClient(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, %s) Entered.\n",
		(int)hostserv_p,
		(int)callback,
		(int)localData,
		(int)freeDataFunc,
		(int)readFunc,
		reservedPort? "reserved": "regular"
		);
	    }
	if
	    (
		(
		ccs_p = ccsNew
		    (
		    hostserv_p,
		    callback,
		    localData,
		    freeDataFunc,
		    readFunc,
		    reservedPort
		    )
		) != NULL
	    )
	    {
	    (void)ccsNextConfig(ccs_p);
	    }

	if(Debug > 2)
	    {
	    (void)fprintf
		(
		stderr,
		"connNewClient() Exited.\n"
		);
	    }
	}

