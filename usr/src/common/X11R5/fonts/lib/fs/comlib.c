/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fonts:lib/fs/comlib.c	1.5"

#define STREAMSCONN  1
#include <X11/Xlibint.h>
#include <X11/Xos.h>
#include <X11/Xauth.h>
#include <X11/Xproto.h>
#include <stdio.h>
#include <tiuser.h>		/* TLI user defs */
#include <sys/param.h>
#include <sys/utsname.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/stropts.h>
#include <netdir.h>
#include <netconfig.h>
#include <sys/poll.h>

#define	MEM_ALLIGN(ptr) ((((unsigned) (ptr + 3)) >> 2) << 2)

#define	CONNECT_TIMEOUT		60
#define	MAX_AUTO_BUF_LEN	256
#define	MAX_DISP_DIGITS		20
#define	MAX_NETS	8

typedef struct _host {
	char	host_name[32];
	int	host_len;
	struct _host *next;
} HOST;

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

#define EMPTY(p)	(p->QueHead == (struct listenCall *) NULL)


typedef struct {
	int	flags;
	char	type;
	int	display;
	char	*inputbuf;
	int	buflen;
	int	bufptr;
	int	msglen;
} IOBUFFER;

typedef struct {
	int	_nnets;
#ifdef SVR4
	struct netconfig *_net[MAX_NETS];
#else
	char	*_net[MAX_NETS];
#endif
	struct listenQue FreeList[MAX_NETS];
	struct listenQue PendingQue[MAX_NETS];
	int	_npeers;
	char	**_peer;
	int	*_peerlen;
	HOST	*_validhosts;
} networkInfo;


typedef struct _Xstream {
	int	(*SetupTheListener)();
	int	(*ConnectNewClient)();
	int	(*CallTheListener)();
	int	(*ReadFromStream)();
	int	(*BytesCanBeRead)();
	int	(*WriteToStream)();
	int	(*CloseStream)();
	int	(*CreateAddress)();
	union ext {
		int	(*NameServer)();
		networkInfo *NetInfo;
	} u;
} Xstream;

/* old shared libraries have the names already fixed */
/* #ifdef USL_COMPAT
*/
#if defined(SVR4) || defined(mips_Destiny) /*<<< mips Destiny */
#define _XsStream		xstream
#define _XSelect 		XSelect
#define _XsErrorCall		ErrorCall		
#define	_XsSetupLocalStream	SetupLocalStream
#define	_XsConnectLocalClient	ConnectLocalClient
#define	_XsOpenSpServer		OpenSpServer
#define	_XsOpenSp2Server	OpenSp2Server
#define	_XsReadLocalStream	ReadLocalStream
#define	_XsConnectTliClient	ConnectTliClient
#define _XsSetupTliStream	SetupTliStream
#define _XsCallTliServer	CallTliServer
#define	_XsCallLocalServer	CallLocalServer
#define _XsTypeOfStream		TypeOfStream
#ifdef SVR4
#define	_XsSetupNamedStream	SetupNamedStream
#ifdef SYSV386
#define _XsSetupSp2Stream	SetupSp2Stream
#define _XsSetupSpStream	SetupSpStream
#else
#define _XsSetupSp2Stream	NULL
#define _XsSetupSpStream	NULL
#endif /* SYSV386 */
#endif /* SVR4 */
#endif /* SVR4 - mips_Destiny */

#define NO_BUFFERING	0
#define BUFFERING	1

/* Network services */

#define OpenDaemonConnection	0
#define	PEER_NAME		1
#define	PEER_ALLOC		2
#define	PEER_FREE		3
#define	ConvertNetAddrToName	4
#define	ConvertNameToNetAddr	5
#define	ConvertNameToTliCall	6
#define	ConvertTliCallToName	7
#define	ConvertNameToTliBind	8

#define	UNAME_LENGTH	14

#define X_LOCAL_STREAM	0
#define X_NAMED_STREAM	1
#ifdef SYSV386          /*<<< mips/SVR4 port */
/* Enhanced Application Compatibility Support */
#define X_SP_STREAM	2
#define X_SP2_STREAM	3
/* End Enhanced Application Compatibility Support */
#endif /* SYSV386 */


#define X_TLI_STREAM	4
#define CLOSED_STREAM	-1

/*
	The following are defined in X.h. Any changes to FamilyUname
	should take X.h into consideration.
*/
/* protocol families */

/*

		#define FamilyInternet		0
		#define FamilyDECnet		1
		#define FamilyChaos		2

		*/

#define FamilyUname	3

#define X_FONT_NAMED_LISTENER "/dev/X/xfont"

#define NAMED_LISTENER "/dev/X/Nserver"
#define LOCAL_LISTENER "/dev/X/server"
#ifdef SYSV386          /*<<< mips/SVR4 port */
/* these older types of streams are required to support
   SCO and ISX client applications on the 386 */
/* Enhanced Application Compatibility Support */
#define SP_LISTENER "/dev/X"
#define SP2_LISTENER "/tmp/.X11-unix/X"
#define SP2_PATH "/tmp/.X11-unix"
#define STREAMX "/dev/spx"
#define SP2_SYM "/dev/X.isc"
/* End Enhanced Application Compatibility Support */
#endif /* SYSV386 */

#define	NAME_SERVER_NODE "/dev/X/nameserver"
#define XNETSPECDIR	"lib/net"
#define XROOTDIR "/usr/X"

#define	MAX_SIMUL_TLI_CALLS	20

#ifdef SYSV386
#define SetupNetworkInfo()   xstream[X_LOCAL_STREAM].u.NetInfo = &Network; \
	xstream[X_NAMED_STREAM].u.NetInfo = &Network; \
/* Enhanced Application Compatibility Support */ \
	xstream[X_SP_STREAM].u.NetInfo = &Network; \
	xstream[X_SP2_STREAM].u.NetInfo = &Network; \
/* End Enhanced Application Compatibility Support */ \
	xstream[X_TLI_STREAM].u.NameServer = nameserver
#else
#define SetupNetworkInfo()   xstream[X_LOCAL_STREAM].u.NetInfo = &Network; \
	xstream[X_NAMED_STREAM].u.NetInfo = &Network; \
	xstream[X_TLI_STREAM].u.NameServer = nameserver
#endif /* SYSV386 */

#define NetworkInfo (xstream[X_LOCAL_STREAM].u.NetInfo)
#define GetNetworkInfo (*xstream[X_TLI_STREAM].u.NameServer)
#define validhosts xstream[X_LOCAL_STREAM].u.NetInfo->_validhosts

/*
 *	header of messages sent by X to the nameserver 
 *      1st int: the size of the entire message.
 *	2nd int: the size of the header itself.
 *  	3rd int: the service number.
 *      4th int: the display number.
 * 	5th int: the length of the network name.
 */

#define HEADERSIZE	(5*sizeof(int))

/*
 *
 *                                 Global data
 *
 * This file should contain only those objects which must be predefined.
 */
#define NEED_EVENTS


/*
 * If possible, it is useful to have the global data default to a null value.
 * Some shared library implementations are *much* happier if there isn't any
 * global initialized data.
 */
#ifdef NULL_NOT_ZERO			/* then need to initialize */
#define SetZero(t,var,z) t var = z
#else 
#define SetZero(t,var,z) t var
#endif

#ifdef USL_SHAREDLIB			/* then need extra variables */
/*
 * If we need to define extra variables for each global
 */
#if defined(__STDC__) && !defined(UNIXCPP)  /* then ANSI C concatenation */
#define ZEROINIT(t,var,val) SetZero(t,var,val); \
  SetZero (long, _libX_##var##Flag, 0); \
  SetZero (void *, _libX_##var##Ptr, NULL)
#else /* else pcc concatenation */
#define ZEROINIT(t,var,val) SetZero(t,var,val); \
  SetZero (long, _libX_/**/var/**/Flag, 0); \
  SetZero (void *, _libX_/**/var/**/Ptr, NULL)
#endif /* concat ANSI C vs. pcc */

#else /* else not USL_SHAREDLIB */
/*
 * no extra crud
 */
#define ZEROINIT(t,var,val) SetZero (t, var, val)

#endif /* USL_SHAREDLIB */


/*
 * Error handlers; used to be in XlibInt.c
 */
typedef int (*funcptr)();
ZEROINIT (funcptr, _XErrorFunction, NULL);
ZEROINIT (funcptr, _XIOErrorFunction, NULL);
ZEROINIT (_XQEvent *, _qfree, NULL);


/*
 * Debugging information and display list; used to be in XOpenDis.c
 */
ZEROINIT (int, _Xdebug, 0);
ZEROINIT (Display *, _XHeadOfDisplayList, NULL);




#ifdef STREAMSCONN


/* The following are how the Xstream connections are used:              */
/*      1)      Local connections over pseudo-tty ports.                */
/*      2)      SVR4 local connections using named streams or SVR3.2    */
/*              local connections using streams.                        */
/*      3)      SVR4 stream pipe code. This code is proprietary and     */
/*              the actual code is not included in the MIT distribution.*/
/*      4)      remote connections using tcp                            */
/*      5)      remote connections using StarLan                        */

/*
 * descriptor block for streams connections
 */

static char _XsTypeOfStream[100] = {
			0 };

extern int write();
extern int close();
#ifdef SVR4
extern int _XsSetupNamedStream();
#ifdef SYSV386
/* Enhanced Application Compatibility Support */
extern int _XsSetupSp2Stream();
extern int _XsSetupSpStream();
/* End Enhanced Application Compatibility Support */
#endif /* SYSV386 */
#endif 
extern int _XsSetupLocalStream();
extern int _XsConnectLocalClient();
extern int _XsCallLocalServer();
extern int _XsReadLocalStream();
extern int _XsErrorCall();
extern int _XsWriteLocalStream();
extern int _XsCloseLocalStream();
extern int _XsSetupTliStream();
extern int _XsConnectTliClient();
extern int _XsCallTliServer();
extern int _XsReadTliStream();
extern int _XsWriteTliStream();
extern int _XsCloseTliStream();


static Xstream _XsStream[] = {

	{
		/* local connections using pseudo-ttys */

		_XsSetupLocalStream,
		    _XsConnectLocalClient,
		    _XsCallLocalServer,
		    _XsReadLocalStream,
		    _XsErrorCall,
		    write,
		    close,
		    NULL
	},
	{
#ifdef SVR4
		/* local connections using named streams */

		_XsSetupNamedStream,
#else
		/* local connections using streams */
		_XsSetupLocalStream,
#endif
		    _XsConnectLocalClient,
		    _XsCallLocalServer,
		    _XsReadLocalStream,
		    _XsErrorCall,
		    write,
		    close,
		    NULL
	},
	/* Enhanced Application Compatibility Support */
	{
#ifdef SVR4
		/* SVR4 stream pipe code */
		_XsSetupSpStream,
#else
		_XsSetupLocalStream,
#endif
		    _XsConnectLocalClient,
		    _XsCallLocalServer,
		    _XsReadLocalStream,
		    _XsErrorCall,
		    write,
		    close,
		    NULL
	},
	/* End Enhanced Application Compatibility Support */

	{
#ifdef SVR4
		/* SVR4 stream pipe code */
		_XsSetupSp2Stream,
#else
		_XsSetupLocalStream,
#endif
		    _XsConnectLocalClient,
		    _XsCallLocalServer,
		    _XsReadLocalStream,
		    _XsErrorCall,
		    write,
		    close,
		    NULL
	},
	{
		/* remote connections using tcp */
		_XsSetupTliStream,
		    _XsConnectTliClient,
		    _XsCallTliServer,
		    _XsReadLocalStream,
		    _XsErrorCall,
		    write,
		    close,
		    NULL
	},
	{
		/* remote connections using StarLan */
		_XsSetupTliStream,
		    _XsConnectTliClient,
		    _XsCallTliServer,
		    _XsReadLocalStream,
		    _XsErrorCall,
		    write,
		    close,
		    NULL
	}
};


#endif /* STREAMSCONN */

#ifdef XTEST1
/*
 * Stuff for input synthesis extension:
 */
/*
 * Holds the two event type codes for this extension.  The event type codes
 * for this extension may vary depending on how many extensions are installed
 * already, so the initial values given below will be added to the base event
 * code that is aquired when this extension is installed.
 *
 * These two variables must be available to programs that use this extension.
 */
int			XTestInputActionType = 0;
int			XTestFakeAckType   = 1;
#endif

/*
 * NOTE: any additional external definition NEED
 * to be inserted BELOW this point!!!
 */

/*
 * NOTE: any additional external definition NEED
 * to be inserted ABOVE this point!!!
 */


_XQEvent * _qfree = NULL;
long _qfreeFlag = 0;
void * _qfreePtr = NULL;

#define _USHORT_H	/* prevent conflicts between BSD sys/types.h and
                           interlan/il_types.h */

#define NEED_REPLIES



extern int errno;
extern char *sys_errlist[];

#ifdef SVR4
#if !defined(__STDC__)
/* buggy SVR4 include file */
char *setnetpath();
struct netconfig *getnetconfigent();
struct netconfig *getnetpath();
int endnetpath();
#endif
#endif


#ifdef DEBUG
#define PRMSG(x,a,b)	fprintf(stderr, x,a,b); fflush(stderr)
#else
#define PRMSG(x,a,b)
#endif


#define	LISTEN_QUE_SIZE	8	/* maximum # of connections for gen. listen */
#define	CLEAR		1
/*
 * Ridiculously high value for maximum number of connects per stream.
 * Transport Provider will determine actual maximum to be used.
 */

#define	MAXCONNECTIONS	100	/* maximum # of connections for gen. listen */


#define MAXLEN	80
#define BUFFERSIZE 2048
#define NOBUFFERNEEDED 512


typedef struct {
	char *DataBuffer;
	int   FirstBytePtr;
	int   LastBytePtr;
} InputBuffer;

#ifdef SVR4
InputBuffer _XsInputBuffer[MAXCONNECTIONS] = {
	NULL};
#else
#ifndef NOFILES_MAX
#define NOFILES_MAX	128
#endif
InputBuffer _XsInputBuffer[NOFILES_MAX] = {
	NULL};
#endif /* SVR4*/


static char	*ptmx = "/dev/ptmx";
static char	*dispno = "0";

#ifdef  SVR4
extern char *GetXWINHome ();
#endif

static  char _dispno[MAX_DISP_DIGITS];

extern int t_errno;

static char	** addheader();
static char	** addtliheader();

static	struct	t_bind bind_ret, bind_req;
static	struct	t_call call;
static  char	ret_buf[MAXLEN], req_buf[MAXLEN], call_buf[MAXLEN];

/*
** The following stubs functions should be kept to keep the
** att libX11_s library (Shared library) happy. In the early versions
** of XWIN, these functions were used.
*/

#ifdef SVR4
void CloseTcpStream () {
}
void WriteTcpStream () {
}
void ReadTcpStream () {
}
void CallTcpServer () {
}
void ConnectTcpClient () {
}
void SetupTcpStream () {
}

int CloseTliStream(){
}
int WriteTliStream(){
}
int ReadTliStream(){
}
int CloseLocalStream(){
}
int WriteLocalStream(){
}
#endif /* USL_COMPAT */

#ifdef USL_SHARELIB
#define fopen (*_libX_fopen)
extern FILE *fopen();
#define t_bind (*_libX_t_bind)
extern int t_bind();
#undef t_bind
#define _iob (*_libX__iob)
extern FILE _iob[];
#endif /* USL_SHARELIB */

extern int t_errno;

#if defined(SYSV) && !defined(mips_Destiny)
#define SIGNAL_T int
#else
#define SIGNAL_T void
#endif
typedef SIGNAL_T (*PFV)();
extern PFV signal();

#define SUCCESS		"1"

static networkInfo Network;
static int NameServer = -1;
static void checkNewEvent();
static int LookForEvents();
static int CheckListenQue();
static void ClearCall(), RemoveCall();
static int OpenLocalServer();
static int OpenNamedServer();
static int nameserver();

/* Routines everybody shares */



static int
ErrorCall()
{
	fprintf(stderr, "ErrorCall: invalid or unsupported subroutine call\n");
	return(-1);
}

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
register struct listenQue *head;
register struct listenCall *lc;
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
register struct listenQue *head;
register struct listenCall *lc;
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
register struct listenQue *head;
{
	register struct listenCall *ret;

	if (head->QueHead == (struct listenCall *) NULL){
		PRMSG("Fatal error. Queue is empty (shouldn't happen)\n",0,0);
		exit(1);
	}
	ret = head->QueHead;
	head->QueHead = ret->NextCall;
	if (head->QueHead == (struct listenCall *) NULL)
		head->QueTail = (struct listenCall *) NULL;
	return(ret);
}

/* Routines for handling local Named streams  */

#ifdef SVR4

static int
SetupNamedStream(servnum, servname)
int 	servnum;
char	*servname;
{
	int 	munix, sunix;
	char *	slave;
	char	buf[MAX_AUTO_BUF_LEN];
	int	type = X_NAMED_STREAM;
	int 	fld[2], ret;
	struct stat sbuf;

	PRMSG("Calling SetupNamedStream()\n",0,0);

	/* if file not there create it, depends on SetupLocalStream to decide whether
			   server already running  , no checking is done here */

	if(strcmp(servname, "xfont") == 0)
		sprintf(buf, "%s", X_FONT_NAMED_LISTENER);
	else	sprintf(buf, "%s.%d", NAMED_LISTENER, servnum);
	PRMSG("Calling SetupNamedStream()-(%s)\n",buf,0);
	fdetach(buf);
	if(stat(buf,  &sbuf)!= 0)	{
		if(errno ==ENOENT)	{
			if(( munix = creat(buf, (mode_t) 0666) ) == -1)	{
				PRMSG(" Can't create: %s\n", buf,0);
				return(-1);
			}
			close(munix);
			if(chmod(buf,(mode_t) 0666)<0)	{
				PRMSG( "Cannot chmod %s", buf,0);
				perror(" ");
				return(-1);
			}
		}
		else	{
			PRMSG("stat err=%d,-%s\n", errno, sys_errlist[errno]);
			return(-1);
		}
	}

	if(pipe(fld) != 0)	{
		fprintf(stderr,"pipe failed, errno=%d:%s\n", errno, sys_errlist[errno]);
		return(-1);
	}

	if((ret=ioctl(fld[0], I_PUSH,"connld")) != 0)	{
		fprintf(stderr,"ioctl error:%s\n", sys_errlist[errno]);
		return(-1);
	}

	if((fattach(fld[0], buf)) !=0)	{
		fprintf(stderr,"fattach failed:%s\n", sys_errlist[errno]);
		return(-1);
	}

	SetupNetworkInfo();
	TypeOfStream[fld[1]] = type;
	NetworkInfo->_nnets++;

	return(fld[1]);
}

#ifdef SYSV386		/*<<< mips/SVR4 port */
/* these older streams pipe functions are included to support
   SCO and ISX client applications on the 386 */
/* Enhanced Application Compatibility Support */

static int
SetupSpStream (display, stype)
char *display;
char *stype;
{
	struct strfdinsert fdins;	/* Used in FDINSERT ioctl's */
	int connmaster;			/* Master pipe for making connections */
	int connother;			/* Other end of connmaster */
	int errsave;			/* Place to save errno */
	char mybuf[sizeof (struct file *)];
	char	buf[MAX_AUTO_BUF_LEN];
	int disp,type = X_SP_STREAM;

	/*
			         * The server creates both ends of a stream pipe on two special
				 * minor devices of the stream pipe driver.  One of these will be the
				 * master connection, which is the value we return.  The other we
				 * simply forget;  we will hold it open as long as the server runs.
				 *
				 * The name of the special minor device is "/dev/X<n>[RS]", where
				 * <n> is the display number.  R is the request device (which is
				 * written to open a connection);  S is the server's end of the
				 * request device.
				 */

	PRMSG("Calling SetupSpStream()\n",0,0);
	disp=atoi(display);

	sprintf(buf,"%s%dS", SP_LISTENER,disp);
	PRMSG("Calling SetupSpStream()-%s\n",buf,0);

	connmaster = open (buf, O_RDWR | O_NDELAY);
	if (connmaster < 0) {
		perror(buf);
		return (-1);
	}

	sprintf(buf,"%s%dR", SP_LISTENER,disp);
	PRMSG("Calling SetupSpStream()-%s\n",buf,0);

	connother = open (buf, O_RDWR | O_NDELAY);
	if (connother < 0) {
		close (connmaster);
		perror(buf);
		return (-1);
	}

	fdins.ctlbuf.maxlen = sizeof mybuf;
	fdins.ctlbuf.len = sizeof mybuf;
	fdins.ctlbuf.buf = mybuf;
	fdins.databuf.maxlen = 0;
	fdins.databuf.len = -1;
	fdins.databuf.buf = NULL;
	fdins.fildes = connother;
	fdins.offset = 0;
	fdins.flags = 0;

	if (ioctl (connmaster, I_FDINSERT, &fdins) < 0) {
		errsave = errno;
		close (connmaster);
		close (connother);
		errno = errsave;
		perror("I_FDINSERT");
		return (-1);
	}

	TypeOfStream[connmaster] = X_SP_STREAM;
	NetworkInfo->_nnets++;

	return (connmaster);
}

static int
SetupSp2Stream (display, stype)
char *display;
char *stype;
{
	char	buf[MAX_AUTO_BUF_LEN];
	int	type = X_SP2_STREAM;
	struct stat sbuf;
	int f1,f2;
	long temp;
	struct strfdinsert stbuf;

	PRMSG("Calling SetupSp2Stream()\n",0,0);

	mkdir (SP2_PATH, 0777);
	chmod(SP2_PATH, 0777);

	sprintf(buf, "%s%d", SP2_LISTENER, atoi(display));
	unlink (buf);
	if(symlink (SP2_SYM, buf)!= 0)	{
		perror("   symlink error: ");
		PRMSG("can't link:%s, to %s\n", SP2_SYM, buf);
		return(-1);
	}

	if ((f2 = open(buf, O_RDWR)) < 0) {
		PRMSG("can't open %s\n", buf,0);
		perror("   error: ");
		return(-1);
	}

	if ((f1 = open(STREAMX, O_RDWR)) < 0) {
		PRMSG("can't open %s\n", STREAMX,0);
		perror("   error: ");
		close(f1);
		return(-1);
	}

	stbuf.databuf.maxlen = -1;
	stbuf.databuf.len = -1;
	stbuf.databuf.buf = NULL;
	stbuf.ctlbuf.maxlen = sizeof(long);
	stbuf.ctlbuf.len = sizeof(long);
	stbuf.ctlbuf.buf = (caddr_t)&temp;
	stbuf.offset = 0;
	stbuf.fildes = f2;
	stbuf.flags = 0;

	if (ioctl(f1, I_FDINSERT, &stbuf) < 0) {
		perror("  I_FDINSERT error: ");
		close(f1);
		close(f2);
		return(-1);
	}



	/*
				 * Set up a file with the name <name> which is a character
				 * special device whose major and minor dev numbers are the
				 * same as <fd>. Returns 0 on success and -1 on failure.
			
				fstat(f2, &sbuf);
				if (mknod(buf, 0020666, sbuf.st_rdev) == -1)	{
					close(f1);
					close(f2);
					return(-1);
				}
				*/


	TypeOfStream[f1] = type;
	NetworkInfo->_nnets++;

	return(f1);
}

static int
ConnectSpClient (connmaster)
int connmaster; /* Master request connection */
{
	struct strfdinsert	fdins;
	int			fd;	/* FD of new connection */
	char			mybuf[sizeof (struct file *)];
	int			slaveno; /* Loop counter */

	if (read (connmaster, &fd, 1) != 1)	/* Read the dummy byte */
		return -1;
	fd = open (STREAMX, O_RDWR);
	if (fd < 0)
		return -1;

	PRMSG("connect client from SP\n",0,0);

	fdins.ctlbuf.maxlen = sizeof mybuf;
	fdins.ctlbuf.len = sizeof mybuf;
	fdins.ctlbuf.buf = mybuf;
	fdins.databuf.maxlen = 0;
	fdins.databuf.len = -1;
	fdins.databuf.buf = NULL;
	fdins.fildes = fd;
	fdins.offset = 0;
	fdins.flags = 0;

	if (ioctl (connmaster, I_FDINSERT, &fdins) < 0) {
		close (fd);
		return -1;
	}

	TypeOfStream[fd] = TypeOfStream[connmaster];
	PRMSG("ConnectSpClient(%d) return success\n", fd,0);
	return fd;
}

/* End Enhanced Application Compatibility Support */

#endif /* SYSV386 */
#endif /* SVR4 */




/* Routines for handling local streams (streams-pipes) */

static int
SetupLocalStream(display, stype)
char *	display;
char	*stype;
{
	int 	munix, sunix;
	char *	slave;
	char	buf[MAX_AUTO_BUF_LEN];
	int	type = X_LOCAL_STREAM;
	int 	nameserver();

	PRMSG("Calling SetupLocalStream()\n",0,0);

	SetupNetworkInfo();
	dispno = display;

	NetworkInfo->_nnets = NetworkInfo->_npeers = 0;
	NetworkInfo->_peer = NULL;
	NetworkInfo->_peerlen = NULL;

#ifdef SVR4
	NetworkInfo->_net[0] = (struct netconfig *) 0;
#else
	NetworkInfo->_net[0] = (char *) 0;
#endif
	NetworkInfo->_nnets++;


	munix = atoi(display);
	/* Sun River work: we now support multiple display
				if(munix != 0){
					fprintf(stderr, "Only display # 0 can be used on this server\n");
					return(-1);
					}
			*/

	sprintf(buf, "%s.%d", LOCAL_LISTENER, munix);

	/* Sun River work: this check is now done in the server
				if((sunix = open(buf, O_RDWR)) >= 0)
				{
					fprintf(stderr, "Server is already running\n");
					close(sunix);
					return(-1);
				}
			*/
	if( (munix = open(ptmx, O_RDWR)) < 0 ){
		fprintf(stderr,"Cannot open %s", ptmx);
		perror(" ");
		return(-1);
	}
	grantpt(munix);
	unlockpt(munix);

	if(unlink(buf) < 0 && errno != ENOENT){
		close(munix);
		fprintf(stderr, "Cannot unlink %s", buf);
		perror(" ");
		return(-1);
	}

	if(! (slave = (char *) ptsname(munix))) {
		close(munix);
		perror("Cannot get slave pt-name");
		return(-1);
	}

	if( link(slave, buf) <0 ){
		close(munix);
		fprintf(stderr, "Cannot link %s to %s", slave, buf);
		perror(" ");
		return(-1);
	}
	if( chmod(buf, 0666) < 0){
		close(munix);
		fprintf(stderr, "Cannot chmod %s", buf);
		perror(" ");
		return(-1);
	}

	sunix = open(buf, O_RDWR);
	if(sunix < 0){
		fprintf(stderr, "Cannot open %s", buf);
		perror(" ");
		close(munix);
		return(-1);
	}

	TypeOfStream[munix] = type;
	TypeOfStream[sunix] = CLOSED_STREAM;

	return(munix);
}

static int
ConnectLocalClient(ufd, MoreConnections)
int	ufd;
char	* MoreConnections;
{

	int fd;
	int read_in;
	unsigned char length;
	char buf[MAX_AUTO_BUF_LEN];
#ifdef SVR4
	struct strrecvfd str;
#endif 


	PRMSG("Calling ConnectLocalClient(%d)\n", ufd,0);

	/* MoreConnections is set to zero because if any more connections are underway
			 * select() will return immediately. It is nicer if we can process all connections
			 * that exist the way we handle TLI connections by setting MoreConnections.
			 * May be I will end up doing it later.
			 */
	*MoreConnections = 0;

#ifdef SVR4

	if( TypeOfStream[ufd] == X_NAMED_STREAM)		{
		PRMSG("Calling ConnectLocalClient(%d) - thru named streams\n", ufd,0);
		if (ioctl(ufd, I_RECVFD, &str) < 0)	{
			fprintf(stderr,"I_RECVFD failed\n");
			return(-1);
		}

		TypeOfStream[str.fd] = TypeOfStream[ufd];
		PRMSG("ConnectNamedClient(%d) return success\n", str.fd,0);
		return(str.fd);
	}

#ifdef SYSV386
	/* these older streams pipe functions are included to support
			   SCO and ISX client applications on the 386 */
	/* Enhanced Application Compatibility Support */

	if( TypeOfStream[ufd] == X_SP_STREAM)	{
		PRMSG("Calling ConnectLocalClient(%d) - thru SP\n", ufd,0);
		fd = ConnectSpClient(ufd);
		TypeOfStream[fd] = TypeOfStream[ufd];
		PRMSG("ConnectSpClient(%d) return success\n", fd,0);
		return(fd);
	}

	if( TypeOfStream[ufd] == X_SP2_STREAM)	{
		PRMSG("Calling ConnectLocalClient(%d) - thru SP2\n", ufd,0);
		if (ioctl(ufd, I_RECVFD, &str) < 0)	{
			fprintf(stderr,"I_RECVFD failed\n");
			return(-1);
		}
		TypeOfStream[str.fd] = TypeOfStream[ufd];
		PRMSG("ConnectSp2Client(%d) return success\n", str.fd,0);
		return(str.fd);
	}

	/* End Enhanced Application Compatibility Support */

#endif /* SYSV386 */
#endif /* SVR4 */

	PRMSG("Calling ConnectLocalClient(%d) - thru psuedo tty\n", ufd,0);

	if( (read_in = read(ufd, &length, 1)) <= 0 ){
		if( !read_in )  /* client closed fd */
		{
			PRMSG("client closed fd\n", 0,0);
			perror("0 bytes read");
		}
		else	perror("Error in reading the local connection msg length");
		return(-1);
	}


	if( (read_in = read(ufd, buf, length)) <= 0 ){
		if( !read_in )  /* client closed fd */
			perror("0 bytes read");
		else	perror("Error in reading the local connection slave name");
		return(-1);
	}

	buf[ length ] = '\0';

	if( (fd = open(buf,O_RDWR)) < 0 ){
		strcat(buf," open fail, clientfd");
		perror(buf);
		return(-1);
	}

	write(fd,SUCCESS,1);

	TypeOfStream[fd] = TypeOfStream[ufd];
	PRMSG("ConnectLocalClient(%d) return success\n", fd,0);
	return(fd);
}

static void dummy (sig)
int sig;
{
}

static int
CallLocalServer(host, idisplay, servname)
char	*host;
int	idisplay;
char	*servname;
{
	char	buf[MAX_AUTO_BUF_LEN];
	int	type;
	char    *listener;
	int	fd;


	PRMSG("Calling CallLocalServer(%s)\n", host,0);

	sprintf(_dispno, "%d", idisplay);
	dispno = _dispno;

	/*
				 * Open channel to server
				 */


#ifdef SVR4

	listener = (char*)getenv("XLOCAL");
	listener = "NAMED";
	if ((listener == 0) ||
	    (strncmp("NAMED", listener,  3) == 0))     {
		type = X_NAMED_STREAM;
		listener = NAMED_LISTENER;
		if(strcmp("xfont", servname) == 0)
			sprintf(buf, "%s", X_FONT_NAMED_LISTENER );
		else	sprintf(buf, "%s.%d", listener, idisplay);
		if((fd = OpenNamedServer(buf)) < 0)
		{
			PRMSG("Cannot open %s\n", buf,0);
#ifdef DEBUG
			perror("XIO");  /* Sorry, but I don't have the dpy handy */
#endif
			return(-1);
		}
	}

#ifdef SYSV386
	/* these older streams pipe functions are included to support
			   SCO and ISX client applications on the 386 */
	else

		/* Enhanced Application Compatibility Support */

		if (strncmp("SP1", listener,  3) == 0)	{
			type = X_SP_STREAM;
			listener = SP_LISTENER;
			sprintf(buf,  "%s%dR",listener, idisplay);
			if((fd = OpenSpServer(buf)) < 0)
			{
				PRMSG("Cannot open %s\n", buf,0);
#ifdef DEBUG
				perror("XIO");	/* Sorry, but I don't have the dpy handy */
#endif
				return(-1);
			}
		}

		else
			if (strncmp("SP2", listener,  3) == 0)	{
				type = X_SP2_STREAM;
				listener = SP2_LISTENER;
				sprintf(buf, "%s%d", listener, idisplay);
				if((fd = OpenSp2Server(buf)) < 0)
				{
					PRMSG("Cannot open %s\n", buf,0);
#ifdef DEBUG
					perror("XIO");	/* Sorry, but I don't have the dpy handy */
#endif
					return(-1);
				}
			}

#endif /* SYSV386 */
			else	{

				/* End Enhanced Application Compatibility Support */
#endif

				type = X_LOCAL_STREAM;
				listener = LOCAL_LISTENER;
				sprintf(buf, "%s.%d", listener, idisplay);
				if((fd = OpenLocalServer(buf)) < 0)
				{
					PRMSG("Cannot open %s\n", buf,0);
#ifdef DEBUG
					perror("XIO");	/* Sorry, but I don't have the dpy handy */
#endif
					return(-1);
				}
#ifdef SVR4
			}
#endif

	TypeOfStream[fd] = type;
	if (_XsInputBuffer[fd].DataBuffer == NULL)
		if ((_XsInputBuffer[fd].DataBuffer = (char *) malloc(BUFFERSIZE)) == NULL)
		{
			errno = ENOMEM;
			perror("Client can't connect to local server");
			return (-1);
		}
	_XsInputBuffer[fd].LastBytePtr = 0;
	_XsInputBuffer[fd].FirstBytePtr = 0;

	PRMSG("Calling CallLocalServer(%s,%d) return success\n", host,fd);

	return(fd);
}




#ifdef SVR4
static int
OpenNamedServer(node)
char *node;
{
	int fld;

	PRMSG("Calling 4.0 -- opening (%s)\n", node,0);

	fld = open(node, O_RDWR);
	if(fld <0)	{
		fprintf(stderr,"OpenNamedServer failed:%s\n", sys_errlist[errno]);
		return(-1);
	}

	if(isastream(fld) != 1)	{
		fprintf(stderr,"OpenNamedServer failed: %s is not a NamedStream\n",
		    node);
		return(-1);
	}

	return (fld);
}

#ifdef SYSV386
/* these older streams pipe functions are included to support
   SCO and ISX client applications on the 386 */
/* Enhanced Application Compatibility Support */

static int
OpenSpServer(node)
char *node;
{
	int errsave;			/* Place to save errno if trouble */
	int flags;			/* Flags to getmsg call */
	int mfd;			/* Fd to talk to master */
	char mybuf[sizeof (struct file *)]; /* Buffer for linkup message */
	struct strbuf myctlbuf;		/* Control reception buffer */
	int retfd;			/* Resulting fd to talk on */

	/*
			         * The server creates both ends of a stream pipe on two special
				 * minor devices of the stream pipe driver.  One of these is the
				 * connection we will make our request on.
				 */


	PRMSG("Calling 4.0 -- opening (%s)\n", node,0);
	mfd = open (node, O_RDWR);
	if (mfd < 0) {
		perror(node);
		return -1;
	}
	PRMSG("opened %s\n", node,0);

	retfd = open (STREAMX, O_RDWR);
	if (retfd < 0) {
		errsave = errno;
		close (mfd);
		errno = errsave;
		return -1;
	}
	PRMSG("opened %s\n", STREAMX,0);

	if (write (mfd, &mfd, 1) != 1) { /* Ask for a connection */
		errsave = errno;
		close (retfd);
		close (mfd);
		errno = errsave;
		return -1;
	}

	myctlbuf.maxlen = sizeof (mybuf);
	myctlbuf.buf = mybuf;
	flags = 0;
	if (getmsg (mfd, &myctlbuf, (struct strbuf *) NULL, &flags) < 0) {
		errsave = errno;
		close (retfd);
		close (mfd);
		errno = errsave;
		return -1;
	}

	if (putmsg (retfd, &myctlbuf, (struct strbuf *) NULL, 0) < 0) {
		errsave = errno;
		close (retfd);
		close (mfd);
		errno = errsave;
		return -1;
	}

	close (mfd);
	return retfd;
}

static int
OpenSp2Server(node)
char *node;

{
	int f, f1, f2;
	long temp;
	struct strfdinsert buf;

	PRMSG("Calling 4.0 -- opening (%s)\n", node,0);
	if ((f1 = open(STREAMX, O_RDWR)) < 0) {
		return(-1);
	}

	if ((f2 = open(STREAMX, O_RDWR)) < 0) {
		close(f1);
		return(-1);
	}


	buf.databuf.maxlen = -1;
	buf.databuf.len = -1;
	buf.databuf.buf = NULL;
	buf.ctlbuf.maxlen = sizeof(long);
	buf.ctlbuf.len = sizeof(long);
	buf.ctlbuf.buf = (caddr_t)&temp;
	buf.offset = 0;
	buf.fildes = f2;
	buf.flags = 0;

	if (ioctl(f1, I_FDINSERT, &buf) < 0) {
		close(f1);
		close(f2);
		return(-1);
	}

	if ((f = open(node, O_RDWR)) < 0) {
		return(-1);
	}
	if (ioctl(f, I_SENDFD, f1) < 0) {
		close(f);
		return(-1);
	}
	close(f);
	return(f2);
}

/* End Enhanced Application Compatibility Support */
#endif /* SYSV386 */

#endif /* SVR4 */

static int
OpenLocalServer(node)
char	*node;
{
	int 	server, fd, c;
	char	buf[MAX_AUTO_BUF_LEN], *slave;
	int	(*savef)();
	/*	PFV	savef;		*/

	PRMSG("Calling OpenLocalServer -- opening (%s)\n", node,0);

	if ((server = open (node, O_RDWR)) < 0)
	{
#ifdef DEBUG
		fprintf(stderr, "open(%s) failed\n", node);
		perror(" ");
#endif
		return(-1);
	}

	/*
				 * Open streams based pipe and get slave name
				 */


	if ((fd = open (ptmx, O_RDWR)) < 0) {
		close (server);
		PRMSG("Cannot open %s\n", ptmx, 0);
		return(-1);
	}

	grantpt (fd);

	unlockpt (fd);

	if (! (slave = (char *) ptsname(fd))) {
		close(fd);
		close(server);
		PRMSG("Cannot get slave pt-name", 0, 0);
		return(-1);
	}


	if (chmod(slave, 0666) < 0)
	{
		close(fd);
		close(server);
		PRMSG("Cannot chmod %s\n", slave,0);
		return(-1);
	}

	c = strlen (slave);

	buf[0] = c;
	sprintf(&buf[1], slave);

	/*
				 * write slave name to server
				 */

	write(server, buf, c+1);
	close (server);
	/*
				 * wait for server to respond
				 */
	savef = (int (*)()) signal (SIGALRM, dummy);

	alarm (CONNECT_TIMEOUT);

	if (read (fd, &c, 1) != 1)
	{
		fprintf(stderr, "No reply from the server.\n");
		close(fd);
		fd = -1;
	}
	alarm (0);
	signal (SIGALRM, savef);

	return(fd);
}

#ifdef DEBUG
static dumpBytes (len, data)
int len;
char *data;
{
	int i;

	fprintf(stderr, "%d: ", len);
	for (i = 0; i < len; i++)
		fprintf(stderr, "%02x ", data[i] & 0377);
	fprintf(stderr, "\n");
}
#endif

static int
ReadLocalStream(fd, buf, count, do_buffering)
int	fd;
char	*buf;
int	count;
int	do_buffering;
{
	int	amount;
	InputBuffer *ioptr = &_XsInputBuffer[fd];
	PRMSG("ReadLocalStream: fd (%d) count (%d) \n", fd, count);
	if (do_buffering == NO_BUFFERING){
		errno = 0;
		amount = read(fd, buf, count);
		PRMSG("ReadLocalStream: read(%d) returns %d chars \n",
		    fd, amount );
		return (amount);
	}

	if (ioptr->LastBytePtr <= ioptr->FirstBytePtr)
	{
		errno = 0;
		if(count > BUFFERSIZE)
		{
			ioptr->LastBytePtr = ioptr->FirstBytePtr = 0;
			amount = read(fd, buf, count);
			return(amount);
		}

	ioptr->LastBytePtr = read(fd, ioptr->DataBuffer, BUFFERSIZE);
	if (ioptr->LastBytePtr == 0 &&
		TypeOfStream[fd] ==X_NAMED_STREAM) {
	    errno = EAGAIN;
	    ioptr->LastBytePtr = -1;
	}
	ioptr->FirstBytePtr = 0;
	}

	if (ioptr->LastBytePtr > 0)
	{
		amount = ioptr->LastBytePtr - ioptr->FirstBytePtr;
		amount = amount > count ? count : amount;
		memcpy(buf, &ioptr->DataBuffer[ioptr->FirstBytePtr], amount);
		ioptr->FirstBytePtr += amount;
		return amount;
	}
	else	{
		return (ioptr->LastBytePtr);
	}
}

static int
ConnectTliClient(sfd,MoreConnections)
int    sfd;
char  * MoreConnections;
{
	register	char	type = TypeOfStream[sfd];
	register	struct  listenQue *freeq, *pendq;

	freeq = &Network.FreeList[type];
	pendq = &Network.PendingQue[type];

	PRMSG("Calling ConnectTliClient(%d)\n", sfd,0);
	LookForEvents(freeq, pendq, sfd);
	return (CheckListenQue(freeq, pendq, sfd, MoreConnections));
}


static void
checkNewEvent(fd)
int	fd;
{
	int	t;

	t = t_look(fd);
	switch(t)
	{
	case T_DATA	  :
		fprintf(stderr, "T_DATA received\n");
		break;
	case T_EXDATA	  :
		fprintf(stderr, "T_EXDATA received\n");
		break;
	case T_DISCONNECT :
		t_rcvdis(fd, NULL);
		fprintf(stderr, "T_DISCONNECT received\n");
		break;
	case T_ERROR	  :
		fprintf(stderr, "T_ERROR received\n");
		break;
	case T_UDERR	  :
		fprintf(stderr, "T_UDERR received\n");
		break;
	case T_ORDREL	  :
		fprintf(stderr, "T_ORDREL received\n");
		break;
	}
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
	int	address;
	short	port, nf;
	struct t_discon disc;
	register struct listenCall *current;
	register struct t_call *call;
	int t;
	char	buf[MAX_AUTO_BUF_LEN];
	int	flag, i;

	if((t = t_look(fd)) < 0) {
		PRMSG("t_look failed. t_errno %d\n", t_errno,0);
		return(-1);
	}
	switch (t) {
	case 0:
		PRMSG("t_look 0\n",0,0);
		break;
		/* no return */
	case T_LISTEN:
		PRMSG("t_look T_LISTEN\n",0,0);
		current = deQue(FreeHead);
		call = current->CurrentCall;

		if (t_listen(fd, call) < 0) {
			PRMSG("t_listen failed\n",0,0);
			return;
		}

		/*
							if(strcmp(Network._net[TypeOfStream[fd]], "it") == 0)
							{
							nf = *(short *) call->addr.buf;
							port = *(short *) (call->addr.buf + sizeof(short));
							address = *(int *) (call->addr.buf + sizeof(short) + sizeof(short));
							printf("TCP address %d-%d-%d\n", nf, port, address);
							}
							else if(strcmp(Network._net[TypeOfStream[fd]], "starlan") == 0)
							{
							call->udata.buf[call->udata.len] = '\0';
							printf("STARLAN address %s\n", call->udata.buf);
							}
						*/
		Que(PendHead, current, ~CLEAR);
		PRMSG("incoming call seq # %d", call->sequence,0);
		break;
	case T_DISCONNECT:
		PRMSG("t_look T_DISCONNECT\n",0,0);
		if (t_rcvdis(fd, &disc) < 0) {
			PRMSG("Received T_DISCONNECT but t_rcvdis failed\n",0,0);
			exit(1);
		}
		PRMSG("incoming disconnect seq # %d", disc.sequence,0);
		RemoveCall(FreeHead, PendHead, &disc);
		t_close(fd);
		TypeOfStream[fd] = CLOSED_STREAM;
		break;
	case T_DATA :
		PRMSG("Reading from  %d\n",fd,0);
		if((i = t_rcv(fd, buf, MAX_AUTO_BUF_LEN, &flag)) > 0)
			PRMSG("Received on %d:\n%s\n", fd, buf);
		break;
	default:
		PRMSG("t_look default %o %x\n", t, t);
		break;
	}
}

/*
 * CheckListenQue:	try to accept a connection
 */

static int
CheckListenQue(FreeHead, PendHead, fd, MoreConnections)
struct listenQue *FreeHead;
struct listenQue *PendHead;
int fd;
char * MoreConnections;
{
	register struct listenCall *current;
	register struct t_call *call;
	int pid, nfd, n;
	char	*retptr, *ptr;

	int	address;
	short	port, nf;

	PRMSG( "in CheckListenQue",0,0);
	if (!(EMPTY(PendHead)))
	{
		current = deQue(PendHead);
		call = current->CurrentCall;
		PRMSG( "try to accept #%d", call->sequence,0);
		if((nfd = OpenVirtualCircuit(fd)) < 0)
		{
			PRMSG( "OpenVirtualCircuit failed\n",0,0);
			Que(FreeHead, current, CLEAR);
			*MoreConnections = !EMPTY(PendHead);
			return(-1);  /* let transport provider generate disconnect */
		}
		/*
							fprintf(stderr, "Trying to Accept a call from Network <<%s>>>\n",
									Network._net[TypeOfStream[fd]]);
						
							if(strcmp(Network._net[TypeOfStream[fd]], "it") == 0)
							{
							nf = *(short *) call->addr.buf;
							port = *(short *) (call->addr.buf + sizeof(short));
							address = *(int *) (call->addr.buf + sizeof(short) + sizeof(short));
							printf("TCP address %d-%d-%d\n", nf, port, address);
							}
							else if(strcmp(Network._net[TypeOfStream[fd]], "starlan") == 0)
							{
							call->udata.buf[call->udata.len] = '\0';
							printf("STARLAN address %s\n", call->udata.buf);
							}
						
								
							fprintf(stderr, "calling t_accept\n");
						*/

		n = t_accept(fd, nfd, call);
		if (n < 0){

			PRMSG( "t_accept failed\n",0,0);
			if (t_errno == TLOOK) {
				t_close(nfd);
				PRMSG( "t_accept collision",0,0);
				PRMSG( "save call #%d", call->sequence,0);
				pQue(PendHead, current);
				*MoreConnections = !EMPTY(PendHead);
				return(-1);
			}
			else {
				PRMSG( "t_accept failed but not t_look\n",0,0);
				t_close(nfd);
				Que(FreeHead, current, CLEAR);
				*MoreConnections = !EMPTY(PendHead);
				return(-1);
			}
		}
		TypeOfStream[nfd] = TypeOfStream[fd];
		retptr = NULL;




		if( nameserver (nfd, Network._net[TypeOfStream[fd]],
		    ConvertTliCallToName, addtliheader(call),  &retptr, NULL) <= 0)

		{
			retptr = NULL;

		}
		ptr = NULL;
		if(retptr != NULL)
		{
			ptr = retptr;
			retptr += sizeof(xHostEntry);
		}
		nameserver (nfd, Network._net[TypeOfStream[fd]], 
		    PEER_ALLOC, &retptr, NULL);
		if(ptr != NULL)
			Xfree(ptr);
		PRMSG( "Accepted call %d", call->sequence,0);
		PRMSG("Channel %d is opened\n", nfd,0);

		Que(FreeHead, current, CLEAR);


		(void) ioctl(nfd, I_POP, "timod");
		if(ioctl(nfd, I_PUSH, "tirdwr") < 0)
		{
			t_close(nfd);
			return(-1);
		}

		PRMSG( "Accepted call %d", call->sequence,0);
		PRMSG("Channel %d is opened\n", nfd,0);

		*MoreConnections = !EMPTY(PendHead);
		return(nfd);
	}

	*MoreConnections = !EMPTY(PendHead);
	return(-1);
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

	PRMSG( "Removing call, sequence # is %d", disc->sequence,0);
	if (EMPTY(pendq)) {
		disc->sequence = -1;
		return;
	}
	p = pendq->QueHead;
	oldp = (struct listenCall *) NULL;
	while (p) {
		if (p->CurrentCall->sequence == disc->sequence) {
			if (oldp == (struct listenCall *) NULL) {
				pendq->QueHead = p->NextCall;
				if (pendq->QueHead == (struct listenCall *) NULL) {
					pendq->QueTail = (struct listenCall *) NULL;
				}
			}
			else if (p == pendq->QueTail) {
				oldp->NextCall = p->NextCall;
				pendq->QueTail = oldp;
			}
			else {
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


static int
nameserver(fd, nettype, service, arg1, arg2, arg3)
int	      fd;
#ifdef SVR4
struct netconfig *nettype;
#else
char     *nettype;
#endif
int	      service;
char     **arg1, **arg2;
int	     *arg3;
{

	char	*ptr;
	int	n;
	int	type;

	PRMSG("in nameserver type %d, fd %d\n", type, fd);

	if(fd >=0 )
		type = TypeOfStream[fd];
	else	type = X_TLI_STREAM;

	if(type < X_TLI_STREAM || type >= Network._nnets)
	{
		if(type == X_LOCAL_STREAM || type == X_NAMED_STREAM)
			return(0);
		if(fd >= 0)
		{
			PRMSG( "in nameserver type %d unknown\n", type, fd);
			return(-1);
		}
	}

	if(nettype == NULL)
		nettype = Network._net[type];


	switch(service){
	case	OpenDaemonConnection :

#ifdef SVR4
		return (InitializeNetPath());
#else
		if(NameServer < 0 )
			NameServer = OpenLocalServer(NAME_SERVER_NODE);
		return(NameServer);
#endif

	case	ConvertTliCallToName :
	case	ConvertNetAddrToName :
	case	ConvertNameToNetAddr :
	case	ConvertNameToTliBind :
	case	ConvertNameToTliCall :
		if((n = CallTheNameServer(service, nettype, arg1, arg2, arg3)) < 0)
			return(-1);
		return(n);

	case	PEER_NAME  :
		if( fd < Network._npeers )
		{
			*arg2 = Network._peer[fd];
			return(1);
		}
		return(-1);
	case	PEER_ALLOC :
		if(fd >= Network._npeers)
			return(-1);

		if(*arg1 == NULL)
			n = 0;
		else	n = strlen(*arg1);

		Network._peerlen[fd] = n;

		if(n > 0){
			if(Network._peerlen[fd] > UNAME_LENGTH)
				Network._peerlen[fd] = UNAME_LENGTH;
			bcopy(*arg1, Network._peer[fd], Network._peerlen[fd]);
			Network._peer[fd][Network._peerlen[fd]] = '\0';
		}
		else {
			Network._peer[fd][0] = '\0';
		}
		return(1);

	case	PEER_FREE  :
		if(fd < Network._npeers && Network._peer[fd] != NULL)
		{
			Network._peer[fd][0] = '\0';
			Network._peerlen[fd] = 0;
		}
		return(1);
	}
}


static	int	_hlen = 0;
static	char	*_hptr = NULL;

static char	**
addheader(string, len)
char	*string;
int	len;
{

	int	n, m, p;
	char	*ptr;

	n = len;
	m = n + sizeof(xHostEntry);
	p = m + 2 * sizeof(int);

	if(p > _hlen){
		if(_hptr == NULL)
			_hptr = malloc(p);
		else	_hptr = realloc(_hptr, p);
	}
	if(_hptr == NULL){
		fprintf(stderr, "addheader(): malloc failed\n");
		exit(1);
	}
	else if(p > _hlen)
		_hlen = p;

	ptr = _hptr;

	*(int *) ptr = m;
	ptr += sizeof(int);
	*(int *) ptr = 1;
	ptr += sizeof(int);

	((xHostEntry *) ptr)-> length = n;
	ptr += sizeof(xHostEntry);
	memcpy(ptr, string, n);

	return(&_hptr);
}

static char	**
addtliheader(call)
struct t_call *call;
{


	char	*ptr;
	int	a, o, u;
	int	ra, ro, ru, rentlen;


	a = call->addr.len;
	o = call->opt.len;
	u = call->udata.len;

	ra = ((a + sizeof(xHostEntry) +3) >>2) << 2;
	ro = ((o + sizeof(xHostEntry) +3) >>2) << 2;
	ru = ((u + sizeof(xHostEntry) +3) >>2) << 2;

	rentlen = ra + ro + ru + 2 * sizeof(int);

	if(rentlen > _hlen){
		if(_hptr == NULL)
			_hptr = malloc(rentlen);
		else	_hptr = realloc(_hptr, rentlen);
	}
	if(_hptr == NULL){
		fprintf(stderr, "addtliheader(): malloc failed\n");
		exit(1);
	}
	else if(rentlen > _hlen)
		_hlen = rentlen;

	ptr = _hptr;

	*(int *) ptr = rentlen - 2 * sizeof(int);
	ptr += sizeof(int);
	*(int *) ptr = 1;
	ptr += sizeof(int);

	((xHostEntry *) ptr)-> length = a;
	if(a > 0){
		memcpy(ptr + sizeof(xHostEntry), call->addr.buf, a);
	}


	ptr += ra;
	((xHostEntry *) ptr)-> length = o;
	if(o > 0)
		memcpy(ptr + sizeof(xHostEntry), call->opt.buf, o);

	ptr += ro;
	((xHostEntry *) ptr)-> length = u;
	if(u > 0){
		memcpy(ptr + sizeof(xHostEntry), call->udata.buf, u);
	}

	return(&_hptr);
}


static int
_XBytesReadable (fd, ptr)
int fd;
int * ptr;
{
	int inbuf;
	int n;
	int flg;
	InputBuffer *ioptr = &_XsInputBuffer[fd];

	inbuf = ioptr->LastBytePtr - ioptr->FirstBytePtr;

	if (inbuf >= SIZEOF(xReply))
	{
		*ptr = inbuf;
		return (0);
	}

	if (ioptr->FirstBytePtr > 0)
	{
		/* move tidbit to front of buffer */
		bcopy(&ioptr->DataBuffer[ioptr->FirstBytePtr],
		    ioptr->DataBuffer, inbuf);

		/* Adjust pointers in buffer to reflect move */
		ioptr->LastBytePtr = inbuf;
		ioptr->FirstBytePtr = 0;
	}

	if (inbuf < 0)
	{
		inbuf = 0;
		ioptr->LastBytePtr = 0;
	}
	/* Read no more than number of bytes left in buffer */

	errno = 0;


	n = read(fd, &ioptr->DataBuffer[inbuf], BUFFERSIZE-inbuf);
	if (n > 0)
	{
		ioptr->LastBytePtr += n;
		*ptr = ioptr->LastBytePtr;
		return (0);
	}
	else
	{
		if (errno == EWOULDBLOCK)
		{
			*ptr = ioptr->LastBytePtr;
			return (0);
		}
		else
		{
			/*
									  			if (n == 0 && TypeOfStream[fd] == X_NAMED_STREAM )
									                        {
									                         		*ptr = ioptr->LastBytePtr;
									
									                                        return (0);
									
									                        }
									*/

			if (n == 0 )
			{
				errno = EPIPE;
				return (-1);
			}
			else
			{
				if (errno != EINTR)
					return (-1);
				else
				{
					*ptr = ioptr->LastBytePtr;
					return (0);
				}
			}
		}
	}
}




#ifndef SVR4


#define POLLERROR		(POLLHUP | POLLNVAL | POLLERR)
#define PFD(fds, i, x) { 	if (fds) 		if (ev & (x)) 			BITSET (fds, i); 		else 			BITCLEAR (fds, i); }
#define ERROR(x) { 	errno = x; 	return -1; }
/*
	simulate BSD select system call with SYSV poll system call
	note that efds parameter is not fully supported (or understood)
*/

extern long ulimit();

static int
pollselect (nfds, rfds, wfds, efds, timeout)
int nfds;
unsigned long *rfds;
unsigned long *wfds;
unsigned long *efds;
struct timeval *timeout;
{
	int i, rc, ev, timevalue;
	struct pollfd pfds[NOFILES_MAX];
	static long _NOFILE = 0;

	PRMSG("in pollselect\n", 0,0);

	if (_NOFILE == 0) {
		_NOFILE = ulimit(4, (long)0);
		if (_NOFILE > NOFILES_MAX)
			_NOFILE = NOFILES_MAX;
	}

	if (nfds > _NOFILE)
		nfds = _NOFILE;   /* make poll happy */

	for (i = 0; i < nfds; i++)
	{
		ev = 0;

		if (rfds && GETBIT (rfds, i)) ev |= POLLIN;
		if (wfds && GETBIT (wfds, i)) ev |= POLLOUT;
		if (ev || (efds && GETBIT (efds, i)))
			pfds[i].fd = i;
		else
			pfds[i].fd = -1;
		pfds[i].events = ev;
	}
	if (timeout)
		timevalue = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
	else
		timevalue = -1;

	while (1)	{
		rc = poll (pfds, (unsigned long)nfds, timevalue);

		if(rc<0 && errno == EAGAIN)
			continue;
		else	break;
	}
	if(rc>0)	{
		if (!efds)
			for (i = 0; i < nfds; ++i)
			{
				ev = pfds[i].revents;
				if (ev & POLLERROR)
					ERROR (EBADF);
			}

		for (i = 0; i < nfds; ++i)
		{
			ev = pfds[i].revents;
			PFD (rfds, i, POLLIN);
			PFD (wfds, i, POLLOUT);
			PFD (efds, i, POLLERROR);
		}
	}

	if(rc==0)	{
		i = (nfds+ 7)/8;
		if ( rfds != NULL)
			memset((char *) rfds, 0, i);
		if ( wfds != NULL)
			memset((char *) wfds, 0, i);
		if ( efds != NULL)
			memset((char *) efds, 0, i);

	}

	return rc;
}
#define SELECT	pollselect


#else
#define SELECT	select
#endif	/* ndef SVR4 */

static void   *handlep = NULL;
static int    family = 0;
static char   tcphalf[4];
static int    *tcpfamilyp = NULL;
static char   **tcpremoteaddrp = NULL;
static int    *tcpremoteaddrlenp = NULL;

static int SetupNetworkStream();
static int BindAndListen();
static int BindAndConnect();
static int addentry();

/* Routines for handling TLI streams */
static int
InitializeNetPath()
{
	if((handlep = setnetpath()) == NULL)
	{
		nc_perror("Cannot set network selection path\n");
		return(-1);
	}
	return(1);

}

static int
SetupTliStream(display, servname)
char *display, *servname;
{
	int	i, n;
	int	fd, type;
	struct	utsname  machine;
	struct listenCall *tmp;
	int	nameserver();
	static int firstime=1;

	PRMSG("Calling SetupTliStream()\n",0,0);

	if(InitializeNetPath() < 0)
		return(-1);

	dispno = display;

	if(uname(&machine) < 0){
		t_error("Cannot get nodename");
		return(-2);
	}

	bind_req.addr.buf = req_buf;

	/* pcc */
	if (firstime)	{
		Network._nnets = X_TLI_STREAM;
		firstime = 0;
	}

	/*	type = Network._nnets;	*/

	bind_ret.addr.buf = ret_buf;
	call.addr.buf	  = call_buf;
	bind_req.addr.maxlen = MAXLEN;
	bind_ret.addr.maxlen = MAXLEN;
	call.addr.maxlen     = MAXLEN;

	fd = SetupNetworkStream(
	    machine.nodename, 
	    atoi(display), 
	    BindAndListen, 
	    &type, 
	    servname 
	    );

	if( fd < 0)
	{
		PRMSG("SetupNetworkStream failed\n",0,0);
		return(-1);
	}

	TypeOfStream[fd] = type;

	/*
			 * set up call save list for general network listen service
			 */
	for (i = 0; i < LISTEN_QUE_SIZE; ++i)
	{
		if((tmp = (struct listenCall *) malloc(sizeof(struct listenCall))) == NULL)
		{
			PRMSG( "malloc failed\n",0,0);
			exit(1);
		}
		if((tmp->CurrentCall = (struct t_call *) t_alloc(fd,T_CALL,T_ALL)) == NULL)
		{
			PRMSG( "t_alloc failed\n",0,0);
			exit(1);
		}
		Que(&Network.FreeList[type], tmp, CLEAR);
	}

	if(Network._npeers > 0 && Network._peer == NULL)
	{
		register	i;
		register	char	*ptr;
		int		n;

		n =  (Network._npeers + 1) * 
		    (sizeof(int) + sizeof(char *) + (1 + UNAME_LENGTH));

		PRMSG("Allocating %d chars for %d peeers names", 
		    n, Network._npeers);

		if((ptr = malloc(n)) == NULL){
			fprintf(stderr,"Cannot malloc space for peers names\n");
			exit(1);
		}


		Network._peerlen = (int *) ptr;
		ptr += Network._npeers * sizeof(int);
		Network._peer = (char **) ptr;
		ptr += Network._npeers * sizeof(char *);
		for(i= 0; i< Network._npeers; i++)
		{
			Network._peerlen[i] = 0;
			Network._peer[i]    = ptr;
#ifdef DEBUG
			/*
												fprintf(stderr, "peer[%d] is %u; peerlen[%d] is %u\n",
													i, Network._peer[i], i, &Network._peerlen[i]);
									*/
#endif
			ptr += (1 + UNAME_LENGTH);
		}
	}
	PRMSG("SetupTliStream () (success) fd = %d\n", fd,0);
	return(fd);
}

static int
CallTliServer(host, idisplay, servname)
char *host, *servname;
int  idisplay;
{
	int	fd, type;
	int	nameserver ();

	PRMSG("Calling CallTliServer()\n",0,0);

	if(InitializeNetPath() < 0)
		return(-1);

	SetupNetworkInfo();
	NetworkInfo->_nnets = NetworkInfo->_npeers = 0;
	NetworkInfo->_peer = NULL;
	NetworkInfo->_peerlen = NULL;

	sprintf(_dispno, "%d", idisplay);
	dispno = _dispno;


	if((fd = SetupNetworkStream(
	    host, 
	    idisplay, 
	    BindAndConnect, 
	    &type, 
	    servname
	    )) < 0)
	{
		PRMSG("SetupNetworkStream failed\n",0,0);
		return(-1);
	}

	if(ioctl(fd, I_POP, "timod") < 0)
	{
		PRMSG("failed to pop timod\n",0,0);
	}
	if(ioctl(fd, I_PUSH, "tirdwr") < 0)
	{
		t_close(fd);
		return(-1);
	}

	PRMSG("A Connection has been established to %s ... \n", host,0);
	TypeOfStream[fd] = type;
	if (_XsInputBuffer[fd].DataBuffer == NULL)
		if ((_XsInputBuffer[fd].DataBuffer = (char *) malloc(BUFFERSIZE)) == NULL)
		{
			errno = ENOMEM;
			perror("Client can't connect to remote server");
			return (-1);
		}
	_XsInputBuffer[fd].LastBytePtr = 0;
	_XsInputBuffer[fd].FirstBytePtr = 0;
	PRMSG("CallTliServer() returns success\n",0,0);

	return(fd);
}


static int
_XMakeStreamsConnection (name, idisplay, retries,
familyp, serveraddrlenp, serveraddrp)
char	*name;
int	idisplay;
int	retries;
int	*familyp;
int	*serveraddrlenp;
char	**serveraddrp;
{
	char	netype[128], sysname[128], nodname[128];
	char	*procname = "Xlib/_XMakeStreamsConnection";
	struct	utsname	 machine;
	int	fd;

	PRMSG("GetConnectionType(%s)\n", name, 0);

	if(uname(&machine) < 0){
		t_error("Cannot get nodename");
		return(-1);
	}
	if(
		name == NULL ||
	    strcmp(name, "") == 0 ||
	    strcmp(name, "unix") == 0 ||
	    strcmp(name, "local") == 0 ||
	    strcmp(name, machine.nodename) == 0
	    )	{

		/*
							     * auth information for local connection set above
							     */

		fd = ((*xstream[X_NAMED_STREAM].CallTheListener)
		    ("unix", idisplay, "local"));

		if (fd >= 0) {
			*familyp = FamilyLocal;
			*serveraddrlenp = strlen (machine.nodename);
			*serveraddrp = (char *) Xmalloc ((*serveraddrlenp) + 1);
			if (!*serveraddrp) {
				*serveraddrlenp = 0;
			} else {
				strcpy (*serveraddrp, machine.nodename);
			}
		}
		return fd;
	}

	if((handlep = setnetpath()) == NULL)
	{
		nc_perror("Cannot set network selection path\n");
		return(-1);
	}


	/* For backward compatibility, we have to pass authorization
				   data in global variables.  Ugh. */
	tcpfamilyp = familyp;
	tcpremoteaddrp = serveraddrp;
	tcpremoteaddrlenp = serveraddrlenp;

	fd = (*xstream[X_TLI_STREAM].CallTheListener)(name, idisplay);
	return(fd);
}


static int
SetupNetworkStream(host, dispno, action, typtr, servname )
char *host, *servname;
int	dispno;
int	(*action)();
int	*typtr;
{
	int	i;
	char	service[MAX_AUTO_BUF_LEN];
	int	fd, type;
	struct nd_hostserv  nd_hostserv;
	struct netconfig   *netconfigp = NULL;
	struct nd_addrlist *nd_addrlistp = NULL;
	struct netbuf	   *netbufp = NULL;


#ifdef DEBUG
	fprintf(stderr, "Calling SetupNetworkStream(%s, %d)\n", host, dispno);
#endif

	sprintf(service , "%s", servname);
        if (strncmp(host, "tcp/", 4) == 0)
        	host += 4;

	nd_hostserv.h_host = host;
	nd_hostserv.h_serv = service;
#ifdef DEBUG
	fprintf(stderr, "Trying to get the binding address for service %s on %s\n", 
	    service, host);
#endif
	if( InitializeNetPath() > 0 )
	{
	while((netconfigp = getnetpath(handlep)) != NULL)
	{
#ifdef DEBUG
fprintf(stderr, "Trying to bind using %s\n", netconfigp->nc_device);
#endif
		if(netdir_getbyname(netconfigp, &nd_hostserv, &nd_addrlistp) == 0)
		{

#ifdef DEBUG
fprintf(stderr, "There are %d ways\n", nd_addrlistp->n_cnt);
#endif
			netbufp = nd_addrlistp->n_addrs;
			for(i=0; i< nd_addrlistp->n_cnt; i++)
			{

#ifdef DEBUG
fprintf(stderr, "Address: len %d maxlen %d \n", 
				    netbufp->len, netbufp->maxlen);
#endif
				if( strcmp(netconfigp->nc_netid, "starlan") == 0 )
				{
					register char *from, *to;
					int	i, len;

					from = to = netbufp->buf;
					len = 0;
					for(i=0; i< netbufp->len; i++, from++)
						if(*from != '.')
						{
							*to++ = *from;
							len++;
						}
					*to = '\0';
					netbufp->len = len;
				}

#ifdef DEBUG
netbufp->buf[netbufp->len] = '\0';
fprintf(stderr, "Address: len %d maxlen %d buf %s\n",
netbufp->len, netbufp->maxlen, netbufp->buf);
#endif
				if((fd = (*action)(netconfigp->nc_device, netbufp)) < 0)
				{
					netbufp++;
					continue;
				}
				if(
                   strcmp(netconfigp->nc_protofmly, "inet") == 0 &&
				    strcmp(netconfigp->nc_proto    , "tcp") == 0
				    )
				{
					memcpy(tcphalf, netbufp->buf, 4);
					if (tcpfamilyp != NULL) {
						*tcpfamilyp = FamilyInternet;
						*tcpremoteaddrlenp = 4;
						*tcpremoteaddrp = Xmalloc(*tcpremoteaddrlenp);
						/* This is a kludge.  What is the right way to get
																					       this info out? */
						memcpy(*tcpremoteaddrp, netbufp->buf+4,
						    *tcpremoteaddrlenp);
#ifdef DEBUG
fprintf(stderr, "tcp remote addr = %0x\n",
*(long *)*tcpremoteaddrp);
#endif
					}
				}
				type = 0;
				for(i=X_TLI_STREAM; i< Network._nnets; i++)
					if(strcmp(Network._net[i]->nc_netid, netconfigp->nc_netid) == 0)
					{
						type = i;
						break;
					}
				if(type == 0)
				{
					Network._net[Network._nnets] = netconfigp;
					type = Network._nnets++;
				}
				*typtr = type;
				/* free(netconfigp) the right way */
				(void) netdir_free((char *)nd_addrlistp, ND_ADDRLIST);

				return(fd);
			}
			/* free(nd_addrlistp) the right way */
			(void) netdir_free((char *)nd_addrlistp, ND_ADDRLIST);
		}
#ifdef DEBUG
else nc_perror("netdir_getbyname() failed\n");
/* MIT
else netdir_perror("netdir_getbyname() failed");	*/
#endif
	}
	} /* endif */
	return(-1);
}


static int
BindAndListen(clonedev, netbufp)
char	*clonedev;
struct   netbuf *netbufp;
{
	int	fd;
	struct  t_bind bindbuf;


	bindbuf.addr.buf = netbufp->buf;
	bindbuf.addr.len = netbufp->len;
	bindbuf.addr.maxlen = netbufp->maxlen;


	if ((fd = t_open(clonedev,  O_RDWR, NULL)) < 0)
	{
		fprintf(stderr, "Cannot open %s\n", clonedev);
		return(-1);
	}
#ifdef DEBUG
	fprintf(stderr, "opening device %s\n", clonedev);
#endif

	bindbuf.qlen = 8;
	if(t_bind (fd, &bindbuf, NULL) < 0)
	{
		t_error("t_bind failed");
		close(fd);
		return(-1);
	}
	return(fd);
}

static int
BindAndConnect(clonedev, netbufp)
char	*clonedev;
struct   netbuf *netbufp;
{
	int	fd;
	struct  t_call callbuf;

	callbuf.addr.buf = netbufp->buf;
	callbuf.addr.len = netbufp->len;
	callbuf.addr.maxlen = netbufp->maxlen;

	callbuf.opt.buf = NULL;
	callbuf.opt.len = 0;
	callbuf.opt.maxlen = 0;

	callbuf.udata.buf = NULL;
	callbuf.udata.len = 0;
	callbuf.udata.maxlen = 0;

	if ((fd = t_open(clonedev,  O_RDWR, NULL)) < 0)
	{
		fprintf(stderr, "Cannot open %s\n", clonedev);
		return(-1);
	}

#ifdef DEBUG
	fprintf(stderr, "Connecting to <%s> through device %s\n", 
	    callbuf.addr.buf, clonedev);
#endif
	if(t_bind(fd, NULL, NULL) < 0)
	{
		t_error("t_bind failed");
		t_close(fd);
		return(-1);
	}
	if(t_connect(fd, &callbuf, NULL) < 0)
	{
#ifdef DEBUG
		t_error("t_connect failed");
		checkNewEvent(fd);
#endif
		t_close(fd);
		return(-1);
	}
	return(fd);
}


/*
#ifndef MEMUTIL
extern	char	*calloc(), *realloc();
#endif
*/

extern  char    *program;
static int	network;
static int	nextentry;

static char *makePacket();
static char *staticalloc();
static char    *TheEnd;
static char    *inbuf;
static int     inlen;
static int     nhosts;
static int	nHosts;
static int     flags = 0;
static struct netconfig   *netconfigp = NULL;

static int
CallTheNameServer(service, nettype, arg1, arg2, arg3)
int service;
struct netconfig   *nettype;
char    **arg1, **arg2;
int     *arg3;
{
	int	n,m, len;
	char	*ptr, *net;
	int	*iptr;

	flags = service;
	netconfigp = nettype;
	ptr = *arg1;

	iptr = (int *) ptr;
	inlen = iptr[0];

#ifdef DEBUG
	fprintf(stderr,"inlen = %d\n", inlen);
#endif
	ptr += sizeof(int);
	nhosts = iptr[1];
#ifdef DEBUG
	fprintf(stderr,"nhosts = %d\n", nhosts);
#endif

	inbuf = ptr + sizeof(int);
	TheEnd = &inbuf[inlen];
#ifdef DEBUG
	write(2, inbuf, inlen);
#endif
	nextentry = ((xHostEntry *) inbuf)->length;
	*arg2 = (char *) makePacket(&len);
	if(arg3 != NULL)
		*arg3 = nHosts;

#ifdef DEBUG
	fprintf(stderr, "CallTheNameserver return %d\n", len);
#endif
	return(len);
}



static int	bufsize = 512;

static char	*getnextentry();

static struct nd_addrlist *
GetHostServiceByName(host, dispno)
char *host;
int	dispno;
{
	struct nd_hostserv  nd_hostserv;
	struct nd_addrlist *nd_addrlistp = NULL;
	struct netbuf	   *netbufp = NULL;
	char	service[MAX_AUTO_BUF_LEN];

	sprintf(service , "xserver%d", dispno);
	nd_hostserv.h_host = host;
	nd_hostserv.h_serv = service;

	if(netdir_getbyname(netconfigp, &nd_hostserv, &nd_addrlistp) == 0)
		return(nd_addrlistp);
	else	return(NULL);
}

static int
ConvertName(pktptr, n, entry, len)
char	**pktptr, *entry;
int	n, len;
{
	struct hostent *hp;
	unsigned long	address;
	int	port;
	char	*ptr;
	int	rndlen;
	struct nd_addrlist *nd_addrlistp = NULL;
	struct netbuf	*netbufp;
	char   *addr;
#ifdef DEBUG
	fprintf(stderr, "in ConvertName %s\n", entry);
#endif
	if((nd_addrlistp = GetHostServiceByName(entry, atoi(dispno))) == NULL)
		return(n);
	netbufp = nd_addrlistp->n_addrs;       /* the netbufs */
	/*
			 xhost needs only the last four bytes of the address
			 over TCP/IP.
			*/
	if(
		strcmp(netconfigp->nc_protofmly, "inet") == 0 &&
	    strcmp(netconfigp->nc_proto    , "tcp") == 0
	    ){
		addr = &netbufp->buf[4];
		len = 4;
		family = FamilyInternet;
	}
	else
	{
		family = FamilyDECnet;
		addr = netbufp->buf;
		len = netbufp->len;
	}
	rndlen = ((sizeof(xHostEntry) + len + 3) >> 2) << 2;

	if((*pktptr = staticalloc(*pktptr, n+rndlen)) == NULL)
	{
		netdir_free(nd_addrlistp, ND_ADDRLIST);
		return(-1);
	}

	ptr = &(*pktptr)[n];
	((xHostEntry *)ptr)->family = family;
	((xHostEntry *)ptr)->length = len;
	ptr += sizeof(xHostEntry);

	memcpy(ptr, addr, len);
	netdir_free(nd_addrlistp, ND_ADDRLIST);

#ifdef DEBUG
	ptr[len] = '\0';
	fprintf(stderr, "creating address for host %s address<%d>\n", entry, ptr);
#endif

	return(n+rndlen);
}

static struct nd_hostservlist *
GetHostServiceByAddr(addr, len)
char *addr;
int	len;
{
	struct nd_hostservlist *nd_hostservlist;
	struct netbuf	   netbuf;

	netbuf.buf = addr;
	netbuf.len = len;
	netbuf.maxlen = len;

	if(netdir_getbyaddr(netconfigp, &nd_hostservlist, &netbuf) == 0)
		return(nd_hostservlist);
	else	return(NULL);
}


static int
ConvertCallToName(pktptr, n, entry, len)
char	**pktptr, *entry;
int	n, len;
{
	int	l, rl;
	char	*ptr;
	struct nd_hostservlist *nd_hostservlist;

	if((nd_hostservlist = GetHostServiceByAddr(entry, len)) == NULL)
		return(n);

	l = strlen(nd_hostservlist->h_hostservs->h_host);

	rl = ((sizeof(xHostEntry) + l + 3) >> 2) << 2;

	if((*pktptr = staticalloc(*pktptr, n+rl)) == NULL)
	{
		netdir_free(nd_hostservlist, ND_HOSTSERVLIST);
		return(-1);
	}

	ptr = &(*pktptr)[n];
	((xHostEntry *)ptr)->family = 0;
	((xHostEntry *)ptr)->length = l;

	ptr += sizeof(xHostEntry);

	sprintf(ptr, nd_hostservlist->h_hostservs->h_host);
	netdir_free(nd_hostservlist, ND_HOSTSERVLIST);

#ifdef DEBUG
	fprintf(stderr, "getting the name for host %s\n", ptr);
#endif

	return(rl+n);
}

int
ConvertAddress(pktptr, n, entry, len)
char	**pktptr, *entry;
int	n, len;
{
	register i;
	char	*ptr;
	int	l, rl;
	struct nd_hostservlist *nd_hostservlist;
	char	*name;
	char	tcpaddr[8], *addr;
	char	addrbuf[MAX_AUTO_BUF_LEN];

#ifdef DEBUG
	entry[len] = '\0';
	fprintf(stderr, "Converting address %s in %s format\n",
	    entry, netconfigp->nc_netid);
#endif
	if(
		strcmp(netconfigp->nc_protofmly, "inet") == 0 &&
	    strcmp(netconfigp->nc_proto    , "tcp") == 0
	    ){
		addr = tcpaddr;
		memcpy(tcpaddr, tcphalf, 4);
		memcpy(&tcpaddr[4], entry, 4);
		len = 8;
#ifdef DEBUG
		fprintf(stderr, "port %d, family %d\n",
		    *(short *) &tcpaddr[2], *(short *) tcpaddr);
#endif
	}
	else
	{
		addr = entry;
	}

	if((nd_hostservlist = GetHostServiceByAddr(addr, len)) == NULL)
	{
		int	i;
		for(i=0; i< len; i++)
			if(entry[i] < ' ' || entry[i] > 0177)
				break;
		if(i < len)
		{
			sprintf(addrbuf, "%d", *(int *) entry);
			name = addrbuf;
			len = strlen(name);
		}
		else	name = entry;
		entry[len] = '\0';
		l = len + 1;
	}
	else
	{
		name = nd_hostservlist->h_hostservs->h_host;
		l = strlen(name) +1;
	}
	rl = ((sizeof(xHostEntry) + l + 3) >> 2) << 2;

	if((*pktptr = staticalloc(*pktptr, n+rl)) == NULL)
	{
		if(nd_hostservlist != NULL)
			netdir_free(nd_hostservlist, ND_HOSTSERVLIST);
		return(-1);
	}

	ptr = &(*pktptr)[n];
	((xHostEntry *)ptr)->family = 0;
	((xHostEntry *)ptr)->length = l;
	ptr += sizeof(xHostEntry);

	memcpy(ptr, name, l);
	if(nd_hostservlist != NULL)
		netdir_free(nd_hostservlist, ND_HOSTSERVLIST);

#ifdef DEBUG
	fprintf(stderr, "getting the name for host %s\n", name);
#endif

	return(n+rl);
}


static char *
getnextentry(plen)
int	*plen;
{
	char	*ptr;
	int	n = nextentry;

#ifdef DEBUG
	fprintf(stderr,"In getnextentry()\n");
#endif
	if(inbuf >= TheEnd)
	{
#ifdef DEBUG
		fprintf(stderr,"In getnextentry() end of buffer\n");
#endif
		*plen = -1;
		return(NULL);
	}

	*plen = nextentry;
	family = ((xHostEntry *) inbuf)->family;
	ptr = inbuf + sizeof(xHostEntry);
	inbuf += ((sizeof(xHostEntry) + *plen + 3) >> 2) << 2;
	nextentry = ((xHostEntry *) inbuf)->length;
	ptr[*plen] = '\0';
	return(ptr);
}

static char *
makePacket(plen)
int *plen;
{
	char *pktptr = NULL, *ptr;
	int	len;
	int	n = 0, m;


#ifdef DEBUG
	fprintf(stderr,"In makePacket()\n");
#endif

	for(nHosts = 0; nHosts < nhosts;)
	{
		ptr = getnextentry(&len);
		if(len < 0)
			break;
		if(len == 0 || ptr == NULL)
			continue;
		m = addentry(&pktptr, n, ptr, len);
		if(m > n){
			nHosts++;
			n = m;
		}
	}
#ifdef DEBUG
	fprintf(stderr, "packet size is %d\n", n);
#endif

	*plen = n;

	return(pktptr);
}

static int
addentry(pktptr, n, entry, len)
char **pktptr, *entry;
int	n, len;
{

#ifdef DEBUG
	fprintf(stderr, "in addEntry %s\n", entry);
#endif

	switch(flags)
	{
	case	ConvertNameToNetAddr:
		return(ConvertName(pktptr, n, entry, len));
	case	ConvertNetAddrToName:
		return(ConvertAddress(pktptr, n, entry, len));
	case	ConvertTliCallToName:
		return(ConvertCallToName(pktptr, n, entry, len));
	}
	return(-1);
}

static char *
staticalloc(ptr, size)
char *ptr;
int	size;
{

	if(ptr == NULL)
	{
		if(bufsize < size)
			bufsize = size;
		ptr = malloc(bufsize);
	}
	if(bufsize < size)
	{
		bufsize = size + 512;
		ptr = realloc(ptr, bufsize);
	}
	return(ptr);
}

static int
OpenVirtualCircuit(lfd)
int     lfd;
{
	char	*clonedev;
	int	fd;

	clonedev = Network._net[TypeOfStream[lfd]]->nc_device;

	if ((fd = t_open(clonedev,  O_RDWR, NULL)) < 0)
	{
		fprintf(stderr, "Cannot open %s\n", clonedev);
		return(-1);
	}

	if(t_bind(fd, NULL, NULL) < 0)
	{
		t_error("t_bind failed");
		t_close(fd);
		return(-1);
	}
	return(fd);
}

int	
SetupListeningPost(servname, servnum, remoteflag, route)
	char *servname, *route;
	int   remoteflag, servnum;
{
	char servbuf[10];

	(void) sprintf(servbuf, "%d", servnum);

	if(!remoteflag)
		return(SetupNamedStream(servbuf, servname));
	return(SetupTliStream(servbuf, servname));
}
int	
CallListeningPost(host, servname, servnum, remoteflag, route)
	char *host;
	char *servname, *route;
	int   remoteflag, servnum;
{
#ifdef DEBUG
printf( " host = %s, servername = %s, servnum = %d, flag = %d \n",
host, servname, servnum, remoteflag );
#endif
	if(host != NULL && *host == '\0'){
		strcpy( host, "unix" );
		}
	if(!remoteflag)
		return(CallLocalServer(host, servnum, servname));
	return(CallTliServer(host, servnum, servname));
}


int
AcceptNewConnections(lfd, MoreConnections)
int	lfd;
char	*MoreConnections;
{
	return((*xstream[TypeOfStream[lfd]].ConnectNewClient)
	    (lfd, MoreConnections));
}


int
CloseConnection(fd)
int	fd;
{
	return((*xstream[TypeOfStream[fd]].CloseStream)(fd));
}

int
ReceiveData(fd, buffer, count)
int	fd, count;
char	*buffer;
{
	return((*xstream[TypeOfStream[fd]].ReadFromStream)
	    (fd, buffer, count, NO_BUFFERING));
}

int
TransmitData(fd, buffer, count)
int	fd, count;
char	*buffer;
{
	return((*xstream[TypeOfStream[fd]].WriteToStream)
	    (fd, buffer, count, NO_BUFFERING));
}

