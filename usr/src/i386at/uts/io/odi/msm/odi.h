/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_MSM_ODI_H   /* wrapper symbol for kernel use */
#define _IO_MSM_ODI_H   /* subject to change without notice */

#ident	"@(#)kern-i386at:io/odi/msm/odi.h	1.3"
#ident	"$Header: $"

#define	FALSE			0x0
#define	TRUE			0x1
#define	UNUSED			0xFFFFFFFF
#define	BOOLEAN			unsigned char
#define ULONG                   ulong_t
#define USHORT                  ushort_t
#define UCHAR                   uchar_t
#define _cdecl

#define BCOPY(from, to, len)    bcopy((caddr_t)from,(caddr_t)to,(size_t)len)
#define BCMP(s1, s2, len)       bcmp((char *)s1,(char *)s2, (size_t)len )
#define BZERO(addr, len)        bzero((caddr_t)addr,(size_t)len)

/*
 * well defined sizes.
 */
#define	PID_SIZE		0x06	/* Number of Octets in Protocol */
					/* Identifier */
#define	ADDR_SIZE		0x06	/* Number of Octets in Address */

#define	DefaultNumECBs		0x00
#define	DefaultECBSize		1518L	/* not including ECB Structure */
#define	MinECBSize		(512+74+42)L	/* Max. ECB size < 64K  */

/*
 * 42 is mac. FDDI MAC Header.
 */
#define	MAXLOOKAHEADSIZE	128L	/* Max. LookAhead Data Size */

/*
 * define assumed Maximum Media Header Size that we'll encounter.
 * assume that it will be Token-Ring (with SRT).
 *
 * AC, FC, Dest[6], Source[6], SRFields[30], 802.2UI[3], SNAP[5] = 52
 */
#define	MAXMEDIAHEADERSIZE	52L

#define	MAXBOARDS		16L
#define	MAXSTACKS		16L

#define	MAXSTACKNAMELENGTH	65L
#define	MAXNAMELENGTH		64L

#define	MAXMULTICASTS		10L

/*
 * chained protocol stack position values.
 */
#define	CHNPOS_FIRST_MUST	0x0000	/* Must be First */
#define	CHNPOS_FIRST_NEXT	0x0001	/* Next available First */
#define	CHNPOS_LOAD_ORDER	0x0002	/* dependent on Load Order */
#define	CHNPOS_LAST_NEXT	0x0003	/* Next available Last */
#define	CHNPOS_LAST_MUST	0x0004	/* Must be Last */

#define	CHNPOS_MAX_POSIT	0x0004	/* Maximum possible Chain position */
#define	CHAINTYPE_TX		0x0000	/* Transmit type Chain Proto Stack */
#define	CHAINTYPE_RX		0x0001	/* Receive type Chain Proto Stack */

/*
 * MLIDCFG_LookAheadSize values.
 */
#define DEFAULT_LOOK_AHEAD_SIZE	0x12    /* Default of 17 look ahead bytes */
#define MAX_LOOK_AHEAD_SIZE	0x80	/* Maximum 128 look ahead bytes */

/*
 * Rx Packet Attributes	(ie. LkAhd_PktAttr)
 */
#define	PAE_CRC_BIT		0x00000001      /* CRC Error */
#define	PAE_CRC_ALIGN_BIT	0x00000002      /* CRC/Frame Alignment Error */
#define	PAE_RUNT_PACKET_BIT	0x00000004      /* Runt Packet */
#define	PAE_TOO_BIG_BIT		0x00000010      /* Packet Too Large for Media */
#define	PAE_NOT_ENABLED_BIT	0x00000020      /* Unsupported Frame */
#define	PAE_MALFORMED_BIT	0x00000040      /* Malformed Packet */
#define	PA_NONCAN_ADDR_BIT	0x00008000      /* Set if Addr. in Immediate */
						/* Address field is */
						/* noncanonical */
/*
 * Rx Packet Destination Address Types	(ie. LkAhd_DestType)
 */
#define DT_MULTICAST		0x00000001	/* Multicast Dest. address */
						/* (Group Address) */
#define	DT_BROADCAST		0x00000002      /* Broadcast Dest. address */
#define DT_REMOTE_UNICAST	0x00000004	/* Remote Unicast Dest. */
						/* address */
#define DT_REMOTE_MULTICAST	0x00000008	/* Unsupported Multicast */
						/* address */
#define DT_NO_SROUTE		0x00000010	/* Source Routed pkt. (other */
						/* than local ring) and */
						/* Source Routing support */
						/* not loaded. If set all */
						/* other bits should be reset */
#define DT_ERRORED		0x00000020      /* Global Error, exculsive */
						/* bit */
#define DT_DIRECT		0x00000080      /* Unicast for this */
						/* workstation */
#define DT_MASK			0x000000FF      /* mask off allowable */
						/* Destination Address Types */
#define DT_8022_TYPE_I		0x00000100      /* set if packet is */
						/* 802.2 Type I */
#define DT_8022_TYPE_II         0x00000200      /* set if packet is */
						/* 802.2 Type II */
#define DT_8022_BYTES_BITS      0x00000300

#define DT_PROMISCUOUS		DT_DIRECT | DT_MULTICAST | DT_BROADCAST | \
				DT_REMOTE_UNICAST | DT_REMOTE_MULTICAST | \
				DT_NO_SROUTE
/*
 * ECB Definitions, stack ID Definitions.
 */
#define ECB_LLC2                0x8000
#define ECB_LLC2_SR             0x8001
#define	ECB_RAWSEND		0xFFFF		/* Raw Send, ie. ECB */
						/* includes MAC Header */
						/* implies MLID should not  */
						/* build MAC Header for */
						/* this Tx. */
#define	ECB_MULTICAST		0x00000001	/* Multicast Dest. address */
						/* (Group Address) */
#define	ECB_BROADCAST		0x00000002	/* Broadcast Dest. address */
#define	ECB_UNICASTREMOTE	0x00000004	/* Remote Unicast Dest. */
						/* address */
#define	ECB_MULTICASTREMOTE	0x00000008	/* Unsupported Multicast */
						/* address */
#define	ECB_NOSOURCEROUTE	0x00000010	/* Source Routed pkt. */
						/* (other than local  */
						/* ring) and Source Routing */
						/* support not loaded.  */
						/* NB. If set all other */
						/* bits should be reset. */
#define	ECB_UNICASTDIRECT	0x00000080	/* Unicast for this */
						/* workstation */
#define	ECB_MASK		0x000000FF	/* mask off allowable */
						/* Destination Address Types */
#define	ECB_TYPE_I		0x00000100	/* Set if packet is 802.2 */
						/* Type I */
#define	ECB_TYPE_II		0x00000200	/* Set if packet is 802.2 */
						/* Type II */

/*
 * PromiscuousChange state and mode values.
 */
#define PROM_STATE_OFF		0x00		/* Disable Promiscuous Mode */
#define PROM_STATE_ON 		0x01		/* Enable Promiscuous Mode */
#define PROM_MODE_QUERY		0x00		/* Query as to prom mode */
#define PROM_MODE_MAC		0x01		/* MAC frames */
#define PROM_MODE_NON_MAC	0x02		/* Non-MAC frames */
#define PROM_MODE_MACANDNON	0x03		/* Both MAC and Non-MAC */
						/* frames */
/*
 * system return code definitions
 */
typedef enum _ODISTAT_ {
	ODISTAT_SUCCESSFUL		= 0,
	ODISTAT_RESPONSE_DELAYED	= 1,
	ODISTAT_SUCCESS_TAKEN		= 2,
	ODISTAT_BAD_COMMAND		= -127,
	ODISTAT_BAD_PARAMETER		= -126,
	ODISTAT_DUPLICATE_ENTRY		= -125,
	ODISTAT_FAIL			= -124,
	ODISTAT_ITEM_NOT_PRESENT	= -123,
	ODISTAT_NO_MORE_ITEMS		= -122,
	ODISTAT_NO_SUCH_DRIVER		= -121,
	ODISTAT_NO_SUCH_HANDLER		= -120,
	ODISTAT_OUT_OF_RESOURCES	= -119,
	ODISTAT_RX_OVERFLOW		= -118,
	ODISTAT_IN_CRITICAL_SECTION	= -117,
	ODISTAT_TRANSMIT_FAILED		= -116,
	ODISTAT_PACKET_UNDELIVERABLE	= -115,
	ODISTAT_CANCELED		= -4
} ODISTAT;

/*
 * MLID Configuration Table Bit Defintions
 */

/*
 * MLID 'Flags' Bit Definitions
 */
#define MF_EISA_BIT		0x0001
#define MF_ISA_BIT		0x0002
#define MF_MCA_BIT		0x0004
#define MF_HUB_MANAGEMENT_BIT	0x0100
#define MF_SOFT_FILT_GRP_BIT	0x0200
#define MF_GRP_ADDR_SUP_BIT	0x0400
#define MF_MULTICAST_TYPE_BITS	0x0600

/*
 * MLID 'ModeFlags' Bit Definitions.
 */
#define MM_REAL_DRV_BIT		0x0001
#define MM_USES_DMA_BIT		0x0002
#define MM_DEPENDABLE_BIT	0x0004		/* should only be set if */
						/* MM_POINT_TO_POINT_BIT */
						/* set, for hardware that is */
						/* normally dependable but */
						/* is not 100% guaranteed */
#define MM_MULTICAST_BIT	0x0008
#define MM_POINT_TO_POINT_BIT	0x0010		/* set if point-to-point */
						/* link, dynamic call setup */
						/* and tear down, eg. X.25  */
#define MM_PREFILLED_ECB_BIT	0x0020		/* MLID supplies pre-filled */
						/* ECBs */
#define MM_RAW_SENDS_BIT	0x0040
#define MM_DATA_SZ_UNKNOWN_BIT	0x0080
#define MM_FRAG_RECEIVES_BIT	0x0400		/* MLID can handle 8/
						/* Fragmented Receive ECB. */
#define MM_C_HSM_BIT		0x0800		/* set if HSM written in C. */
#define MM_FRAGS_PHYS_BIT	0x1000		/* set if HSM wants Frags */
						/* with Physical Addresses. */
#define MM_PROMISCUOUS_BIT	0x2000		/* set if supports */
						/* Promiscuous Mode. */
#define MM_NONCANONICAL_BIT	0x4000		/* set if Config Node */
						/* Address Non-Canonical */
#define MM_PHYS_NODE_ADDR_BIT	0x8000		/* set if MLID utilizes */
						/* Physical Node Address. */
#define MM_CANONICAL_BITS	0xC000

/*
 * MLID 'SharingFlags' Bit Defintions.
 */
#define MS_SHUTDOWN_BIT		0x0001
#define MS_SHARE_PORT0_BIT	0x0002
#define MS_SHARE_PORT1_BIT	0x0004
#define MS_SHARE_MEMORY0_BIT	0x0008
#define MS_SHARE_MEMORY1_BIT	0x0010
#define MS_SHARE_IRQ0_BIT	0x0020
#define MS_SHARE_IRQ1_BIT	0x0040
#define MS_SHARE_DMA0_BIT	0x0080
#define MS_SHARE_DMA1_BIT	0x0100
#define MS_HAS_CMD_INFO_BIT	0x0200
#define MS_NO_DEFAULT_INFO_BIT	0x0400

/*
 * MLID 'LineSpeed' Bit Definitions.
 */
#define MLS_MASK		0x7FFFF
#define MLS_KILO_IND_BIT	0x80000

/*
 * StatTableEntry Definitons.
 */
#define	ODI_STAT_UNUSED		0xFFFF	/* Statistics Table Entry not in use.*/
#define	ODI_STAT_UINT32		0x0000	/* Statistics Table Entry UINT32 */
					/* Counter */
#define	ODI_STAT_UINT64		0x0001	/* Statistics Table Entry UINT64 */
					/* Counter */
/*
 * following is TYPEDEF definitions for parameters used ANSI C ODI Interface.
 */
typedef	unsigned char	MEON;

/*
 * definition for MEON Strings, NB. MEON_STRING is really used as mnemonic.
 * by convention MEON_STRINGS are NULL terminated.
 */
typedef	unsigned char	MEON_STRING;
typedef	unsigned char	UINT8;
typedef	unsigned short	UINT16;
typedef	unsigned int	UINT32;

typedef	struct	_UINT64_ {
	UINT32	Low_UINT32;
	UINT32	High_UINT32;
} UINT64;

typedef	void		VOID;
typedef MEON		UNICODE_STRING;
typedef MEON		*PUNICODE_STRING;
typedef MEON		*PWSTR;

/*
 * declare the pointer for the ODI definitions
 */
typedef	MEON		*PMEON;
typedef	UINT8		*PUINT8;
typedef	UINT16		*PUINT16;
typedef	UINT32		*PUINT32;
typedef	UINT64		*PUINT64;
typedef	VOID		*PVOID;

typedef struct _PROT_ID_ {
        UINT8   protocolID[6];
} PROT_ID;

typedef struct  _NODE_ADDR_ {
        UINT8   nodeAddress[6];
} NODE_ADDR;

/*
 * Set PRAGMA to pack these structures
 */
#pragma	pack(1)

typedef	struct _StatTableEntry_	{
	UINT32		StatUseFlag;	/* ODI_STAT_UNUSED Statistics Table */
					/* Entry not in use, OR */
					/* ODI_STAT_UINT32 *StatConter is a */
					/* pointer to an UINT32 Counter, OR */
					/* ODI_STAT_UINT32 *StatConter is a */
					/* a pointer to an UINT64 Counter */
	VOID		*StatCounter;	/* pointer to a UINT32 or UINT64 */
					/* counter. */
	MEON_STRING	*StatString;	/* pointer to a MEON String, */
					/* describing the statistics counter */
} StatTableEntry, *PStatTableEntry;

/*
 * definitions for Information Block for passing API's, eg. Function Lists
 */
typedef	struct _INFO_BLOCK_ {
	UINT32	NumberOfAPIs;
	VOID	(**SupportAPIArray)();
} INFO_BLOCK, *PINFO_BLOCK;

struct	_ECB_;

/*
 * definitions for LookAhead and Event Control Blocks (ECB).
 */
typedef	struct	_AESECB_ {
	struct	_AESECB_	*AES_Link;
	UINT32			AES_MSecondValue;
	UINT16			AES_Status;
	VOID			(*AES_ESR)(struct _ECB_ *);
	VOID			*AES_ResourceObj;
	VOID			*AES_Context;
} AESECB, *PAESECB;

typedef	struct	_FRAGMENTSTRUCT_ {
	VOID			*FragmentAddress;
	UINT32			FragmentLength;
} FRAGMENTSTRUCT, *PFRAGMENTSTRUCT;

struct	msgb;
struct  lslsap;

typedef struct _manage_ecb_ {
	struct	_ECB_		*manage_nextlink;
	struct	_ECB_		*manage_prevlink;
	struct	msgb		*manage_pmsgb;
	VOID			(* manage_esr)(struct _ECB_ *);
	struct	lslsap		*manage_sap;
} manage_ecb;

typedef	struct _ECB_	{
	UINT8			ECB_DriverWS[8];
	UINT16			ECB_Status;
	UINT8			ECB_filler1[4];
	UINT16			ECB_StackID;
	PROT_ID			ECB_ProtocolID;
	UINT32			ECB_BoardNumber;
	NODE_ADDR		ECB_ImmediateAddress;
	union {
		UINT8		DWs_i8val[4];
		UINT16		DWs_i16val[2];
		UINT32		DWs_i32val;
		VOID		*DWs_pval;
	} ECB_DriverWorkspace;
	struct	_manage_ecb_	*ECB_management;
	UINT8			ECB_filler2[4];
	UINT32			ECB_DataLength;
	UINT32			ECB_FragmentCount;
	FRAGMENTSTRUCT		ECB_Fragment[1];
} ECB, *PECB;

#define	ECB_NextLink		ECB_management->manage_nextlink
#define	ECB_PreviousLink	ECB_management->manage_prevlink
#define	ECB_mblk		ECB_management->manage_pmsgb
#define	ECB_ESR			ECB_management->manage_esr
#define ECB_sap                 ECB_management->manage_sap

#define MAX_ECB_FRAGS		16
#define	ECB_TAIL		12
#define RAW_SEND 		0xffff		/* if in ECB.ECB_StackID */
						/* raw send */
typedef	struct	_LOOKAHEAD_ {
	ECB		*LkAhd_PreFilledECB;
	UINT8		*LkAhd_MediaHeaderPtr;
	UINT32		LkAhd_MediaHeaderLen;
	UINT8		*LkAhd_DataLookAheadPtr;
	UINT32		LkAhd_DataLookAheadLen;
	UINT32		LkAhd_BoardNumber;
	UINT32		LkAhd_PktAttr;	/* now Packet Attributes instead */
					/* of ErrorStatus */
	UINT32		LkAhd_DestType;

	UINT32		LkAhd_FrameDataSize;
	UINT16		LkAhd_PadAlignBytes1;
	PROT_ID		LkAhd_ProtocolID;
	UINT16		LkAhd_PadAlignBytes2;
	NODE_ADDR	LkAhd_ImmediateAddress;
	UINT32		LkAhd_FrameDataStartCopyOffset;
	UINT32		LkAhd_FrameDataBytesWanted;
	ECB		*LkAhd_ReturnedECB;
	PVOID		LkAhd_Reserved0;
	PVOID		LkAhd_Reserved1;
} LOOKAHEAD, *PLOOKAHEAD;

/*
 * definitions for MLID Configuration, statistics tables and
 * misc. structures.
 */
typedef	struct _MLID_ConfigTable_ {
	MEON		MLIDCFG_Signature[26];
	UINT8		MLIDCFG_MajorVersion;
	UINT8		MLIDCFG_MinorVersion;
	NODE_ADDR	MLIDCFG_NodeAddress;
	UINT16		MLIDCFG_ModeFlags;
	UINT16		MLIDCFG_BoardNumber;
	UINT16		MLIDCFG_BoardInstance;
	UINT32		MLIDCFG_MaxFrameSize;
	UINT32		MLIDCFG_BestDataSize;
	UINT32		MLIDCFG_WorstDataSize;
	MEON_STRING	*MLIDCFG_CardName;
	MEON_STRING	*MLIDCFG_ShortName;
	MEON_STRING	*MLIDCFG_FrameTypeString;
	UINT16		MLIDCFG_Reserved0;
	UINT16		MLIDCFG_FrameID;
	UINT16		MLIDCFG_TransportTime;
	UINT32		(*MLIDCFG_SourceRouting)(UINT32, VOID*, VOID**,
				BOOLEAN);
	UINT16		MLIDCFG_LineSpeed;
	UINT16		MLIDCFG_LookAheadSize;
	UINT8		MLIDCFG_Reserved1[2];
	UINT16		MLIDCFG_PrioritySup;
	UINT32		MLIDCFG_BusTag;
	UINT8		MLIDCFG_DriverMajorVer;
	UINT8		MLIDCFG_DriverMinorVer;
	UINT16		MLIDCFG_Flags;
	UINT16		MLIDCFG_SendRetries;
	VOID		*MLIDCFG_DriverLink;
	UINT16		MLIDCFG_SharingFlags;
	UINT16		MLIDCFG_Slot;
	UINT16		MLIDCFG_IOPort0;
	UINT16		MLIDCFG_IORange0;
	UINT16		MLIDCFG_IOPort1;
	UINT16		MLIDCFG_IORange1;
	VOID		*MLIDCFG_MemoryAddress0;
	UINT16		MLIDCFG_MemorySize0;
	VOID		*MLIDCFG_MemoryAddress1;
	UINT16		MLIDCFG_MemorySize1;
	UINT8		MLIDCFG_Interrupt0;
	UINT8		MLIDCFG_Interrupt1;
	UINT8		MLIDCFG_DMALine0;
	UINT8		MLIDCFG_DMALine1;
	VOID		*MLIDCFG_ResourceTag;
	VOID		*MLIDCFG_Config;
	VOID		*MLIDCFG_CommandString;
	MEON_STRING	MLIDCFG_LogicalName[18];
	void		*MLIDCFG_LinearMemory0;
	void		*MLIDCFG_LinearMemory1;
	UINT16		MLIDChannelNumber;
	VOID		*MLIDIOReserved[2];
} MLID_ConfigTable, *PMLID_ConfigTable;

typedef struct _IO_CONFIG_ {
	struct _IO_CONFIG_		*IO_DriverLink;
	UINT16				IO_SharingFlags;
	UINT16				IO_Slot;
	UINT16				IO_IOPort0;
	UINT16				IO_IORange0;
	UINT16				IO_IOPort1;
	UINT16				IO_IORange1;
	void				*IO_MemoryAddress0;
	UINT16				IO_MemorySize0;
	void				*IO_MemoryAddress1;
	UINT16				IO_MemorySize1;
	UINT8				IO_Interrupt0;
	UINT8				IO_Interrupt1;
	UINT8				IO_DMALine0;
	UINT8				IO_DMALine1;
	struct ResourceTagStructure	*IO_ResourceTag;
	void				*IO_Config;
	void				*IO_CommandString;
	MEON_STRING			IO_LogicalName[18];
	UINT32				IO_LinearMemory0;
	UINT32				IO_LinearMemory1;
	UINT16				IO_ChannelNumber;
	void				*IO_IOReserved[2];
} IO_CONFIG;

typedef	struct _MLID_StatsTable_ {
	UINT16		MStatTableMajorVer;
	UINT16		MStatTableMinorVer;
	UINT32		MNumGenericCounters;
#ifdef	NT
	StatTableEntry	(*MGenericCountsPtr)[];
	UINT32		MNumMediaCounters;
	StatTableEntry	(*MMediaCountsPtr)[];
	UINT32		MNumCustomCounters;
	StatTableEntry	(*MCustomCountersPtr)[];
#else
	StatTableEntry	*MGenericCountsPtr;
	UINT32		MNumMediaCounters;
	StatTableEntry	*MMediaCountsPtr;
	UINT32		MNumCustomCounters;
	StatTableEntry	*MCustomCountersPtr;
#endif
} MLID_StatsTable, *PMLID_StatsTable;

#define TOTAL_TX_PACKET_COUNT                   0
#define TOTAL_RX_PACKET_COUNT                   1
#define NO_ECB_AVAILABLE_COUNT                  2
#define PACKET_TX_TOO_BIG_COUNT                 3
#define PACKET_TX_TOO_SMALL_COUNT               4
#define PACKET_RX_OVERFLOW_COUNT                5
#define PACKET_RX_TOO_BIG_COUNT                 6
#define PACKET_RX_TOO_SMALL_COUNT               7
#define PACKET_TX_MISC_ERROR_COUNT              8
#define PACKET_RX_MISC_ERROR_COUNT              9
#define RETRY_TX_COUNT				10
#define CHECKSUM_ERROR_COUNT			11
#define HARDWARE_RX_MISMATCH_COUNT              12
#define TOTAL_TX_OK_BYTE_COUNT                  13
#define TOTAL_RX_OK_BYTE_COUNT                  14
#define TOTAL_GROUP_ADDR_TX_COUNT               15
#define TOTAL_GROUP_ADDR_RX_COUNT               16
#define ADAPTER_RESET_COUNT			17
#define ADAPTER_OPR_TIME_STAMP                  18
#define Q_DEPTH					19

typedef	struct _MLID_Reg_ {
	VOID		(*MLIDSendHandler)(ECB*);
	INFO_BLOCK	*MLIDControlHandler;
} MLID_Reg, *PMLID_Reg;

/*
 * reset PRAGMA to normal after packing above structures.
 */
#pragma	pack()

#define MSM_IOCTL_GetMLIDConfiguration          0
#define MSM_IOCTL_GetMLIDStatistics             1
#define MSM_IOCTL_AddMulticastAddress           2
#define MSM_IOCTL_DeleteMulticastAddress        3
#define MSM_IOCTL_Reserved0                     4
#define MSM_IOCTL_MLIDShutdown                  5
#define MSM_IOCTL_MLIDReset                     6
#define MSM_IOCTL_Reserved1                     7
#define MSM_IOCTL_Reserved2                     8
#define MSM_IOCTL_SetLookAheadSize              9
#define MSM_IOCTL_PromiscuousChange             10
#define MSM_IOCTL_RegisterMonitor       	11
#define MSM_IOCTL_Reserved3                     12
#define MSM_IOCTL_Reserved4                     13
#define MSM_IOCTL_MLIDManagement                14

#endif
