#ident	"%W%"
#ident	"$Id: nwdiagd.c,v 1.18 1994/09/12 16:21:56 vtag Exp $"

/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 *
 */

#include <stdlib.h>	/* Put here so it doesn't break net mgt on AIX */
#include "nwdiagd.h"

/*
**  DPRINTF prints only if compiled with -DDEBUG
*/
#ifdef DEBUG
#define DPRINTF(arg) fprintf arg
#else
#define DPRINTF(arg)
#endif

/* Diagnostics structures
 */
/* # bytes in struct containing
 * IPX Diagnostics request response
 */
static size_t	ipxConfigSize = 0;

/* Request currently being answered */
static spxDiagReq_t	spxDiagReq;

/* Response currently being built */
static spxDiagResp_t	spxDiagResp;

/* struct containing IPX Diagnostics response */
static ipxConfiguration_t	*ipxConfig = NULL;

/* struct containing Diagnostic Statistics */
static DIAGSTAT	diagStatistics = {0};

static time_t	time0;

/* info on file descriptors that need to be polled */
static struct	StructPollFds {
	int		initialized;
	int		nElements;
	struct pollfd		*pollFds;
} polls = {0};

/* Table of all servers known to this machine */
static struct	StructServersTbl {
	int		initializing;
	int		initialized;
	int		nElements;
	SAP_ID_PACKET	*table;
} serversTbl = {0};

/* Table of all networks known to this machine */
static struct	StructNetsTbl {
	int		initialized;
	int		nElements;
	routeInfo_t		*table;
} netsTbl = {0};

static char	program[] = "NWDIAGD";
static char *ripxDevice="/dev/ripx";
static char *ipxDevice="/dev/ipx";
static char *spxDevice="/dev/nspx";

/*********************************************************************/
/* forward declarations */
static int		ServersTblMaint(void);
static int		InitNetsTbl(void);
static int		NetsTblMaint(void);
static int		DoSpxRequest(void);
static void	SpxDiagnostics(void);
static void	SetupIpxDiags(void);
static void	diagExit( int status);
static int		GetNetEntry(uint32, netInfo_t *);
static int		InitServersTbl(int);
static int		SapServerCmp(const void	*, const void *);
static int		SetupSpxDiags(uint16 *);
static void	PollForEvents(int);
static int		routeInfoCmp(const void *r1, const void *r2);
static int		routeInfoCmpNet(const void *r1, const void *r2);
static int		IpxSpxReturnIpxSpxVersion(void);
static int		IpxSpxReturnIpxStats(void);
static int		IpxSpxReturnSpxStats(void);
static int		IpxSpxPtPtSend(uint8, uint8 *, int);
static int 	IpxSpxCountPkts(uint8, int);
static void	IpxDiagnostics(void);
static int		BDriverReturnStats(int nbytes);
static int		BDriverReturnStatus(void);
static int		BDriverReturnConfig(uint8 *, int);
static int		BridgeRtnLocalTables(void);
static int		BridgeRtnAllKnownNets(uint8 *, int);
static int		BridgeRtnSpecificNetInfo(uint8 *, int);
static int		BridgeRtnAllKnownServers(uint8 *, int);
static int		BridgeRtnSpecificServerInfo(uint8 *, int);
static int		BridgeRtnStats(void);
static int		BridgeResetLanBoard(void);
static int		BridgeReinitRoutingTables(void);
static void	UpdateTitle(void);
static void	InitializeLogFile(void);

/*ARGSUSED*/
static void
Hangup( int sig)
{
    diagExit(0);
    /* NOTREACHED */
}

/*********************************************************************/
/* GetNetEntry:	submit an ioctl to IPX to return hops, status, and
* time-to-net info for a specific network. Called by
* BridgeRtnSpecificNetInfo().
**********************************************************************/
static int 
GetNetEntry(uint32 network, netInfo_t *netPtr )
{	struct strioctl ioc;

	memset((char *)netPtr, (char )0, sizeof(struct netInfo) );

	netPtr->netIDNumber = network;
	ioc.ic_cmd = RIPX_GET_NET_INFO;
	ioc.ic_timout = 5;
	ioc.ic_len = sizeof(netInfo_t);
	ioc.ic_dp = (char *)netPtr;

	if (ioctl(polls.pollFds[RIPX_DIAG_FD].fd, I_STR, &ioc) < 0) {
		fprintf(stderr, MsgGetStr(DIAG_IOCTL_NET),
			"RIPX_DIAG_FD",network);
		perror("");
		return (-1);
	}

	return(0);
}

/*********************************************************************/
/* GetLanInfo: Submit an ioctl to IPX to retrieve state & net/node #
 * information on all lower IPX connections (all Lan cards doing IPX
 * work have a lower stream to IPX).
 *********************************************************************/
static int 
GetLanInfo()
{	struct strioctl ioc;
	int lan;

	/* error check malloc */
	lanInfoTable = (lanInfo_t *)malloc(ipxMaxConnectedLans * sizeof(lanInfo_t));
	if (!lanInfoTable) {
		fprintf(stderr, MsgGetStr(DIAG_ALLOC_LINFO),
			ipxMaxConnectedLans * sizeof(lanInfo_t));
		return FAILURE;
	}

	ioc.ic_cmd = IPX_GET_LAN_INFO;
	ioc.ic_timout = 5;

	for( lan = 0; lan < ipxMaxConnectedLans; lan++) {
		lanInfoTable[lan].lan = lan;
		ioc.ic_len = sizeof(lanInfo_t);
		ioc.ic_dp = (char *)&lanInfoTable[lan];

		if (ioctl(polls.pollFds[LIPMX_DIAG_FD].fd, I_STR, &ioc) == -1) {
			fprintf(stderr, MsgGetStr(DIAG_IOCTL), "IPX_GET_LAN_INFO");
			perror("");
			free(lanInfoTable);
			lanInfoTable = NULL;
			return -1;
		}
	}
	return 0;
}

/*********************************************************************/
/* nwdiagd: This daemon is spawned by npsd on NWU startup. It opens
 * IPX and binds to socket 0x0456 (the socket reserved for IPX
 * Diagnostics). It then polls the socket for incoming requests.
 * When the first request is received, nwdiagd queries IPX for
 * lan information, attempts to open a dedicated ephemeral SPX
 * socket for handling SPX Diagnostics requests, and saves this
 * static information to answer all IPX Diagnostics requests with.
 * Both IPX and SPX Diagnostics sockets are then polled for their
 * respective requests and answered as appropriate. As additional
 * IPX opens are required to answer certain types of SPX requests,
 * their file descriptors are polled also.
 ********************************************************************/
/*ARGSUSED*/
int
main(int argc, char *argv[], char *envp[])
{	struct strioctl ioc;
	int		i;
	pid_t	pid;
	IpxConfiguredLans_t	configLan;	
	int ccode;


	time0 = time(NULL);
	diagStatistics.StartTime = time0;

	/*
    **  Close all open file descriptors
    */
    for(i=3; i<20; i++)
        close(i);
    errno = 0;  /* clear probable EBADF from bogus close */

	ccode = MsgBindDomain(MSG_DOMAIN_DIAG, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);
    if(ccode != NWCM_SUCCESS) {
		/* Do not internationalize */
        fprintf(stderr,"%s: Unable to bind message domain. NWCM error = %d. Exiting.\n",
            titleStr, ccode);
        exit(-1);
    }

	if (!getenv("NWUENV")) {
		switch ((int)fork()) {
		case -1:
            fprintf(stderr, MsgGetStr(DIAG_FORK_FAIL));
            perror("");
            diagExit(-1);
            /*NOTREACHED*/
		case 0:
			if(setpgrp() == -1) {
                fprintf(stderr, MsgGetStr(DIAG_SESSION));
                perror("");
                diagExit(-1);
                /*NOTREACHED*/
            }

            if((pid = fork()) < 0) {
                fprintf(stderr, MsgGetStr(DIAG_FORK_FAIL));
                perror("");
                diagExit(-1);
                /*NOTREACHED*/
            }
            else if(pid > 0)
                exit(-1);    /* second child */

            umask(022);
            break;

		default:
			exit(0);
		}
	}

	sigignore( SIGCLD );
	signal( SIGHUP, Hangup );

	SetupIpxDiags();
	sleep(2);

	ioc.ic_cmd = IPX_GET_CONFIGURED_LANS;
	ioc.ic_timout = 0;
	ioc.ic_dp = (char *)&configLan;
	ioc.ic_len = sizeof(IpxConfiguredLans_t);

	if(ioctl(polls.pollFds[LIPMX_DIAG_FD].fd, I_STR, &ioc)<0) {
		fprintf(stderr, MsgGetStr(DIAG_IOCTL), "IPX_GET_CONFIGURED_LANS");
		perror("");
		diagExit(-1);
	}
	ipxMaxConnectedLans = configLan.lans;

	InitializeLogFile();
	UpdateTitle();
	fprintf(stderr, MsgGetStr(DIAG_START));
	PollForEvents(-1);
	diagExit(0);
	/*NOTREACHED*/
	return(0);
}	

/*********************************************************************/
/* PollForEvents: Poll all active file descriptors for events. When
 * something interesting happens on a given fd, respond appropriately.
 *********************************************************************/
static void
PollForEvents(int timeout)
{	int		heard;
#ifdef HARDDEBUG
	int i;
#endif

	while(heard = poll(polls.pollFds, polls.nElements, timeout)) {
		if(heard > 0) {
			UpdateTitle();
#ifdef HARDDEBUG
			for(i=0; i<polls.nElements; i++) {
				if(polls.pollFds[i].revents & POLLERR)
					 printf("[%d].revents & POLLERR\n", i);
				if(polls.pollFds[i].revents & POLLHUP)
					 printf("[%d].revents & POLLHUP\n", i);
				if(polls.pollFds[i].revents & POLLNVAL)
					 printf("[%d].revents & POLLNVAL\n", i);
				else
					printf("[%d].revents = 0x%X	[%d].fd = %d\n",
						i, polls.pollFds[i].revents, i, polls.pollFds[i].fd);
			}
#endif
			/* If HANGUP we are done */
			if(polls.pollFds[IPX_DIAG_FD].revents & POLLHUP) {
				diagExit(0);
			}
			/* function pointers in polls ??? */
			if(polls.pollFds[IPX_DIAG_FD].revents & POLLIN) {
				IpxDiagnostics();
			}
			if(polls.nElements > NUM_IPX_FD) {
				if(polls.pollFds[IPX_QUERY_SAPD_FD].fd >= 0) {
					 if (polls.pollFds[IPX_QUERY_SAPD_FD].revents & POLLIN) {
						InitServersTbl(1);
					 }
				}
				if(polls.pollFds[IPX_COUNT_PKTS_FD].revents & POLLIN) {
					IpxSpxCountPkts(0, 0);
				}
				if(polls.pollFds[SPX_DIAG_FD].revents & POLLIN) {
					SpxDiagnostics();
				}
			}
		} else {
			if(errno == EAGAIN || errno == EINTR)
				continue;
			UpdateTitle();
			fprintf(stderr, MsgGetStr(DIAG_POLL), errno);
			diagExit(-1);
			/*NOTREACHED*/
		}
	}
	return;
}

/*********************************************************************/
/*
 * IpxDiagnostics: Respond to valid Ipx Configuration Request packets
 * addressed to socket 0x0456.
 *
 * The request will be an ipxHdr, followed by an exclusion count and 
 * 0 to 80 node addresses. Do not respond if our node is listed.
 *
 * The response will return the diagnostics version number, the
 * Spx socket number to which Spx diagnostics requests should be
 * addressed, how we are configured, and a list of lan card info.
 *
 * If SetupSpxDiagnostics() is successful, a non-zero SPX socket
 * number will be returned by reference, and we will advertise this
 * socket for Diagnonstics requestors to use for further queries.
 * Otherwise, report ourself as IPX_ONLY in our responses.
 *********************************************************************/
static void
IpxDiagnostics(void)
{	static int		initialized = 0;
	static uint8	ipxAddr[12] = {0}, ipxPktType = 0;
	static uint16	spxDiagSock = 0;
	static struct t_unitdata	udgram;

	int		i,
			flags,
			lanBoards;	/* count of physical boards doing routing */
	uint8	*component, ipxDiagReq[IPX_MAX_PACKET_DATA];
	ipxHdr_t	*reqHeader;
	lanDriver_t		*lanDriver = 0;
	bridgeDrivers_t		*bridgeDrivers;
	exclusionList_t		*excludeList;
	exclusionEntry_t	*exclusion;

	if(!initialized) {
		udgram.addr.len = sizeof(ipxAddr_t);
		udgram.addr.maxlen = sizeof(ipxAddr_t);
		udgram.addr.buf = (char *)&ipxAddr[0];
		udgram.opt.len = sizeof(ipxPktType);
		udgram.opt.maxlen = sizeof(ipxPktType);
		udgram.opt.buf = (char *)&ipxPktType;
		udgram.udata.maxlen = IPX_MAX_PACKET_DATA;
	}
	udgram.udata.buf = (caddr_t)ipxDiagReq;
	if(t_rcvudata(polls.pollFds[IPX_DIAG_FD].fd, &udgram, &flags) <0) {
		fprintf(stderr, MsgGetStr(DIAG_NORECV),
				t_look(polls.pollFds[IPX_DIAG_FD].fd), t_errno, errno);
		fprintf(stderr, MsgGetStr(DIAG_T_RCV));
		t_error("");
		t_rcvuderr(polls.pollFds[IPX_DIAG_FD].fd, NULL);
		return;
	}
	/*
	 * Make sure the Data is at least big enough to hold
	 * an IPX Header and a 1 byte count.
	 */
	/* can this ever happen ??? */
	if (!udgram.udata.len) {
		fprintf(stderr, MsgGetStr(DIAG_BADPKT), udgram.udata.len);
		return;
	}
	/* Check exclusion addresses for server address.
	 * If on list, go away.
	 */
	excludeList = (exclusionList_t *)udgram.udata.buf;
	exclusion = (exclusionEntry_t *) ((caddr_t)excludeList + 1);

	if (!lanInfoTable)
		if (GetLanInfo())
			diagExit(-1);

	reqHeader = (ipxHdr_t *)udgram.addr.buf;
	while((excludeList->exclusionCount)--
			&& (((caddr_t)exclusion - (caddr_t)reqHeader) < udgram.udata.len))
		if(IPXCMPNODE(exclusion, lanInfoTable[0].nodeAddress))
			return;
		else
			exclusion++;

#ifdef NOT_IMPLEMENTED
	uint8 save;
	/* If the request is a broadcast, the reply should
	 * be delayed (by time = f(last byte))
	 * per "System Interface Technical Overview, sec. 6 pg. 5.
	 * Otherwise, respond immediately ???
	 */
	save = reqHeader->dest.node[0];
	for (i = 1; i < 6 ; ++i)
		save &= reqHeader->dest.node[i];
	
	if(save != 0xff)
		?
	else
		?
#endif

	if (!initialized) {
		if(!ipxConfig)
			if(!(ipxConfig = (ipxConfiguration_t *)
								malloc(sizeof(ipxConfiguration_t))))
				diagExit(-1);
		if(SetupSpxDiags(&spxDiagSock))
			diagExit(-1);

		ipxConfig->majorVersion = DIAGS_VER_MAJ;
		diagStatistics.MajorVersion = ipxConfig->majorVersion;

		ipxConfig->minorVersion = DIAGS_VER_MIN;
		diagStatistics.MinorVersion = ipxConfig->minorVersion;

		IPXCOPYSOCK(&spxDiagSock, &ipxConfig->spxDiagSock);
		diagStatistics.SPXDiagSocket = spxDiagSock;

		ipxConfig->numberOfComponents = 0;

		component = &ipxConfig->componentStructure[0];

		/* If SPX is active, we are IPX/SPX, else IPX_ONLY
		 * (unless we're non-dedicated).
		 */
		if((polls.nElements > 1) && polls.pollFds[SPX_DIAG_FD].fd)
			/* This can hose workstations making calls to
			 * FindComponentOffset() in C-interface for
			 * DOS (bad code there).
			 */
			*component = IPX_SPX_COMPONENT;
		else
			*component = IPX_ONLY;
		++component;
		++(ipxConfig->numberOfComponents);

		/* We are never(?) a shell, shell driver, or VAP shell, so we
		 * must be a bridge driver.
		 */
		*component = BRIDGE_DRIVER_COMPONENT;
		++component;
		++(ipxConfig->numberOfComponents);

		/* We must do Internal and or External bridging.
		 * Usually it is assumed that an Internal Bridge is a
		 * file server (not true for NWU). Are we non-dedicated ???
		 * Can we do both on mutually exclusive nets ???
		 */

		/* NULL so we can later test Internal vs Ext. */
		*component = 0;
		for (i = 0, lanBoards = 0; i < ipxMaxConnectedLans; i++) {
			/* non-NULL netPtr means this lan is registered
			 * in the routing table.
			 */
			if (lanInfoTable[i].network) {
				if(i) {
					++lanBoards;
				} else /* lan 0 can be routed to - we do Internal */
					/* Real confusion here if this is supposed to
		 			* indicate FILE_SERVER or INTERNAL_BRIDGE. The
		 			* latter is easier, so guess what we do?
		 			*/
					/* if we're getting this, the internal
					 * bridge is up, so INTERNAL_BRIDGE
					 */
					*component = INTERNAL_BRIDGE;

				/* multiple registered lanBoards and active
				 * routing means we're an external bridge,
				 * so tell the requester.
				 */
				if(lanBoards > 1) {
					/* If we're not an Internal Bridge (which seems
					 * to subsume External Bridging as far as
					 * diagnostics is concerned), let requester
					 * know we de external routing. If type == INTERNAL,
					 * does this assume EXTERNAL ???
					 */
					if(!(*component))
						*component = EXTERNAL_BRIDGE;
					break;
				}
			}
		}

		/* If we're doing anything useful, report what lans
		 * we are doing it on.
		 */
		if(*component) {
			++component;
			++(ipxConfig->numberOfComponents);

			bridgeDrivers = (bridgeDrivers_t *)component;
			bridgeDrivers->numberOfNets = 0;
			lanDriver = bridgeDrivers->drivers;
			for(i = 0; i < ipxMaxConnectedLans; i++) {
				if(lanInfoTable[i].network) {
					++(bridgeDrivers->numberOfNets);
					lanDriver->localNetworkType
						= i ? LAN_BOARD : VIRTUAL_BOARD;
					IPXCOPYNET(&lanInfoTable[i].network, &lanDriver->network);
					IPXCOPYNODE(lanInfoTable[i].nodeAddress, lanDriver->node);
					++lanDriver;
				}
			}
			component = (uint8 *)lanDriver;
		}

		ipxConfigSize = (size_t)((caddr_t)component
									- (caddr_t)ipxConfig);
		if(!(ipxConfig = (ipxConfiguration_t *)
							realloc((void *)ipxConfig, ipxConfigSize)))
			diagExit(-1);

		initialized = 1;
	}
	if (*(udgram.opt.buf) == DIAG_STATISTICS) 
		{
		udgram.udata.len = sizeof(diagStatistics);
		udgram.udata.buf = (caddr_t)&diagStatistics;
		}
	else
		{
		udgram.udata.len = ipxConfigSize;
		udgram.udata.buf = (caddr_t)ipxConfig;
		}

	t_sndudata(polls.pollFds[IPX_DIAG_FD].fd, &udgram);
	return;
}

/*********************************************************************/
/* SetupIpxDiags: Open IPX and attempt to bind to socket 0x0456. If
 * successful, set ourself up to be polled by inserting ouself into
 * the pollfd array.
 *********************************************************************/
static void
SetupIpxDiags(void)
{	int		fd;
	struct t_info	info;
	struct t_bind	*bind, *bindRtn;
	uint16 ipxDiagSock = GETINT16(IPX_DIAGNOSTIC_SOCKET);

	if ((fd = t_open(ipxDevice, O_RDWR, &info)) < 0) {
		fprintf(stderr, MsgGetStr(DIAG_TOPEN_FAIL), ipxDevice);
		perror("");
		diagExit(-1);
		/*NOTREACHED*/
	}
	if((bind = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR)) == NULL) {
		fprintf(stderr, MsgGetStr(DIAG_TALLOC_FAIL), "t_bind");
		t_error("");
		diagExit(-1);
		/*NOTREACHED*/
	}
	if((bindRtn = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR)) == NULL) {
		fprintf(stderr, MsgGetStr(DIAG_TALLOC_FAIL), "t_bind");
		t_error("");
		diagExit(-1);
		/*NOTREACHED*/
	}
	bind->addr.len = info.addr;
	IPXCOPYSOCK(&ipxDiagSock, &bind->addr.buf[10]);
	if(t_bind(fd, bind, bindRtn) < 0) {
		fprintf(stderr, MsgGetStr(DIAG_TBIND_FAIL), ipxDevice);
		t_error("");
		diagExit(-1);
		/*NOTREACHED*/
	}

	/* check for proper bound socket */
	if(!IPXCMPSOCK(&ipxDiagSock, &bindRtn->addr.buf[10])) {
		diagExit(-1);
		/*NOTREACHED*/
	}
	t_free((char *)bind, T_BIND);
	t_free((char *)bindRtn, T_BIND);

	if(!(polls.pollFds = 
			(struct pollfd *)malloc( sizeof(struct pollfd) * NUM_IPX_FD))) {
		perror("");
		diagExit(-1);
		/*NOTREACHED*/
	}
	++polls.nElements;
	polls.pollFds[IPX_DIAG_FD].fd = fd;
	polls.pollFds[IPX_DIAG_FD].events = POLLIN;
	polls.pollFds[IPX_DIAG_FD].revents = 0;

	polls.pollFds[LIPMX_DIAG_FD].fd = fd;
	polls.pollFds[LIPMX_DIAG_FD].events = POLLIN;
	polls.pollFds[LIPMX_DIAG_FD].revents = 0;

	if ((fd = open(ripxDevice, O_RDWR, &info)) < 0) {
		fprintf(stderr, MsgGetStr(DIAG_OPEN_FAIL), ripxDevice);
		perror("");
		diagExit(-1);
		/*NOTREACHED*/
	}
	polls.pollFds[RIPX_DIAG_FD].fd = fd;
	polls.pollFds[RIPX_DIAG_FD].events = POLLIN;
	polls.pollFds[RIPX_DIAG_FD].revents = 0;
	return;
}

/*********************************************************************/
/* SetupSpxDiags: Attempt to open SPX and bind to any ephemeral socket
 * with 1 outstanding listen. If bind succeeds, SPX is active, and we
 * adjust the pollfd array to include space for all the file descriptors
 * SPX Diagnostics may need. Otherwise, we conclude that SPX is not
 * active, and leave.
 *********************************************************************/
 /* ??? change to return socket, or make void */
static int
SetupSpxDiags(uint16 *spxDiagSockPtr)
{	int		fd, i;
	struct t_bind	*bind;
	struct t_info	info;

	if ((fd = t_open(spxDevice, O_RDWR, &info)) == -1) {
		fprintf(stderr, MsgGetStr( DIAG_TOPEN_FAIL), spxDevice);
		perror("");
		return(0);
	}
	if((bind = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR)) == NULL) {
		fprintf(stderr, MsgGetStr(DIAG_TALLOC_FAIL), "t_bind");
		t_error("");
		diagExit(-1);
	}
	bind->qlen = 1;
	bind->addr.len = info.addr;
	for(i=0; i<info.addr; i++)
		bind->addr.buf[i] = 0;

	if(t_bind(fd, bind, bind) < 0) {
		fprintf(stderr, MsgGetStr(DIAG_TBIND_FAIL), spxDevice);
		t_error("");
		return(0);
	}
	IPXCOPYSOCK(&bind->addr.buf[10], spxDiagSockPtr);
	t_free((char *)bind, T_BIND);
	/* check for correct range of bound address ??? */
	if(!(polls.pollFds = (struct pollfd *)realloc(polls.pollFds,
								NUM_DIAG_FD * sizeof(struct pollfd)))) {
		fprintf(stderr, MsgGetStr(DIAG_ALLOC_POLL),
			NUM_DIAG_FD * sizeof(struct pollfd));
		diagExit(-1);
	}
	polls.nElements += NUM_SPX_FD;
	polls.pollFds[IPX_QUERY_SAPD_FD].fd = -1;
	polls.pollFds[IPX_COUNT_PKTS_FD].fd = -1;
	polls.pollFds[IPX_SND_PKTS_FD].fd = -1;
	polls.pollFds[SPX_DIAG_FD].fd = fd;
	polls.pollFds[SPX_DIAG_FD].events = POLLIN;
	polls.pollFds[SPX_DIAG_FD].revents = 0;
	return(0);
}

/*********************************************************************/
/* SpxDiagnostics: We're here because something interesting has hap-
 * pened on th SpxDiag file descriptor. Maintain connection state -
 * if connection is currently active, we're here because data has
 * arrived (a request) or a disconnect needs to be handled. If con-
 * nection is not active, this must be a connection request. If this
 * is a request, answer it. If it is a disconnect indication, handle it.
 * If it is a connect indication, accept the connection.
 *********************************************************************/
static void
SpxDiagnostics()
{	static struct t_call	*connection = NULL;

	if(connection) {
		if(DoSpxRequest() == -1) {
			if(t_look(polls.pollFds[SPX_DIAG_FD].fd) == T_DISCONNECT) {
				if(t_rcvdis(polls.pollFds[SPX_DIAG_FD].fd, NULL) < 0) {
					fprintf(stderr, MsgGetStr(DIAG_UDATA_FAIL), spxDevice);
					t_error("");
					exit(-1);
				}
				t_free((char *)connection, T_CALL);
				connection = NULL;
			}
		}
	/* if T_ORDREL (when supported), do t_rcvrel & t_sndrel */
	} else {
		if((connection = (struct t_call *)t_alloc(polls.pollFds[SPX_DIAG_FD].fd,
										T_CALL, T_ALL)) == NULL) {
			fprintf(stderr, MsgGetStr(DIAG_TALLOC_FAIL), "t_call");
			t_error("");
			exit(-1);
		}

		if(t_listen(polls.pollFds[SPX_DIAG_FD].fd, connection) < 0) {
			fprintf(stderr, MsgGetStr(DIAG_LISTEN_FAIL), spxDevice);
			t_error("");
			exit(-1);
		}
		if(t_accept(polls.pollFds[SPX_DIAG_FD].fd,
					polls.pollFds[SPX_DIAG_FD].fd, connection) < 0) {
			if(t_errno == TLOOK)
				if(t_rcvdis(polls.pollFds[SPX_DIAG_FD].fd, NULL) < 0) {
					fprintf(stderr, MsgGetStr(DIAG_UDATA_FAIL), spxDevice);
					t_error("");
					exit(-1);
				}
		}
	}
	return;
}

/*********************************************************************/
/* DoSpxRequest: We have been dispatched to answer an event on the
 * SPX diagnostic socket. Attemp to receive the request. if the receive
 * fails, this is a disconnect indication, so return -1 and let our
 * caller handle it. Otherwise, ascertain the request type, dispatch the
 * appropriate handler to formulate the response, and send it off.
 *********************************************************************/
static int
DoSpxRequest(void)
{	int		flags = 0;
	int		nbytes, addBytes = 0;
	time_t	ticksUp;
	uint8	component,
			i,
			numComponents = ipxConfig->numberOfComponents,
			numNets,
			*ptr;

	time(&diagStatistics.TimeOfLastReq);
	spxDiagResp.cCode = 0xFF;

	if((nbytes = t_rcv(polls.pollFds[SPX_DIAG_FD].fd,
			(char *)&spxDiagReq, 534, &flags)) != -1) {
/* check for min pkt length ??? */
		if(nbytes < 2) {
			goto sendReply;
		}
		nbytes -= 2;
		if(spxDiagReq.index >= numComponents) {
			goto sendReply;
		}

		ptr = (uint8 *)&ipxConfig->componentStructure;
		component = *ptr;
		while(spxDiagReq.index--) {
			/* walk to next legal offset in ipxConfig struct */
			if(component > IPX_ONLY) {
				goto sendReply;
			}
			if((component < EXTERNAL_BRIDGE) || (component == IPX_ONLY))
				ptr++;	/* skip past one-byte field */
			else {	/* skip past bridgeDriver_t component */
				numNets = *(++ptr);
				for(i=0; i<numNets; i++)
					ptr += sizeof(lanDriver_t);
			}
			component = *ptr;
		}

#ifdef DEBUG
		if(ptr >= ((uint8 *)ipxConfig + ipxConfigSize)) {
			fprintf(stderr, "bogus component - ipxComponent has been stomped on !\n");
			goto sendReply;
		}
#endif	/* DEBUG */

		ptr = &spxDiagReq.data[0];
		switch (component)
		{	case IPX_SPX_COMPONENT:
				switch(spxDiagReq.type)
				{	case IPXSPX_VERSION:
						addBytes = IpxSpxReturnIpxSpxVersion();
						diagStatistics.IPXSPXReqs ++;
						break;
					case IPXSPX_RTN_IPX_STATS:
						addBytes = IpxSpxReturnIpxStats();
						diagStatistics.IPXSPXReqs ++;
						break;
					case IPXSPX_RTN_SPX_STATS:
						addBytes = IpxSpxReturnSpxStats();
						diagStatistics.IPXSPXReqs ++;
						break;
					case IPXSPX_START_SEND:
					case IPXSPX_ABORT_SEND:
						addBytes = IpxSpxPtPtSend(spxDiagReq.type, ptr, nbytes);
						diagStatistics.IPXSPXReqs ++;
						break;
					case IPXSPX_START_PKT_CNT:
					case IPXSPX_RTN_RCVD_PKT_CNT:
						addBytes = IpxSpxCountPkts(spxDiagReq.type, nbytes);
						diagStatistics.IPXSPXReqs ++;
						break;

					default:
						diagStatistics.UnknownReqs ++;
						break;
				}
				break;

			case BRIDGE_DRIVER_COMPONENT:
				switch(spxDiagReq.type)
				{	case BDRIVER_RTN_STATUS:
						addBytes = BDriverReturnStatus();
						diagStatistics.LanDvrReqs ++;
						break;
					case BDRIVER_RTN_CONFIG:
						addBytes = BDriverReturnConfig(ptr, nbytes);
						diagStatistics.LanDvrReqs ++;
						break;
					case BDRIVER_RTN_DIAG_STATS:
						addBytes = BDriverReturnStats(nbytes);
						diagStatistics.LanDvrReqs ++;
						break;

					default:
						diagStatistics.UnknownReqs ++;
						break;
				}
				break;

			case EXTERNAL_BRIDGE:
			case INTERNAL_BRIDGE:
				switch(spxDiagReq.type)
				{	case BRIDGE_RTN_LOCAL_TABLES:
						addBytes = BridgeRtnLocalTables();
						if(component == INTERNAL_BRIDGE)
							diagStatistics.FileSrvReqs ++;
						else
							diagStatistics.ExtBridgeReqs ++;
  						break;
					case BRIDGE_RTN_ALL_KNOWN_NETS:
						addBytes = BridgeRtnAllKnownNets(ptr, nbytes);
						if(component == INTERNAL_BRIDGE)
							diagStatistics.FileSrvReqs ++;
						else
							diagStatistics.ExtBridgeReqs ++;
						break;
					case BRIDGE_RTN_SPECIFIC_NET_INFO:
						addBytes = BridgeRtnSpecificNetInfo(ptr, nbytes);
						if(component == INTERNAL_BRIDGE)
							diagStatistics.FileSrvReqs ++;
						else
							diagStatistics.ExtBridgeReqs ++;
						break;
					case BRIDGE_RTN_ALL_KNOWN_SERVERS:
						addBytes = BridgeRtnAllKnownServers(ptr, nbytes);
						if(component == INTERNAL_BRIDGE)
							diagStatistics.FileSrvReqs ++;
						else
							diagStatistics.ExtBridgeReqs ++;
						break;
					case BRIDGE_RTN_SPECIFIC_SERVER_INFO:
						addBytes = BridgeRtnSpecificServerInfo(ptr, nbytes);
						if(component == INTERNAL_BRIDGE)
							diagStatistics.FileSrvReqs ++;
						else
							diagStatistics.ExtBridgeReqs ++;
						break;

					case BRIDGE_RTN_STATS:
						/* addBytes = BridgeRtnStats(); */
					case BRIDGE_RESET_LAN_BOARD:
						/* addBytes = BridgeResetLanBoard(); */
					case BRIDGE_REINIT_ROUTING_TABLES:
						/* addBytes = BridgeReinitRoutingTables(); */
					default:
						diagStatistics.UnknownReqs ++;
						break;
				}
				break;

			default:
				diagStatistics.UnknownReqs ++;
				break;
		}

	sendReply:
		if(addBytes < 0)
			goto Return;
		/* pretend we speak pc clock ticks */
		/* static time0 ??? */
		ticksUp = REVGETINT32((int) ((60.0 * 60.0 / (float)0xFFFF) * difftime(time(NULL), time0)));
		GETALIGN32(&ticksUp,&(spxDiagResp.interval[0]));

		if(t_snd(polls.pollFds[SPX_DIAG_FD].fd,
				(char *)&spxDiagResp, 5 + addBytes, 0) < 0) {
			fprintf(stderr, MsgGetStr(DIAG_FUNC_FAIL), "t_snd");
			t_error("");
			exit(-1);
		}
	Return:
		return(flags & T_MORE);
	}
	return(-1);
}

/*********************************************************************/
/* IpxSpxReturnIpxSpxVersion:	IPX/SPX component, request type 0
 * Return the version numbersof IPX and SPX that we are running.
 ********************************************************************/
static int
IpxSpxReturnIpxSpxVersion(void)
{	spxDiagResp.cCode = 0x00;
	spxDiagResp.data[0] = IPX_MAJ_VER;
	spxDiagResp.data[1] = IPX_MIN_VER;
	spxDiagResp.data[2] = SPX_MAJ_VER;
	spxDiagResp.data[3] = SPX_MIN_VER;

	return(4);
}

/*********************************************************************/
/* IpxSpxReturnIpxStats:	IPX/SPX component, request type 1
 * Return statistics kept by IPX.	NWU - stats not currently
 * implemented - when drivers are instrumented, this function needs
 * to be updated to return meaningful numbers. For now we just lie.
 ********************************************************************/
static int
IpxSpxReturnIpxStats(void)
{	int		i;
	uint16	maxSocks;
	IpxStats_t	*fakeStruct = (IpxStats_t *)spxDiagResp.data;
	int		Cret;

	if((Cret = NWCMGetParam( IPX_SOCKETS, NWCP_INTEGER, &i)) != SUCCESS) {
		fprintf(stderr, MsgGetStr(DIAG_CFG), IPX_SOCKETS);
		NWCMPerror(Cret, "");
	}
	maxSocks = REVGETINT16(i);

	GETALIGN16(&maxSocks, &(fakeStruct->maxConfiguredSocketsCount[0]));

	for(i=0; i<2; i++) {
		fakeStruct->malformedPacketCount[i] = 0;
		fakeStruct->postponedAESEventCount[i] = 0;
		fakeStruct->maxOpenSocketsCount[i] = 0;
		fakeStruct->openSocketFailureCount[i] = 0;
		fakeStruct->ECBCancelFailureCount[i] = 0;
		fakeStruct->findRouteFailureCount[i] = 0;
	}

	for(i=0; i<4; i++) {
		fakeStruct->sendPacketCount[i] = 0;
		fakeStruct->getECBRequestCount[i] = 0;
		fakeStruct->getECBFailureCount[i] = 0;
		fakeStruct->AESEventCount[i] = 0;
		fakeStruct->listenECBCount[i] = 0;
	}
	spxDiagResp.cCode = 0x00;
	return(34);
}

/*********************************************************************/
/* IpxSpxReturnSpxStats:	IPX/SPX component, request type 2
 * Return statistics kept by SPX.	NWU - stats not currently
 * implemented - when drivers are instrumented, this function needs
 * to be updated to return meaningful numbers. For now we just lie.
	fakeUint16 = (uint16 *)&(fakeStruct->maxConnectionsCount[0]);
	*fakeUint16 = REVGETINT16(SPX_MAX_CONNECTIONS);
	Need to do ioctl to get real numbers ???
 ********************************************************************/
static int
IpxSpxReturnSpxStats(void)
{
	int		i, Cret;
	uint16	maxConx;
	SpxStats_t	*fakeStruct = (SpxStats_t *)spxDiagResp.data;

	if((Cret = NWCMGetParam( SPX_SOCKETS, NWCP_INTEGER, &i)) != SUCCESS) {
		fprintf(stderr, MsgGetStr(DIAG_CFG), SPX_SOCKETS);
		NWCMPerror(Cret, "");
	}
	maxConx = REVGETINT16(i);
	GETALIGN16(&maxConx, &(fakeStruct->maxConnectionsCount[0]));

	for(i=0; i<2; i++) {
		fakeStruct->maxUsedConnectionsCount[i];
		fakeStruct->establishConnectionRequest[i];
		fakeStruct->establishConnectionFailure[i];
		fakeStruct->listenConnectionRequestCount[i];
		fakeStruct->listenConnectionFailureCount[i];
		fakeStruct->badSendPacketCount[i];
		fakeStruct->sendFailureCount[i];
		fakeStruct->abortConnectionCount[i];
		fakeStruct->badListenPacketCount[i];
		fakeStruct->badIncomingPacketCount[i];
		fakeStruct->suppressedPacketCount[i];
		fakeStruct->noSessionListenECBCount[i];
		fakeStruct->watchdogDestroySessionCount[i];
	}
	for(i=0; i<4; i++) {
		fakeStruct->sendPacketCount[i];
		fakeStruct->windowChokeCount[i];
		fakeStruct->listenPacketCount[i];
		fakeStruct->incomingPacketCount[i];
	}

	spxDiagResp.cCode = 0x00;
	return(44);
}

/*********************************************************************/
/* IpxSpxPtPtSend:	IPX/SPX component, request type 3
 * Handle sending of packets to an assigned address. This function must
 * be re-entrant, since it can indirectly recurse on itself. Called
 * in two different instances - to initiate packet sending, and to abort
 * sending before all requested packets have been sent. Send packets at
 * a pace and volume determined by parameters in the "Start Sending
 * Packets" request packet.
 ********************************************************************/
static int
IpxSpxPtPtSend(uint8 action, uint8 *ptr, int nbytes)
{	static char	sndPkt[] =
"Well, this here's just a little ramblin' bit of prose to be sent across the net for an SPX Diagnostics point-to-point test. Send it off, see where it goes, count 'em on the other end, and hope for the best. Send packetsPerTickInterval packets off every timerTickInterval ticks (1/18th sec), until a total of numberOfPackets packets have been sent. The first pkt sent will be pktSize bytes long (adjusted to between 30 and 512, inclusive), and adjusted up or down by changeSize bytes on subsequent sends.";
	int		i, fd;
	int		curPktSize;
	uint16	sentPktCount, xmitErrCount;
	double	msPerSend;
	time_t	start, lastSend;
	sendPktsRequest_t	*sendPktsRequest = (sendPktsRequest_t *)ptr;

	static int		initialized = 0,
					sendingNow = 0;
	static uint8	queryAddr[12],
					ipxPktType = (uint8)0;
	static struct t_info	info;
	static struct t_unitdata	udSend;

	if(action == IPXSPX_START_SEND) {
		if(sendingNow || (nbytes != 26))
			return(0);
		sentPktCount = 0;
		xmitErrCount = 0;
	} else {	/* IPXSPX_ABORT_SEND */
		sendingNow = 0;
		return(-1);
	}

	if(!initialized) {
		if((fd = t_open(ipxDevice, O_WRONLY, &info)) <0) {
			fprintf(stderr, MsgGetStr(DIAG_TOPEN_FAIL), ipxDevice);
			t_error("");
			return(-1);
		}
		if(t_bind(fd, NULL, NULL) <0) {
			fprintf(stderr, MsgGetStr(DIAG_TBIND_FAIL), ipxDevice);
			t_error("");
			t_close(fd);
			return(-1);
		}

		udSend.addr.len = sizeof(ipxAddr_t);
		udSend.addr.buf = (char *)&queryAddr[0];
		udSend.opt.len = sizeof(ipxPktType);
		udSend.opt.buf = (char *)&ipxPktType;
		polls.pollFds[IPX_SND_PKTS_FD].fd = fd;
		/* POLLIN is bogus */
		polls.pollFds[IPX_SND_PKTS_FD].events = POLLIN;
		polls.pollFds[IPX_SND_PKTS_FD].revents = 0;

		initialized = 1;
	}
	sendingNow = 1;

	sendPktsRequest->numberOfPkts = REVGETINT16(sendPktsRequest->numberOfPkts);
	sendPktsRequest->changeSize = REVGETINT16(sendPktsRequest->changeSize);
	sendPktsRequest->pktSize = REVGETINT16(sendPktsRequest->pktSize);
	curPktSize = GETINT16(sendPktsRequest->pktSize) - IPX_HDR_SIZE;
	for(i=0; i<info.addr; i++)
		queryAddr[i] = sendPktsRequest->target[i];

	time(&start);
	time(&lastSend);
	msPerSend = (double)sendPktsRequest->ticksPerInterval
				* (1000.0/18.0)
				/ (double)sendPktsRequest->pktsPerInterval;
	for(sentPktCount = 0; sentPktCount < sendPktsRequest->numberOfPkts;
			++sentPktCount) {
		curPktSize += sendPktsRequest->changeSize;
		if(curPktSize < 0) {
			curPktSize = 0;
			sendPktsRequest->changeSize = -sendPktsRequest->changeSize;
		}
		if(curPktSize > 482) {
			curPktSize = 482;
			sendPktsRequest->changeSize = -sendPktsRequest->changeSize;
		}
		if((difftime(time(0), lastSend) * 1000.0) < msPerSend) {
#ifdef HARDDEBUG
			printf("polling for %d ms, %d ms elapsed\n",
				(int)msPerSend, (int)(difftime(time(0), lastSend) * 1000.0));
#endif
			PollForEvents(msPerSend);
		}
		if(!sendingNow)
			break;
		time(&lastSend);
		udSend.udata.len = curPktSize;
		udSend.udata.buf = sndPkt;
		if(t_sndudata(polls.pollFds[IPX_SND_PKTS_FD].fd, &udSend) <0) {
			fprintf(stderr, MsgGetStr(DIAG_FUNC_FAIL), "t_sndudata ");
			t_error("");
			++xmitErrCount;
		}
	}

	/* doneSending: */
	spxDiagResp.cCode = 0x00;
	xmitErrCount = REVGETINT16(xmitErrCount);
	IPXCOPYSOCK(&xmitErrCount, &spxDiagResp.data[0]);
	sendingNow = 0;
	return(sizeof(xmitErrCount));
}

/*********************************************************************/
/* IpxSpxPtPtSend:	IPX/SPX component, request type 4
 * When entered initially, it is to respond to a "Start Counting
 * Packets" request. Subsequently, packets on an IPX socket, and we
 * are dispatched to receive and count the packet. Finally, we are
 * dispatched to stop counting and return the received packet count.
 *********************************************************************/
static int
IpxSpxCountPkts(uint8 action, int nbytes)
{
	int		fd, flags;
	struct t_info	info;
	struct t_bind	*bind;

	static int		initialized = 0,
					countingNow = 0;
	static char		rcvPkt[512];
	static uint8	queryAddr[12], ipxPktType = 0;
	static uint16	rcvdPktCount = 0,
					ipxCountPktsSock;
	static struct t_unitdata	udRcv;


	switch(action) {
		case IPXSPX_START_PKT_CNT:
			if(countingNow || nbytes)
				return(0);
			rcvdPktCount = 0;
			countingNow = 1;

			if(!initialized) {
				if((fd = t_open(ipxDevice, O_RDONLY | O_NDELAY, &info)) <0) {
					fprintf(stderr, MsgGetStr(DIAG_TOPEN_FAIL), ipxDevice);
					t_error("");
					break;
				}
				if((bind = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR)) == NULL) {
					fprintf(stderr, MsgGetStr(DIAG_TALLOC_FAIL), "t_bind");
					t_error("");
					break;
				}
				if(t_bind(fd, NULL, bind) <0) {
					printf(" error binding to %s ", ipxDevice);
					fprintf(stderr, MsgGetStr(DIAG_TBIND_FAIL), ipxDevice);
					t_error("");
					t_close(fd);
					break;
				}
				IPXCOPYSOCK(&bind->addr.buf[10], &ipxCountPktsSock);

				udRcv.addr.len = sizeof(ipxAddr_t);
				udRcv.addr.maxlen = sizeof(ipxAddr_t);
				udRcv.addr.buf = (char *)&queryAddr[0];
				udRcv.opt.len = sizeof(ipxPktType);
				udRcv.opt.maxlen = sizeof(ipxPktType);
				udRcv.opt.buf = (char *)&ipxPktType;
				udRcv.udata.maxlen = 512;
				udRcv.udata.buf = rcvPkt;

				polls.pollFds[IPX_COUNT_PKTS_FD].fd = fd;
				polls.pollFds[IPX_COUNT_PKTS_FD].events = POLLIN;
				polls.pollFds[IPX_COUNT_PKTS_FD].revents = 0;
				initialized = 1;
			}

			IPXCOPYSOCK(&ipxCountPktsSock, &spxDiagResp.data[0]);
			spxDiagResp.cCode = 0x00;
			return(sizeof(ipxSock_t));
			/*NOTREACHED*/
			break;

		case IPXSPX_RTN_RCVD_PKT_CNT:
			if(!countingNow || nbytes)
				break;
			spxDiagResp.cCode = 0x00;
			rcvdPktCount = REVGETINT16(rcvdPktCount);
			IPXCOPYSOCK(&rcvdPktCount, &spxDiagResp.data[0]);
			countingNow = 0;
			return(sizeof(rcvdPktCount));
			/*NOTREACHED*/
			break;

		case 0:
			if(nbytes)
				break;
			if(t_rcvudata(polls.pollFds[IPX_COUNT_PKTS_FD].fd, &udRcv, &flags) <0) {
				fprintf(stderr, MsgGetStr(DIAG_FUNC_FAIL), "t_rcvudata ");
				t_error("");
				break;
			}
			if(countingNow)
				++rcvdPktCount;
			else {
				fprintf(stderr, MsgGetStr(DIAG_COUNT),
					t_look(polls.pollFds[IPX_COUNT_PKTS_FD].fd));
			}
			return(-1);
			/*NOTREACHED*/
			break;
		default:
			fprintf(stderr, MsgGetStr(DIAG_BCALL), action);
			break;
	}
	return(0);
}

/*********************************************************************/
/* BDriverReturnStatus:	Bridge Driver component, request type 0
 * Return the status of all possible connected lans
 *********************************************************************/
static int
BDriverReturnStatus(void)
{	uint8	*ptr = &spxDiagResp.data[0];
	int		i = 0;

	if(lanInfoTable) {
		for(i=0; i<ipxMaxConnectedLans; i++)
			/* We don't know how to tell if
			 * BDRIVER_BOARD_DEAD.
			if(i && (lanInfoTable[i].state == IPX_IDLE))
			 */
			if(lanInfoTable[i].state || !i)
				*ptr++ = BDRIVER_BOARD_RUNNING;
			else
				*ptr++ = BDRIVER_BOARD_NOEXIST;
		spxDiagResp.cCode = 0x00;
	}

	return(i);
}

/*********************************************************************/
/* BDriverReturnConfig:	Bridge Driver component, request type 2
 * Return the detailed configuration of each Lan card driver.
 * NWU - return Bogus info, as we do not have access to such info.
 *********************************************************************/
static int
BDriverReturnConfig(uint8 *ptr, int nbytes)
{	char	*test;
	bDriverConfig_t	*fakeStruct = (bDriverConfig_t *)spxDiagResp.data;

	if((nbytes == 1) && lanInfoTable) {
		memset((void *)fakeStruct, (int)0, sizeof(bDriverConfig_t));

#ifdef FAKED_DATA
		networkAddress[4];
		nodeAddress[6];
		LANMode;
		nodeAddressType;
		maxDataSize[2];
		reserved1[2];
		LANHardwareID;
		transportTime[2];
		reserved2[11];
		majorVersion;
		minorVersion;
		ethernetFlagBits;
		selectedConfiguration;
		LANDescription[80];
		IOAddress1[2];
		IODecodeRange1[2];
		IOAddress2[2];
		IODecodeRange2[2];
		memoryAddress1[3];
		memoryDecodeRange1[2];
		memoryAddress2[3];
		memoryDecodeRange2[2];
		interruptIsUsed1;
		interruptLine1;
		interruptIsUsed2;
		interruptLine2;
		DMAIsUsed1;
		DMALine1;
		DMAIsUsed2;
		DMALine2;
		microChannelFlagBits;
		reserved3;
		textDescription[80];
#endif
		IPXCOPYNET(&lanInfoTable[*ptr].network,
					&(fakeStruct->networkAddress[0]));
		IPXCOPYNODE(&lanInfoTable[*ptr].nodeAddress[0],
					&(fakeStruct->nodeAddress[0]));
		fakeStruct->LANMode = 1;
		fakeStruct->transportTime[0] = 1;
		test = "None of your business";
		strncpy((char *)fakeStruct->textDescription,
			test, strlen(test));
		spxDiagResp.cCode = 0x00;
		return(sizeof(bDriverConfig_t));
	}
	else
		return(0);
}

/*********************************************************************/
/* BDriverReturnStats:	Bridge Driver component, request type 3
 * NWU - Lan drivers not instrumented or info not accessible, so lie.
 *********************************************************************/
static int
BDriverReturnStats(int nbytes)
{	bDriverStats_t	*fakeStruct = (bDriverStats_t *)spxDiagResp.data;

	if((nbytes == 1) && lanInfoTable) {
		memset((void *)fakeStruct, (char)0, 36);
#ifdef FAKED_DATA
		driverVersion[2];
		statisticsVersion[2];
		totalTxPacketCount[4];
		totalRxPacketCount[4];
		noECBAvailableCount[2];
		packetTxTooBigCount[2];
		packetTxTooSmallCount[2];
		packetRxOverflowCount[2];
		packetRxTooBigCount[2];
		packetRxTooSmallCount[2];
		packetTxMiscErrorCount[2];
		packetRxMiscErrorCount[2];
		retryTxCount[2];
		checksumErrorCount[2];
		hardwareRxMismatchCount[2];
		numberOfCustomVariables[2];
		variableData[495];
#endif
		spxDiagResp.cCode = 0x00;
		return(36);
	} else
		return(0);
}

/*********************************************************************/
/* BridgeRtnStats:	Bridge component, request type 0
 * NWU - not instrumented, so return failure.
 *********************************************************************/
static int
BridgeRtnStats(void)
{	spxDiagResp.cCode = 0xFF;
	return(0);
}

/*********************************************************************/
/* BridgeRtnLocalTables:	Bridge component, request type 1
 * Identify attached LANs (those one hop away).
 *********************************************************************/
static int
BridgeRtnLocalTables(void)
{	uint8	*ptr = &spxDiagResp.data[0];
	int		i;

	if(lanInfoTable) {
		for(i=0; i<ipxMaxConnectedLans; i++) {
			IPXCOPYNET(&lanInfoTable[i].network, ptr);
			ptr += sizeof(ipxNet_t);
		}
		for(i=0; i<ipxMaxConnectedLans; i++) {
			IPXCOPYNODE(&lanInfoTable[i].nodeAddress[0], ptr);
			ptr += sizeof(ipxNode_t);
		}
		spxDiagResp.cCode = 0x00;
		return(ptr - &spxDiagResp.data[0]);
	} else
		return(0);
}

/*********************************************************************/
/* BridgeRtnAllKnownNets:	Bridge component, request type 2
 * Return up to 128 of all the networks we know about.
 *********************************************************************/
static int
BridgeRtnAllKnownNets(uint8 *ptr, int nbytes)
{	int		idx;
	uint32	rtnBytes;
	uint16	skip, netCount;
	ipxNet_t	*ipxNet;

	if(nbytes != 2)
		return(0);

	skip = REVGETINT16(*(uint16 *)ptr);

	if(!NetsTblMaint())
		return(0);

	ipxNet = (ipxNet_t *)&spxDiagResp.data[2];
	for (idx = skip, netCount = 0;
			(idx < netsTbl.nElements) && (netCount < 128);
			idx++, netCount++)
		IPXCOPYNET(&netsTbl.table[idx].net, ipxNet++);

	rtnBytes = (int)netCount * sizeof(ipxNet_t) + 2;
	netCount = REVGETINT16(netCount);
	/* Cheat - 2byte copy */
	IPXCOPYSOCK(&netCount, &spxDiagResp.data[0]);
	spxDiagResp.cCode = 0x00;
	return(rtnBytes);
}

/*********************************************************************/
/* Maintain the global Networks Table used by the Bridge/Net functions.
 * Return !0 if maintenance succeeded.
 * Return 0 if IPX_GET_ROUTER_TABLE query failed.
 *********************************************************************/
static int
NetsTblMaint(void)
{
	/* signal(SIGALRM), alarm(), signal handler.
	 * In signal handler - backoff, aging,
	 * handle servers and nets, timeSinceLastQuery,
	 * timeSinceLastAlarm, queriesSinceLastAlarm,
	 * uptime (timeSinceStartup), reset
	 * serversTblInitialized so next request triggers
	 * update.
	 */
	if(netsTbl.initialized)
		return(1);
	return(InitNetsTbl());
}

/*********************************************************************/
/* Initialize the global Networks Table. Issue IPX_GET_ROUTER_TABLE
 * ioctl to IPX, then gather responses. Return !0 if the initialization
 * completed successfully, else return 0.
 *********************************************************************/
static int
InitNetsTbl(void)
{	int		flags;
	int		nReplyEntries;
	char	routeBuffer[ROUTE_TABLE_SIZE];
	routeInfo_t		*tmp;
	time_t	start;
	static time_t	init_time;
	static int		initialized = 0;
	static struct strioctl	ioc = {0};
	static struct strbuf	data = {0};

	if(!initialized) {
#ifdef WHATISTHIS
	/* Need to occasionally uninitialize so data isn't stale ???*/
#endif
		ioc.ic_cmd = RIPX_GET_ROUTER_TABLE;
		ioc.ic_timout = 0;
		ioc.ic_len = 0;
		ioc.ic_dp = NULL;

		data.maxlen = sizeof(routeBuffer);
		data.buf = routeBuffer;

		time(&init_time);
		initialized = 1;
	}
	time(&start);

	/* IPX_QUERY_SAPD_FD ??? */
	if (ioctl(polls.pollFds[RIPX_DIAG_FD].fd, I_STR, &ioc) < 0) {
		(void) fprintf(stderr, MsgGetStr(DIAG_IOCTL), "RIPX_DIAG_FD");
		perror("");
		/* Flush any messages at the stream head ??? */
		return(0);
	}

	flags = 0;
	for( ;; ) {
		if (getmsg(polls.pollFds[RIPX_DIAG_FD].fd, (struct strbuf *) NULL, &data, &flags) < 0) {
			if (errno == EINTR) {
				continue;
			}
			fprintf(stderr, MsgGetStr(DIAG_FUNC_FAIL), "getmsg");
			perror("");
			goto badExit;
		}

		if( (data.len != 0) && (data.len % sizeof(routeInfo_t))) {
			fprintf(stderr, MsgGetStr(DIAG_SIZE), data.len);
			goto badExit;
		}

		nReplyEntries = data.len / sizeof(routeInfo_t);
		if(netsTbl.table)
			tmp = (routeInfo_t *)
					realloc(netsTbl.table,
							(netsTbl.nElements + nReplyEntries)
							* sizeof(routeInfo_t));
		else /* Since Sun doesn't do realloc right */
			tmp = (routeInfo_t *)
					malloc((netsTbl.nElements + nReplyEntries)
							* sizeof(routeInfo_t));
		if(!tmp) {
			goto badExit;
		}

		memcpy((char *)&tmp[netsTbl.nElements], routeBuffer, data.len);
		netsTbl.table = tmp;
		netsTbl.nElements += nReplyEntries;
		if(((routeInfo_t *)routeBuffer)[nReplyEntries - 1].endOfTable)
			break;
	}
#ifdef HARDDEBUG
	for(flags = 0; flags < netsTbl.nElements; flags += 50)
		 printf("Net : 0x%08X\n", netsTbl.table[flags].net);
#endif
	(void) qsort((char *)netsTbl.table, netsTbl.nElements,
					sizeof(routeInfo_t), routeInfoCmp);
	netsTbl.initialized = 1;
	return(1);

badExit:
	if(netsTbl.table) {
		free(netsTbl.table);
		netsTbl.table = NULL;
		netsTbl.initialized = 0;
		netsTbl.nElements = 0;
	}
	initialized = 0;
	return(0);
}

/*********************************************************************/
/* routeInfoCmpNet: Comparison function used by bsearch to find Nets
 * Table entries. Compare by network number.
 *********************************************************************/
static int
routeInfoCmpNet(const void *r1, const void *r2)
{	int		net1, net2;

	memcpy((void *)&net1, r1, 4);
	memcpy((void *)&net2, r2, 4);
	if (net1 > net2)
		return(1);
	if (net1 < net2)
		return(-1);
	return(0);
}

/*********************************************************************/
/* routeInfoCmp: Comparison function used by qsort to organize Nets
 * Table. Compare according to Net, then time, then hops.
 *********************************************************************/
static int
routeInfoCmp(const void *p1, const void *p2)
{
	routeInfo_t *r1;
	routeInfo_t *r2;
	r1 = (routeInfo_t *)p1;
	r2 = (routeInfo_t *)p2;
	if (r1->net < r2->net)
		return(-1);
	if (r1->net > r2->net)
		return(1);

	/* nets same - sort by time */
	if (r1->time < r2->time)
		return(-1);
	if (r1->time > r2->time)
		return(1);

	/* nets same - time same - sort by hops */
	if (r1->hops < r2->hops)
		return(-1);
	if (r1->hops > r2->hops)
		return(1);
	return(0);
}

/*********************************************************************/
/* BridgeRtnSpecificNetInfo:	Bridge component, request type 3
 * Return hops, time, and router info on a net we reported on earlier.
 *********************************************************************/
static int
BridgeRtnSpecificNetInfo(uint8 *ptr, int nbytes)
{
	uint16			ticks, numRouters;
	uint32			net;
	routeInfo_t		*routePtr;
	netInfo_t	netInfo;
	RoutingInfo_t	*routeInfo;
	SpecificNetworkInfo_t		*lanInfo;

	if(nbytes != 4)
		return(0);

	if(!NetsTblMaint())
		return(0);

	GETALIGN32(ptr, &net);

	/* fix key here */
	if(!(routePtr = (routeInfo_t *)bsearch((char *)&net,
											(char *)netsTbl.table,
											netsTbl.nElements,
											sizeof(routeInfo_t),
											routeInfoCmpNet))) {
		(void) fprintf(stderr, MsgGetStr(DIAG_NETSTBL));
		return(0);
	}
	if(GetNetEntry(net, (netInfo_t *)&netInfo)) {
		(void) fprintf(stderr, MsgGetStr(DIAG_GETNET));
		return(0);
	}

	lanInfo = (SpecificNetworkInfo_t *)&spxDiagResp.data[0];
	GETALIGN32(&netInfo.netIDNumber, lanInfo->networkAddress);
	lanInfo->hopsToNet = netInfo.hopsToNet;
	ticks = netInfo.timeToNet;
	ticks = ticks ? REVGETINT16(ticks) : REVGETINT16(1);
	GETALIGN16(&ticks, lanInfo->routeTimeToNet);
	numRouters = REVGETINT16((uint16)1);
	GETALIGN16(&numRouters, lanInfo->numberOfKnownRouters);

	/* do we need to do multiple routes here ??? */
	routeInfo = (RoutingInfo_t *)&lanInfo->routingInfo[0];
	IPXCOPYNODE(routePtr->node, routeInfo->routerForwardingAddress);
	routeInfo->routerBoardNumber = 0xFF;
	routeInfo->routeHops = routePtr->hops;
	ticks = routePtr->time;
	ticks = ticks ? REVGETINT16(ticks) : REVGETINT16(1);
	GETALIGN16(&ticks, routeInfo->routeTime);
	spxDiagResp.cCode = 0x00;
	return(sizeof(SpecificNetworkInfo_t));
}

/*********************************************************************/
/* BridgeRtnAllKnownServers:	Bridge component, request type 4
 * Return type and name of all known servers in the serversTbl,
 * 10 at a time.
 *********************************************************************/
static int
BridgeRtnAllKnownServers(uint8 *ptr, int nbytes)
{	int		idx;
	uint16	skip, serverCount, rtnBytes;
	serverInfo_t	*serverInfo;

	if(nbytes != 2)
		return(0);
	skip = REVGETINT16(*((uint16 *)ptr));
	if(!ServersTblMaint())
		return(0);
	serverInfo = (serverInfo_t *)&spxDiagResp.data[2];
	for(idx = skip, serverCount = 0;
		(idx < serversTbl.nElements) && (serverCount < 10);
		idx++, serverCount++)
	{
		GETALIGN16(&serversTbl.table[idx].serverType,
					serverInfo[serverCount].serverType);
		strncpy((char *)serverInfo[serverCount].serverName,
				(char *)serversTbl.table[idx].serverName,
				(size_t)NWMAX_SERVER_NAME_LENGTH);
	}
	rtnBytes = (serverCount * sizeof(serverInfo_t)) + 2;
	serverCount = REVGETINT16(serverCount);
	IPXCOPYSOCK(&serverCount, &spxDiagResp.data[0]);
	spxDiagResp.cCode = 0x00;
	return(rtnBytes);
}

/*********************************************************************/
/* BridgeRtnSpecificServerInfo:	Bridge component, request type 5
 * Return Type, Name, Hops, and all route info for a specific server.
 *********************************************************************/
static int
BridgeRtnSpecificServerInfo(uint8 *ptr, int nbytes)
{	uint16	hops, numRoutes = REVGETINT16((uint16)1);
	routeInfo_t		key, *routePtr;
	SAP_ID_PACKET	*serverInfo;
	specificServerInfo_t	*specificInfo;

	/* need 2 bytes for server type, along
	 * with 48 bytes for server name.
	 */
	if(nbytes != 50)
		return(0);

	/* need serversTbl *and* netsTbl
	 */
	if(!ServersTblMaint())
		return(0);
	if(!NetsTblMaint())
		return(0);

	/* find server by name in serversTbl
	 */
	if(!(serverInfo = (SAP_ID_PACKET *)bsearch(ptr,
											(char *)serversTbl.table,
											serversTbl.nElements,
											sizeof(SAP_ID_PACKET),
											SapServerCmp))) {
		(void) fprintf(stderr, MsgGetStr(DIAG_SRVRSTBL));
		return(0);
	}

	specificInfo = (specificServerInfo_t *)&spxDiagResp.data[0];
	memcpy((char *)&specificInfo->sapId, (char *)serverInfo,
			sizeof(SAP_ID_PACKET));
	hops = REVGETINT16(serverInfo->hops);
	GETALIGN16(&hops, specificInfo->sapId.hops);
	/* ??? more than one here */
	GETALIGN16(&numRoutes, specificInfo->numberOfRoutes);

	IPXCOPYNET(serverInfo->network, &key.net );
	if(!(routePtr = (routeInfo_t *)bsearch(&key, (char *)netsTbl.table,
											netsTbl.nElements,
											sizeof(routeInfo_t),
											routeInfoCmpNet))) {
		(void) fprintf(stderr, MsgGetStr(DIAG_NETSTBL));
		return(0);
	}
	hops = REVGETINT16(routePtr->hops);
	GETALIGN16(&hops,
				specificInfo->routeSourceInfo[0].routeHopsToSource);
	IPXCOPYNODE(routePtr->node,
				specificInfo->routeSourceInfo[0].routeSourceAddress);
	spxDiagResp.cCode = 0x00;
	return(sizeof(specificServerInfo_t));
}

/*********************************************************************/
/* Maintain the global Servers Table.
 * Return !0 if maintenance succeeded.
 * Return 0 if sap query failed.
 */
static int
ServersTblMaint(void)
{

#ifdef DO_THIS_IF_STALE
	if(serversTbl.table) {
		free(serversTbl.table);
		serversTbl.initialized = 0;
		serversTbl.nElements = 0;
		serversTbl.table = NULL;
	}
#endif
	/* signal(SIGALRM), alarm(), signal handler.
	 * In signal handler - backoff, aging,
	 * handle servers and nets, timeSinceLastQuery,
	 * timeSinceLastAlarm, queriesSinceLastAlarm,
	 * uptime (timeSinceStartup), reset
	 * serversTblInitialized so next request triggers
	 * update.
	 */
	if(!serversTbl.initialized)
		if(InitServersTbl(0))
			return(1);
	if(serversTbl.initialized)
		return(1);
	/* sapd ??? */
	return(0);
}

/*********************************************************************/
/* Initialize the global Servers Table.
 * Return !0 if the initialization completed
 * successfully, else return 0.
 */
static int
InitServersTbl(int	entry)
{	static struct t_unitdata	udSend;
	static struct t_unitdata	udRcv;
	static SAP_REQUEST_PACKET	sapQuery = {0};
	static ipxAddr_t	queryAddr = {{0}, {0}, {0}};
	static time_t	init_time;
	static uint16	sapSock = GETINT16(SAP_SOCKET);
	static uint16	sapQueryType = GETINT16(GENERAL_SERVICE_REQUEST);
	static uint16	sapServerType = GETINT16(ALL_SERVER_TYPES);
	static uint8	ipxPktType = (uint8)0;
	static int	initialized = 0;
	sapUpdatePacket_t	sapUpdate;
	SAP_ID_PACKET	*tmp;
	ipxAddr_t	inAddr;
	time_t	start;
	int		fd;
	int		flags;
	int		nPktEntries;
	int		queryWaitMs = 5000;

#ifdef WHATISTHIS
	/* Need to occasionally uninitialize so data isn't stale ???*/
#endif
	if(serversTbl.initialized)
		return(1);
	if(!initialized) {
		memset((void *)queryAddr.node, (char)0xFF, sizeof(ipxNode_t));
		IPXCOPYSOCK(&sapSock, queryAddr.sock);

		sapQuery.requestType = sapQueryType;
		sapQuery.serverType = sapServerType;

		udSend.addr.len = sizeof(ipxAddr_t);
		udSend.addr.buf = (char *)&queryAddr;
		udSend.opt.len = sizeof(ipxPktType);
		udSend.opt.buf = (char *)&ipxPktType;
		udSend.udata.len = sizeof(SAP_REQUEST_PACKET);
		udSend.udata.buf = (char *)&sapQuery;

		udRcv.addr.maxlen = sizeof(ipxAddr_t);
		udRcv.opt.maxlen = sizeof(ipxPktType);
		udRcv.opt.buf = (char *)&ipxPktType;
		udRcv.udata.maxlen = sizeof(sapUpdatePacket_t);

		if((fd = t_open(ipxDevice, O_RDWR, (struct t_info *)NULL)) <0) {
			fprintf(stderr, MsgGetStr(DIAG_TOPEN_FAIL), ipxDevice);
			t_error("");
			goto errorReturn;
		}
		
		if(t_bind(fd, NULL, NULL) <0) {
			fprintf(stderr, MsgGetStr(DIAG_TBIND_FAIL), ipxDevice);
			t_error("");
			goto errorReturn;
		}
		polls.pollFds[IPX_QUERY_SAPD_FD].fd = fd;
		polls.pollFds[IPX_QUERY_SAPD_FD].events = POLLIN;
		polls.pollFds[IPX_QUERY_SAPD_FD].revents = 0;
		time(&init_time);
		initialized = 1;
	}
	switch(entry) {
		case	0:	/* called from Maint */
			if(serversTbl.initializing) {
				return(0); /* ??? */
			}
			if(t_sndudata(polls.pollFds[IPX_QUERY_SAPD_FD].fd,
							&udSend) <0) {
				fprintf(stderr, MsgGetStr(DIAG_SAPQ_FAIL));
				t_error("");
				goto errorReturn;
			}
			serversTbl.initializing = 1;
			time(&start);

			/* Will loop and we'll get called re-entrantly
			 * until no more data, then we'll come back here
			 * ready to return to original caller
			 */
			PollForEvents(queryWaitMs);
			serversTbl.initializing = 0;
			break;
		case	1:	/* called from Poll() - data ready */
			udRcv.addr.buf = (char *)&inAddr;
			udRcv.udata.buf = (char *)&sapUpdate;
			if(t_rcvudata(polls.pollFds[IPX_QUERY_SAPD_FD].fd,
							&udRcv, &flags) < 0) {
				fprintf(stderr, MsgGetStr(DIAG_SAPR_FAIL));
				t_error("");
				t_rcvuderr(polls.pollFds[IPX_QUERY_SAPD_FD].fd, NULL);
				goto errorReturn;
			}
			if(udRcv.udata.len < sizeof(SAP_REPLY_PACKET)) {
					goto errorReturn;
			}
			switch(GETINT16(sapUpdate.replyType)) {
			case GETINT16(GENERAL_SERVICE_REPLY):
				if((udRcv.udata.len - 2) % sizeof(SAP_ID_PACKET)) {
					goto errorReturn;
				}
				nPktEntries = (udRcv.udata.len - 2)
								/ sizeof(SAP_ID_PACKET);
				if(nPktEntries > SAP_MAX_UPDATE) {
					goto errorReturn;
				}
				if(serversTbl.table)
					tmp = (SAP_ID_PACKET *)
							realloc(serversTbl.table,
									(serversTbl.nElements + nPktEntries)
										* sizeof(SAP_ID_PACKET));
				else /* because realloc doesn't work right on Sun */
					tmp = (SAP_ID_PACKET *)
							malloc((serversTbl.nElements + nPktEntries)
										* sizeof(SAP_ID_PACKET));
				if(!tmp) {
					goto errorReturn;
				}
#ifdef HARDDEBUG
				for(flags = 0; flags < nPktEntries; flags++)
				{	strncpy(tmp[serversTbl.nElements + flags].serverName,
							sapUpdate.sapId[flags].serverName,
							NWMAX_SERVER_NAME_LENGTH);
					if(!isalpha(*tmp[serversTbl.nElements + flags].serverName))
						printf("flags = %d  Unusual Name: %s\n", flags,
							tmp[serversTbl.nElements + flags].serverName);
					tmp[serversTbl.nElements + flags].serverType =
							sapUpdate.sapId[flags].serverType;
				}
#endif

				memcpy((char *)&tmp[serversTbl.nElements], (char *)&sapUpdate.sapId[0],
						nPktEntries * sizeof(SAP_ID_PACKET));
				serversTbl.table = tmp;
				serversTbl.nElements += nPktEntries;
				break;
			default:
				break;
			}
			goto errorReturn;
			/*NOTREACHED*/
			break;
		default:
			break;
	}

	if(!serversTbl.table || !serversTbl.nElements) {
		fprintf(stderr, MsgGetStr(DIAG_SERVERS));
		serversTbl.initializing = 0;
		goto errorReturn;
	}
	qsort(serversTbl.table, serversTbl.nElements,
			sizeof(SAP_ID_PACKET), SapServerCmp);
#ifdef HARDEBUG
	printf("	%d servers found.\n", serversTbl.nElements);
	for(flags = 0; flags < serversTbl.nElements; flags += 10)
		printf("Name : %s\n", serversTbl.table[flags].serverName);
#endif
	serversTbl.initialized = 1;
	serversTbl.initializing = 0;
	return(1);
	
errorReturn:
	t_close(fd);
	initialized = 0;
	fprintf(stderr, MsgGetStr(DIAG_SERVERT));
	return(0);
}

/*********************************************************************/
static int
SapServerCmp(const void *p1, const void *p2)
{
	int		result;
	SAP_ID_PACKET *r1, *r2;

	r1 = (SAP_ID_PACKET *)p1;
	r2 = (SAP_ID_PACKET *)p2;
	result = strncmp((char *)r1->serverName, (char *)r2->serverName,
						NWMAX_SERVER_NAME_LENGTH);
	if(result)
		return(result);
	result = r1->serverType - r2->serverType;
	return((result > 0) ? 1 : (result < 0) ? -1 : 0);
}

static void
diagExit( int status)
{
	int i;

    if( status) {
        fprintf(stderr, MsgGetStr(DIAG_KILLING_NPSD));
        for( i=0; i<20; i++) {
            if( killNPSD() == SUCCESS) {
                break;
            }
            sleep(1);
        }
        fprintf(stderr, MsgGetStr(DIAG_ERROR_EXIT));
    } else {
        fprintf(stderr, MsgGetStr(DIAG_NORMAL_EXIT));
    }
    exit(status);
}

static void
UpdateTitle(void)
{
	time_t	now;
	struct	tm *tp;
	char	timeStr[40];

	time(&now);
	if ((tp = localtime(&now)) != NULL)
		strftime(timeStr, sizeof(timeStr), "%x %X", tp);
	else
		strcpy(timeStr, MsgGetStr(DIAG_NO_TIME));

	titleStr[0] = '\0';
	strcpy(titleStr, program);
	strcat(titleStr,": ");
	strcat(titleStr, timeStr);
	return;
}

static void
InitializeLogFile(void)
{
	char logOutValue[NWCM_MAX_STRING_SIZE];
	char path[NWCM_MAX_STRING_SIZE];
	int  NWs;
	int  errorFd;

	if( (NWs = NWCMGetParam( "diagnostics_log_file", NWCP_STRING, logOutValue))
			!= SUCCESS) {
		fprintf(stderr, MsgGetStr(DIAG_CFG), "diagnostics_log_file");
		NWCMPerror(NWs, "");
		diagExit(-1);
		/*NOTREACHED*/
	}

	if( strchr( logOutValue, '/') == NULL) {
		if( (NWs = NWCMGetParam( "log_directory", NWCP_STRING, path))
				!= SUCCESS) {
			fprintf(stderr, MsgGetStr(DIAG_MAP), "log_directory");
			NWCMPerror(NWs, "");
			diagExit(-1);
			/*NOTREACHED*/
		}
		strcat(path,"/");
		strcat(path, logOutValue);
	} else {
		strcpy(path, logOutValue);
	}
	if ((errorFd = open(path,O_RDWR|O_CREAT|O_TRUNC,0644)) < 0) {
		fprintf(stderr, MsgGetStr(DIAG_OPEN_FAIL), path);
		perror("");
		diagExit(-1);
		/*NOTREACHED*/
	}
	close(2);
	dup(errorFd);
	return;
}

/*********************************************************************/
/* Stubs for NAKing unacceptable requests.
 *********************************************************************/
static int
BridgeResetLanBoard(void)
{	return(0);
}

static int
BridgeReinitRoutingTables(void)
{	return(0);
}
