/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_CPQ_CET_TOKEN_DLPI_TOKEN_H /* wrapper symbol for kernel use */
#define _IO_DLPI_CPQ_CET_TOKEN_DLPI_TOKEN_H /* subject to change without notice */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF			*/
/*	Novell/UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any		*/
/*	actual or intended publication of such source code.		*/

/*	Copyright (c) 1993, 1994 Novell, Inc.				*/
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  		All Rights Reserved			  	*/


#ident	"@(#)kern-i386at:io/dlpi_cpq/cet/token/dlpi_token.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" ,{
#endif
#ifdef	_KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifndef _IO_STREAM_H
#include <io/stream.h>	/* REQUIRED */
#endif

#ifndef _FS_IOCCOM_H
#include <fs/ioccom.h>
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/stream.h>	/* REQUIRED */
#include <sys/ioccom.h>

#else

#include <sys/types.h>
#include <sys/ioccom.h>
#include <sys/stream.h>

#endif	/* _KERNEL_HEADERS */

#ifndef	ESMP
#define	ESMP
#endif	/* ESMP */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/*
 *  DLIP ethernet IOCTL defines.
 */
#define	DLIOCSMIB	_IOW('D', 0, int)	/* Set MIB		     */
#define	DLIOCGMIB	_IOR('D', 1, int)	/* Get MIB		     */
#define	DLIOCSENADDR	_IOW('D', 2, int)	/* Set ethernet address	     */
#define	DLIOCGENADDR	_IOR('D', 3, int)	/* Get ethernet address	     */
#define	DLIOCSLPCFLG	_IOW('D', 4, int)	/* Set local packet copy flag*/
#define	DLIOCGLPCFLG	_IOR('D', 5, int)	/* Get local packet copy flag*/
#define	DLIOCSPROMISC	_IOW('D', 6, int)	/* Toggle promiscuous state  */
#define	DLIOCGPROMISC	_IOR('D', 7, int)	/* Get promiscuous state     */
#define	DLIOCADDMULTI	_IOW('D', 8, int)	/* Add multicast address     */
#define	DLIOCDELMULTI	_IOW('D', 9, int)	/* Delete multicast address  */
#define	DLIOCDISABLE	_IOW('D',10, int)	/* Disable controller        */
#define	DLIOCENABLE	_IOW('D',11, int)	/* Enable controller         */
#define	DLIOCRESET	_IOW('D',12, int)	/* Reset controller          */
#define DLIOCCSMACDMODE _IOW('D',13, int)	/* Toggle CSMA-CD mode       */
#define DLIOCGETSAP	_IOW('D',14, int)	/* List of sap ...temp*/
#define DLIOCGETMULTI	_IOW('D',15, int)	/* Get multicast address list */
#define DLIOCRAWMODE	_IOW('D',16, int)	/* Toggle RAWMODE mode       */
#define DLIOCLLC2MODE	_IOW('D',17, int)	/* Toggle LLC2MODE mode      */

/*
 * The following defines are included for binary compatibility with uw1.1
 * dlpi token-ring.
 */
#define DLGBROAD	(('D' << 8) | 3)  /* get broadcast address entry */
#define DLGADDR		(('D' << 8) | 5)  /* get physical addr of interface */
#define DLSADDR		(('D' << 8) | 7)  /* set physical addr of interface */
#define DLGSTAT		(('D' << 8) | 16) /* get statistics for a sap*/
#define DLSLLC2		(('D' << 8) | 17) /* register/deregister LLC2 module */
#define DLSRAW		(('D' << 8) | 18) /* set raw mode */
#define MACIOC(x)	(('M' << 8) | (x))
#define MACIOC_GETADDR	MACIOC(8)       /* get mac address */

/*
 *  Some other defines
 */
#define DL_MAX_PLUS_HDR		1514	/* Absolute MAX Packet (cf. DIX v2.0) */
#define	DL_MAC_ADDR_LEN		6
#define	DL_SAP_LEN		2
#define DL_TOTAL_ADDR_LEN	(DL_MAC_ADDR_LEN + DL_SAP_LEN)
#define	DL_ID			(ENETM_ID)
#define	DL_PRIMITIVES_SIZE	(sizeof(union DL_primitives))

#define	IS_MULTICAST(eaddr)	(eaddr.bytes[ 0 ] & 1)

#if defined(DL_STRLOG) && !defined(lint)
#define	DL_LOG(x)	 DLstrlog ? x : 0
#else
#define DL_LOG(x)
#endif

typedef	union DL_primitives	DL_primitives_t;

/*
 *  Special SAP ID's
 */
#define	PROMISCUOUS_SAP	((ushort_t)0xffff)	/* Matches all SAP ID's	     */

/*
 *  Standard DLPI ethernet address type
 */
typedef union {
	uchar_t		bytes[ DL_MAC_ADDR_LEN ];
	ushort_t	words[ DL_MAC_ADDR_LEN / 2 ];
} DL_eaddr_t;

/*
 *  TokenRing Statistics
 */
typedef struct
   {
   ulong LineErrors;			/* From Read Error Log output. */
   ulong BurstErrors;			/* From Read Error Log output. */
   ulong ACErrors;			/* From Read Error Log output. */
   ulong AbortTransErrors;		/* Not reported. */
   ulong InternalErrors;		/* From Adapter Check output. */
   ulong LostFrameErrors;		/* From Read Error Log output. */
   ulong ReceiveCongestions;		/* From Read Error Log output. */
   ulong FrameCopiedErrors;		/* From Read Error Log output. */
   ulong TokenErrors;			/* From Read Error Log output. */
   ulong SoftErrors;			/* From Ring Status output. */
   ulong HardErrors;			/* From Ring Status output. */
   ulong SignalLoss;			/* From Ring Status output. */
   ulong TransmitBeacons;		/* From Ring Status output. */
   ulong Recoverys;			/* From Ring Status output. */
   ulong LobeWires;			/* From Ring Status output. */
   ulong Removes;			/* From Ring Status output. */
   ulong Singles;			/* From Ring Status output. */
   ulong FreqErrors;			/* Not reported. */
   } TKR_MIB;		/* Token ring (dot5) statistics. */

/*
 *  Interface statistics compatible with MIB II SNMP requirements.
 */
typedef	struct {
	int		ifIndex;	/* ranges between 1 and ifNumber     */
	int		ifDescrLen;	/* len of desc. following this struct*/
	int		ifType;		/* type of interface                 */
	int		ifMtu;		/* datagram size that can be sent/rcv*/
	ulong_t		ifSpeed;	/* estimate of bandwith in bits PS   */
	uchar_t		ifPhyAddress[ DL_MAC_ADDR_LEN ];  /* Ethernet Address*/
	int		ifAdminStatus;	/* desired state of the interface    */
	int		ifOperStatus;	/* current state of the interface    */
	ulong_t		ifLastChange;	/* sysUpTime when state was entered  */
	ulong_t		ifInOctets;	/* octets received on interface      */
	ulong_t		ifInUcastPkts;	/* unicast packets delivered         */
	ulong_t		ifInNUcastPkts;	/* non-unicast packets delivered     */
	ulong_t		ifInDiscards;	/* good packets received but dropped */
	ulong_t		ifInErrors;	/* packets received with errors      */
	ulong_t		ifInUnknownProtos; /* packets recv'd to unbound proto*/
	ulong_t		ifOutOctets;	/* octets transmitted on interface   */
	ulong_t		ifOutUcastPkts;	/* unicast packets transmited        */
	ulong_t		ifOutNUcastPkts;/* non-unicast packets transmited    */
	ulong_t		ifOutDiscards;	/* good outbound packets dropped     */
	ulong_t		ifOutErrors;	/* number of transmit errors         */
	ulong_t		ifOutQlen;	/* length of output queue            */
	TKR_MIB		ifSpecific;	/* token ring specific stats         */
} DL_mib_t;

/*
 *  ifAdminStatus and ifOperStatus values
 */
#define			DL_UP	1	/* ready to pass packets             */
#define			DL_DOWN	2	/* not ready to pass packets         */
#define			DL_TEST	3	/* in some test mode                 */

/*
 *  Board related info.
 */

#ifdef _KERNEL
typedef struct bdconfig{
	major_t		major;		/* major number for device	     */
	ulong_t		io_start;	/* start of I/O base address	     */
	ulong_t		io_end;		/* end of I/O base address	     */
	paddr_t		mem_start;	/* start of base mem address	     */
	paddr_t		mem_end;	/* start of base mem address	     */
	int		irq_level;	/* interrupt request level	     */
	int		max_saps;	/* max service access points (minors)*/
	int		flags;		/* board management flags	     */
#define				BOARD_PRESENT	0x01
#define				BOARD_DISABLED	0x02
#define				TX_BUSY		0x04
#define				TX_QUEUED	0x08
	int		bd_number;	/* board number in multi-board setup */
	int		tx_next;	/* round robin service of SAP queues */
	int		timer_id;	/* watchdog timer ID		     */
	int 		timer_val;      /* watchdog timer value              */
	int		promisc_cnt;	/* count of promiscuous bindings     */
	int		multicast_cnt;	/* count of multicast address sets   */
	struct sap	*sap_ptr;	/* ptr to SAP array for this board   */
	struct ifstats	*ifstats;	/* ptr to IP stats structure (TCP/IP)*/
	DL_eaddr_t	eaddr;		/* Ethernet address storage	     */
	caddr_t		bd_dependent1;	/* board dependent value	     */
	caddr_t		bd_dependent2;	/* board dependent value	     */
	caddr_t		bd_dependent3;	/* board dependent value	     */
	int		ttl_valid_sap;
	struct sap	*valid_sap;
	DL_mib_t	mib;		/* SNMP interface statistics	     */
#ifdef	ESMP
	lock_t		*bd_lock;
	void		*bd_intr_cookiep;/* used by system's interrupt attach
					 * and detach functions.
					 */
#endif
} DL_bdconfig_t;

/*
 *  SAP related info.
 */
typedef struct sap {
	int		state;		/* DLPI state			     */
	ushort_t	sap_addr;	/* bound SAP address		     */
	ushort_t	snap_local;	/* lower order 16 bits of the PIF    */
	ulong_t		snap_global;	/* Higher order 24 bits of the PIF   */
	queue_t		*read_q;	/* the read queue pointer	     */
	queue_t		*write_q;	/* the write queue pointer	     */
	int		flags;		/* SAP management flags		     */

#define				PROMISCUOUS		0x01
#define				SEND_LOCAL_TO_NET	0x02
#define				PRIVILEDGED		0x04
#define				RAWCSMACD		0x08
#define				SNAPCSMACD		0x10
#define				RAWMODE			0x20
#define				LLC2MODE		0x40

	int		max_spdu;	/* largest amount of user data	     */
	int		min_spdu;	/* smallest amount of user data	     */
	int		mac_type;	/* DLPI mac type		     */
	int		service_mode;	/* DLPI servive mode		     */
	int		provider_style;	/* DLPI provider style		     */
	DL_bdconfig_t	*bd;		/* pointer to controlling bdconfig   */
	struct sap 	*next_sap;	/* pointer to the next valid/idle sap*/
#ifdef	ESMP
	sv_t		*sap_sv;	/* general purpose sync variable     */
#endif
} DL_sap_t;

/*
 * This typedef and its set of defined values are used when passing M_CTL
 * messages between netflex token-ring driver and netflex modules
 * (i.e. source * routing).
 */
typedef unsigned long cet_mctl_t;
/*
 * CETMCTL_ENABLE_BYPASS_SR: indicates that Source Routing functionality is
 *	to be bypassed.
 * CETMCTL_DISABLE_BYPASS_SR: indicates that Source Routing functionality is
 *	NOT to be bypassed.
 */
#define	CETMCTL_ENABLE_BYPASS_SR		(0x0000) 
#define	CETMCTL_DISABLE_BYPASS_SR		(0x0001) 

#endif /* _KERNEL or _KERNEL_HEADERS */

#if defined(ESMP) || defined(CETDEBUG) || defined(DEBUG)
#ifdef	ESMP
#if defined(CETDEBUG) || defined(DEBUG)
#define	DLPI_LOCK(X, Y)		(LOCK((X), (Y))) ; (DL_lockinfo(1))
#define	DLPI_UNLOCK(X, Y)	(DL_lockinfo(0)); (UNLOCK((X), (Y)))
#else
#define	DLPI_LOCK(X, Y)		(LOCK((X), (Y)))
#define	DLPI_UNLOCK(X, Y)	(UNLOCK((X), (Y)))
#endif /*CETDEBUG || DEBUG*/
#else
#define	DLPI_LOCK(X, Y)		(splstr())
#define	DLPI_UNLOCK(X, Y)	(splx((Y)))
#endif /*ESMP*/
#else
#define	DLPI_LOCK(X, Y)		(splstr())
#define	DLPI_UNLOCK(X, Y)	(splx((Y)))
#endif

/* LLC specific definitions and declarations */

#define LLC_LSAP_HDR_SIZE	3
#define SNAP_LSAP_HDR_SIZE	5 	/* SNAP HDR excluding LLC header */
#define SNAPSAP			0xaa	/* Value of SNAP sap */
#define MAC_ADDR_LEN		6	/* length of 802(.3/.4/.5) address */

/* define llc class 1 and mac structures and macros */

struct tok_machdr {
	uchar_t mac_dst[6];
	uchar_t mac_src[6];
	uchar_t src_route[18];
};

typedef struct tok_machdr machdr_t;

union llc_header {
	struct llctype {
		uchar_t		llc_dsap;
		uchar_t		llc_ssap;
		uchar_t		llc_control;
		uchar_t		llc_info[3];
	}llc_sap;

	struct llc_snap {
		uchar_t		llc_dsap;
		uchar_t		llc_ssap;
		uchar_t		llc_control;
		uchar_t		org_code[3];
		ushort_t	ether_type; 
		uchar_t		llc_info[3];
	}llc_snap;

	uchar_t src_route[18];
};

typedef union llc_header	llc_hdr_t;

struct mac_llc_hdr {
	uchar_t		mac_dst[6];
	uchar_t		mac_src[6];
	llc_hdr_t	llc;
};

struct llc_info {
	ushort_t	ssap;		/* Source SAP */
	ushort_t	dsap;		/* Destination SAP */
	ushort_t	snap;		/* SNAP field */
	ushort_t	control;	/* LLC control Field */
	machdr_t	*mac_ptr;	/* Pointer to the MAC header */
	llc_hdr_t	*llc_ptr;	/* Pointer to the LLC header */
	uchar_t		*data_ptr;	/* Pointer to first byte of data */
	ushort_t	rsize;		/* Number of byte in routing field */
	ushort_t	b_cast;		/* Broadcast field if routing field */
	ushort_t	direction;	/* Direction bit */
	ushort_t	lf;		/* Size of longest frame */
};

typedef struct llc_info	llc_info_t;

#define LLC_SAP_LEN		1	/* length of sap only field */
#define LLC_LSAP_LEN		2	/* length of sap/type field  */
#define	LLC_CNTRL_LEN		1	/* Length of control field */
#define LLC_SNAP_LEN		5	/* Length of LLC SNAP fields */
#define LLC_SAP_H_LEN		3	/* Length of LLC SAP header */
#define LLC_SNAP_H_LEN		5	/* Length of LLC SNAP header */
#define LLC_XID_INFO_SIZE	3	/* length of the INFO field */


/* Length of MAC address fields */
#define MAC_HDR_SIZE	(MAC_ADDR_LEN+MAC_ADDR_LEN)

/* Length of LAN header size (minimum). */
#define MIN_LAN_HDR_SIZE 0x0E

/* Length of 802.2 LLC Header */
#define LLC_HDR_SIZE	(MAC_HDR_SIZE+LLC_SAP_LEN+LLC_SAP_LEN+LLC_CNTRL_LEN)

/* Length of extended LLC header with SNAP fields */
#define LLC_EHDR_SIZE	(LLC_HDR_SIZE + LLC_SNAP_LEN)

/* Length of LLI address message fields */
#define LLC_LIADDR_LEN		(MAC_ADDR_LEN+LLC_SAP_LEN)
#define LLC_ENADDR_LEN		(MAC_ADDR_LEN+LLC_LSAP_LEN)
#define	LLC_ENR_MAX_LEN		(LLC_ENADDR_LEN+MAX_ROUTE_FLD)
#define	LLC_LIR_MAX_LEN		(LLC_LIADDR_LEN+MAX_ROUTE_FLD)

#define	MAX_ROUTE_FLD	18	/* Maximum of 18 bytes of routing info */

union llc_bind_fmt {

	struct llca {
		uchar_t		lbf_addr[MAC_ADDR_LEN];
		ushort_t	lbf_sap;
	} llca;

	struct llcb {
		uchar_t		lbf_addr[MAC_ADDR_LEN];
		ushort_t	lbf_sap;
		ulong_t		lbf_xsap;
		ulong_t		lbf_type;
	} llcb;

	struct llcc {
		uchar_t		lbf_addr[MAC_ADDR_LEN];
		uchar_t		lbf_sap;
	} llcc;
};

#define MAXSAPVALUE	0xFF	/* largest LSAP value */

#define	DL_802		0x01	/* for type field 802.2 type packets */
#define	DL_SNAP		0x02	/* for type field 802.2 SNAP type packets */
#define DL_ROUTE	0x04	/* to indicate IBM routing field */
#define DL_RESPONSE	0x08	/* to indicate a response type packet */

/* Largest Frame Size */

#define	S_516	0x0000	/* Up to 516 octects in the information field */
#define	S_1500	0x0010	/* Up to 1500 octects in the information field */
#define	S_2052	0x0100	/* Up to 2052 octects in the information field */
#define	S_4472	0x0110	/* Up to 4472 octects in the information field */
#define	S_8191	0x1000	/* Up to 8191 octects in the information field */

#define	S_MYMAX	S_1500	/* This is the MAX of this bridge */

/* recoverable error conditions */

#define E_OK		0	/* normal condition */
#define E_NOBUFFER	1	/* couldn't allocb */
#define E_INVALID	2	/* operation isn't valid at this time */

/* LLC specific data - should be in separate header (later) */

#define LLC_UI		0x03	/* unnumbered information field */
#define LLC_XID		0xAF	/* XID with P == 0 */
#define LLC_TEST	0xE3	/* TEST with P == 0 */

#define LLC_P		0x10	/* P bit for use with XID/TEST */
#define LLC_XID_FMTID	0x81	/* XID format identifier */
#define LLC_SERVICES	0x80	/* Services supported */
#define LLC_GLOBAL_SAP	0XFF	/* Global SAP address */
#ifndef	LLC_NULL_SAP
#define LLC_NULL_SAP	0X00	/* NULL SAP address */
#endif
#define LLC_SNAP_SAP	0xAA	/* SNAP SAP */
#define LLC_GROUP_ADDR	0x01	/* indication in DSAP of a group address */
#define LLC_RESPONSE	0x01	/* indication in SSAP of a response */

#if defined(__cplusplus)
	}
#endif
#endif /* _IO_DLPI_CPQ_CET_TOKEN_DLPI_TOKEN_H */
