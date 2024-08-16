/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/xti.h	1.18"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef _NET_XTI_H	/* wrapper symbol for kernel use */
#define _NET_XTI_H	/* subject to change without notice */
#define _SYS_XTI_H	/* SVR4.0COMPAT */

/*
 * The following are the error codes needed by both the kernel
 * level transport providers and the user level library.
 */

#define	TBADADDR		1	/* incorrect addr format         */
#define	TBADOPT			2	/* incorrect option format       */
#define	TACCES			3	/* incorrect permissions         */
#define TBADF			4	/* illegal transport fd	         */
#define TNOADDR			5	/* couldn't allocate addr        */
#define TOUTSTATE	        6	/* out of state                  */
#define TBADSEQ		        7       /* bad call sequence number      */
#define TSYSERR			8	/* system error              */
#define TLOOK		        9	/* event requires attention  */
#define TBADDATA	       10	/* illegal amount of data    */
#define TBUFOVFLW	       11       /* buffer not large enough   */
#define TFLOW		       12 	/* flow control		     */
#define	TNODATA		       13	/* no data		     */
#define TNODIS		       14	/* discon_ind not found on q */
#define TNOUDERR	       15	/* unitdata error not found  */
#define TBADFLAG	       16       /* bad flags                 */
#define TNOREL		       17       /* no ord rel found on q     */
#define TNOTSUPPORT	       18       /* primitive not supported   */
#define TSTATECHNG	       19	/* state is in process of changing */
#define	TNOSTRUCTYPE	       20	/* invalid struct_type requested   */
#define	TBADNAME	       21	/* invalid transport provider name */
#define	TBADQLEN	       22	/* qlen is zero                    */
#define	TADDRBUSY	       23	/* address in use                  */
#define	TINDOUT		       24	/* outstanding connect indications */
#define	TPROVMISMATCH	       25	/* transport provider mismatch     */
#define	TRESQLEN	       26	/* resfd specified to accept w/qlen>0 */
#define	TRESADDR	       27	/* resfd not bound to same address as fd */
#define	TQFULL		       28	/* incoming connection queue full  */
#define	TPROTO		       29	/* XTI protocol error              */
/* IF NEW ERRORS ARE ADDED TO THE LIST ABOVE, please update the value
 * of t_nerr in libnsl:common/lib/libnsl/nsl/t_strerror.c
 */

/* 
 * The following are the events returned by t_look
 */
#define T_LISTEN	0x0001 	/* connection indication received */
#define T_CONNECT	0x0002	/* connect confirmation received  */
#define T_DATA		0x0004	/* normal data received           */
#define	T_EXDATA	0x0008	/* expedited data received        */
#define T_DISCONNECT	0x0010	/* disconnect received            */
#define T_ERROR		0x0020	/* fatal error occurred		  */
#define T_UDERR	 	0x0040	/* data gram error indication     */
#define T_ORDREL	0x0080	/* orderly release indication     */
#define	T_GODATA	0x0100	/* sending normal data is again possible    */
#define	T_GOEXDATA	0x0200	/* sending expedited data is again possible */
#define T_EVENTS	0x03ff	/* event mask	                  */

/*
 * The following are the flag definitions needed by the
 * user level library routines.
 */

#define T_MORE		0x001		/* more data        */
#define T_EXPEDITED	0x002		/* expedited data   */
#define T_NEGOTIATE	0x004		/* set opts         */
#define T_CHECK		0x008		/* check opts       */
#define T_DEFAULT	0x010		/* get default opts */
#define T_SUCCESS	0x020		/* successful       */
#define T_FAILURE	0x040		/* failure          */
#define	T_CURRENT	0x080		/* get current opts */
#define T_PARTSUCCESS	0x100		/* partial success */
#define T_READONLY	0x200		/* read-only */
#define T_NOTSUPPORT	0x400		/* not supported */

/*
 * protocol specific service limits
 */

struct t_info {
	long addr;	/* size of protocol address                */
	long options;	/* size of protocol options                */
	long tsdu;	/* size of max transport service data unit */
	long etsdu;	/* size of max expedited tsdu              */
	long connect;	/* max data for connection primitives      */
	long discon;	/* max data for disconnect primitives      */
	long servtype;	/* provider service type		   */
	long flags;	/* information about the provider          */
};

/* 
 * Service type defines
 */
#define T_COTS	   01	/* connection oriented transport service  */
#define T_COTS_ORD 02	/* connection oriented w/ orderly release */
#define T_CLTS	   03	/* connectionless transport service       */

/*
 * Values for flags field in t_info structure and ti_provider.flgs field of 
 * tiuser structure. Some of these values are assigned by X/Open while
 *  the others by USL. If USL are defining flags not assigned by X/Open,
 *  the assignment of values should be from the bottom upwards.
 */

#define	T_SENDZERO	0x0000001	/* provider supports 0-length TSDUs */
#define T_EXPINLINE     0x8000000	/* provider wants in-line exp. data */

/*
 * XPG4 (XTI version 2) symbols
 * (these are the symbols that libnsl.so supplies)
 */

#define t_accept _xti_accept
#define t_alloc _xti_alloc
#define t_bind _xti_bind
#define t_close _xti_close
#define t_connect _xti_connect
#define t_error _xti_error
#define t_free _xti_free
#define t_getprotaddr _xti_getprotaddr
#define t_getinfo _xti_getinfo
#define t_getstate _xti_getstate
#define t_listen _xti_listen
#define t_look _xti_look
#define t_open _xti_open
/*
 * t_optmgmt is still done TLI-style
 *
#define t_optmgmt _xti_optmgmt
 */
#define t_rcv _xti_rcv
#define t_rcvconnect _xti_rcvconnect
#define t_rcvdis _xti_rcvdis
#define t_rcvrel _xti_rcvrel
#define t_rcvudata _xti_rcvudata
#define t_rcvuderr _xti_rcvuderr
#define t_snd _xti_snd
#define t_snddis _xti_snddis
#define t_sndrel _xti_sndrel
#define t_sndudata _xti_sndudata
#define t_strerror _xti_strerror
#define t_sync _xti_sync
#define t_unbind _xti_unbind

/*
 * netbuf structure
 */

struct netbuf {
	unsigned int maxlen;
	unsigned int len;
	char *buf;
};

/*
 * t_bind - format of the addres and options arguments of bind 
 */

struct t_bind {
	struct netbuf	addr;
	unsigned	qlen;
};

/* 
 * options management
 */
struct t_optmgmt {
	struct netbuf	opt;
	long		flags;
};

/*
 * disconnect structure
 */
struct t_discon {
	struct netbuf udata;		/* user data          */
	int reason;			/* reason code        */
	int sequence;			/* sequence number    */
};

/*
 * call structure
 */
struct t_call {
	struct netbuf addr;		/*  address           */
	struct netbuf opt;		/* options	      */
	struct netbuf udata;		/* user data          */
	int sequence;			/* sequence number    */
};

/*
 * data gram structure
 */
struct t_unitdata {
	struct netbuf addr;		/*  address           */
	struct netbuf opt;		/* options	      */
	struct netbuf udata;		/* user data          */
};

/*
 * unitdata error
 */
struct t_uderr {
	struct netbuf addr;		/* address		*/
	struct netbuf opt;		/* options 		*/
	long	      error;		/* error code		*/
};

/*
 * The following are structure types used when dynamically
 * allocating the above structures via t_alloc().
 */
#define T_BIND		1		/* struct t_bind	*/
#define T_OPTMGMT	2		/* struct t_optmgmt	*/
#define T_CALL		3		/* struct t_call	*/
#define T_DIS		4		/* struct t_discon	*/
#define T_UNITDATA	5		/* struct t_unitdata	*/
#define T_UDERROR	6		/* struct t_uderr	*/
#define T_INFO		7		/* struct t_info	*/

/*
 * The following bits specify which fields of the above
 * structures should be allocated by t_alloc().
 */
#define T_ADDR	0x01			/* address   */
#define T_OPT	0x02			/* options   */
#define T_UDATA	0x04			/* user data */
#define T_ALL	0xffff			/* all the above */


/* 
 * the following are the states for the user
 */

#define T_UNINIT	0		/* uninitialized  		*/
#define T_UNBND		1		/* unbound 	      		*/
#define T_IDLE		2		/* idle				*/
#define	T_OUTCON	3		/* outgoing connection pending 	*/
#define T_INCON		4		/* incoming connection pending  */
#define T_DATAXFER	5		/* data transfer		*/
#define T_OUTREL        6               /* outgoing release pending     */
#define T_INREL		7		/* incoming release pending     */
#define T_FAKE		8		/* fake state used when state   */
					/* cannot be determined		*/
#define T_HACK		12		/* needed to maintain compatibility !!!
					 * (used by switch statement in 
					 * t_getstate.c)
					 * DO NOT REMOVE UNTIL _spec FILE
					 * REORDERED!!!!
					 */

#define T_NOSTATES 	9


#define ROUNDUP(X)	((X + 0x03)&~0x03)

/*
 * The following are TLI user level events which cause
 * state changes.
 */

#define T_OPEN 		0
#define T_BIND		1
#define T_OPTMGMT	2
#define T_UNBIND	3
#define T_CLOSE		4
#define T_SNDUDATA	5
#define T_RCVUDATA	6
#define T_RCVUDERR	7
#define T_CONNECT1	8
#define T_CONNECT2	9
#define T_RCVCONNECT	10
#define T_LISTN		11
#define T_ACCEPT1	12
#define T_ACCEPT2	13
#define	T_ACCEPT3	14
#define T_SND		15
#define T_RCV		16
#define T_SNDDIS1	17
#define T_SNDDIS2	18
#define T_RCVDIS1	19
#define T_RCVDIS2	20
#define T_RCVDIS3	21
#define T_SNDREL	22
#define T_RCVREL	23
#define T_PASSCON	24

#define T_NOEVENTS	25

#define nvs 	127 	/* not a valid state change */

extern const char tiusr_statetbl[T_NOEVENTS][T_NOSTATES];

/* macro to change state */
/* TLI_NEXTSTATE(event, current state) */
#define TLI_NEXTSTATE(X,Y)	tiusr_statetbl[X][Y]

/*
 * Flags for t_getname.
 */
#define LOCALNAME	0
#define REMOTENAME	1

/*
 * Band definitions for data flow.
 */
#define TI_NORMAL	0
#define TI_EXPEDITED	1


/*
 * t_opthdr structure.
 */

struct t_opthdr {
	unsigned long	len;	/* total lenght of option;i.e.
				   sizeof(struct t_opthdr) + length of
				   option value in bytes */
	unsigned long	level;	/* protocol affected */
	unsigned long	name;	/* option name */
	unsigned long	status;	/* status value */
	/* followed by the option value */
};

/*
 * General purpose defines.
 */

#define T_YES 		1
#define T_NO		0
#define T_UNUSED	(-1)
#define T_NULL		0
#define T_ABSREQ	0x8000
#define T_INFINITE	(-1)
#define T_INVALID	(-2)

/* T_INFINITE and T_INVALID are values of t_info */


/*
 * General definitions for option management.
 */
#define T_UNSPEC	(~0 - 2)
#define T_ALLOPT	0
#define T_ALIGN(p)	(((unsigned long)(p) + (sizeof(long) - 1)) &~ (sizeof(long) - 1))


#define OPT_NEXTHDR(pbuf,buflen,popt) (((char *)(popt) + T_ALIGN((popt)->len) < (char *)pbuf + buflen) ? (struct t_opthdr *)((char*)(popt) + T_ALIGN((popt)->len)) : (struct t_opthdr *)NULL)

	/* OPTIONS ON XTI LEVEL */

/* XTI-level */

#define XTI_GENERIC	0xffff
#define XTI_DEBUG	0x0001	/* enable debugging */
#define XTI_LINGER	0x0080	/* linger on close if data present */
#define XTI_RCVBUF	0x1002	/* receive buffer size */
#define XTI_RCVLOWAT	0x1004	/* receive low-water mark */
#define XTI_SNDBUF	0x1001	/* snd buffer size */
#define XTI_SNDLOWAT	0x1003	/* send low-water mark */

/*
 * Structure used with linger option.
 */

struct t_linger {
	long	l_onoff;	/* option on/off */
	long	l_linger;	/* linger time */
};

/*
 * Definition of the ISO transport classes.
 */

#define T_CLASS0	0
#define T_CLASS1	1
#define T_CLASS2	2
#define T_CLASS3	3
#define T_CLASS4	4

/*
 * Definition of the priorities.
 */

#define T_PRITOP	0
#define T_PRIHIGH	1
#define T_PRIMID	2
#define T_PRILOW	3
#define T_PRIDFLT	4

/*
 * Definitions of the protection levels.
 */

#define	T_NOPROTECT		1
#define T_PASSIVEPROTECT	2
#define T_ACTIVEPROTECT		4

/*
 * Default value for the length of TPDUs.
 */

#define T_LTPDUDFLT	128

/*
 * rate structure.
 */

struct rate {
	long	targetvalue;	/* target value */
	long	minacceptvalue;	/* value of minimum acceptable quality */
};

/*
 * reqvalue structure.
 */

struct reqvalue {
	struct rate called;	/* called rate */
	struct rate calling;	/* calling rate */
};

/*
 * thrpt structure.
 */

struct thrpt {
	struct reqvalue maxthrpt;	/* maximum throughput */
	struct reqvalue avgthrpt;	/* average throughput */
};

/*
 * transdel structure.
 */

struct transdel {
	struct reqvalue	maxdel;		/* maximum transit delay */
	struct reqvalue	avgdel;		/* average transit delay */
};

/*
 * Protocol Levels
 */

#define ISO_TP		0x0100

/*
 * Options for Quality of Service and Expedited Data (ISO 8072:1986).
 */

#define TCO_THROUGHPUT		0x0001
#define TCO_TRANSDEL		0x0002
#define TCO_RESERRORRATE	0x0003
#define TCO_TRANSFFAILPROB	0x0004
#define TCO_ESTFAILPROB		0x0005
#define TCO_RELFAILPROB		0x0006
#define TCO_ESTDELAY		0x0007
#define TCO_RELDELAY		0x0008
#define TCO_CONNRESIL		0x0009
#define TCO_PROTECTION		0x000a
#define TCO_PRIORITY		0x000b
#define TCO_EXPD		0x000c

#define TCL_TRANSDEL		0x000d
#define TCL_RESERRORRATE	TCO_RESERRORRATE
#define TCL_PROTECTION		TCO_PROTECTION
#define TCL_PRIORITY		TCO_PRIORITY

/*
 * Management Options.
 */

#define TCO_LTPDU		0x0100
#define TCO_ACKTIME		0x0200
#define TCO_REASTIME		0x0300
#define TCO_EXTFORM		0x0400
#define TCO_FLOWCTRL		0x0500
#define TCO_CHECKSUM		0x0600
#define TCO_NETEXP		0x0700
#define TCO_NETRECPTCF		0x0800
#define TCO_PREFCLASS		0x0900
#define TCO_ALTCLASS1		0x0a00
#define TCO_ALTCLASS2		0x0b00
#define TCO_ALTCLASS3		0x0c00
#define TCO_ALTCLASS4		0x0d00

#define TCL_CHECKSUM		TCO_CHECKSUM

	/* INTERNET SPECIFIC ENVIRONMENT */

/*
 * TCP level.
 */

#define INET_TCP	0x6

/*
 * TCP-level Options.
 */

#ifndef TCP_NODELAY		/* if not defined by tcp.h, define. */
#define TCP_NODELAY	0x1	/* don't delay packets to coalesce */
#endif /* TCP_NODELAY */

#ifndef TCP_MAXSEG		/* if not defined by tcp.h, define. */
#define TCP_MAXSEG	0x2	/* get maximum segment size */
#endif /* TCP_MAXSEG */

#define TCP_KEEPALIVE	0x8	/* check, if connections are alive */

/*
 * Structure used with TCP_KEEPALIVE option.
 */

struct t_kpalive {
	long	kp_onoff;	/* option on/off */
	long	kp_timeout;	/* timeout in minutes */
};

#define T_GARBAGE	0x02

/*
 * UDP level.
 */

#define INET_UDP	0x11

/*
 * UDP-level Options.
 */

#define UDP_CHECKSUM	TCO_CHECKSUM	/* checksum computation */

/*
 * IP level.
 */

#define INET_IP		0x0

/*
 * IP-level Options.
 */

#ifndef IP_OPTIONS		/* if not defined by in.h, define. */
#define IP_OPTIONS	0x1	/* IP per-packet options */
#endif /* IP_OPTION */

#ifndef IP_TOS			/* if not defined by in.h, define. */
#define IP_TOS		0x2	/* IP per-packet type of service */
#endif /* IP_TOS */

#define IP_TTL		0x3	/* IP per-packet time to live */
#define IP_REUSEADDR	0x4	/* allow local address reuse */
#define IP_DONTROUTE	0x10	/* just use interface addresses */
#define IP_BROADCAST	0x20	/* permit sending of broadcast msgs */

/*
 * IP_TOS precedence levels.
 */

#define T_ROUTINE	0
#define T_PRIORITY	1
#define T_IMMEDIATE	2
#define T_FLASH		3
#define T_OVERRIDEFLASH	4
#define T_CRITIC_ECP	5
#define T_INETCONTROL	6
#define T_NETCONTROL	7

/*
 * IP_TOS type of service.
 */

#define T_NOTOS		0
#define T_LDELAY	1 << 4
#define T_HITHRPT	1 << 3
#define T_HIREL		1 << 2

#define SET_TOS(prec,tos)	((0x7 & (prec)) << 5 | (0x1c & (tos)))
#ifdef __STDC__

extern int		set_t_errno(int);
extern int		get_t_errno(void);
extern int		*_t_errno(void);

#else /* !__STDC__ */

extern int		set_t_errno();
extern int		get_t_errno();
extern int		*_t_errno();

#endif /* __STDC__ */

#ifdef _REENTRANT

#define t_errno 	(*(_t_errno()))

#else /* !_REENTRANT */

extern int		t_errno;

#endif /* _REENTRANT */

/*
 * XTI Library Function Prototypes
 */
#ifdef __STDC__
extern int t_accept(int, int, struct t_call *);
extern char *t_alloc(int, int, int);
extern int t_bind(int, struct t_bind *, struct t_bind *);
extern int t_close(int);
extern int t_connect(int, struct t_call *, struct t_call *);
extern int t_free(char *, int);
extern int t_getinfo(int, struct t_info *);
extern int t_getprotaddr(int, struct t_bind *, struct t_bind *);
extern int t_getstate(int);
extern int t_listen(int, struct t_call *);
extern int t_look(int);
extern int t_open(char *, int, struct t_info *);
extern int t_optmgmt(int, struct t_optmgmt *, struct t_optmgmt *);
extern int t_rcv(int, char *, unsigned int, int *);
extern int t_rcvconnect(int, struct t_call *);
extern int t_rcvdis(int, struct t_discon *);
extern int t_rcvrel(int);
extern int t_rcvudata(int, struct t_unitdata*, int *);
extern int t_rcvuderr(int, struct t_uderr *);
extern int t_snd(int, char *, unsigned int, int);
extern int t_snddis(int, struct t_call *);
extern int t_sndrel(int);
extern int t_sndudata(int, struct t_unitdata *);
extern char *t_strerror(int);
extern int t_sync(int);
extern int t_unbind(int);

#ifndef _T_ERROR	/* t_error() conflicts with tiuser.h & SVID */
#define _T_ERROR
extern int t_error(char *);
#endif /* _T_ERROR */

#else /* !__STDC__ */
extern int t_accept();
extern char *t_alloc();
extern int t_bind();
extern int t_close();
extern int t_connect();
extern int t_free();
extern int t_getinfo();
extern int t_getprotaddr();
extern int t_getstate();
extern int t_listen();
extern int t_look();
extern int t_open();
extern int t_optmgmt();
extern int t_rcv();
extern int t_rcvconnect();
extern int t_rcvdis();
extern int t_rcvrel();
extern int t_rcvudata();
extern int t_rcvuderr();
extern int t_snd();
extern int t_snddis();
extern int t_sndrel();
extern int t_sndudata();
extern char *t_strerror();
extern int t_sync();
extern int t_unbind();

#ifndef _T_ERROR	/* t_error() conflicts with tiuser.h & SVID */
#define _T_ERROR
extern int t_error();
#endif /* _T_ERROR */

#endif /* __STDC__ */

#endif /* _NET_XTI_H */

#if defined(__cplusplus)
	}
#endif
