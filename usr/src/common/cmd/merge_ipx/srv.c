/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mergeipx:srv.c	1.1"

/* 	Copyright (c) 1992 Univel(r)
 *	All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Univel(r).
 *	The copyright notice above does not evidence any
 *	actual or intended publication of such source code.
 */

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stropts.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <tiuser.h>
#include <stropts.h>
#include <poll.h>
#include <sys/stream.h>
#include <sys/ipc.h>
#include <sys/nwctypes.h>
#include <sys/ipx_app.h>
#include <merge.h>
#include <mpip_user.h>


#define SUCCESS 0
#define NOTDONE 1
#define FAIL -1
#define EX_BAD -1
#define LOOKAHEAD 4
#define TABSIZ	20 /*One more than MaxSockets on the client side*/
#define IPXOPENSOCKET	0
#define IPXDIE		1
#define IPXCLOSESOCKET	2
#define IPXSENDPACKET	4
#define IPXMAXDATA	1024
#define HEADERLENGTH	30
#define	PAGESIZ		4096
#define	MAXSEQNUM	7
#define CLEARTOSEND	0xff

/*The following is from sapd code attributed there to Ted Cowan*/

#define	swapint32( x) {uint32 i; i = x; x = (((i) & 0xff) << 24 ) | (((i) & 0xff00) << 8) | (((i) & 0xff0000) >> 8) | (((i) & 0xff000000) >> 24); }

struct ecbtype {
	uint8	checksum[2];
	uint8	length[2];/*have to get the interpretation*/
	uint8	xport_cntl;
	uint8	packettype;
	uint8	dnet[4];
	uint8	dnode[6];
	uint8	dsock[2];
	uint8	snet[4];
	uint8	snode[6];
	uint8	ssock[2];
	uint8	buf[IPXMAXDATA];
};
	
struct lookupitem {
	int fd;
	ipxAddr_t saddr;/*This is the socket number assigned by TLI*/
	ipxAddr_t caddr;/*This is the socket number assigned by client*/
	int next; /*This is for keeping track of freeing stuff*/
};
extern int errno;
static uint8 *MemWindow;
static uint8 *SendWindow;
static uint8 *RecvWindow;
static short SendWindowSize;
static short SendSeqNum;
static short RecvSeqNum;
static char filename[40];
static FILE *logfile;
static struct lookupitem lookuptab[TABSIZ+1];
static struct pollfd pollfds[TABSIZ+1];
static int nfds=0;
static int vpifd;
static int next; /*this is for the lookuptable*/
static long svctime = 0;
static long recvtime = 0;
static uint8	localnet[4];
static	uint8	Lannum = 0; /*Is this an AS, if so we will be on lan 1*/

/* 
   The client assigns socket number based on its calculations, on the server side we map the
   server socket number that TLI assigns with the client's address using the look up table
*/
void
add(fd, csock, saddr)
int fd;
uint8 *csock;
ipxAddr_t *saddr;
{
 int tmp;
 int i;
 
 	if ( next == -1) { 
		fprintf (logfile, "lookuptable overflow \n");
		fflush(logfile);
	  	exit(-1);
	}
 	tmp = next;
 	next = lookuptab[tmp].next;
 	lookuptab[tmp].fd = fd;
	pollfds[nfds++].fd = fd;
	if (saddr != NULL) {
		memcpy(lookuptab[tmp].saddr.sock, saddr->sock, 2);
		memcpy(lookuptab[tmp].caddr.sock, csock, 2);
	}
} 

delete(fd)
int fd;
{
	int i;

	for (i = 0; i < TABSIZ; i++) 
	if (lookuptab[i].fd ==fd) {
	  lookuptab[i].next = next;
	  lookuptab[i].fd = -1;
	  next = i;
	  break;
	}
/*This compacts the pollfds array after a delete*/
	for (i = 0; i < TABSIZ && pollfds[i].fd != fd ; i++);
	for (; i < nfds; i++) {
		pollfds[i].fd = pollfds[i+1].fd;
		pollfds[i].revents = pollfds[i+1].revents;
	}
	pollfds[i].fd = -1;
	pollfds[i].revents = 0;
	nfds--; 
}
	    

static int
initvpi(argc, argv)
int argc;
char *argv[];
{
	struct strioctl ioc;
	mpipInitT mergeinit;
	static	int	shmId;
	int	i;
	uint8	ready;

	vpifd = open("/dev/mpip", O_RDWR|O_SYNC, 0);
	if ( vpifd == FAIL) {
		fprintf(logfile, "netwsrv: open failed: errno %d\n", argc);
		fflush(logfile);
		return FAIL;
	}
	mergeinit.vm86pid = atoi(argv[1]);
	mergeinit.irqNum = atoi(argv[2]);
	mergeinit.ioBasePort  = atoi(argv[3]);

	fprintf(logfile, "netwsrv: args %d %d %d\n", mergeinit.vm86pid, mergeinit.irqNum, mergeinit.ioBasePort);
	fflush(logfile);
	ioc.ic_cmd = MPIP_INIT;
	ioc.ic_timout = INFTIM;
	ioc.ic_len = sizeof(mergeinit);
	ioc.ic_dp = (char *) &mergeinit;
	
	if (ioctl( vpifd, I_STR, &ioc) == FAIL)
	{
	fprintf(logfile, "netwsrv: I_STR failed: errno %d\n", errno);
		fflush(logfile);
	return FAIL;
	}
	shmId = atoi(argv[5]);
	MemWindow = (uint8 *)shmat(shmId, 0, 0);
	if (MemWindow == NULL) {
		fprintf(logfile, "Failed to attach shared memory with dos\n");
		fflush(logfile);
	}
	shmctl(shmId, IPC_RMID, 0);
	RecvWindow = MemWindow;
	SendWindow = (uint8 *)((uint8 *)MemWindow + PAGESIZ);
	SendSeqNum = 0;
	RecvSeqNum = 0;
	ready = 1;
	i = write(vpifd, &ready, 1);
	if (i > 0)
		return SUCCESS;
	return FAIL;
}

int
fdlookup(fd)
int fd;
{
	int i;

	for (i = 0; i < TABSIZ; i++)  {
		if (lookuptab[i].fd == fd ) {
			return(i);
		}
	}
}

int
socketlookup(socket)
uint8 *socket;
{
	int i;
	for(i= 0; i < TABSIZ; i++)
		if (memcmp(lookuptab[i].caddr.sock, socket, 2) == 0) {
   		return(lookuptab[i].fd);
		}
	return (-1);
}

void
svc_request()
{

	struct ecbtype *ecb;
	ipxAddr_t rqaddr;
	struct strbuf data;
	struct strioctl ioc;
	uint8 socket[2];
	int opcode;
	int rval;
	uint16 len;
	int fd;
	int i;
	int n;

	ecb = (struct ecbtype *)RecvWindow ;
	len = ecb->length[0];
	len <<= 8;
	len |= ecb->length[1];
	if (len == 6) {
	 /*Six byte messages are control messages from client*/
		switch (ecb->checksum[0]) {
		case IPXOPENSOCKET:
		case IPXCLOSESOCKET: 
			socket[0] = *((uint8 *)ecb + 4);
			socket[1] = *((uint8 *)ecb + 5);
			opcode = ecb->checksum[0];
		break;
		case IPXDIE:
			for (i = 0; i < nfds; i++) {
			 close(pollfds[i].fd);
			}
			unlink(filename);
			exit(0);
		}
	} else opcode = IPXSENDPACKET;
	switch(opcode)   {
	case IPXCLOSESOCKET:
		fd = socketlookup(socket);
		if (fd < 0)
			break;
		close(fd);
		delete(fd);
	break;

	case IPXOPENSOCKET:
		fd = open("/dev/ipx", O_RDWR);
		if ( fd < 0)
			fprintf(logfile, "open failed: errno %d\n", errno);
		rqaddr.sock[0] = 0;
		rqaddr.sock[1] = 0;
		ioc.ic_cmd = IPX_SET_SOCKET;
		ioc.ic_timout = 0;
		ioc.ic_len = sizeof(rqaddr.sock);
		ioc.ic_dp = (char *)&rqaddr.sock;
		fprintf (logfile, "Opened socket ");
		for (i =0; i < 2; i++)
			fprintf (logfile, "%02x ", (uint8) (socket[i]));
		fprintf (logfile, "\n");
		fflush (logfile);
		if (ioctl(fd, I_STR, &ioc) < 0) {
			fprintf(logfile, "Unable to set socket\n");
			fflush(logfile);
		}
		add(fd, socket, &rqaddr);
	break;
	case IPXSENDPACKET:
		fd = socketlookup(ecb->ssock);
		if (fd < 0)
			break;
		if (Lannum && ((ecb->dnet[0]| ecb->dnet[1] | ecb->dnet[2] | ecb->dnet[3]) == 0 ))
		memcpy(ecb->dnet, localnet, 4);
		memcpy(ecb->snet, &lookuptab[fdlookup(fd)].saddr, sizeof(ipxAddr_t)); 
		len = ecb->length[0];
		len <<= 8;
		len |= ecb->length[1];
		data.len = len;
		data.maxlen = len;
		data.buf = (char *)ecb;
		if ((rval = putmsg(fd, NULL, &data, 0)) < 0)
			fprintf(logfile, "snd failed: errno %d\n", errno);
		}
		fprintf (logfile, "Sending ");
		for (i =0; i < len; i++)
			fprintf (logfile, "%02x ", (uint8) *((uint8 *)ecb + i));
		fprintf (logfile, "\n");
		fflush (logfile);
		RecvSeqNum++;
		if (RecvSeqNum == MAXSEQNUM) {
			RecvWindow = MemWindow;
			RecvSeqNum = 0;
		} else
			RecvWindow = (uint8 *) ((uint8 *)RecvWindow + 576);
}

void
process_recv(fd)
int fd;
{

	static struct ecbtype *ecb;
	struct strbuf data;
	struct strbuf ctlinfo;
	char	junk[10];
	unsigned int len;
	int flags =0;
	int n;
	int i;

	
	ecb = (struct ecbtype *) SendWindow;
	data.len = IPXMAXDATA;
	data.maxlen = IPXMAXDATA;
	data.buf = (char *)ecb;
	ctlinfo.len = 10;
	ctlinfo.maxlen = 10;
	ctlinfo.buf = (char *)junk;

	if (getmsg(fd, &ctlinfo, &data, &flags) < 0)
		fprintf(logfile, "rcv failed: errno %d fd = %d\n", errno, fd);
	memcpy(ecb->dsock, &lookuptab[fdlookup(fd)].caddr.sock, 2); 
	ecb->xport_cntl = 0;
	len = data.len;
	fprintf (logfile, "\nRecvd ");
	for (i =0; i< len; i++)
		fprintf (logfile, "%02x ", (uint8) *((uint8 *)ecb + i));
	fprintf (logfile, "\n");
	fflush(logfile);
	SendSeqNum++;
	if (SendSeqNum == MAXSEQNUM) {
		SendWindow = (uint8 *)((uint8 *)MemWindow + PAGESIZ);
		SendSeqNum = 0;
	} else  SendWindow = (uint8 *) ((uint8 *)SendWindow + 576);
}

int
main(argc, argv)
int argc;
char *argv[];
{
	struct	t_info	info;
	netInfo_t *netinfo;
	struct strioctl ioc;
	ipxAddr_t localaddr;
	uint8 sig;
	uint8 isipxup = 1;
	uint8 socket[2];
	int fd;
	int hasstuff;
	int will_it_route;
	uint32 numlans = 1;
	int i;
	int ret;
	unsigned int tmpnet;

	strcpy(filename, "/var/spool/merge_ipx/logfile.");
	strcat(filename, argv[1]);
	logfile = fopen(filename, "w");

	for (i=0; i < argc; i++)
		fprintf(logfile, "arg %d: %s\n", i, argv[i]);
	fflush(logfile);

	fd = open("/dev/ipx", O_RDWR);
	if ( fd < 0) {
		fprintf(logfile, "t_open failed errno = %d\n", errno);
		fflush(logfile);
	}
	socket[0] = socket[1] = 0;
	ioc.ic_cmd = IPX_SET_SOCKET;
	ioc.ic_timout = 5;
	ioc.ic_len = sizeof(socket);
	ioc.ic_dp = (char *)socket;
	ret = ioctl(fd, I_STR, &ioc);
	ioc.ic_cmd = IPX_WILL_ROUTE;
	ioc.ic_timout = 1;
	ioc.ic_len = sizeof(int);
	ioc.ic_dp = (char *) &will_it_route;
	ret = ioctl(fd, I_STR, &ioc);
	if ( ret >= 0 ) {
		Lannum = 1;
		ioc.ic_cmd = IPX_GET_MAX_CONNECTED_LANS;
		ioc.ic_timout = 0;
		ioc.ic_len = sizeof(numlans);
		ioc.ic_dp = (char *) &numlans;
		if (ioctl(fd, I_STR, &ioc) < 0) {
			fprintf(logfile, "Unable to get max lans\n");
			fflush(logfile);
			isipxup = 0;
		}
	}
	netinfo = (netInfo_t *) malloc (numlans * sizeof(netInfo_t));
	ioc.ic_cmd = IPX_GET_LAN_INFO;
	ioc.ic_timout = 5;
	ioc.ic_len = sizeof(*netinfo);
	ioc.ic_dp = (char *) netinfo;
	if (ioctl(fd, I_STR, &ioc) < 0) {
		fprintf(logfile, "Unable to get lan info\n");
		fflush(logfile);
		isipxup = 0;
	}
	swapint32(netinfo[Lannum].network);
	memcpy(localnet, &netinfo[Lannum].network, 4);/*lan_1_network*/
	memcpy(localaddr.net, localnet, 4);/*lan_1_network*/
	memcpy(localaddr.node, netinfo[Lannum].nodeAddress, 6);
	close(fd);
	for (i = 0; i < TABSIZ; i++) {
		lookuptab[i].fd = -1;
		lookuptab[i].next = i + 1;
		memcpy(&lookuptab[i].saddr, &localaddr, sizeof(ipxAddr_t));
		pollfds[i].fd = -1;
		pollfds[i].events = POLLIN | POLLRDNORM;
		pollfds[i].revents = 0;
	}
	next = 0;
	lookuptab[TABSIZ].next = -1;

	if ((initvpi(argc, argv) == FAIL) || (isipxup == 0)) {
		fprintf(logfile, "Could not initialize the vpi driver\n");
		fflush(logfile);
		exit();
	}
	add(vpifd, NULL, NULL);

	while (NOTDONE) {
		hasstuff = poll(pollfds, nfds, INFTIM );
		if (pollfds[0].revents & POLLHUP) {
			for (i = 0; i < TABSIZ; i++)
				if  (pollfds[i].fd != -1)
			 		close(pollfds[i].fd);
			unlink(filename);
			exit(0); 	/*Die..*/
		}
		for ( i = 0; hasstuff > 0; i++) {
			if (pollfds[i].revents) {
				hasstuff--;
				pollfds[i].revents = 0;
				if (pollfds[i].fd  == vpifd)  {
					svc_request();
					ret = read (vpifd, &sig, 1);
					fprintf(logfile, "Read Stuff\n");
					fflush(logfile);
				}
				else  process_recv(pollfds[i].fd);
			}
		}
	}
}
