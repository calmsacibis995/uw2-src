/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_MSM_CMSM_H   /* wrapper symbol for kernel use */
#define _IO_MSM_CMSM_H   /* subject to change without notice */

#ident	"@(#)kern-i386at:io/odi/msm/cmsm.h	1.6"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#include <io/odi/msm/odi.h>
#include <io/odi/msm/odi_nsi.h>
#include <io/odi/msm/odi_portable.h>
#include <util/ksynch.h>

#elif defined(_KERNEL)

#include <sys/odi.h>
#include <sys/odi_nsi.h>
#include <sys/odi_portable.h>
#include <sys/ksynch.h>

#endif /* _KERNEL_HEADERS */

struct	_DRIVER_PARM_BLOCK_;
struct	_TSM_PARM_BLOCK_;
struct	_GENERIC_STATS_;
struct	_MEDIA_DATA_;
struct	_MSM_PROTECTION_OBJ_;

typedef MLID_ConfigTable        CONFIG_TABLE;

#ifndef	ALL_ONES
#define	ALL_ONES	-1
#endif

#ifndef	DRIVER_DATA
struct	_DRIVER_DATA_;
#endif

/*
 * portable alias definitions.
 */

/*
 * BEGIN TODO for portability interfaces work (PIN).
 * the following 4 will be replaced.
 */
#define	Mov8(destBusTag, destAddrPtr, srcBusTag, srcAddrPtr, count) {	\
	*((PUINT8)destAddrPtr) = *((PUINT8)srcAddrPtr);			\
}

#define	Mov32(destBusTag, destAddrPtr, srcBusTag, srcAddrPtr, count) {	\
	*((PUINT32)destAddrPtr) = *((PUINT32)srcAddrPtr);		\
}

#define	Set8(busTag, memAddrPtr, value, count) {			\
	int	cnt = count;						\
	while(cnt--) {							\
		*((PUINT8)memAddrPtr) = ((UINT8)value);			\
	}								\
}

#define	Set32(busTag, memAddrPtr, value, count) {			\
	int	cnt = count;						\
	while(cnt--) {							\
		*((PUINT32)memAddrPtr) = ((UINT32)value);		\
	}								\
}

/*
 * END TODO for PIN
 */

#define	COPY_UINT16(dest_addr, src_addr)	\
					COPY_WORD(src_addr, dest_addr)
#define	COPY_UINT32(dest_addr, src_addr)	\
					COPY_LONG(src_addr, dest_addr)
#define	COPY_ADDR(dest_addr, src_addr)	COPY_NODE(src_addr, dest_addr)

#define	GET_UINT16(addr)		GET_WORD(addr)
#define	GET_UINT32(addr)		GET_LONG(addr)
#define	PUT_UINT16(addr, value)		PUT_WORD(value, addr)
#define	PUT_UINT32(addr, value)		PUT_LONG(value, addr)

#define	GET_HILO_UINT16(addr)		GET_HILO_WORD(addr)
#define	GET_HILO_UINT32(addr)		GET_HILO_LONG(addr)
#define	GET_LOHI_UINT16(addr)		GET_LOHI_WORD(addr)
#define	GET_LOHI_UINT32(addr)		GET_LOHI_LONG(addr)

#define	PUT_HILO_UINT16(addr, value)	PUT_HILO_WORD(value, addr)
#define	PUT_HILO_UINT32(addr, value)	PUT_HILO_LONG(value, addr)
#define	PUT_LOHI_UINT16(addr, value)	PUT_LOHI_WORD(value, addr)
#define	PUT_LOHI_UINT32(addr, value)	PUT_LOHI_LONG(value, addr)

#define	VALUE_TO_HILO_UINT16(addr)	VALUE_TO_HILO_WORD(addr)
#define	VALUE_TO_HILO_UINT32(addr)	VALUE_TO_HILO_LONG(addr)
#define	VALUE_TO_LOHI_UINT16(addr)	VALUE_TO_LOHI_WORD(addr)
#define	VALUE_TO_LOHI_UINT32(addr)	VALUE_TO_LOHI_LONG(addr)

#define	VALUE_FROM_HILO_UINT16(addr)	VALUE_FROM_HILO_WORD(addr)
#define	VALUE_FROM_HILO_UINT32(addr)	VALUE_FROM_HILO_LONG(addr)
#define	VALUE_FROM_LOHI_UINT16(addr)	VALUE_FROM_LOHI_WORD(addr)
#define	VALUE_FROM_LOHI_UINT32(addr)	VALUE_FROM_LOHI_LONG(addr)

#define	COPY_TO_HILO_UINT16(dest_addr, src_addr)	\
					COPY_TO_HILO_WORD(src_addr, dest_addr)
#define	COPY_TO_HILO_UINT32(dest_addr, src_addr)	\
					COPY_TO_HILO_LONG(src_addr, dest_addr)
#define	COPY_TO_LOHI_UINT16(dest_addr, src_addr)	\
					COPY_TO_LOHI_WORD(src_addr, dest_addr)
#define	COPY_TO_LOHI_UINT32(dest_addr, src_addr)	\
					COPY_TO_LOHI_LONG(src_addr, dest_addr)

#define	COPY_FROM_HILO_UINT16(dest_addr, src_addr)	\
					COPY_FROM_HILO_WORD(src_addr, dest_addr)
#define	COPY_FROM_HILO_UINT32(dest_addr, src_addr)	\
					COPY_FROM_HILO_LONG(src_addr, dest_addr)
#define	COPY_FROM_LOHI_UINT16(dest_addr, src_addr)	\
					COPY_FROM_LOHI_WORD(src_addr, dest_addr)
#define	COPY_FROM_LOHI_UINT32(dest_addr, src_addr)	\
					COPY_FROM_LOHI_LONG(src_addr, dest_addr)

#define	HOST_TO_HILO_UINT16(addr)	HOST_TO_HILO_WORD(addr)
#define	HOST_TO_HILO_UINT32(addr)	HOST_TO_HILO_LONG(addr)
#define	HOST_TO_LOHI_UINT16(addr)	HOST_TO_LOHI_WORD(addr)
#define	HOST_TO_LOHI_UINT32(addr)	HOST_TO_LOHI_LONG(addr)

#define	HOST_FROM_HILO_UINT16(addr)	HOST_FROM_HILO_WORD(addr)
#define	HOST_FROM_HILO_UINT32(addr)	HOST_FROM_HILO_LONG(addr)
#define	HOST_FROM_LOHI_UINT16(addr)	HOST_FROM_LOHI_WORD(addr)
#define	HOST_FROM_LOHI_UINT32(addr)	HOST_FROM_LOHI_LONG(addr)

#define	UINT16_EQUAL(addr1, addr2)	WORD_EQUAL(addr1, addr2)
#define	UINT32_EQUAL(addr1, addr2)	LONG_EQUAL(addr1, addr2)
#define	ADDR_EQUAL(addr1, addr2)	NODE_EQUAL(addr1, addr2)


#define	MSG(str, value)			str

typedef enum _AES_TYPE_ {
	AES_TYPE_PRIVILEGED_ONE_SHOT	= 0,
	AES_TYPE_PRIVILEGED_CONTINUOUS	= 1,
	AES_TYPE_PROCESS_ONE_SHOT	= 2,
	AES_TYPE_PROCESS_CONTINUOUS	= 3
} AES_TYPE;

typedef struct _MLID_AES_ECB_ {
	struct _MLID_AES_ECB_	*NextLink;
	void			(* _cdecl DriverAES)(struct _DRIVER_DATA_ *,
					CONFIG_TABLE *, ...);
	AES_TYPE		AesType;
	UINT32			TimeInterval;
	void			*AesContext;
	UINT8			AesReserved[30];
} MLID_AES_ECB;

typedef struct	_EXTRA_CONFIG_ISR_ {
	void			(_cdecl * ISRRoutine)
					(const struct _DRIVER_DATA_ *);
	void			*ISRReserved0;
	void			*ISRReserved1;
	void			*ISRReserved2;
	void			*ISRReserved3;
} EXTRA_CONFIG_ISR;

typedef struct _EXTRA_CONFIG_ {
	struct _EXTRA_CONFIG_	*NextLink;
	void			(_cdecl * ISRRoutine0)
					(const struct _DRIVER_DATA_ *);
	void			*ISR0Reserved0;
	void			*ISR0Reserved1;
	void			*ISR0Reserved2;
	void			*ISR0Reserved3;
	void			(_cdecl * ISRRoutine1)
					(const struct _DRIVER_DATA_ *);
	void			*ISR1Reserved0;
	void			*ISR1Reserved1;
	void			*ISR1Reserved2;
	void			*ISR1Reserved3;
	IO_CONFIG		IOConfig;
} EXTRA_CONFIG;

typedef enum _REG_TYPE_ {
	REG_TYPE_NEW_ADAPTER,
	REG_TYPE_NEW_FRAME,
	REG_TYPE_NEW_CHANNEL,
	REG_TYPE_FAIL
} REG_TYPE;

typedef	struct	_MULTICAST_TABLE_ {
	UINT8	MulticastAddress[ADDR_SIZE];
	UINT16	EntryUsed;
} MULTICAST_TABLE;

typedef	struct _RCB_ {
	union {
		UINT8	RWs_i8val[8];
		UINT16	RWs_i16val[4];
		UINT32	RWs_i32val[2];
		UINT64	RWs_I64val;
	} RCBDriverWS;
	UINT8		RCBReserved[(UINT32)&(((ECB *)0)->ECB_FragmentCount) -
				(UINT32)&(((ECB *)0)->ECB_Status)];
	UINT32		RCBFragCount;
	FRAGMENTSTRUCT	RCBFragStruct;
} RCB;

#define	SPARE_ECB_STATUS	0xeeee

typedef	struct	_SHARED_DATA_ {
	UINT32				CMSMReservedForCHSM[20];
	struct _DRIVER_PARM_BLOCK_	*CMSMAsmParmBlock;
	UINT32				CMSMAdapterIntDisabled;
	UINT32				CMSMNeedContiguousECB;
	struct _GENERIC_STATS_		*CMSMGenericStatsPtr;
	struct _MSM_PROTECTION_OBJ_	*CMSMProtectionObject;
	UINT32				CMSMMaxFrameHeaderSize;
	UINT8				CMSMPhysNodeAddress[8];
	void				*CMSMSendListHead;
	void				*CMSMSendListTail;
	struct _DRIVER_PARM_BLOCK_	*CMSMDPBlockPtr;
	void				*CMSMMediaDataPtr;
	CONFIG_TABLE			*CMSMDefaultVirtualBoard;
	UINT32				CMSMDriverInterrupt;
	UINT32				CMSMInCriticalSection;
	MLID_StatsTable			*CMSMStatsPtr;
	CONFIG_TABLE			*CMSMVirtualBoardLink[4];
	UINT32				CMSMStatusFlags;
	UINT32				CMSMTxFreeCount;
} SHARED_DATA;

#define	DADSP_TO_CMSMADSP(n)	((SHARED_DATA *)n - 1)

/*
 * CMSMStatusFlags bit definitions.
 */
#define	SHUTDOWN		0x01
#define	TXQUEUED		0x02

/*
 * DRIVER_OPTION Type values.
 */
#define	CUSTOM_OPTION		0
#define	INT_OPTION		1
#define	PORT_OPTION		2
#define	DMA_OPTION		3
#define	MEMORY_OPTION		4
#define	SLOT_OPTION		5
#define	CARD_OPTION		6

/*
 * DRIVER_OPTION Flags values.
 */
#define	OPTION_REQUIRED		0x01
#define	USE_THIS_OPTION		0x02
#define	VALUE_REQUIRED		0x04
#define	SPECIFIC_VALUE_REQUIRED	0x08

#define	DEFAULT_VALUE		0x10
#define	SHAREABLE_OPTION	0x20

typedef struct	_DRIVER_OPTION_ {
	MEON_STRING	Name[32];
	UINT32		Parameter0;
	UINT32		Parameter1;
	UINT32		Parameter2;
	UINT16		Type;
	UINT16		Flags;
	MEON		*String;
} DRIVER_OPTION;

typedef struct  _SUPER_DRIVER_OPTION_ {
        struct  _SUPER_DRIVER_OPTION_   *SOptionLink;
        DRIVER_OPTION			Option;
} SUPER_DRIVER_OPTION;

struct	_TCB_;
struct	_TCB_FRAGMENTSTRUCT_;
struct _MEDIA_HEADER_;

typedef	struct _FRAME_DATA_ {
	CONFIG_TABLE			ConfigTable;
	void				*Reserved[20];
	void				*DriverData;
	UINT32				InternalMediaID;
	UINT32				MediaHeaderSize;
	UINT8				PacketType[8];
	UINT32				BitSwapAddressFlag;
	struct _DRIVER_PARM_BLOCK_	*DPBlock;
	struct _TSM_PARM_BLOCK_		*MPBlock;
	void				*FirmwareBuffer;
	UINT32				HardwareIsRegistered;
	void				(_cdecl * SendEntry)(ECB *ecb,
						CONFIG_TABLE *configTable);
	UINT32				(_cdecl * SendRoutine)(
						SHARED_DATA *sharedData,
						ECB *ecb, struct _TCB_ *tcb);
	UINT32				(_cdecl * Driver_Control)(void);
	SUPER_DRIVER_OPTION		*SuperOptionList;
} FRAME_DATA;

#define CONFIG_TO_MSM(n)	(FRAME_DATA *)((UINT8 *)n -	\
				(UINT32)&((FRAME_DATA *)0)->ConfigTable)
#define AES_TO_MLID(n)		(MLID_AES_ECB *)((UINT8 *)n -	\
				(UINT32)&((MLID_AES_ECB *)0)->AesReserved)
#define MILL_TO_TICKS(n)	(n/55)

typedef	struct _CHSM_STACK_ {
	PVOID		ModuleHandle;
	PVOID		ScreenHandle;
	MEON		*CommandLine;
	MEON		*ModuleLoadPath;
	UINT32		UnitializedDataLength;
	void		*CustomDataFileHandle;
   	UINT32		(* FileRead)(void *FileHandle, UINT32 FileOffset,
				void *FileBuffer, UINT32 FileSize);
	UINT32		*CustomDataOffset;
	UINT32		CustomDataSize;
	UINT32		NumMsgs;
   	MEON		**Msgs;
} CHSM_STACK;

typedef struct _DRIVER_PARM_BLOCK_ {
	UINT32			DriverParameterSize;
	CHSM_STACK		*DriverInitParamPointer;
	PVOID			DriverModuleHandle;
	void			*DBP_Reserved0;
	void			*DriverAdapterPointer;
	CONFIG_TABLE		*DriverConfigTemplatePtr;
	UINT32			DriverFirmwareSize;
	void			*DriverFirmwareBuffer;
	UINT32			DriverNumKeywords;
	MEON		     	**DriverKeywordText;
	UINT32			*DriverKeywordTextLen;
	UINT32			(_cdecl ** DriverProcesskeywordTab)
					(CONFIG_TABLE *configTable, PMEON
					string, UINT32 value);
	UINT32			DriverAdapterDataSpaceSize;
	struct _DRIVER_DATA_	*DriverAdapterDataSpacePtr;
	UINT32			DriverStatisticsTablePtr;
	UINT32			DriverEndOfChainFlag;
	UINT32			DriverSendWantsECBs;
	UINT32			DriverMaxMulticast;
	UINT32			DriverNeedsBelow16Meg;
	void			(_cdecl * DPB_Reserved2)(struct _DRIVER_DATA_
					*driverData, CONFIG_TABLE
					*configTable);
	void			(_cdecl * DPB_Reserved3)(struct _DRIVER_DATA_
					*driverData, CONFIG_TABLE
					*configTable);
	void			(_cdecl * DriverISRPtr)(struct _DRIVER_DATA_
					*driverData);
	void			(_cdecl * DriverMulticastChangePtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE *configTable,
					MULTICAST_TABLE *mcTable,
					UINT32 numEntries, UINT32
					functionalTable);
	void			(_cdecl * DriverPollPtr)(struct _DRIVER_DATA_
					*driverData, CONFIG_TABLE
					*configTable);
	ODISTAT			(_cdecl * DriverResetPtr)(struct _DRIVER_DATA_
					*driverData, CONFIG_TABLE
					*configTable);
 	void			(_cdecl * DriverSendPtr)(struct _DRIVER_DATA_
					*driverData, CONFIG_TABLE
					*configTable, struct _TCB_ *tcb,
					UINT32 pktSize, void *PhysTcb);
	ODISTAT			(_cdecl * DriverShutdownPtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE *configTable,
					UINT32 shutDownType);
	void			(_cdecl * DriverTxTimeoutPtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE *configTable);
	void			(_cdecl * DriverPromiscuousChangePtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE *configTable, UINT32
					promiscuousMode);
	void			(_cdecl * DriverStatisticsChangePtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE *configTable);
	void			(_cdecl * DriverRxLookAheadChangePtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE *configTable);
	ODISTAT			(_cdecl * DriverManagementPtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE *configTable, ECB *ecb);
	void			(_cdecl * DriverEnableInterruptPtr)(struct
					_DRIVER_DATA_ *driverData);
	BOOLEAN			(_cdecl * DriverDisableInterruptPtr)(struct
					_DRIVER_DATA_ *driverData);
	void			(_cdecl * DriverISRPtr2)(struct _DRIVER_DATA_
					*driverData);
	MEON			**DriverMessagesPtr;
} DRIVER_PARM_BLOCK;

#define	PERMANENT_SHUTDOWN	0
#define	TEMPORARY_SHUTDOWN	1

typedef struct _TSM_CONFIG_LIMITS_ {
	UINT8	*MinNodeAddress;
	UINT8	*MaxNodeAddress;
	UINT32	MinRetries;
	UINT32	MaxCRetries;
	UINT32	NumberFrames;
} TSM_CONFIG_LIMITS;

typedef struct	_TSM_PARM_BLOCK_ {
	UINT32			MediaParameterSize;
	TSM_CONFIG_LIMITS	*MediaConfigLimits;
	MEON_STRING		**MediaFrameDescriptTable;
	UINT8			(*MediaProtocolIDArray)[6];
	UINT8			*MediaIDArray;
	UINT8			*MediaHeaderSizeArray;
	UINT32			(_cdecl ** MediaSendRoutineArray)(
					SHARED_DATA *sharedData, ECB *,
					struct _TCB_ *tcb);
	UINT32			MediaAdapterDataSpaceSize;
	struct _MEDIA_DATA_	*MediaAdapterPtr;
	ODISTAT			(_cdecl * MediaAdjustPtr)(FRAME_DATA
					*frameData);
	ODISTAT			(_cdecl * MediaInitPtr)(const struct
					_DRIVER_DATA_ *driverData,
					FRAME_DATA *frameData);
	ODISTAT			(_cdecl * MediaResetPtr)(struct _DRIVER_DATA_
					*driverData, FRAME_DATA *frameData);
	ODISTAT			(_cdecl * MediaShutdownPtr)(struct
					_DRIVER_DATA_ *driverData,
					FRAME_DATA *frameData, UINT32
					shutdownType);
	void			(_cdecl * MediaSendPtr)(ECB *ecb,
					CONFIG_TABLE *configTable);
	void			*(_cdecl * MediaGetNextSendPtr)(struct
					_DRIVER_DATA_ *driverData,
					CONFIG_TABLE **configTable, UINT32
					*PacketSize, void **PhysTcb);
	ODISTAT			(_cdecl * MediaAddMulticastPtr)(struct
					_DRIVER_DATA_ *driverData, UINT8
					*McAddress);
	ODISTAT			(_cdecl * MediaDeleteMulticastPtr)(struct
					_DRIVER_DATA_ *driverData, UINT8
					*McAddress);
	ODISTAT			(_cdecl * MediaNodeOverridePtr)(FRAME_DATA
					*frameData, MEON mode);
	ODISTAT			(_cdecl * MediaAdjustNodeAddressPtr)(
					FRAME_DATA *frameData);
	ODISTAT			(_cdecl * MediaSetLookAheadSizePtr)(struct
					_DRIVER_DATA_ *driverData, FRAME_DATA
					*frameData, UINT32 size);
	ODISTAT			(_cdecl * MediaPromiscuousChangePtr)(struct
					_DRIVER_DATA_ *driverData, FRAME_DATA
					*frameData, UINT32 PromiscuousState,
					UINT32 *PromiscuousMode);
	ODISTAT			(_cdecl * MediaRegisterMonitorPtr)(const struct
					_DRIVER_DATA_ *driverData,
					FRAME_DATA *frameData, UINT32
					(* _cdecl TXRMonRoutine)
					(struct _TCB_ *), BOOLEAN MonitorState);
} TSM_PARM_BLOCK;

typedef struct _GENERIC_STATS_ {
	UINT32	Gen_TotalTxPackets;
	UINT32	Gen_TotalRxPackets;
	UINT32	Gen_NoECBs;
	UINT32	Gen_TxTooBig;
	UINT32	Gen_TxTooSmall;
	UINT32	Gen_RxOverflow;
	UINT32	Gen_RxTooBig;
	UINT32	Gen_RxTooSmall;
	UINT32	Gen_TxMiscError;
	UINT32	Gen_RxMiscError;
	UINT32	Gen_TxRetryCount;
	UINT32	Gen_RxCheckSumError;
	UINT32	Gen_RxMisMatchError;
	UINT32	Gen_TotalTxOkByteLow;
	UINT32	Gen_TotalTxOkByteHigh;
	UINT32	Gen_TotalRxOkByteLow;
	UINT32	Gen_TotalRxOkByteHigh;
	UINT32	Gen_TotalGroupAddrTx;
	UINT32	Gen_TotalGroupAddrRx;
	UINT32	Gen_AdapterReset;
	UINT32	Gen_AdapterOPRTimeStamp;
	UINT32	Gen_Qdepth;
	UINT32	Gen_TxOKSingleCollisions;
	UINT32	Gen_TxOKMultipleCollisions;
	UINT32	Gen_TxOKButDeferred;
	UINT32	Gen_TxAbortLateCollision;
	UINT32	Gen_TxAbortExcessCollision;
	UINT32	Gen_TxAbortCarrierSense;
	UINT32	Gen_TxAbortExcessiveDeferral;
	UINT32	Gen_RxAbortFrameAlignment;
} GENERIC_STATS;

#ifdef	IAPX386

/*
 * v3.00 Assembly HSMs stats table.
 */
typedef struct _MSTATS_ {
	UINT8		MSTATS_MajorVer;
	UINT8		MSTATS_MinorVer;
	UINT16		MSTATS_NumGenericCounters;
	UINT32		MSTATS_ValidMask0;
	GENERIC_STATS	MSTATS_Generic;
}MSTATS;

#endif

#define AES_PROCESS_SIG		0x50534541		/* 'PSEA' */
#define ALLOC_SIG		0x54524C41		/* 'TRLA' */
#define ALLOC_STACK_SIG		0x53524C41		/* 'SRLA' */
#define CACHE_BELOW_16_MEG_MEMORY_SIG	0x36314243	/* '61BC' */
#define ECB_SIG			0x53424345		/* 'SBCE' */
#define EVENT_SIG		0x544E5645		/* 'TNVE' */
#define INTERRUPT_SIG		0x50544E49		/* 'PTNI' */
#define IO_REGISTRATION_SIG	0x53524F49		/* 'SROI' */
#define LOADABLE_ROUTER_SIG	0x5452444C		/* 'TRDL' */
#define LSL_AES_EVENT_SIG	0x5345414C		/* 'SEAL' */
#define LSL_DEFAULT_STACK_SIG	0x444C534C		/* 'DLSL' */
#define LSL_PRESCAN_STACK_SIG	0x504C534C		/* 'PLSL' */
#define LSL_STACK_SIG		0x534C534C		/* 'SLSL' */
#define LSL_TX_PRESCAN_STACK_SIG	0x544C534C	/* 'TLSL' */
#define MLID_SIG		0x44494C4D		/* 'DILM' */
#define POLLING_PROCEDURE_SIG	0x52504C50		/* 'RPLP' */
#define PROCESS_SIG		0x53435250		/* 'SCRP' */
#define TIMER_SIG		0x524D4954		/* 'RMIT' */

/*
 * MSM Specific declarations.
 */
void *	_cdecl	CMSMAlloc(const struct _DRIVER_DATA_ *driverData,
			UINT32 nBytes);
void *	_cdecl	CMSMAllocPages(const struct _DRIVER_DATA_ *driverData,
			UINT32 nBytes);
RCB *	_cdecl	CMSMAllocateRCB(const struct _DRIVER_DATA_ *driverData,
			UINT32 nBytes, void **PhysicalRCB);
void	_cdecl	CMSMDisableHardwareInterrupt(const struct _DRIVER_DATA_
			*driverData);
void	_cdecl	CMSMDriverRemove(PVOID moduleHandle);
void	_cdecl	CMSMEnableHardwareInterrupt(const struct _DRIVER_DATA_
			*driverData);
ODISTAT	_cdecl	CMSMEnablePolling(const struct _DRIVER_DATA_ *driverData);
void	_cdecl	CMSMEnqueueSend(FRAME_DATA *frameData, const struct
			_DRIVER_DATA_ *driverData, ECB *ecb);
void	_cdecl	CMSMFree(const struct _DRIVER_DATA_ *driverData, void *dataPtr);
void	_cdecl	CMSMFreePages(const struct _DRIVER_DATA_ *driverData,
			void *dataPtr);
ODI_NSI	_cdecl	CMSMGetCardConfigInfo(UINT32 busTag, UINT32 slot, UINT32 size,
			UINT32 parm1, UINT32 parm2, void *configInfo);
UINT32	_cdecl	CMSMGetCurrentTime(void);
ODISTAT	_cdecl	CMSMGetHardwareBusType(UINT32 BusTag, UINT32 *BusType);

/*
 * CMSMGetHardwareBusType return values.
 */
#define	ISA_BUS			0x00
#define	MICRO_CHANNEL_BUS	0x01
#define	EISA_BUS		0x02

void *	_cdecl	CMSMGetLogical(void *physicalAddr);
UINT32	_cdecl	CMSMGetMicroTimer(void);
void *	_cdecl	CMSMGetPhysical(void *logicalAddr);
ECB *	_cdecl	CMSMGetPhysicalECB(struct _DRIVER_DATA_ *driverData, ECB *ecb);
void *	_cdecl	CMSMInitAlloc(UINT32 nbytes);

#define	MONO			0xb0000
#define	COLOR			0xb8000
#define	UPPER_LINE		10
#define	LOWER_LINE		22

void		CMSMOutUINT16(UINT16 value, UINT32 ScreenBase);
void		CMSMOutUINT32(UINT32 value, UINT32 ScreenBase);
void		CMSMOutUINT8(UINT8 value, UINT32 ScreenBase);
void		CMSMOutMEON(MEON Char, UINT32 screen);

typedef	enum _MSG_TYPE_ {
	MSG_TYPE_INIT_INFO,
	MSG_TYPE_INIT_WARNING,
	MSG_TYPE_INIT_ERROR,
	MSG_TYPE_RUNTIME_INFO,
	MSG_TYPE_RUNTIME_WARNING,
	MSG_TYPE_RUNTIME_ERROR
} MSG_TYPE;

void	_cdecl	CMSMPrintString(const CONFIG_TABLE *configTable, MSG_TYPE
			msgType, MEON_STRING *msg, void *parm1, void *parm2);
void	_cdecl	CMSMReadPhysicalMemory(UINT32 nbytes, PUINT8 destAddr,
			PUINT8 physSrcAddr);
REG_TYPE _cdecl	CMSMRegisterHardwareOptions(CONFIG_TABLE *configTable,
			struct _DRIVER_DATA_ **driverData);
ODISTAT	_cdecl	CMSMRegisterMLID(const struct _DRIVER_DATA_ *driverData,
			CONFIG_TABLE *configTable);
ODISTAT	_cdecl	CMSMRegisterResource(const struct _DRIVER_DATA_	*driverData,
			CONFIG_TABLE *configTable, EXTRA_CONFIG *extraConfig);
ODISTAT	_cdecl	CMSMRegisterTSM(DRIVER_PARM_BLOCK *DriverParameterBlock,
			TSM_PARM_BLOCK *MediaParameterBlock, CONFIG_TABLE
			**configTable);
void	_cdecl	CMSMReturnDriverResources(const CONFIG_TABLE *configTable);
ECB *	_cdecl	CMSMReturnPhysicalECB(const struct _DRIVER_DATA_ *driverData,
			ECB *ecb);
void	_cdecl	CMSMReturnRCB(const struct _DRIVER_DATA_ *driverData,
			RCB *rcb);
ODISTAT	_cdecl	CMSMScheduleAES(const struct _DRIVER_DATA_ *driverData,
			MLID_AES_ECB *mlidAESECB);
void	_cdecl	CMSMServiceEvents(void);
void		junk(const struct _DRIVER_DATA_ *driverData);

ODISTAT	_cdecl	CMSMSetHardwareInterrupt(struct _DRIVER_DATA_ *driverData,
			const CONFIG_TABLE *configTable);
void	_cdecl	CMSMWritePhysicalMemory(UINT32 nbytes, void *physDestAddr,
			void *srcAddr);

#define CMP_NODE(a,b) ( \
	((UINT32 *)(a))[0] == ((UINT32 *)(b))[0] &&\
	((UINT16 *)(a))[2] == ((UINT16 *)(b))[2] \
)

struct IOOptionStructure {
        UINT32  NumberOfOptions;
        UINT32  OptionData[1];
};

typedef struct AdapterOptionDefinitionStructure {
        struct IOOptionStructure *IOSlot;
        struct IOOptionStructure *IOPort0;
        struct IOOptionStructure *IOLength0;
        struct IOOptionStructure *IOPort1;
        struct IOOptionStructure *IOLength1;
        struct IOOptionStructure *MemoryDecode0;
        struct IOOptionStructure *MemoryLength0;
        struct IOOptionStructure *MemoryDecode1;
        struct IOOptionStructure *MemoryLength1;
        struct IOOptionStructure *Interrupt0;
        struct IOOptionStructure *Interrupt1;
        struct IOOptionStructure *DMA0;
        struct IOOptionStructure *DMA1;
        struct IOOptionStructure *Channel;
} HSM_OPTIONS;

/*
 * flags that can be set on the OptionData 
 */
#define OD_IS_RANGE                     0x80000000
#define OD_HAS_INCREMENT        	0x40000000

/*
 * ParseIOParameters bits
 */
#define NeedsSlotBit                    0x00000001
#define NeedsIOPort0Bit                 0x00000002
#define NeedsIOLength0Bit               0x00000004
#define NeedsIOPort1Bit                 0x00000008
#define NeedsIOLength1Bit               0x00000010
#define NeedsMemoryDecode0Bit   	0x00000020
#define NeedsMemoryLength0Bit   	0x00000040
#define NeedsMemoryDecode1Bit   	0x00000080
#define NeedsMemoryLength1Bit   	0x00000100
#define NeedsInterrupt0Bit              0x00000200
#define NeedsInterrupt1Bit              0x00000400
#define NeedsDMA0Bit                    0x00000800
#define NeedsDMA1Bit                    0x00001000
#define NeedsChannelBit                 0x00002000

/*
 * ParseLANParameters needFlags defines
 */
#define CAN_SET_NODE_ADDRESS    	0x40000000
#define MUST_SET_NODE_ADDRESS   	0x80000000

/*
 * RegisterForEventNotification defines
 */
#define EVENT_DOWN_SERVER		4
#define EVENT_CHANGE_TO_REAL_MODE	5
#define EVENT_RETURN_FROM_REAL_MODE	6
#define EVENT_EXIT_TO_DOS		7
#define EVENT_MODULE_UNLOAD		8

#define EVENT_PRIORITY_OS		0
#define EVENT_PRIORITY_APPLICATION	20
#define EVENT_PRIORITY_DEVICE		40

#define CALLS_INTO_HSM 15

typedef struct _MSM_PROTECTION_OBJ_ {
        lock_t                  *AdapterSpinLock;
        lock_t                  *QueueSpinLock;
} MSM_PROTECTION_OBJ;

/*
 * flags for msm_init_alloc.
 */
#define	MSM_NONE				0
#define	MSM_DMA_LOGICALTOPHYSICAL_BELOW16	1
#define	MSM_DMA					2
#define	MSM_DMA_BELOW16				3

/*
 * flags for msm_read_eisa_config.
 */
#define	MSM_EISA_SUCCESS			0x0
#define	MSM_EISA_INVALID_SLOT			0x80
#define	MSM_EISA_INVALID_FUNC			0x81
#define	MSM_EISA_EMPTY_SLOT			0x83
#define	MSM_EISA_NOMEM				0x87

#endif
