/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_MSM_MSMSTRUC_H_
#define	_IO_MSM_MSMSTRUC_H_

#ident	"@(#)kern-i386at:io/odi/msm/msmstruc.h	1.2"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#include <io/odi/msm/cmsm.h>

#elif defined(_KERNEL)

#include <sys/cmsm.h>

#endif /* _KERNEL_HEADERS */


#define GET_HI_LO(x)						\
	((UINT16)(x) << 8) | (UINT16)((x) >> 8)

#define BYTE_MOVE(Dest, Source)					\
	(UINT8)*Dest = (UINT8)*Source

#define WORD_MOVE(Dest, Source)					\
	*(PUINT16)Dest = *(PUINT16)Source

#define DWORD_MOVE(Dest, Source)				\
	*(PUINT32)Dest = *(PUINT32)Source

#define CMPADDRESS(a1, a2)					\
	((*(PUINT32)a1 ^ *(PUINT32)a2)	\
	|| (((PUINT16)(a1))[2] ^ ((PUINT16)(a2))[2]))

#define RESERVED		0

typedef struct _MESSAGE_ARGS_ {
	const	CONFIG_TABLE	*ConfigTable;
	MSG_TYPE		MsgType;
	MEON_STRING		*Msg;
	PVOID			Parm1;
	PVOID			Parm2;
	PVOID			WorkItem;
} MESSAGE_ARGS;

typedef	struct _ADAPTER_DATA_ {
	struct _ADAPTER_DATA_		*InstanceLink;
	DRIVER_PARM_BLOCK		DPBlock;
	TSM_PARM_BLOCK			MPBlock;
	AESECB				MSMPollECB;
	UINT16				AES0Pad;
	UINT32				PollInterval;
	AESECB				AsmAesEcb;
	UINT16				AES1Pad;
	UINT32				AesWaitTime;
	AESECB				AsmIntAesEcb;
	UINT16				AES2Pad;
	UINT32				IntAesWaitTime;
	UINT32				HSMInC;
#ifdef NT
	LAN_ADAPTER_OBJECT		*LanObject;
	ISR_CONTEXT			Isr1Context;
	ISR_CONTEXT			Isr2Context;
#else
	UINT32				AdapterMatchValue;
	VOID				*InterruptCookie;
	clock_t				isr_time;
	UINT32				Pad[8]; /* space that can be used! */
#endif
	UINT32				MaxTxFreeCount;
	ECB				*TxECBList;
	ECB				*TxECBHead;
	UINT32				Bus;
	EXTRA_CONFIG			*ExtraConfigList;
	MLID_AES_ECB			*AESList;

	/*
	 * the following variables will be accessed by TSM's and HSM's and
	 * must remain in the following order.
	 */
	UINT32				ReservedForCHSM[20];
	DRIVER_PARM_BLOCK		*AsmParmBlock;
	UINT32				AdapterIntDisabled;
	UINT32				NeedContiguousECB;
	GENERIC_STATS			*GenericStatsPtr;
#ifdef NT
	void				*AdapterSpinLock;
#else
	MSM_PROTECTION_OBJ		*ProtectionObject;
#endif
	UINT32				MaxFrameHeaderSize;
	UINT8				PhysNodeAddress[ADDR_SIZE];
	UINT8				PhysNodeFill[2];
	void				*SendListHead;
	void				*SendListTail;
	DRIVER_PARM_BLOCK		*DPBlockPtr;
	void				*MediaDataPtr;
	CONFIG_TABLE			*DefaultVirtualBoard;
	UINT32				DriverInterrupt;
	UINT32				InCriticalSection;
	MLID_StatsTable			*StatsPtr;
	CONFIG_TABLE			*VirtualBoardLink[4];
	UINT32				StatusFlags;
	UINT32				TxFreeCount;
} ADAPTER_DATA;

/*
 * definitions for Custom Keyword Types.
 */
#define	T_REQUIRED			0x8000	/* Keyword must be entered. */
#define	T_STRING			0x0100	/* String input(default). */
#define	T_NUMBER			0x0200	/* Dword Decimal input. */
#define	T_HEX_NUMBER			0x0300	/* Dword Hex input. */
#define	T_HEX_STRING			0x0400	/* 6-byte hex string input. */
#define	T_LENGTH_MASK			0x00FF	/* Length Mask */
#define T_TYPE_MASK         		0xFF00  /* Type Mask. */

/*
 * fake type definition to keep .h files happy. _DRIVER_DATA_ is
 * defined in the HSM and not accessed directly by the MSM or TSM.
 */
typedef	struct _DRIVER_DATA_ {
	void	*FakeData;
} DRIVER_DATA;

#define MSM_HIERADAPTER         18
#define MSM_HIERQUEUE           20

#endif
