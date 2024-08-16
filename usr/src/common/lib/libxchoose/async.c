/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libxchoose:async.c	1.4"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libxchoose/async.c,v 1.3 1994/03/23 23:55:33 jodi Exp $"

#include	"util.h" 
#include	<syslog.h>
#include	<unistd.h>

/*---------------------------------------------------------------------
/*	Module:	async.c
/*
/*	Purpose:	This file contains all routines necessary to handle
/*				connection requests from remote clients.  In
/*				particular it handles the problem of asynchronize
/*				event error.
/*
/*				All but two routines in this file came from X11R5
/*				Xstreams.c file.  Really need to rethink this whole
/*				thing when time permits.  The functionality is here
/*				but the structure sucks.
/*---------------------------------------------------------------------

extern char *gettxt();
/*
 * Structure for handling multiple connection requests on the same stream.
 */

struct listenCall {
	struct t_call *CurrentCall;
	struct listenCall *NextCall;
};

struct listenQue {
	struct listenCall *QueHead;
	struct listenCall *QueTail;
};

#define	MAX_NETS	8

typedef struct {
    struct listenQue FreeList[MAX_NETS];
	struct listenQue PendingQue[MAX_NETS];
   	char   *device[MAX_NETS];
   	int   	fd[MAX_NETS];
} networkInfo;

static networkInfo Network;
int MoreConnections = 0;

#define	MAX_AUTO_BUF_LEN	256
#define EMPTY(p)	(p->QueHead == (struct listenCall *) NULL)
#define	LISTEN_QUE_SIZE	8	/* maximum # of connections for gen. listen */
#define	CLEAR		1

/*
 *	Visible Function prototypes
 */
int SetupTliCallBuf(int fd,const char *devicename);
int ReleaseTliCallBuf(int fd);
int ConnectTliClient(int conn_fd,int *MoreConnections,struct t_call **callptr);

/*
 *	Local Function prototypes
 */
static int OpenVirtualCircuit();
static int LookForEvents();
static int CheckListenQue();
static void ClearCall(), RemoveCall();
static void CopyCall();


/*
 * Following are some general queueing routines.  The call list head contains
 * a pointer to the head of the queue and to the tail of the queue.  Normally,
 * calls are added to the tail and removed from the head to ensure they are
 * processed in the order received, however, because of the possible interruption
 * of an acceptance with the resulting requeueing, it is necessary to have a
 * way to do a "priority queueing" which inserts at the head of the queue for
 * immediate processing
 */

/*
 * Que:
 *
 * add calls to tail of queue
 */


static	void
Que(head, lc, flag)
struct listenQue *head;
struct listenCall *lc;
char	 flag;
{
	if(flag == CLEAR)
		ClearCall(lc->CurrentCall);
	
	if (head->QueTail == (struct listenCall *) NULL) {
		lc->NextCall = (struct listenCall *) NULL;
		head->QueHead = head->QueTail = lc;
	}
	else {
		lc->NextCall = head->QueTail->NextCall;
		head->QueTail->NextCall = lc;
		head->QueTail = lc;
	}
}


/*
 * pQue:
 *
 * priority queuer, add calls to head of queue
 */

static void
pQue(head, lc)
struct listenQue *head;
struct listenCall *lc;
{
	if (head->QueHead == (struct listenCall *) NULL) {
		lc->NextCall = (struct listenCall *) NULL;
		head->QueHead = head->QueTail = lc;
	}
	else {
		lc->NextCall = head->QueHead;
		head->QueHead = lc;
	}
}


/*
 * dequeue:
 *
 * remove a call from the head of queue
 */


static struct listenCall *
deQue(head)
struct listenQue *head;
{
	struct listenCall *ret;

	if (head->QueHead == (struct listenCall *) NULL){
		syslog(LOG_ERR,gettxt("libxchoosemsgs:1", " Fatal error. Queue is empty (shouldn't happen)"));
		exit(1);	
	}
	ret = head->QueHead;
	head->QueHead = ret->NextCall;
	if (head->QueHead == (struct listenCall *) NULL)
		head->QueTail = (struct listenCall *) NULL;
	return(ret);
}

/*
 * ReleaseTliCallBuf
 *
 * Free all call structures associated with a transport endpoint
 */

ReleaseTliCallBuf(int fd)
{
	int		i, index;
	int		found = FALSE;
	struct  listenQue *freeq, *pendq;
	struct  listenCall *tmpc, *tmpc1;

	for(index=0; index < MAX_NETS; index++)
	{
		if ( Network.fd[index] == fd )
		{
			found = TRUE;
			break;
		}
	}
	if ( found != TRUE )
		return(-1);
	/*
	 *	Mark the fd as free for reuse
	 *	Release the devicename string memory
	 */
	Network.fd[index] = 0;
   	free(Network.device[index]);

	/*
	 *	Release the t_call structures associated with
	 *	this file descriptor and the linked list space.
	 */
	freeq = &Network.FreeList[index];
	pendq = &Network.PendingQue[index];
	tmpc1 = tmpc = freeq->QueHead;
	for (i = 0; i < LISTEN_QUE_SIZE; ++i) 
	{
		if ( tmpc == NULL )
			break;
	    t_free((char *)tmpc->CurrentCall,T_CALL);
		tmpc = tmpc->NextCall;
		free(tmpc1);
		tmpc1 = tmpc;
	}
	tmpc1 = tmpc = pendq->QueHead;
	for (i = 0; i < LISTEN_QUE_SIZE; ++i) 
	{
		if ( tmpc == NULL )
			break;
	    t_free((char *)tmpc->CurrentCall,T_CALL);
		tmpc = tmpc->NextCall;
		free(tmpc1);
		tmpc1 = tmpc;
	}
	return(1);
}
/*
 * SetupTliCallBuf
 *
 * Acquire all call structures associated with a transport endpoint
 */
SetupTliCallBuf(int fd,const char *devicename)
{
	int		i, index;
	struct	listenCall *tmp;
	int		found = FALSE;

	for(index=0; index < MAX_NETS; index++)
	{
		if ( Network.fd[index] == 0 )
		{
			found = TRUE;
			Network.fd[index] = fd;
			break;
		}
	}
	if ( found != TRUE )
		return(-1);
	
   	Network.device[index] = (char *)strdup(devicename);
	/*
	 * set up call save list for general network listen service
	 */
	for (i = 0; i < LISTEN_QUE_SIZE; ++i) 
	{
	    if((tmp = (struct listenCall *) 
									malloc(sizeof(struct listenCall))) == NULL)
	    {
			syslog(LOG_ERR, gettxt("libxchoosemsgs:2", "Malloc failed during CallList creation."));
			exit(1);
	    }
	    if((tmp->CurrentCall = (struct t_call *) 
											t_alloc(fd,T_CALL,T_ALL)) == NULL)
		{
			syslog(LOG_ERR, gettxt("libxchoosemsgs:3", "t_alloc failed during CallList creation."));
			exit(1);
	    }
	    Que(&Network.FreeList[index], tmp, CLEAR);
	}
	return(1);
}


static int
OpenVirtualCircuit(lfd)
    int     lfd;
{
	int		fd;
	int		found,index;

	for(index=0;index<MAX_NETS;index++)
	{
		if ( Network.fd[index] == lfd )
		{
			found = TRUE;
			break;
		}
	}
	if ( found != TRUE )
		return(-1);

	if ((fd = t_open( Network.device[index],  O_RDWR, NULL)) < 0)
	{
		syslog(LOG_DEBUG, "t_open %s", Network.device[index]);
		return(-1);
	}
	
	if(t_bind(fd, NULL, NULL) < 0)	
	{
		tli_error(" ",TO_SYSLOG ,LOG_DEBUG);
		t_close(fd);
		return(-1);
	}
	return(fd);
}


/*
 * ConnectTliClient
 *
 * Attempt to connect a client
 */
ConnectTliClient(conn_fd,MoreConnections,callptr)
int		conn_fd;
int		* MoreConnections;
struct	t_call **callptr;
{
	int	index;
	int	found;
	struct  listenQue *freeq, *pendq;

	for(index =0;index <MAX_NETS;index ++)
	{
		if ( Network.fd[index ] == conn_fd )
		{
			found = TRUE;
			break;
		}
	}
	if ( found != TRUE )
		return(-1);

	freeq = &Network.FreeList[index];
	pendq = &Network.PendingQue[index];

	LookForEvents(freeq, pendq, conn_fd);
	return (CheckListenQue(freeq, pendq, conn_fd, MoreConnections,callptr));
}

/*
 * LookForEvents:	handle an asynchronous event
 */

static
LookForEvents(FreeHead, PendHead, fd)
struct listenQue *FreeHead;
struct listenQue *PendHead;
int fd;
{
	struct	t_discon disc;
	struct	listenCall *current;
	struct	t_call *call;
	int		t;
	char	buf[MAX_AUTO_BUF_LEN];
	int		flag;

	if((t = t_look(fd)) < 0) 
	{
		tli_error(" ",TO_SYSLOG,LOG_DEBUG);
		return(-1);
	}
	switch (t) 
	{
		case 0:
			break;
			/* no return */
		case T_LISTEN:
			syslog(LOG_DEBUG,"LookForEvents():T_LISTEN");
			current = deQue(FreeHead);
			call = current->CurrentCall;
			if (t_listen(fd, call) < 0) 
			{
				tli_error(" ",TO_SYSLOG,LOG_DEBUG);
				return;
			}
			Que(PendHead, current, ~CLEAR);
			syslog(LOG_DEBUG," seq = %d", call->sequence);
			break;
		case T_DISCONNECT:
			syslog(LOG_DEBUG,"LookForEvents():T_DISCONNECT");
			if (t_rcvdis(fd, &disc) < 0) 
			{
				tli_error(" ",TO_SYSLOG,LOG_ERR);
				exit(1);
			}
			syslog(LOG_DEBUG,"seq = %d", disc.sequence);
			RemoveCall(FreeHead, PendHead, &disc);
			t_close(fd);
			break;
		case T_DATA :
			syslog(LOG_DEBUG,"LookForEvents():T_DATA: fd = %d",fd);
			if((t_rcv(fd, buf, MAX_AUTO_BUF_LEN, &flag)) > 0)
			{
				syslog(LOG_DEBUG," %d", fd);
				syslog(LOG_DEBUG," %s", buf);
			}
			break;
		default:
			syslog(LOG_DEBUG,"LookForEvents():default %o %x", t, t);
			break;
	}
	return(0);
}

/*
 * CheckListenQue:	try to accept a connection
 */

static int
CheckListenQue(FreeHead, PendHead, fd, MoreConnections,callptr)
struct	listenQue *FreeHead;
struct	listenQue *PendHead;
int		fd;
int		* MoreConnections;
struct	t_call **callptr;
{
	struct	listenCall *current;
	struct	t_call *call;
	int		n,nfd;
	char	*retptr, *ptr;

	if (!(EMPTY(PendHead))) 
	{
		current = deQue(PendHead);
		call = current->CurrentCall;
		if((nfd = OpenVirtualCircuit(fd)) < 0)
		{
			syslog(LOG_DEBUG, "OpenVirtualCircuit() nfd = %d",nfd);
			Que(FreeHead, current, CLEAR);
			*MoreConnections = !EMPTY(PendHead);
			t_snddis(fd,call); /* tell transport to disconnect */
			return(-1);  
		}
		n = t_accept(fd, nfd, call);
		if (n < 0)
		{
			syslog(LOG_DEBUG, "t_accept n = %d",n);
			if (t_errno == TLOOK) 
			{
				tli_error("t_accept ",TO_SYSLOG,LOG_DEBUG);
				t_close(nfd);
				syslog(LOG_DEBUG, "sequence = %d", call->sequence);
				pQue(PendHead, current);
				*MoreConnections = !EMPTY(PendHead);
				return(-1);
			}
			else 
			{
				tli_error("t_accept ",TO_SYSLOG,LOG_DEBUG);
				t_close(nfd);
				Que(FreeHead, current, CLEAR);
				*MoreConnections = !EMPTY(PendHead);
				return(-1);
			}
		}
		retptr = NULL;
		CopyCall(callptr,call);

		ptr = NULL;
		Que(FreeHead, current, CLEAR);

		*MoreConnections = !EMPTY(PendHead);
		return(nfd);
	}	

	*MoreConnections = !EMPTY(PendHead);
	return(-1);
}

/*
 * RemoveCall: remove call from pending list
 */

static void
RemoveCall(freeq, pendq, disc)
struct listenQue *freeq;
struct listenQue *pendq;
struct t_discon *disc;
{
	register struct listenCall *p, *oldp;

	if (EMPTY(pendq)) 
	{
		disc->sequence = -1;
		return;
	}
	p = pendq->QueHead;
	oldp = (struct listenCall *) NULL;
	while (p) 
	{
		if (p->CurrentCall->sequence == disc->sequence) 
		{
			if (oldp == (struct listenCall *) NULL) 
			{
				pendq->QueHead = p->NextCall;
				if (pendq->QueHead == (struct listenCall *) NULL) 
				{
					pendq->QueTail = (struct listenCall *) NULL;
				}
			}
			else if (p == pendq->QueTail) 
			{
				oldp->NextCall = p->NextCall;
				pendq->QueTail = oldp;
			}
			else 
			{
				oldp->NextCall = p->NextCall;
			}
			Que(freeq, p, CLEAR);
			disc->sequence = -1;
			return;
		}
		oldp = p;
		p = p->NextCall;
	}
	disc->sequence = -1;
	return;
}

static void
CopyCall(callptr,call)
struct	t_call **callptr;
struct	t_call	*call;
{
	char *tmp_addrbuf;
	char *tmp_optbuf;
	char *tmp_udatabuf;
	struct	t_call	*call_1;

	call_1  = *callptr;
	tmp_addrbuf = call_1->addr.buf;
	tmp_optbuf = call_1->opt.buf;
	tmp_udatabuf = call_1->udata.buf;

	(void) memcpy(call_1,call, sizeof(struct t_call));

	call_1->addr.buf = tmp_addrbuf; 
	call_1->opt.buf = tmp_optbuf; 
	call_1->udata.buf = tmp_udatabuf; 

	(void) memcpy(call_1->addr.buf,call->addr.buf, (int) call->addr.maxlen);
	(void) memcpy(call_1->opt.buf,call->opt.buf, (int) call->opt.maxlen);
	(void) memcpy(call_1->udata.buf,call->udata.buf,(int) call->udata.maxlen);
}

/*
 * ClearCall:	clear out a call structure
 */

static void
ClearCall(call)
struct t_call *call;
{

	call->sequence = 0;
	call->addr.len = 0;
	call->opt.len = 0;
	call->udata.len = 0;
	memset(call->addr.buf, 0, call->addr.maxlen);
	memset(call->opt.buf, 0, call->opt.maxlen);
	memset(call->udata.buf, 0, call->udata.maxlen);
}
