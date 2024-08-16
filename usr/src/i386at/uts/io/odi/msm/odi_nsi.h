/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_MSM_ODI_NSI_H   /* wrapper symbol for kernel use */
#define _IO_MSM_ODI_NSI_H   /* subject to change without notice */

#ident	"@(#)kern-i386at:io/odi/msm/odi_nsi.h	1.1"
#ident	"$Header:$ "

typedef enum _ODI_NSI_ {
	ODI_NSI_SUCCESSFUL =		0x00000000,
	ODI_NSI_PROTECTION_VIOLATION =	0x00000001,
	ODI_NSI_HARDWARE_ERROR =	0x00000002,
	ODI_NSI_MEMORY_ERROR =		0x00000003,
	ODI_NSI_PARAMETER_ERROR =	0x00000004,
	ODI_NSI_UNSUPPORTED_OPERATION = 0x00000005,
	ODI_NSI_ITEM_NOT_PRESENT =	0x00000006
} ODI_NSI;

#ifdef NSI

void			BusLock(UINT32 busTag);
void			BusUnLock(UINT32 busTag);
UINT32			DisableSystemInterrupts(UINT32 busTag, UINT32
				interrupt);
void			DMACleanup(UINT32 DMAChannel);
ODI_NSI			DMAStart(UINT32 destBusTag, UINT32 destAddrType,
				const void *destAddrPtr, UINT32 srcBusTag,
				UINT32 srcAddrType, const void *srcAddrPtr,
				UINT32 len, UINT32 DMAChannel, UINT32 DMAMode);
UINT32			DMAStatus(UINT32 DMAChannel);
UINT32			EnableSystemInterrupts(UINT32 busTag, UINT32 interrupt);
ODI_NSI 		FreeLogicalMemory(UINT32 busTag1, const void
				*memAddrPtr, UINT32 busTag2, const void
				*mappedAddrPtr, UINT32 len);
UINT32 			GetAlignment(UINT32 type);
ODI_NSI			GetBusInfo(UINT32 busTag, UINT32 *logicalMemAddrSize,
				UINT32 *physicalMemAddrSize,
				UINT32 *IOAddrSize);
const MEON_STRING	*GetBusName(UINT32 busTag);
ODI_NSI			GetBusTag(const MEON_STRING *busName, UINT32 *busTag);
ODI_NSI			GetCardConfigInfo(UINT32 busTag, UINT32 slot,
				UINT32 size, UINT32 parm1, UINT32 parm2,
				void *configInfo);
UINT8			In8(UINT32 busTag, const void *IOAddrPtr);
UINT16			In16(UINT32 busTag, const void *IOAddrPtr);
UINT32			In32(UINT32 busTag, const void *IOAddrPtr);
ODI_NSI			InBuff8(UINT8 *bufferPtr, UINT32 IOBusTag, const void
				*IOAddrPtr, UINT32 count);
ODI_NSI			InBuff16(UINT8 *bufferPtr, UINT32 IOBusTag, const void
				*IOAddrPtr, UINT32 count);
ODI_NSI			InBuff32(UINT8 *bufferPtr, UINT32 IOBusTag, const void
				*IOAddrPtr, UINT32 count);
ODI_NSI			MapLogicalMemory(UINT32 busTag1, const void
				*memAddrPtr, UINT32 busTag2, void
				*mappedAddrPtr, UINT32 len);
ODI_NSI			MovFast(UINT32 destBusTag, const void *destAddrPtr,
				UINT32 srcBusTag, void *srcAddrPtr,
				UINT32 count);
ODI_NSI			Mov8(UINT32 destBusTag, void *destAddrPtr, UINT32
				srcBusTag, const void *srcAddrPtr,
				UINT32 count);
ODI_NSI			Mov16(UINT32 destBusTag, void *destAddrPtr, UINT32
				srcBusTag, const void *srcAddrPtr,
				UINT32 count);
ODI_NSI			Mov32(UINT32 destBusTag, void *destAddrPtr, UINT32
				srcBusTag, const void *srcAddrPtr,
				UINT32 count);
void			Out8(UINT32 busTag, const void *IOAddrPtr,
				UINT8 outputVal);
void			Out16(UINT32 busTag, const void *IOAddrPtr,
				UINT16 outputVal);
void			Out32(UINT32 busTag, const void *IOAddrPtr,
				UINT32 outputVal);
ODI_NSI			OutBuff8(UINT32 IOBusTag, void *IOAddrPtr, const void
				*bufferPtr, UINT32 count);
ODI_NSI			OutBuff16(UINT32 IOBusTag, void *IOAddrPtr, const void
				*bufferPtr, UINT32 count);
ODI_NSI			OutBuff32(UINT32 IOBusTag, void *IOAddrPtr, const void
				*bufferPtr, UINT32 count);
UINT8			Rd8(UINT32 busTag, const void *memAddrPtr);
UINT16			Rd16(UINT32 busTag, const void *memAddrPtr);
UINT32			Rd32(UINT32 busTag, const void *memAddrPtr);
void			RestoreSystemInterrupts(UINT32 magicNumber);
ODISTAT 		SearchFirstAdapter(UINT32 busType, UINT32 productIDLen,
				const MEON *productID, UINT32 *busTag,
				UINT32 *slot);
ODISTAT 		SearchNextAdapter(UINT32 busType, UINT32 productIDLen,
				const MEON *productID, UINT32 *busTag,
				UINT32 *slot);

ODI_NSI			Set8(UINT32 busTag, const void *memAddrPtr, UINT8
				value, UINT32 count);
ODI_NSI			Set16(UINT32 busTag, const void *memAddrPtr, UINT16
				value, UINT32 count);
ODI_NSI			Set32(UINT32 busTag, const void *memAddrPtr, UINT32
				value, UINT32 count);
void			Slow(void);
void			Wrt8(UINT32 busTag, void *memAddrPtr, UINT8 writeVal);
void			Wrt16(UINT32 busTag, void *memAddrPtr, UINT16 writeVal);
void			Wrt32(UINT32 busTag, void *memAddrPtr, UINT32 writeVal);


#pragma aux DisableSystemInterrupts parm [EAX] [EAX];

#pragma aux DisableSystemInterrupts = \
	0x9C                            	/* pushfd */	\
	0x58                            	/* pop eax */	\
	0xFA                            	/* cli */	\
	parm [EAX] [EAX]					\
	modify exact [EAX];

void			DMACleanup(UINT32 DMAChannel);
ODI_NSI			DMAStart(UINT32 destBusTag, UINT32 destAddrType,
				const void *destAddrPtr, UINT32 srcBusTag,
				UINT32 srcAddrType, const void *srcAddrPtr,
				UINT32 len, UINT32 DMAChannel, UINT32 DMAMode);
UINT32			DMAStatus(UINT32 DMAChannel);
UINT32			EnableSystemInterrupts(UINT32 busTag, UINT32 interrupt);

#pragma aux EnableSystemInterrupts parm [EAX] [EAX];

#pragma aux EnableSystemInterrupts = \
	0x9C                            	/* pushfd */	\
	0x58                            	/* pop eax */	\
	0xFB                            	/* sti */	\
	parm [EAX] [EAX]					\
	modify exact [EAX];

#pragma aux In8 parm [EAX] [EDX];

#pragma aux In8 = \
	0xec					/* in al, dx */	\
	parm [EAX] [EDX] 					\
	modify exact [EAX];

#pragma aux In16 parm [EAX] [EDX];

#pragma aux In16 = \
	0x66 0xed				/* in ax, dx */		\
	parm [EAX] [EDX] 						\
	modify exact [EAX];

#pragma aux In32 parm [EAX] [EDX];

#pragma aux In32 = \
	0xed					/* in eax, dx */	\
	parm [EAX] [EDX]						\
	modify exact [EAX];

#pragma aux InBuff8 parm [EDI] [EAX] [EDX] [ECX];

#pragma	aux InBuff8 = \
	0xF3 0x6C                       /* rep insb */			\
	0xB8 0x00 0x00 0x00 0x00	/* mov eax, ODI_NSI_SUCCESSFUL */ \
	parm [EDI] [EAX] [EDX] [ECX] \
	modify exact [EAX ECX EDI];

#pragma aux InBuff16 parm [EDI] [EAX] [EDX] [ECX];

#pragma	aux InBuff16 = \
	0xF3 0x66 0x6D			/* rep insw */			\
	0xB8 0x00 0x00 0x00 0x00	/* mov eax, ODI_NSI_SUCCESSFUL */ \
	parm [EDI] [EAX] [EDX] [ECX] \
	modify exact [EAX ECX EDI];

#pragma aux InBuff32 parm [EDI] [EAX] [EDX] [ECX];

#pragma	aux InBuff32 = \
	0xF3 0x6D			/* rep insd */			\
	0xB8 0x00 0x00 0x00 0x00	/* mov eax, ODI_NSI_SUCCESSFUL */ \
	parm [EDI] [EAX] [EDX] [ECX]					\
	modify exact [EAX ECX EDI];

#pragma aux MovFast parm [EAX] [EDI] [EAX] [ESI] [ECX]

#pragma aux MovFast = \
	0x8B 0xC1                         	/* mov eax, ecx*/	\
	0xC1 0xE9 0x02				/* shr ecx, 2 */	\
	0xF3 0xA5                        	/* rep movsd */		\
	0x8B 0xC8                         	/* mov ecx, eax */	\
	0x83 0xE1 0x03				/* and ecx, 3 */	\
	0xF3 0xA4                        	/* rep movsb */		\
	0xB8 0x00 0x00 0x00 0x00	/* mov eax, ODI_NSI_SUCCESSFUL */ \
	parm [EAX] [EDI] [EAX] [ESI] [ECX]				\
	modify exact [EAX ESI EDI ECX];

#pragma aux Mov8 parm [EAX] [EDI] [EAX] [ESI] [ECX]

#pragma aux Mov8 = \
	0xF3 0xA4                        	/* rep movsb */		\
	0xB8 0x00 0x00 0x00 0x00	/* mov eax, ODI_NSI_SUCCESSFUL */ \
	parm [EAX] [EDI] [EAX] [ESI] [ECX]				\
	modify exact [EAX ESI EDI ECX];

#pragma aux Mov16 parm [EAX] [EDI] [EAX] [ESI] [ECX]

#pragma aux Mov16 = \
	0xF3 0x66 0xA5                     	/* rep movsw */		\
	0xB8 0x00 0x00 0x00 0x00	/* mov eax, ODI_NSI_SUCCESSFUL */ \
	parm [EAX] [EDI] [EAX] [ESI] [ECX]				\
	modify exact [EAX ESI EDI ECX];

#pragma aux Mov32 parm [EAX] [EDI] [EAX] [ESI] [ECX]

#pragma aux Mov32 = \
	0xF3 0xA5                     		/* rep movsw */		\
	0xB8 0x00 0x00 0x00 0x00	/* mov eax, ODI_NSI_SUCCESSFUL */ \
	parm [EAX] [EDI] [EAX] [ESI] [ECX]				\
	modify exact [EAX ESI EDI ECX];

#pragma aux Out8 parm [EAX] [EDX] [EAX];

#pragma aux Out8 = \
	0xee					/* out dx, al */	\
	parm [EAX] [EDX] [EAX]						\
	modify exact [];

#pragma aux Out16 parm [EAX] [EDX] [EAX];

#pragma aux Out16 = \
	0x66 0xef				/* out dx, ax */	\
	parm [EAX] [EDX] [EAX]						\
	modify exact [];

#pragma aux Out32 parm [EAX] [EDX] [EAX];

#pragma aux Out32 = \
	0xef					/* out dx, eax */	\
	parm [EAX] [EDX] [EAX]						\
	modify exact [];

#pragma aux OutBuff8 parm [EAX] [EDX] [ESI] [ECX];

#pragma aux OutBuff8 = \
	0xF3 0x6E				/* rep outsb */		\
	0xB8 0x00 0x00 0x00 0x00	/* mov eax, ODI_NSI_SUCCESSFUL */ \
	parm [EAX] [EDX] [ESI] [ECX]					\
	modify exact [EAX ESI ECX];

#pragma aux OutBuff16 parm [EAX] [EDX] [ESI] [ECX];

#pragma aux OutBuff16 = \
	0xF3 0x66 0x6F				/* rep outsw */		\
	0xB8 0x00 0x00 0x00 0x00	/* mov eax, ODI_NSI_SUCCESSFUL */ \
	parm [EAX] [EDX] [ESI] [ECX]					\
	modify exact [EAX ESI ECX];

#pragma aux OutBuff32 parm [EAX] [EDX] [ESI] [ECX];

#pragma aux OutBuff32 = \
	0xF3 0x6F                     		/* rep outsd */		\
	0xB8 0x00 0x00 0x00 0x00	/* mov eax, ODI_NSI_SUCCESSFUL */ \
	parm [EAX] [EDX] [ESI] [ECX]					\
	modify exact [EAX ESI ECX];


#pragma aux Rd8 = \
	0x0F 0xB6 0x00			/* movzx eax, byte ptr [eax] */	\
	parm [EAX] [EAX]						\
	modify exact [EAX];

#pragma aux Rd16 parm [EAX] [EAX];

#pragma aux Rd16 = \
	0x0F 0xB7 0x00 			/* movzx eax, word ptr [eax] */	\
	parm [EAX] [EAX]						\
	modify exact [EAX];

#pragma aux Rd32 parm [EAX] [EAX];

#pragma aux Rd32 = \
	0x8B 0x00                      		/* mov	eax, [eax] */	\
	parm [EAX] [EAX]						\
	modify exact [EAX];

#pragma aux RestoreSystemInterrupts parm [EAX];

#pragma aux RestoreSystemInterrupts = \
	0x50                            	/* push	eax */		\
	0x9D					/* popfd */		\
	parm [EAX]							\
	modify exact [EAX];

#pragma aux Set8 parm [EAX] [EDI] [EAX] [ECX];

#pragma aux Set8 = \
	0xF3 0xAA                        	/* rep stosb */		\
	0xB8 0x00 0x00 0x00 0x00	/* mov eax, ODI_NSI_SUCCESSFUL */ \
	parm [EAX] [EDI] [EAX] [ECX]					\
	modify exact [EDI ECX];

#pragma aux Set16 parm [EAX] [EDI] [EAX] [ECX];

#pragma aux Set16 = \
	0xF3 0x66 0xAB                     	/* rep stosw */		\
	0xB8 0x00 0x00 0x00 0x00	/* mov eax, ODI_NSI_SUCCESSFUL */ \
	parm [EAX] [EDI] [EAX] [ECX]					\
	modify exact [EDI ECX];

#pragma aux Set32 parm [EAX] [EDI] [EAX] [ECX];

#pragma aux Set32 = \
	0xF3 0xAB                     		/* rep stosd */		\
	0xB8 0x00 0x00 0x00 0x00	/* mov eax, ODI_NSI_SUCCESSFUL */ \
	parm [EAX] [EDI] [EAX] [ECX]					\
	modify exact [EDI ECX];

#pragma aux Wrt8 parm [EAX] [ECX] [EAX];

#pragma aux Wrt8 = \
	0x88 0x01                         	/* mov [ecx], al */	\
	parm [EAX] [ECX] [EAX]						\
	modify exact [EAX ECX];

#pragma aux Wrt16 parm [EAX] [ECX] [EAX];

#pragma aux Wrt16 = \
	0x66 0x89 0x01				/* mov [ecx], ax */	\
	parm [EAX] [ECX] [EAX] \
	modify exact [EAX ECX];

#pragma aux Wrt32 parm [EAX] [ECX] [EAX];

#pragma aux Wrt32 = \
	0x89 0x01                      		/* mov [ecx], eax */	\
	parm [EAX] [ECX] [EAX]						\
	modify exact [EAX ECX];

#endif	/* NSI */

#endif	/* _IO_MSM_ODI_NSI_H */
