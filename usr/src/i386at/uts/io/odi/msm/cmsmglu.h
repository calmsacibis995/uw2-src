/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/odi/msm/cmsmglu.h	1.1"
#ident	"$Header: $"

extern VOID	MSMAlertFatal(void);
extern VOID	MSMAlertWarning(void);
extern VOID	MSMAlloc(void);
extern VOID	MSMAllocPages(void);
extern VOID	MSMAllocateRCB(void);
extern VOID	MSMDeRegisterSharedMemory(void);
extern VOID	MSMDriverRemove(void);
extern VOID	MSMEnablePolling(void);
extern VOID	MSMFree(void);
extern VOID	MSMFreePages(void);
extern VOID	GetCurrentTime(void);
extern VOID	GetHardwareBusType(void);
extern VOID	GetProcessorSpeedRating(void);
extern VOID	MSMInitAlloc(void);
extern VOID	MSMInitFree(void);
extern VOID	MSMParseDriverParameters(void);
extern VOID	MSMParseCustomKeywords(void);
extern VOID	MSMPrintString(void);
extern VOID	MSMPrintStringFatal(void);
extern VOID	MSMPrintStringWarning(void);
extern VOID	MSMReadEISAConfig(void);
extern VOID	MSMReadPhysicalMemory(void);
extern VOID	MSMRegisterHardwareOptions(void);
extern VOID	MSMRegisterMLID(void);
extern VOID	MSMRegisterSharedMemory(void);
extern VOID	MSMReturnDriverResources(void);
extern VOID	MSMReturnRcvECB(void);
extern VOID	MSMScheduleAESCallBack(void);
extern VOID	MSMScheduleIntTimeCallBack(void);
extern VOID	LSLServiceEvents(void);
extern VOID	MSMSetHardwareInterrupt(void);
extern VOID	MSMWritePhysicalMemory(void);
extern VOID	AsmDriverAES(PVOID driverData, MLID_ConfigTable
			*configTable);
extern VOID	AsmDriverCallBack(PVOID driverData, MLID_ConfigTable
			*configTable);
extern VOID	AsmDriverISR(PVOID driverData);
extern VOID	AsmDriverMulticastChange(PVOID driverData, MLID_ConfigTable
			*configTable, PVOID mcTable, ULONG numEntries, PVOID
			funcaddr);
extern VOID	AsmDriverPoll(PVOID driverData);
extern ULONG	AsmDriverReset(PVOID driverData, MLID_ConfigTable *configTable);
extern ULONG	AsmDriverSend(PVOID driverData, MLID_ConfigTable *configTable,
			PVOID tcb, ULONG pktSize, PVOID *physEcb);
extern ULONG	AsmDriverShutdown(PVOID driverData, MLID_ConfigTable
			*configTable, ULONG shutDownType);
extern ULONG	AsmDriverTxTimeout(void);
extern ULONG	AsmDriverPromiscuousChange(PVOID driverData,
			MLID_ConfigTable *configTable, ULONG promiscuousMode);
extern ULONG	AsmDriverStatisticsChange(void);
extern ULONG	AsmDriverLookAheadChange(void);
extern ULONG	AsmDriverManagement(PVOID driverData, MLID_ConfigTable *,
			ECB *);
extern ULONG	AsmDriverEnableInterrupt(PVOID driverData);
extern ULONG	AsmDriverDisableInterrupt(PVOID driverData);
extern VOID	AsmCustomKeywordRoutine(MLID_ConfigTable *, PVOID *,
			int, UINT32 (*fcn)(MLID_ConfigTable *, PUINT8, UINT32));
