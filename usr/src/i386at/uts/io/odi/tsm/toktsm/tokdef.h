/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_ODI_TSM_TOKDEF_H
#define _IO_ODI_TSM_TOKDEF_H

#ident	"@(#)kern-i386at:io/odi/tsm/toktsm/tokdef.h	1.5"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#include <io/odi/msm/cmsm.h>
#include <io/odi/msm/msmstruc.h>

#elif defined(_KERNEL)

#include <sys/cmsm.h>
#include <sys/msmstruc.h>

#endif /* _KERNEL_HEADERS */

/*
 * These are used for indexing into a specific Frame-Type.
 */
#define  TOKEN_INTERNALID  0
#define  TSNAP_INTERNALID  1

/*
 * Brouter Request Definitions.
 */
#define  BROUTER_SUPPORT               0x00
#define  SELECT_T_BRIDGING             0x01
#define  SELECT_SR_BRIDGING            0x02
#define  SELECT_PROMISCUOUS_MODE       0x03
#define  UPDATE_ADDRESS_FILTER_TABLE   0x04

#define  PMT_NONE                      0
#define  PMT_TRANSPARENT               1
#define  PMT_SOURCE_ROUTINE            2
#define  PMT_TSR_TRANSPARENT           3

#define  FTA_CLEAR_TABLE               0
#define  FTA_ADD_ADDRESSES             1
#define  FTA_DELETE_ADDRESSES          2

/*
 * These are the NOVELL assigned Frame IDs.
 */
#define	TOKEN_ID    			4
#define	TSNAP_ID     			11

#define	SIZE_TOKEN			14
#define	SIZE_TOKENSNAP			17

/*
 * This is the Maximum size a packet can be.
 */
#define MAX_PACKET_SIZE			17960

#define MULTICASTBIT			0x80
#define GROUPBIT			0x80
#define LOCALBIT			0x40
#define BROADCAST			0xffffffff

/*
 * These bits define 802.2 types.
 */
#define S_OR_U_FORMAT			0x01
#define U_FORMAT			0x02

#define FC_NON_MAC_FRAME		0x40
#define SOURCE_ROUTING_BIT		0x80
#define SOURCE_SIZE_MASK		0x1f
#define TOKENSNAP_INFO			0x03AAAA
#define SNAP_INFO_SIZE			5
#define SOURCE_MAX_SIZE			30

/*
 * This is the Maximum size a header can be.
 */
#define MAX_MAC_HEADER			sizeof (MEDIA_HEADER) + SOURCE_MAX_SIZE

/*
 * v3.00 Assembly Statistics Table.
 */
#define STATISTICSMASK			0x0D380000
#define TOKENSTATSCOUNT			15
#define LONGCOUNTER			0x00
#define LARGECOUNTER			0x01

#define NUMBER_OF_PROMISCUOUS_COUNTERS	32
#define MAX_MULTICAST			32

/*
 * Brouter Request Type Definitions.
 */

typedef struct	_HUB_ECB_ {
	struct _HUB_ECB_	*HECB_NextLink;
	struct _HUB_ECB_	*HECB_PreviousLink;
	UINT16               	HECB_Status;
	void                 	(*HECB_ESR)(struct _HUB_ECB_ *);
	UINT16               	HECB_StackID;
	UINT8                	HECB_ProtocolID [6];
	UINT16               	HECB_BrouterRequestCode;
} HUB_ECB;

typedef struct	_HUB_BR_ECB_ {
	HUB_ECB     		BR_ECB;
	void			*BR_BrouterSupportStatus;
	UINT8       		BR_FilterTableSize;
} HUB_BR_ECB;

typedef struct	_HUB_TB_ECB_ {
	HUB_ECB     		TB_ECB;
	void        		*TB_ServiceHandler;
} HUB_TB_ECB;

typedef struct	_HUB_SR_ECB_ {
	HUB_ECB     		SR_ECB;
	void        		*SR_ServiceHandler;
	UINT8       		SR_PartitionSize;
	UINT8       		SR_FilterSTE;
	UINT16      		SR_MaximumRDSize;
	UINT16      		SR_RingInNumber;
	UINT16      		SR_RingOutNumber;
	UINT16      		SR_BridgeNumber;
} HUB_SR_ECB;

typedef struct	_HUB_PM_ECB_ {
	HUB_ECB     		PM_ECB;
	void        		*PM_PromiscuousModeType;
} HUB_PM_ECB;

typedef struct	_HUB_UA_ECB_
{
	HUB_ECB     		UA_ECB;
	UINT8       		UA_FilterTableAction;
	UINT8       		UA_FilterTableAddresses;
	UINT8       		UA_FilterTableAddress1;
} HUB_UA_ECB;

typedef struct {
	UINT8			*MinNodeAddress;
	UINT8			*MaxNodeAddress;
	UINT32			MinRetries;
	UINT32			MaxRetries;
	UINT32			NumberFrames;
} LANConfigurationLimitStructure;

typedef struct	_MEDIA_HEADER_ {
	UINT8		   	MH_AccessControl;
	UINT8		   	MH_FrameControl;
	NODE_ADDR   	   	MH_Destination;
	NODE_ADDR	   	MH_Source;
	UINT8		   	MH_DSAP;
	UINT8	   	   	MH_SSAP;
	UINT8		   	MH_Ctrl0;
	UINT8		   	MH_SNAP [5];
} MEDIA_HEADER;

typedef	struct	_TCB_FRAGMENTSTRUCT_ {
	UINT32			TCB_FragmentCount;
	FRAGMENTSTRUCT		TCB_Fragment;
} TCB_FRAGMENTSTRUCT;

typedef	struct _TCB_ {
	UINT32			TCB_DriverWS[3];
	UINT32			TCB_DataLen;
	TCB_FRAGMENTSTRUCT	*TCB_FragStruct;
	UINT32			TCB_MediaHeaderLen;
	MEDIA_HEADER		TCB_MediaHeader;
} TCB , *PTCB;

typedef	struct _TCBMORE_ {
	UINT32			TCB_DriverWS[3];
	UINT32			TCB_DataLen;
	TCB_FRAGMENTSTRUCT	*TCB_FragStruct;
	UINT32			TCB_MediaHeaderLen;
	MEDIA_HEADER		TCB_MediaHeader;
	UINT8			SOURCEROUTE[30];
} TCBMORE , *PTCBMORE;

typedef	struct	_FUNCTIONAL_TABLE_ {
	UINT8			FunctionalBit[32];
} FUNCTIONAL_TABLE, *PFUNCTIONAL_TABLE;

typedef struct	_MEDIA_DATA_	{
	UINT32			MaxMulticastAddresses;
	MULTICAST_TABLE		*MulticastAddressList;
	UINT32			MulticastAddressCount;
	FUNCTIONAL_TABLE    	*FunctionalAddressList;
	TCB			*TCBHead;
	TCB			*TCBList;
	MLID_StatsTable		*NewStatsPtr;
	void			(*TransmitRoutine)(TCB *);
	UINT32			PromiscuousMode;
	UINT8			PromiscuousCounters
					[NUMBER_OF_PROMISCUOUS_COUNTERS];
	UINT32			RxStatus;
} MEDIA_DATA;

typedef struct	_M_ADAPTER_DS_	{
	UINT32			MADS_MulticastCount;
} M_ADAPTER_DS;

ODISTAT		CTokenTSMRegisterCHSM(DRIVER_PARM_BLOCK *DriverParameterBlock,
			CONFIG_TABLE **configTable);
MULTICAST_TABLE	*FindAddressInMCTable(SHARED_DATA *sharedData,
			UINT8 *mcAddress);
ODISTAT		MediaAdjust(FRAME_DATA *frameData);
ODISTAT		MediaInit(const DRIVER_DATA *driverData, FRAME_DATA
			*frameData);
ODISTAT		MediaReset(DRIVER_DATA *driverData, FRAME_DATA *frameData);
ODISTAT		MediaShutdown(DRIVER_DATA *driverData, FRAME_DATA *frameData,
			UINT32 shutdownType);
void	   	MediaSend(ECB *ecb, CONFIG_TABLE *configTable);
ODISTAT		MediaAddMulticast(DRIVER_DATA *driverData, UINT8 *McAddress);
ODISTAT		MediaDeleteMulticast(DRIVER_DATA *driverData,
			UINT8 *McAddress);
ODISTAT		MediaNodeOverride(FRAME_DATA *frameData, MEON mode);
ODISTAT		MediaAdjustNodeAddress(FRAME_DATA *frameData);
ODISTAT		MediaSetLookAheadSize(DRIVER_DATA *driverData, FRAME_DATA
			*frameData, UINT32 size);
ODISTAT		MediaPromiscuousChange(DRIVER_DATA *driverData, FRAME_DATA
			*frameData, UINT32 PromiscuousState, UINT32
			*PromiscuousMode);
ODISTAT		MediaRegisterMonitor(const DRIVER_DATA *driverData,
			FRAME_DATA *frameData, UINT32 (*TXRMonRoutine)(TCB *),
			BOOLEAN MonitorState);
void     	TokenBuildASMStatStrings(StatTableEntry *tableEntry);
UINT32		CTokenTSMUpdateMulticast(DRIVER_DATA *driverData);
UINT32   	CMediaSend8022(SHARED_DATA *sharedData, ECB *ecb, TCB *tcb);
UINT32   	CMediaSendSNAP(SHARED_DATA *sharedData, ECB *ecb, TCB *tcb);
void		*CTokenTSMGetNextSend(DRIVER_DATA *driverData, CONFIG_TABLE
			**configTable, UINT32 *lengthToSend, void
			**TCBPhysicalPtr);
void		CTokenTSMFastSendComplete(DRIVER_DATA *driverData, TCB *tcb);
void		CTokenTSMSendComplete(DRIVER_DATA *driverData, TCB *tcb);
RCB 		*CTokenTSMProcessGetRCB(void *driverData, RCB *rcb, UINT32
			pktSize, UINT32	rcvStatus, UINT32 newRcbSize);
RCB 		*CTokenTSMFastProcessGetRCB(void *driverData, RCB *rcb,
			UINT32 pktSize, UINT32 rcvStatus, UINT32 newRcbSize);
RCB		*CTokenTSMGetRCB(void *driverData, MEDIA_HEADER *lookAheadData,
			UINT32 pktSize, UINT32 rcvStatus, UINT32 *startByte,
			UINT32 *numBytes);
void		CTokenTSMFastRcvComplete(void *driverData, ECB *ecb);
void		CTokenTSMFastRcvCompleteStatus(void *driverData, ECB *ecb,
			UINT32 packetLength, UINT32 packetStatus);
UINT32		CTokenTSMGetHSMIFLevel();
void		CTokenTSMRcvComplete(void *driverData, ECB *ecb);
void		CTokenTSMRcvCompleteStatus(void	*driverData, ECB *ecb,
			UINT32 packetLength, UINT32 packetStatus);

#endif /* _IO_ODI_TSM_TOKDEF_H */
