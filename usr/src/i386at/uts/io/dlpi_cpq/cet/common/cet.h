/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_CPQ_CET_COMMON_CET_H	/* wrapper symbol for kernel use */
#define _IO_DLPI_CPQ_CET_COMMON_CET_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/dlpi_cpq/cet/common/cet.h	1.9"
#ident	"$Header: $"

/*
	Copyright (c) Compaq Computer Corporation, 1990, 1991, 1992, 1993, 1994 

	"cet.h" -	defines for the Compaq Ethernet/Token Ring driver.
*/

#ifdef	_KERNEL_HEADERS

#ifdef	ETHER
#ifndef	_IO_DLPI_ETHER_DLPI_ETHER_H
#include <io/dlpi_ether/dlpi_ether.h>		/* REQUIRED */
#endif
#endif

#ifdef	TOKEN
#ifndef	_IO_DLPI_CPQ_CET_TOKEN_DLPI_TOKEN_H
#include <io/dlpi_cpq/cet/token/dlpi_token.h>	/* REQUIRED */
#endif
#endif

#ifndef	_NET_INET_IF_H
#include <net/inet/if.h>			/* REQUIRED */
#endif

#ifndef	_MEM_IMMU_H
#include <mem/immu.h>				/* REQUIRED */
#endif

#elif defined(_KERNEL)

#ifdef	ETHER
#include <sys/dlpi_ether.h>			/* REQUIRED */
#endif

#ifdef TOKEN
#include <sys/dlpi_token.h>			/* REQUIRED */
#endif

#include <net/if.h>				/* REQUIRED */
#include <sys/immu.h>				/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define		CET_ID			('C' << 24 | 'E' << 16 | 'T' << 8)
#define		CET_STARTUP		(CET_ID | 1)
#define		CET_SHUTDOWN		(CET_ID | 2)
#define		CET_GETCONFIG		(CET_ID | 8)
#define		CET_MIB2_STATS		(CET_ID | 9)
#define		MAXNSAPLEN		24
#define		MAX_EISA_SLOTS		16


#define		CET_MAX_UNITS		4	/* Max units (interfaces). */
/* These structures are used for code download and don't need to be "packed". */

#define	CET_MAX_MODS		4	/* Maximum modules for a board id. */
#define	CET_MAX_MEDS		3	/* Maximum media types. */

#pragma pack(1)

typedef struct
   {
   uchar_t zc80;
   uchar_t zc81;
   uchar_t zc82;
   uchar_t zc83;
   } BID_PORTS;		/* Compressed EISA board id port reference. */

typedef union
   {
   BID_PORTS port;	/* Individual ports. */
   ulong whole;		/* Compressed EISA board id as a long integer. */ 
   } BID;		/* EISA board id reference. */

#pragma pack()

typedef struct
   {
   short module[CET_MAX_MODS];		/* On-board code module indexes. */
   } CET_MODULE;

typedef struct
   {
   char ascii_id[8];		/* Expanded ASCII text EISA board id. */
   BID eisa_id;			/* Compressed EISA board id. */
   CET_MODULE medium[CET_MAX_MEDS];  /* Modules for Ethernet and Token Ring. */
   } CET_BOARD_ID;

typedef struct cet_obc	/* Compaq Ethernet/Token Ring on-board code. */
   {
   char name[32];	/* Name of the on-board code file. */
   caddr_t obc;		/* Pointer to the "on-board" code file in memory. */
   ushort obcs;		/* Size of the "on-board" code file in memory. */
   } CET_OBC;

typedef struct
   {
   CET_OBC on_board_code[16];	/* On-board code reset structures. */
   CET_BOARD_ID board_ids[8];	/* Board-id to on-board code map. */
   uchar_t node_address[CET_MAX_UNITS][6];	/* One for each unit. */
   } CET_INIT;


#if defined(_KERNEL)

#define		OK			1
#define		NOT_OK			0

#define		SLOT_HEADER		(sizeof(ushort) + sizeof(NVM_SLOTINFO)) 

#define		CET_MAX_BOARDS		4	/* Max boards. */
/*
 * Moved outside of _KERNEL defined area since it is needed by
 * cet_start and cet_stop commands.
 *#define		CET_MAX_UNITS		4	 Max units (interfaces).
 */
#define		CET_MAX_IPB		2	/* Max interfaces per board. */
#define		CET_BOARD_SIZE		(SLOT_HEADER + CET_MAX_IPB * sizeof(NVM_FUNCINFO))
#define		CET_EISA_BUF_SIZ	(CET_MAX_BOARDS * (CET_BOARD_SIZE))
#define		CET_MAX_BOARD_IDS	16	/* Board ids supported. */

#define		TR_MAX_SIZE	4472
#define		TR_MAX_FRAME		(TR_MAX_SIZE - (14+30+5+3))
#define		E_MAX_FRAME		1500
#define		TR_MIN_FRAME		sizeof(uchar_t)
#define		E_MIN_FRAME		46
#define		MAX_MINORS		8	/* Max minor devices. */
#define		MAX_MCAs		6	/* Max multi-cast addresses. */
#ifdef TOKEN
#define		TBS			4472	/* Streams buffer size for the
						   pool of transmit blocks. */
#define		RBS			4472	/* Streams buffer size for the
						   pool of receive blocks. */
#endif
#ifdef ETHER
#define		TBS			2048	/* Streams buffer size for the
						   pool of transmit blocks. */
#define		RBS			2048	/* Streams buffer size for the
						   pool of receive blocks. */
#endif

#define		CET_DMASIZE		32	/* Number of bits "dma" (bus
						   master can address. */

#define		HTOA(d)			("0123456789ABCDEF"[(d)])

#ifdef	ESMP
/*
 * Locking Hierarchy Definitions
 *
 * DL_TIMDELCK_HIER: locking hierarchy when locking DL_timer_lck.
 * DL_BOARDLCK_HIER: locking hierarchy when locking individual DL_bdconfig_t
 *	structures via bd_lock.
 */
#ifdef	ETHER
#define		NFLXE_BOARDLCK_HIER		2
#define		NFLXE_TIMERLCK_HIER		3
#else
#define		NFLXT_BOARDLCK_HIER		2
#define		NFLXT_TIMERLCK_HIER		3
#endif	/* ETHER */

extern lock_t	*DL_timer_lck;
extern int	DL_initialized;
#endif	/* ESMP */
#if defined(CETDEBUG) || defined(DEBUG)
extern unsigned long DL_current_lockaddr;
extern unsigned long DL_previous_lockaddr;
extern void DL_lockinfo(int);
#define	CETDEBUGPRNT(x)	(x)
#else
#define	CETDEBUGPRNT(x)
#endif

/*	These can be used to transform Intel byte-order to Motorola
		and vice-versa. */

#define		REVERSE_4_BYTES(v, r)	((uchar_t *)(r))[0] = ((uchar_t *)(v))[3];\
					((uchar_t *)(r))[1] = ((uchar_t *)(v))[2];\
					((uchar_t *)(r))[2] = ((uchar_t *)(v))[1];\
					((uchar_t *)(r))[3] = ((uchar_t *)(v))[0]

#define		REVERSE_2_BYTES(v, r) 	((uchar_t *)(r))[0] = ((uchar_t *)(v))[1];\
					((uchar_t *)(r))[1] = ((uchar_t *)(v))[0]

#define		REVERSE_2_WORDS(v, r) 	((ushort *)(r))[0] = ((ushort *)(v))[1];\
					((ushort *)(r))[1] = ((ushort *)(v))[0]

/*	This can be used to determine if a hardware address is a broadcast
	address (all '1's) or any other sort of network address matching. */
 
#define	ADDR_MATCH(a1, a2)	(*(ulong *)a1 == *(ulong *)a2 && *(ushort *)(a1 + sizeof(ulong)) == *(ushort *)(a2 + sizeof(ulong)))

#define	MAX_PDU(board)		MAX_FRAME_SIZE

#define MCA_TO_LONG(mca)	(*(ulong *)((uchar_t *)(mca) + 2))

typedef struct obc_chap	/* TMS380 on-board code chapter header. */
   {
   ushort chapter;	/* TMS380 chapter address. */
   ushort address;	/* TMS380 start address within chapter. */
   ushort bytes;	/* TMS380 bytes of code from start address. */
   } OBC_CHAP;

typedef struct obc_hdr	/* TMS380 on-board code header. */
   {
   ushort length;	/* Total length of on-board code header. */
   OBC_CHAP chap_hdr;	/* First chapter header. There may be more. This is
			   just a template for the beginning of the header. */
   } OBC_HDR;

#ifdef	ESMP
/*
 * This maps Board ID's to Board Names.  It is declared in the space.c file and
 * is used by the driver "init" function to map board id's to board names.
 *
 * cetbrd_brdid		-uncompressed Board ID.
 * cetbrd_brdname	-Board name corresponding to Board ID.
 * cetbrd_otherbrdname	-other Board name corresponging to Board ID.
 *			 Certain Board IDs resquire additional checking on the
 *			 physical board itself to determine the type of board
 *			 (i.e. either Etherner or Token-Ring) and hence its
 *			 Board name.  This entry MUST be NULL if no other
 *			 Board name exists.
 * cetbrd_numunits	-number of physical connections (aka units) the board
 *			 supports (eg. Daul Port NetFlex NICs support 2 units).
 */
struct cetboard_idtoname {
	char	*cetbrd_brdid;
	char	*cetbrd_brdname;
	char	*cetbrd_otherbrdname;
	ushort	cetbrd_numunits;
};

/*
 * This maps NVM sub-type strings to Board's network type, speed and media.
 * It is declared in Space.c and is used by the driver "init" function.
 *
 * cetnet_tm_string	-This is the string to match on in Board's the NVM	 *			 function sub-type field.
 * cetnet_tm_nettype	-This is the mapped network type and speed.  (eg.
 *			 10MBPS Ethernet).
 * cetnet_tm_netmedia	-This is the mapped network media (e.g. AUI).
 */
struct cetnet_type_media {
	char	*cetnet_tm_string;
	ushort	cetnet_tm_nettype;
	ushort	cetnet_tm_netmedia;
};
#endif	/* ESMP */


/* The remaining data relates directly to the board and needs to be "packed". */
 
#pragma pack(1)

/* The slot/unit structure is defined in the space.c file and used by the
   driver "init" routine to detect differences between the EISA configuration
   and the kernel configuration. */

typedef struct slot_unit
   {
   short slot;		/* EISA slot number. */
   short interface;	/* Interface number in slot. */
   short unit;		/* External unit number. */
   ushort net_type;
   ushort net_media;
   char id[4];
   uchar_t board_id[18];
   char id_string[32];
   void	*intr_cookiep;  /* used by system's interrupt attach
			 * and detach functions.
			 */
   } SLOT_UNIT;

typedef struct arpsentto
   {
   long to_pend;	/* Pending timeout flag. */
   long to_index;	/* Pending timeout index. */
   long arp_tpa;	/* ARP Target Protocol Address. */
   } ARPSENTTO;

typedef ushort TMS380_WORD;

/*	NetFlex configuration registers		*/

#define	CetEISAcfg		0x01c	/* EISA configuration register. */
#define	CetFDdisabler		0x0c64	/* Full-duplex disable register. */
#define	CetFDenabler		0x0c65	/* Full-duplex enable register. */
#define NetFlexFDenable		0x0c65	/* Full-duplex enable for NetFlex. */
#define	NetFlexFDverify		0x0c64	/* Full-duplex verify for NetFlex. */
#define DualPort1FDenable	0x0c66	/* Full-duplex enable for DualPort 1. */
#define DualPort1FDverify	0x0c66	/* Full-duplex verify for DualPort 1. */
#define DualPort2FDenable	0x0c67	/* Full-duplex enable for DualPort 2. */
#define DualPort2FDverify	0x0c67	/* Full-duplex verify for DualPort 2. */
#define PrimaryIF		0x04	/* DualPort interface 1 config bit. */
#define SecondaryIF		0x08	/* DualPort interface 2 config bit. */
#define FullDuplexVerify	0x80	/* Full-duplex enable detection bit. */
#define	NetFlexZc82		0x61	/* MS byte of NetFlex board id. */
#ifdef ETHER
#define	DualPortZc82		0x62	/* MS byte of DualPort board id. */
#endif
#ifdef TOKEN
#define	DualPortZc82		0x63	/* MS byte of DualPort board id. */
#endif
#define	FullDuplexMode		0x2000	/* For setting open options. */
#define	TenBaseT		0	/* Network medium type. */
#define	TenBase5		1	/* Network medium type. */

/*	Proteon Registers	*/

#define	PROT_CFG1		0x0c84	/* Proteon configuration register. */

/*	Proteon Commands	*/

#define	RESET_ASSERT		0	/* Assert RESET line. */
#define	RESET_DEASSERT		1	/* Deassert RESET line. */

/*	TMS380 Command codes (swapped bytes)  */

#define	CMD_OPEN	0x0300	/* Open adapter */
#define	CMD_XMT 	0x0400	/* Transmit frame */
#define	CMD_XMT_HLT	0x0500	/* Transmit halt */
#define	CMD_RCV		0x0600	/* Receive  */
#define	CMD_CLOSE	0x0700	/* Close adapter */
#define	CMD_SET_GROUP	0x0800	/* Set group address */
#define	CMD_SET_FUNC	0x0900	/* Set functional address */
#define	CMD_READ_LOG	0x0A00	/* Read statistics/error log command */
#define	CMD_READ_ADAP	0x0B00	/* Read adapter */
#define	CMD_MOD_OPEN	0x0D00	/* Modify open parameters */
#define	CMD_RES_OPEN	0x0E00	/* Restore open parameters */
#define	CMD_SET_GRP16	0x0F00	/* Set 16 bits grp addr (Eagle) */
#define	CMD_SET_BRIDGE	0x1000	/* Set bridge params (Eagle) */
#define	CMD_CFG_BRIDGE	0x1100	/* Config bridge parms (Eagle) */
#define CMD_SET_MCA	0x1200	/* Set Multicast Address (Super Eagle) */

#define CET_DEL_MCA	0x0000	/* Delete a specific multicast address. */
#define CET_ADD_MCA	0x0100	/* Add a specific multicast address. */
#define CET_DEL_ALL_MCA	0x0200	/* Clear all multicast addresses. */
#define CET_SET_ALL_MCA	0x0300	/* Set all multicast addresses. */

/*	Adapter control register bits (Eagle only) */

#define	ACTL_SWHLDA		0x0800	/* Software hold acknowledge */
#define	ACTL_SWDDIR		0x0400	/* Current SDDIR signal value */
#define	ACTL_SWHRQ		0x0200	/* Current SHRQ signal value */
#define	ACTL_PSDMAEN		0x0100	/* Pseudo DMA enable */
#define	ACTL_ARESET		0x0080	/* Adapter reset */
#define	ACTL_CPHALT		0x0040	/* Communication processor halt */
#define	ACTL_BOOT		0x0020	/* Bootstrapped CP code */
#define	ACTL_SINTEN		0x0008	/* System interrupt enable */
#define	ACTL_PEN		0x0004	/* Adapter parity enable */

/* Network type selection constants (NSELOUT and TEST bits) */

#define	ACTL_NSELOUT0		0x0002	/* Network Select Output 0 */
#define	ACTL_NSELOUT1		0x0001	/* Network Select Output 1 */
#define ACTL_TEST0		0x8000	/* Test bit 0 */
#define ACTL_TEST1		0x4000	/* Test bit 1 */
#define ACTL_TEST2		0x2000	/* Test bit 2 */
#define NET_ETHERNET		0x0000
#define NET_TPR_16Mbps		(ACTL_TEST1 >> 14)
#define NET_IEEE_8023		(ACTL_TEST0 >> 14)
#define NET_TPR_4Mbps		((ACTL_TEST0 | ACTL_TEST1) >> 14)
#define NET_SELECT_ACTIVE	(ACTL_TEST2)
#define	TOKEN_RING_4Mbps	(ACTL_NSELOUT0)
#define	DB_CONNECTOR		(ACTL_NSELOUT1)
#define	TP_CONNECTOR		0x0000

typedef struct net_types
   {
   ushort net_type;		/* Index of network type. */
   char *type_string;		/* Text description of network type. */
   ushort code_select;		/* Index to a particular on-board code file. */

   /* These variables perform the Ethernet/Token Ring abstraction. */

   uchar_t (*valid_haddr)();	/* Media-independent hardware address
				   validation function pointer, initialized
				   in "cet_start". */
   uchar_t (*valid_mca)();	/* Media-independent MCA validation function
				   pointer, initialized in "cet_start". */
   void (*mod_mca)();		/* Media-independent multicast address install
				   function pointer, initialized in "cet_start". */
   void (*log_stats)();		/* Media-independent board statistics logging
				   function pointer. */
   ushort open_opts;
   ulong bps;			/* Theoretical speed of the network medium. */
   uchar_t *broadcast;		/* Media-independent broadcast address. */
   uchar_t *multicast;		/* Media-independent multicast address. */
   ulong max_sdu;		/* Maximum Protocol Data Unit. */
   ulong min_sdu;		/* Minimum Protocol Data Unit. */
   ulong mac_type;		/* Network type, Ethernet vs Token Ring. */
   } NET_TYPE;

/*	DIO register offsets */

#define	SIF_DATA	0	/* sif data port */
#define	SIF_INC		2	/* sif inc port */
#define	SIF_INT		6	/* sif interrupt port */
#define	PCON		8	/* ACTL port */
#define	SIF_ADR		10	/* sif address port */
#define	SIF_ADRX	12	/* sif extended address port */

/*	SIF interrupt register commands */

#define SIF_INT_ENABLE		0	/* Re-enable interrupts from adapter. */
#define	SIF_RESET		0xFF00	/* Reset the adapter */
#define	SIF_SSBCLR		0xA080	/* System status block clear */
#define	SIF_CMDINT		0x9080	/* Interrupt and execute scb command  */
#define	SIF_SCBREQ		0x0880	/* Request interrupt when scb is clear */
#define	SIF_RCVCONT		0x0480	/* Purge active rcv frame from adapter */
#define	SIF_RCVVAL		0x8280	/* Receive list is valid */
#define	SIF_XMTVAL		0x8180	/* Transmit list is valid */

/*	SIF interrupt register status definitions */

#define	SIF_DIAGOK		0x0040	/* Bring up diags OK */
#define	SIF_INITOK		0x0070	/* Initialize OK if && 0 */
#define	SIF_SYSINT		0x0080	/* Interrupt system valid */

/*	SSB command completion codes (Swapped bytes) */

#define	SSB_DIROK		0x0080	/* Direct command success */
#define COMMAND_REJECT		0x0200	/* Command code in SSB for rejected command. */

/*	Adapter -> Host interrupt types. */

#define INTERRUPT_TYPE		0x000f	/* Defined by SIF Status bits 12-15. */
#define ADAPTER_CHECK		0x0000	/* Adapter Check interrupt. */
#define RING_STATUS		0x0004	/* Ring Status interrupt. */
#define SCB_CLEAR		0x0006	/* SCB Cleared interrupt. */
#define COMMAND_STATUS		0x0008	/* Command Status interrupt. */
#define RECEIVE_STATUS		0x000a	/* Receive Status interrupt. */
#define TRANSMIT_STATUS		0x000c	/* Transmit Status interrupt. */

/*	Transmit CSTAT rest bits (Swapped bytes) */

#define	XMT_VALID		0x0080	/* Transmit list is valid */
#define	XMT_CPLT		0x0040	/* Transmit list complete */
#define	XMT_SOF			0x0020	/* Start of frame transmit list */
#define	XMT_EOF			0x0010	/* End of frame transmit list */
#define	XMT_FRM_INT		0x0008	/* Interrupt when frame xmit complete */

/*	Transmit CSTAT complete bits (Swapped bytes) */

#define	XMT_ERROR		0x0004	/* Transmit error */

/*	Receive CSTAT rest bits (Swapped bytes) */

#define	RCV_VALID		0x0080	/* Receive list valid */
#define	RCV_FRM_INT		0x0008	/* Single frame interrupt */
#define	RCV_FRMWT		0x0004	/* Interframe wait */
#define	RCV_CRC			0x0002	/* Pass CRC */

/*	Receive CSTAT complete bits (Swapped bytes) */

#define	RCV_SOF	 		0x0020	/* Start of frame transmit list */
#define	RCV_EOF			0x0010	/* End of frame transmit list */
#define	RCV_CPLT		0x0040	/* Receive list complete */

/*
	This is a line diagram of the driver states with respect to the
	UNITED or JUPITER boards. Each word in upper case is a state.
	Each word in mixed upper and lower case is a successful occurrence
	of an event or action causing a state transition. Unsuccessful
	occurrences of events or actions are not shown, implying that no
	state transition occurs. The sequence of possible transitions is
	very simple. At POR, the board is DOWN. During system boot, the
	board is "started" or "brought up". This either succeeds or fails,
	on a one-time basis. If state "UP" is reached, an attempt is made
	at attaining the OPEN state. If state "OPEN" is reached, a "success"
	message is displayed by the startup program. When the startup
	program closes the driver, a transition is made to state "UP" as
	the startup program was the last (only) program connected to the
	driver. When the next program (protocol stack) opens the driver, the
	state will be "UP" and an attempt will be made to attain the "OPEN"
	state. After successfully reaching the "OPEN" state, the driver will
	remain there until one of the two events shown occurs. If the last
	program connected to the driver closes the connection, the driver
	makes a transition to state "UP" and remains there until the next
	program opens it. In the case of the second possible event (failure),
	the driver makes a transition to state "FAIL" and attempts to 
	reattain the "OPEN" state without disturbing any of the programs
	connected to it. In the case of the third possible event (set address),
	the driver makes a transition to state "REOPEN" and attempts to re-
	attain the "OPEN" state with the new hardware address.


					  |-Last-Close-> UP
					  |
	DOWN -Bring-Up-> UP -Open-> OPEN -|-Failure-> FAIL -Open-> OPEN
					  |
					  |-Set-Address-> REOPEN -Open-> OPEN
*/

#define DOWN		0	/* Initial POR state. */
#define UP		1	/* Code downloaded, reset and initialized. */
#define OPEN		2	/* Open, receiving and transmitting. */
#define FAIL		3	/* Failure recovery. */
#define REOPEN		4	/* Reopening after changing physical address. */

/*	Miscellaneous adapter stuff */

#define	RAM_START		0x0640	/* Starting RAM addr (Swapped) */
#define	RAM_END			0xFE7F	/* Ending RAM addr (Swapped)     */
#define	DFL_BUF			1024	/* Default buffer size		 */
#define	XMT_RES			8	/* Transmit buffers reserved. */
#define	CHK_ADDR		0x05E0	/* Address of adapter chk info */
#ifdef ETHER
#define	TBPL			1	/* Num of data blks in xmt list */
#define	RBPL			1	/* Num of data blks in rcv list */
#endif
#ifdef TOKEN
#define	TBPL			1	/* Num of data blks in xmt list */
#define	RBPL			1	/* Num of data blks in rcv list */
#endif
#define	XMTLISTSIZ		8 + (TBPL * 6) /* Transmit list size  */
#define	RCVLISTSIZ		8 + (RBPL * 6)	/* Receive list size */
#ifdef ETHER
#define	XMTLIST			32	/* Number of xmt lists */
#define	RCVLIST			32	/* Number of rcv lists */
#endif
#ifdef TOKEN
#define	XMTLIST			32	/* Number of xmt lists */
#define	RCVLIST			32	/* Number of rcv lists */
#endif
#define OBC_HDR_END		0x7ffe	/* On-board code header end marker. */
#define MULTI_NIC_OFFSET	0x20	/* Offset between multi-interface NIC
					   i/o spaces (Godzilla, Rodan). */

/*	Timeouts for reset and init */

#define	RESET_TRYS		1000	/* Number of times to read the SIF_INT
					   register before failing, while
					   waiting for a reset to complete. */
#define	INIT_TRYS		1800	/* Number of times to read the SIF_INT
					   register before failing, while
					   waiting for an init to complete. */
#define	SFT_TICS      		5	/* Clock tics to wait after soft reset */
#define	DIAG_TICS		400	/* Clock tics for diagnostics to complete */
#define	OPEN_WAIT		2500	/* Clock tics to allow for "open". */
#define MIN_TICKS		3	/* Minimum number of "ticks" one should
					   use when setting a "timeout". */

#define	CET_FPA			0x0010	/* ACR bit 11 FPA. */
#define	CET_NON_FPA		0x0000	/* ACR bit 11 non-FPA. */

#define	SIGNAL_LOSS		0x0080	/* Signal Loss ring interrupt. */
#define	HARD_ERROR		0x0040	/* Hard Error ring interrupt. */
#define	SOFT_ERROR		0x0020	/* Soft Error ring interrupt. */
#define	TRANSMIT_BEACON		0x0010	/* Transmit Beacon ring interrupt. */
#define	LOBE_WIRE_FAULT		0x0008	/* Lobe Wire Fault ring interrupt. */
#define	AUTO_REMOVAL_ERROR	0x0004	/* Auto Removal Error ring interrupt. */
#define	REMOVE_RECEIVED		0x0001	/* Remove Received ring interrupt. */
#define	COUNTER_OVERFLOW	0x8000	/* Counter Overflow ring interrupt. */
#define	SINGLE_STATION		0x4000	/* Single Station ring interrupt. */
#define	RING_RECOVERY		0x2000	/* Ring Recovery ring interrupt. */

/*	Initialization Parameter Block */

#define	IPB_OPTS		0x9300	/* Default INIT_OPTIONS */
#define	IPB_ADDR	 	0x0A00	/* Initialization address */
#define HARD_ADDR		0x0
#define SOFT_ADDR		0x4
#define SPEED			0x0c
#define	IPB_DMAT		0x0505	/* DMA abort threshold	 */

typedef struct ipb
   {
   ushort options;		/* Initialization options */
   uchar_t cmd_vec;		/* Command status vector */
   uchar_t xmt_vec;		/* Transmit status vector */
   uchar_t rcv_vec;		/* Receive status vector */
   uchar_t rng_vec;		/* Ring status vector */
   uchar_t ssb_clr;		/* SSB clear vector */
   uchar_t adp_chk;		/* Adapter check vector */
   ushort rcv_brst;		/* Receive burst size */
   ushort xmt_brst;		/* Transmit burst size */
   ushort dma_abrt;		/* DMA abort treshold */
   paddr_t scb_addr;		/* Physical address of SCB, word-swapped. */
   paddr_t ssb_addr;		/* Physical address of SSB, word-swapped. */
   } IPB;

/*	System Command Block	*/

typedef struct scb
   {
   ushort cmd;		/* Command code */
   ushort parm0;	/* Parameter 0 */
   ushort parm1;	/* Parameter 1 */
   } SCB;

/*	System Status Block	*/

typedef struct ssb
   {
   ushort cmd;		/* Command code */
   ushort parm0;	/* First parameter */
   ushort parm1;	/* Second parameter */
   ushort parm2;	/* Third parameter */
   } SSB;

/* These represent normal open options for ethernet and token ring. */

#define	E_MANDATORY		0x4000
#define E_OPB_OPTS		(E_MANDATORY)
#define TR_OPB_OPTS		0x0000

/* These represent promiscuous mode open options for ethernet and token ring. */

#define FRAME_HOLD		0x0002
#define	COPY_MAC		0x0400
#define	COPY_NON_MAC		0x0200
#define	TR_COPY_ALL		(COPY_MAC | COPY_NON_MAC | FRAME_HOLD)
#define	COPY_ALL_FRAMES		0x0200
#define	E_COPY_ALL		(COPY_ALL_FRAMES | FRAME_HOLD)

/*	Open Parameter Block */

typedef struct opb
   {
   ushort options;		/* Open options */
   uchar_t node[6];		/* Node address */
   uchar_t group[4];		/* Group address */
   uchar_t func[4];		/* Functional address */
   ushort rcv_size;		/* Receive list size */
   ushort xmt_size;		/* Transmit list size */
   ushort buf_size;		/* Internal buffer size */
   ushort ram_start;		/* RAM start address */
   ushort ram_end;		/* RAM end address */
   uchar_t xmt_min;		/* Transmit buffer minimum */
   uchar_t xmt_max;		/* Transmit buffer maximum */
   ulong prod_id;		/* Physical address of Product ID (Motorola). */
   } OPB;

typedef struct data_block
   {
   ushort count;		/* Data block size */
   ulong data;			/* Pointer to data block */
   } DBLK;

/*	Transmit list structure */

typedef struct xmt_list
   {
   ulong p_fwd;			/* Forward pointer (physical address). */
   ushort cstat;		/* Transmit status bits */
   ushort size;			/* Frame size */
   DBLK blocks[TBPL];		/* Data count and pointer to data. */
   uchar_t *mb[TBPL];		/* Virtual address of data block for each
					block in this transmit list. */
   struct xmt_list *v_bwd;	/* Backward pointer (virtual address). */
   struct xmt_list *v_fwd;	/* Forward pointer (virtual address). */
   } XMT_LIST;

/*	Receive list structure */

typedef struct rcv_list
   {
   ulong p_fwd;			/* Forward pointer (physical address). */
   ushort cstat;		/* Receive status bits */
   ushort size;			/* Frame size */
   DBLK blocks[RBPL];		/* Data count and pointer to data. */
   uchar_t *mb[RBPL];		/* Virtual address of data block for each
					block in this receive list. */
   struct rcv_list *v_bwd;	/* Backward pointer (virtual address). */
   struct rcv_list *v_fwd;	/* Forward pointer (virtual address). */
   } RCV_LIST;

/* unix 4.2 glue */
typedef struct sap	MINOR;

/* Multi-cast address to open minor device mapping. */

typedef struct dam
   {
   uchar_t address[6];		/* A multi-cast address. */
   MINOR *devices[MAX_MINORS];	/* A list of minor devices it's mapped to. */
   } DAM;

/* Multi-cast address data. */

typedef struct mca
   {
   DAM dam[MAX_MCAs];	/* Minor device to multi-cast address mapping. */
   } MCA;

/* Multicast Parameter Block for Ethernet (Super Eagle). */

typedef struct mpb
   {
   ushort options;
   uchar_t address[6];		/* A multi-cast address. */
   } MPB;

/* Stucture of each entry in the MACGETSTAT ioctl data block. */

typedef struct counter
   {
   long name, value;
   } CNTR;

typedef struct
   {
   uchar_t line_err;
   uchar_t reserved;
   uchar_t burst_err;
   uchar_t ari_fci_err;
   uchar_t res1;
   uchar_t res2;
   uchar_t lost_frm_err;
   uchar_t rx_cong_err;
   uchar_t frm_cpyd_err;
   uchar_t res3;
   uchar_t token_err;
   uchar_t res4;
   uchar_t dma_bus_err;
   uchar_t dma_par_err;
   } TR_STATS;

typedef struct
   {
   ulong rx_ok;
   ulong reserved;
   ulong fcs_err;
   ulong align_err;
   ulong deferred_tx;
   ulong xs_coll;
   ulong late_coll;
   ulong carr_sense_err;
   ulong tx_ok;
   ulong coll_1;
   ulong coll_2;
   ulong coll_3;
   ulong coll_4;
   ulong coll_5;
   ulong coll_6;
   ulong coll_7;
   ulong coll_8;
   ulong coll_9;
   ulong coll_10;
   ulong coll_11;
   ulong coll_12;
   ulong coll_13;
   ulong coll_14;
   ulong coll_15;
   ulong fcs_err_last;
   ulong align_err_last;
   ulong defer_tx_last;
   ulong xs_coll_last;
   ulong late_coll_last;
   ulong carr_sense_last;
   ulong coll_1_last;
   ulong coll_2_last;
   ulong coll_3_last;
   ulong coll_4_last;
   ulong coll_5_last;
   ulong coll_6_last;
   ulong coll_7_last;
   ulong coll_8_last;
   ulong coll_9_last;
   ulong coll_10_last;
   ulong coll_11_last;
   ulong coll_12_last;
   ulong coll_13_last;
   ulong coll_14_last;
   ulong coll_15_last;
   ulong coll_last;
   } E_STATS;

typedef union
   {
   TR_STATS tr;
   E_STATS e;
   } STATS_LOG;

#endif /* _KERNEL */
 
typedef struct
   {
   ulong AlignmentErrors;		/* Same as MAC statistics. */
   ulong FCSErrors;			/* Same as MAC statistics. */
   ulong SingleCollisionFrames;		/* From on-board statistics. */
   ulong MultipleCollisionFrames;	/* From on-board statistics. */
   ulong SQETestErrors;			/* Not reported. */
   ulong DeferredTransmissions;		/* Same as MAC statistics. */
   ulong LateCollisions;		/* Same as MAC statistics. */
   ulong ExcessiveCollisions;		/* Same as MAC statistics. */
   ulong InternalMacTransmitErrors;	/* Not reported. */
   ulong CarrierSenseErrors;		/* Same as MAC statistics. */
   ulong FrameTooLongs;			/* Same as MAC statistics. */
   ulong InternalMacReceiveErrors;	/* Not reported. */
   } E_MIB2;		/* Ethernet (dot3) statistics. */

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
   } TR_MIB2;		/* Token ring (dot5) statistics. */

typedef union
   {
   TR_MIB2 tr;
   E_MIB2 e;
   } MIB2_STATS;	/* SNMP MIB 2 statistics. */

/*
 *  struct used by CET_GETCONFIG
 */
struct configinfo {
	ulong_t		io_start;	/* start of I/O base address	     */
	paddr_t		mem_start;	/* start of base mem address	     */
	int		irq_level;	/* interrupt request level	     */
	int		slot;		/* eisa slot if applicable	     */
};
 
#if defined(_KERNEL)

/*	Board Command Queue Entry	*/

typedef struct cqe
   {
   ushort cmd;		/* Command code. */
   int status;		/* Status code; 0 or Streams message type code. */
   int *error;		/* Pointer to system error field (iocp->ioc_error for
			   ioctls or u.u_error for "open" or "close"). */
   ushort reply;	/* Flag indicating when it's time to reply. */
   int time_out;	/* Return code from "timeout" function. */
   void (*fail_func)();	/* The function to be executed when a timeout pops. */
   queue_t *q;		/* Pointer to the Streams queue that needs a reply. */
   mblk_t *mp;		/* The Streams message we'll reply with. */
   struct iocblk *iocp;	/* Pointer to "ioctl" header. */
   void (*cmd_func)();	/* The function that executes the command. */
   uchar_t *cmd_vec_arg;	/* Pointer to a vector-type argument. */
   ulong cmd_int_arg;	/* An integer-type argument. */
   void (*ack_func)();	/* The function that processes the acknowledgement. */
   uchar_t *ack_vec_arg;	/* Pointer to a vector-type argument. */
   ulong ack_int_arg;	/* An integer-type argument. */
   struct cqe *next;	/* Pointer to the next entry in the command queue. */
   } CQE;		/* Command Queue Entry. */

/*	Qircular queue of board command	queue entries */

typedef struct cq
   {
   CQE *next;		/* The next command to be issued or acknowledged. */
   CQE *free;		/* The next free entry for adding commands. */
   CQE list[MAX_MINORS]; /* The list of commands pending. */
   } CQ;		/* Board Command Queue. */

/*	List of Streams queues to send a frame to. */

typedef struct
   {
   queue_t *queues[MAX_MINORS + 1];
   queue_t **local;			/* Marker indicating where queues
					   may be added to the list. See
					   MAC_PROMISC ioctl code. */
   } Q_LIST;

/*	Per-board data structure	*/

#define BOARD_DATA_STRUCT	\
\
/* Data the board will DMA in and out of. Keep it up front. */\
\
   IPB ipb;			/* Initialization Parameter Block. */\
   SCB scb;			/* System Control Block */\
   SSB ssb;			/* System Status Block */\
   OPB opb;			/* Open Parameter Block. */\
   MPB mpb;			/* Multicast Parameter Block for Ethernet. */\
   XMT_LIST xmtlist[XMTLIST];	/* Transmit lists */\
   RCV_LIST rcvlist[RCVLIST];	/* Receive lists */\
   uchar_t hw_address[6];	/* "burned-in" station address. */\
   uchar_t sw_address[6];	/* Station address currently in use. */\
   uchar_t prod_id[18];		/* Adapter Product ID. */\
   STATS_LOG stats_log;		/* Board-maintained statistics. */\
\
/* End of data the board will DMA in and out of. */\
\
   paddr_t bia;			/* Board pointer to burned-in-address. */\
   paddr_t spa;			/* Board pointer to soft programmed address. */\
   ulong rcv_phys;		/* Physical address of receive lists. */\
   XMT_LIST *xmt_next;		/* Pointer to next available transmit list. */\
   ushort rcv_blk_size;		/* Size of Streams buffers allocated for the\
					receive buffer pool (rcvlist.mbp). */\
   ushort list_size;		/* Sum of block counts in Motorola format. */\
   CQ cmd_q;			/* Queue of commands to board. */\
   ushort base;			/* I/O base address */\
   MINOR minors[MAX_MINORS];	/* Structures ref'd by streams "q->q_ptr". */\
   uchar_t state;		/* Board state. */\
   MCA mca;			/* Multi-cast address data. */\
   struct ifstats ifstats;	/* BSD style statistics. */\
   MIB2_STATS mib2_stats;	/* SNMP MIB 2 dot3 and dot5 statistics. */\
\
   /* These variables perform the Ethernet/Token Ring abstraction. */\
\
   short ext_unit;		/* External unit number. */\
   ushort net_media;		/* The media (cable type) used. */\
   ushort net_type;		/* Network type (TEST0,TEST1 bits). */\
   struct board *next;		/* Pointer to next board in "vec" group. */\
\
   /* unix 4.2 glue */\
   DL_bdconfig_t *bd;		/* ptr to board structure in dlpi level. */\
   DL_sap_t *next_sap;		/* next write_q to read when transmit done. */\
   int interface;		/* interface id for dual port netflex.	*/\
   char id_string[32];

typedef struct board_data
   {
   BOARD_DATA_STRUCT
   } BOARD_DATA;

typedef struct board
   {
   BOARD_DATA_STRUCT
   uchar_t pad[NBPP - sizeof(BOARD_DATA) % NBPP];
   } BOARD;

#define	BOARDP(x)	(&DL_boards[((BOARD *)(x) - DL_boards)])

typedef struct
   {
   BOARD *head;		/* Pointer to "head" board in "irq" group. */
   } IRQG;		/* Interrupt Request Group. */

#endif /* _KERNEL */

#pragma pack()
#endif /* _IO_DLPI_CPQ_CET_COMMON_CET_H */
