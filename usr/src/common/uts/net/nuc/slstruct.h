/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/slstruct.h	1.15"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/slstruct.h,v 2.53.2.4 1995/02/12 23:37:59 hashem Exp $"

#ifndef _NET_NUC_SPIL_SLSTRUCT_H
#define _NET_NUC_SPIL_SLSTRUCT_H


/*
 *  Netware Unix Client
 *
 *	MODULE: slstruct.h
 *
 *	ABSTRACT: Structures used by the Service Provider Interface
 *	layer used to manage services and tasks at the generic level.
 */

#ifdef _KERNEL_HEADERS
#include <util/ksynch.h>
#include <net/nuc/ncpiopack.h>


#define SPI_SERVICE_NAME_LENGTH		(int)128

typedef struct {
	void_t		*credentialsPtr;
	void_t		*spiServicePtr;
	void_t		*protoTaskPtr;	
	uint32		resourceCount;	/* resources allocated for this task */
	int32		useCount;		/* number of processes using this task */
	uint32		mode;		/* see spilcommon.h for mode bits */
	sleep_t		*spiTaskSleepLock;
	lock_t		*spiTaskLock;
	sv_t		*spiTaskSV;
	clock_t		freeTime;		/* when placed on free list */
	md4_t		*md4;
	uint32		ipxchecksum;
	uint8		badSignatureRetries;
} SPI_TASK_T;

typedef struct {
	void_t		*protoServicePtr;
	void_t		*taskList;
	void_t		*spi_ops;
	uint32		state;
#define SPI_TASK_LIST_UNDER_CONSTRUCTION	0x01
#define SPI_SERVICE_DRAINING				0x02
	uint32		flags;
#define SERVICE_IS_LOCAL	0x01
#define BINDERY_SERVER	0x02
	uint32		transportProtocol;
	uint32		serviceProtocol;
	struct netbuf	*address;
} SPI_SERVICE_T;

typedef struct NWslHandle {
	uint32	stamp;					/* See spilcommon.h */
	void_t	*sProtoResHandle;
	void_t	*localResHandle;		/* SPI_SERVICE_T structure */
	void_t	*spilTaskPtr;			/* ptr to the corresponding SPI_TASK_T */
} SPI_HANDLE_T;

#endif _KERNEL_HEADERS

#define NWslSetHandleSProtoResHandle( handle, resHandle ) \
	((SPI_HANDLE_T *)(handle))->sProtoResHandle = (void_t *)(resHandle)

#define NWslGetHandleSProtoResHandle( handle, resHandle ) \
	*(resHandle) = (void_t *)((SPI_HANDLE_T *)(handle))->sProtoResHandle

#define NWslSetHandleLocalResHandle( handle, resHandle ) \
	((SPI_HANDLE_T *)handle)->localResHandle = (void_t *)(resHandle)

#define NWslGetHandleLocalResHandle( handle, resHandle ) \
	*(resHandle) = (void_t *)((SPI_HANDLE_T *)(handle))->localResHandle

#define NWslGetHandleStamp( handle, stamp ) \
	*(stamp) = (void_t *)((SPI_HANDLE_T *)(handle))->stamp

#define NWslGetNSpaceObjectType( nameSpacePtr, objectType ) \
	*(objectType) = (int32 *)((NWSI_NAME_SPACE_T *)(nameSpacePtr))->nodeType

#define NWslGetServiceProtoHandle( servPtr, protoHandle ) \
	*(protoHandle) = (void_t *)((SPI_SERVICE_T *)(servPtr))->protoServicePtr

#define NWslSetServiceProtoHandle( servPtr, protoHandle ) \
	((SPI_SERVICE_T *)(servPtr))->protoServicePtr = (void_t *)(protoHandle)

#define NWslGetServiceOps( servPtr, ops ) \
	*(ops) = (void_t *)((SPI_SERVICE_T *)(servPtr))->spi_ops

#define NWslSetServiceOps( servPtr, ops ) \
	((SPI_SERVICE_T *)(servPtr))->spi_ops = (void_t *)(ops)

#define NWslGetServiceTaskList( servPtr, tList ) \
	*(tList) = (void_t *)((SPI_SERVICE_T *)(servPtr))->taskList

#ifdef FS_ONLY
#define NWslGetServiceTask( servPtr, credPtr, taskPtr, xautoFlags ) \
	NWslGetTask((servPtr), (credPtr), (taskPtr), (xautoFlags))
#endif /* FS_ONLY */

/*
 * NWslCheckAuthState assumes that the NWC_AUTH_STATE_NONE is defined to
 * be 0x0000
 */
#define NWslCheckAuthState( taskPtr ) \
	((ncp_channel_t *)((ncp_task_t *)((SPI_TASK_T*)taskPtr)->protoTaskPtr)->channelPtr)->authenticationState

#define NWslLinkTaskToService( servPtr, taskPtr ) \
	NWtlAddToSLList( ((SPI_SERVICE_T *)(servPtr))->taskList, (taskPtr) )

#define NWslGetTaskCredentials( taskHandle, credPtr ) \
	*(credPtr) = (void_t *)((SPI_TASK_T *)(taskHandle))->credentialsPtr

#define NWslSetTaskCredentials( taskHandle, credPtr ) \
	((SPI_TASK_T *)(taskHandle))->credentialsPtr = (void_t *)(credPtr)

#define NWslGetTaskService( taskHandle, serviceHandle ) \
	*(serviceHandle) = (void_t)((SPI_TASK_T *)(taskHandle))->spiServicePtr
	
#define NWslSetTaskService( taskHandle, serviceHandle ) \
	((SPI_TASK_T *)(taskHandle))->spiServicePtr = (void_t *)(serviceHandle)

#define NWslGetTaskProtoHandle( taskHandle, protoHandle ) \
	*(protoHandle) = ((SPI_TASK_T *)(taskHandle))->protoTaskPtr
	
#define NWslSetTaskProtoHandle( taskHandle, protoHandle ) \
	((SPI_TASK_T *)(taskHandle))->protoTaskPtr = (void_t *)(protoHandle)

#define NWslSetTaskMode( taskHandle, tmode ) \
	((SPI_TASK_T *)(taskHandle))->mode = (tmode)
	
#define NWslGetTaskMode( taskHandle, tmode ) \
	*(tmode) = ((SPI_TASK_T *)(taskHandle))->mode

#endif /* _NET_NUC_SPIL_SLSTRUCT_H */
