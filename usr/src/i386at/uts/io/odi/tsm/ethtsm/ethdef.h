/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_TSM_ETHTSM_ETHDEF_H
#define _IO_TSM_ETHTSM_ETHDEF_H

#ident	"@(#)kern-i386at:io/odi/tsm/ethtsm/ethdef.h	1.2"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#include <io/odi/msm/cmsm.h>

#elif defined(_KERNEL)

#include <sys/cmsm.h>

#endif /* _KERNEL_HEADERS */

/*
 * This defines the size of each Frame-Types MAC Header.
 */
#define	SIZE_E8023			14
#define	SIZE_E8022			17
#define	SIZE_EII			14
#define	SIZE_ESNAP			22

/*
 * These are used for indexing into a specific Frame-Type.
 */
#define E8022_INTERNALID		0
#define E8023_INTERNALID		1
#define EII_INTERNALID			2
#define ESNAP_INTERNALID		3

/*
 * These are the NOVELL assigned Frame IDs.
 */
#define	E8023_ID			5
#define	EII_ID				2
#define	E8022_ID			3
#define	ESNAP_ID			10

/*
 * This is the Maximum size a packet can be.
 */
#define MAX8023LENGTH			1500
#define MAX_PACKET_LENGTH		1514

#define MULTICASTBIT			0x01
#define GROUPBIT			0x01
#define LOCALBIT			0x02

#define MIN_PKT_SIZE			60

/*
 * These bits define 802.2 types.
 */
#define S_OR_U_FORMAT			0x01
#define U_FORMAT			0x02

typedef struct	_MEDIA_HEADER_ {
	UINT8		MH_Destination[ADDR_SIZE];
	UINT8		MH_Source[ADDR_SIZE];
	UINT8		MH_Length[2];
	UINT8		MH_DSAP;
	UINT8		MH_SSAP;
	UINT8		MH_Ctrl0;
	UINT8		MH_SNAP[5];
} MEDIA_HEADER;

typedef	struct	_TCB_FRAGMENTSTRUCT_ {
	UINT32		TCB_FragmentCount;
	FRAGMENTSTRUCT	TCB_Fragment;
} TCB_FRAGMENTSTRUCT;

typedef	struct _TCB_ {
	UINT32			TCB_DriverWS[3];
	UINT32			TCB_DataLen;
	TCB_FRAGMENTSTRUCT	*TCB_FragStruct;
	UINT32			TCB_MediaHeaderLen;
	MEDIA_HEADER		TCB_MediaHeader;
} TCB;

/*
 * v3.00 Assembly Statistics Table.
 */
#define STATISTICSMASK			0x0D300003
#define ETHERNETSTATSCOUNT		8
#define	NUMBER_OF_PROMISCUOUS_COUNTERS	32
#define MAX_MULTICAST			32

typedef struct _MEDIA_DATA_ {
	UINT32			MaxMulticastAddresses;
	MULTICAST_TABLE		*MulticastAddressList;
	UINT32			MulticastAddressCount;
	TCB			*TCBHead;
	TCB			*TCBList;
	MLID_StatsTable		*NewStatsPtr;
	void			(*TransmitRoutine)(TCB *);
	UINT32			PromiscuousMode;
	UINT8		PromiscuousCounters[NUMBER_OF_PROMISCUOUS_COUNTERS];
	UINT32			RxStatus;
} MEDIA_DATA;

ODISTAT		CEtherTSMRegisterCHSM(DRIVER_PARM_BLOCK *DriverParameterBlock,
			CONFIG_TABLE **configTable);
RCB 		*CEtherTSMFastProcessGetRCB(DRIVER_DATA *driverData, RCB *rcb,
			UINT32 pktSize, UINT32 rcvStatus, UINT32 newRcbSize);
void		CEtherTSMFastSendComplete(DRIVER_DATA *driverData, TCB *tcb);
void		*CEtherTSMGetNextSend(DRIVER_DATA *driverData, CONFIG_TABLE
			**configTable, UINT32 *lengthToSend,
			void **TCBPhysicalPtr);
RCB		*CEtherTSMGetRCB(DRIVER_DATA *driverData, MEDIA_HEADER
			*lookAheadData, UINT32 pktSize, UINT32 rcvStatus,
			UINT32 *startByte, UINT32 *numBytes);
RCB 		*CEtherTSMProcessGetRCB(DRIVER_DATA *driverData, RCB *rcb,
			UINT32 pktSize, UINT32 rcvStatus, UINT32 newRcbSize);
void		CEtherTSMFastRcvComplete(DRIVER_DATA *driverData, ECB *ecb);
void		CEtherTSMFastRcvCompleteStatus(DRIVER_DATA *driverData, RCB
			*rcb, UINT32 packetLength, UINT32 packetStatus,
			UINT32 startByte, UINT32 numBytes);
UINT32		CEtherTSMGetHSMIFLevel();
void		CEtherTSMRcvComplete(DRIVER_DATA *driverData, ECB *ecb);
void		CEtherTSMRcvCompleteStatus(DRIVER_DATA *driverData, RCB *rcb,
			UINT32 packetLength, UINT32 packetStatus, UINT32
			startByte, UINT32 numBytes);
void		CEtherTSMSendComplete(DRIVER_DATA *driverData, TCB *tcb);
UINT32		CEtherTSMUpdateMulticast(DRIVER_DATA *driverData);
UINT32 		CMediaSendRaw8023(SHARED_DATA *, ECB *ecb, TCB *tcb);
UINT32 		CMediaSend8022Over8023(SHARED_DATA *, ECB *ecb, TCB *tcb);
UINT32 		CMediaSend8022Snap(SHARED_DATA *, ECB *ecb, TCB *tcb);
UINT32 		CMediaSendEthernetII(SHARED_DATA *, ECB *ecb, TCB *tcb);
MULTICAST_TABLE *FindAddressInMCTable(SHARED_DATA *sharedData,
			UINT8 *mcAddress);
ODISTAT		MediaAdjust(FRAME_DATA *frameData);
ODISTAT		MediaInit(const DRIVER_DATA *driverData, FRAME_DATA *frameData);
ODISTAT		MediaReset(DRIVER_DATA *driverData, FRAME_DATA *frameData);
ODISTAT		MediaShutdown(DRIVER_DATA *driverData, FRAME_DATA *frameData,
			UINT32 shutdownType);
void		MediaSend(ECB *ecb, CONFIG_TABLE *configTable);
ODISTAT		MediaAddMulticast(DRIVER_DATA *driverData, UINT8 *McAddress);
ODISTAT		MediaDeleteMulticast(DRIVER_DATA *driverData, UINT8 *McAddress);
ODISTAT		MediaNodeOverride(FRAME_DATA *frameData, MEON mode);
ODISTAT		MediaAdjustNodeAddress(FRAME_DATA *frameData);
ODISTAT		MediaSetLookAheadSize(DRIVER_DATA *driverData, FRAME_DATA
			*frameData, UINT32 size);
ODISTAT		MediaPromiscuousChange(DRIVER_DATA *driverData, FRAME_DATA
			*frameData, UINT32 PromiscuousState, UINT32
			*PromiscuousMode);
ODISTAT		MediaRegisterMonitor(const DRIVER_DATA *driverData, FRAME_DATA
			*frameData, UINT32 (*TXRMonRoutine)(TCB *), BOOLEAN
			MonitorState);
void		EtherBuildASMStatStrings(StatTableEntry *tableEntry);

#endif	/* _IO_TSM_ETHTSM_ETHDEF_H */
