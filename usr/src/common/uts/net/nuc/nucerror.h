/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nucerror.h	1.16"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nucerror.h,v 2.53.2.5 1995/02/04 02:10:17 hashem Exp $"

#ifndef _NET_NUC_NUCERROR_H
#define _NET_NUC_NUCERROR_H

/*
 *  Netware Unix Client
 *
 *	MODULE:
 *		nucerror.h -The NetWare UNIX Client File System diagnostic
 *		codes used by the following:
 *
 *			Service Provider Interface layer (SPIL) 	
 *			NetWare File System layer (NWfs)
 *			Virtual File System layer (NWfi)
 *			Generic Transport Services layer (GTS)
 *			Generic Interprocess Communication layer (GIPC)
 *			Various NetWare Management Portal (NWMP) users
 *
 *	ABSTRACT:
 *		This header file is included by various layers (listed above)
 *		and by user-space code to provide a consistent representation 
 *		and a central repository of diagnostic codes passed between layers.
 *
 *		The enum type NUC_DIAG is used to type the variable used to contain
 *		diagnostic codes passed between layers.
 */

#define NUCFS_RANGE 1024		/* NUCFS Range is 1024-2047 */
#define SPIL_RANGE  2048		/* SPIL  Range is 2048-3071 */
#define GTS_RANGE   3072		/* GIPC  Range is 3072-4095 */
#define GIPC_RANGE  4096		/* GIPC  Range is 4096-5119 */

/*
 *	NetWare UNIX Client Diagnostics
 */
enum NUC_DIAG {
	/*
	 *  Common diagnostics
	 */

	/*
	 * NUCFS Diagnostics.
	 */

	NUCFS_ACCESS_DENIED		=	NUCFS_RANGE,	/*	1024	0x400	*/
	NUCFS_ALLOC_MEM_FAILED,						/*	1025	0x401	*/
	NUCFS_ALLOCATE_NODE_FAILED,					/*	1026	0x402	*/
	NUCFS_COPY_IN_FAILED,						/*	1027	0x403	*/
	NUCFS_COPY_OUT_FAILED,						/*	1028	0x404	*/
	NUCFS_DIRECTORY_NOT_EMPTY,					/*	1029	0x405	*/
	NUCFS_DUP_CRED_FAILED,						/*	1030	0x406	*/
	NUCFS_DOT_NODE_NAME,						/*	1031	0x407	*/
	NUCFS_INVALID_BLOCK_NUMBER,					/*	1032	0x408	*/
	NUCFS_INVALID_DATA,							/*	1033	0x409	*/
	NUCFS_INVALID_LOCK,							/*	1034	0x40a	*/
	NUCFS_INVALID_NAME,							/*	1035	0x40b	*/
	NUCFS_INVALID_NODE_TYPE,					/*	1036	0x40c	*/
	NUCFS_INVALID_OFFSET,						/*	1037	0x40d	*/
	NUCFS_INVALID_SIZE,							/*	1038	0x40e	*/
	NUCFS_NODE_ALREADY_EXISTS,					/*	1039	0x40f	*/
	NUCFS_NODE_IS_DIRECTORY,					/*	1040	0x410	*/
	NUCFS_NODE_NOT_FOUND,						/*	1041	0x411	*/
	NUCFS_FILE_WAS_OPENED,						/*	1042	0x412	*/
	NUCFS_NOT_A_DIRECTORY,						/*	1043	0x413	*/
	NUCFS_NOT_SAME_VOLUME,						/*	1044	0x414	*/
	NUCFS_OPEN_FAILED,							/*	1045	0x415	*/
	NUCFS_VOLUME_BUSY,							/*	1046	0x416	*/
	NUCFS_VOLUME_IS_READ_ONLY,					/*	1047	0x417	*/
	NUCFS_VOLUME_NOT_FOUND,						/*	1048	0x418	*/
	NUCFS_STALE,								/*	1049	0x419	*/
	NUCFS_EIO,									/*	1050	0x41a	*/
	NUCFS_NOT_CHILD,							/*	1051	0x41b	*/
	NUCFS_PROTOCOL_ERROR,						/*	1052	0x41c	*/

	/*
	 * SPIL Diagnostics.
	 */

	SPI_ACCESS_DENIED	=	SPIL_RANGE,			/*	2048	0x800	*/
	SPI_AUTHENTICATION_FAILURE,					/*	2049	0x801	*/
	SPI_DEAUTHENTICATION_FAILURE,				/*	2050	0x802	*/
	SPI_BAD_HANDLE,								/*	2051	0x803	*/
	SPI_BAD_BYTE_RANGE,							/*	2052	0x804	*/
	SPI_CANT_OPEN_DIRECTORY,					/*	2053	0x805	*/
	SPI_CLIENT_RESOURCE_SHORTAGE,				/*	2054	0x806	*/
	SPI_CROSS_DIRECTORY_RENAME,					/*	2055	0x807	*/
	SPI_DIRECTORY_FULL,							/*	2056	0x808	*/
	SPI_DIRECTORY_NOT_EMPTY,					/*	2057	0x809	*/
	SPI_FILE_ALREADY_EXISTS,					/*	2058	0x80a	*/
	SPI_FILE_IN_USE,							/*	2059	0x80b	*/
	SPI_FILE_TOO_BIG,							/*	2060	0x80c	*/
	SPI_GENERAL_FAILURE,						/*	2061	0x80d	*/
	SPI_INACTIVE,								/*	2062	0x80e	*/
	SPI_INVALID_OFFSET,							/*	2063	0x80f	*/
	SPI_INVALID_PATH,							/*	2064	0x810	*/
	SPI_INVALID_SPROTO,							/*	2065	0x811	*/
	SPI_LOCK_COLLISION,							/*	2066	0x812	*/
	SPI_LOCK_ERROR,								/*	2067	0x813	*/
	SPI_LOCK_SHORTAGE,							/*	2068	0x814	*/
	SPI_LOCK_TIMEOUT,							/*	2069	0x815	*/
	SPI_MEMORY_EXHAUSTED,						/*	2070	0x816	*/
	SPI_MEMORY_FAILURE,							/*	2071	0x817	*/
	SPI_NAME_TOO_LONG,							/*	2072	0x818	*/
	SPI_NODE_IS_DIRECTORY,						/*	2073	0x819	*/
	SPI_NODE_NOT_DIRECTORY,						/*	2074	0x81a	*/
	SPI_NODE_NOT_FOUND,							/*	2075	0x81b	*/
	SPI_NO_MORE_DIR_HANDLES,					/*	2076	0x81c	*/
	SPI_NO_MORE_ENTRIES,						/*	2077	0x81d	*/
	SPI_NO_MORE_SERVICE,						/*	2078	0x81e	*/
	SPI_NO_MORE_TASK,							/*	2079	0x81f	*/
	SPI_NO_PERMISSIONS,							/*	2080	0x820	*/
	SPI_NO_SUCH_DIRECTORY,						/*	2081	0x821	*/
	SPI_NO_SUCH_SERVICE,						/*	2082	0x822	*/
	SPI_NO_SUCH_TASK,							/*	2083	0x823	*/
	SPI_NO_SUCH_VOLUME,							/*	2084	0x824	*/
	SPI_REQUEST_NOT_SUPPORTED,					/*	2085	0x825	*/
	SPI_SERVER_FAILURE,							/*	2086	0x826	*/
	SPI_SERVER_RESOURCE_SHORTAGE,				/*	2087	0x827	*/
	SPI_SERVER_UNAVAILABLE,						/*	2088	0x828	*/
	SPI_SERVICE_BUSY,							/*	2089	0x829	*/
	SPI_SERVICE_EXISTS,							/*	2090	0x82a	*/
	SPI_TASK_EXISTS,							/*	2091	0x82b	*/
	SPI_TASK_HOLDING_RESOURCES,					/*	2092	0x82c	*/
	SPI_TASK_TERMINATING,						/*	2093	0x82d	*/
	SPI_SET_NAME_SPACE_DENIED,					/*	2094	0x82e	*/
	SPI_TOO_MANY_LINKS,							/*	2095	0x82f	*/
	SPI_USER_MEMORY_FAULT,						/*	2096	0x830	*/
	SPI_MORE_ENTRIES_EXIST,						/*	2097	0x831	*/
	SPI_INTERRUPTED,							/*	2098	0x832	*/
	SPI_BAD_CONNECTION,							/*	2099	0x833	*/
	SPI_NO_CONNECTIONS_AVAILABLE,				/*	2100	0x834	*/
	SPI_SERVER_DOWN,							/*	2101	0x835	*/
	SPI_TYPE_NOT_SUPPORTED,						/*	2102	0x836	*/
	SPI_BAD_ARGS,								/*	2103	0x837	*/
	SPI_NLM_NOT_LOADED,							/*	2104	0x838	*/
	SPI_BAD_ARGS_TO_SERVER,						/*	2105	0x839	*/
	SPI_NO_ACTUAL_SIZE,							/*	2106	0x83a	*/
	SPI_OUT_OF_DISK_SPACE,						/*	2107	0x83b	*/
	SPI_INVALID_MOVE,							/*	2108	0x83c	*/

	/*
	 * GTS Diagnostics.
	 */

	NWD_GTS_ADDRESS_INUSE = GTS_RANGE, /* Local EndPoint Addr in use 0xc00 */
	NWD_GTS_BAD_ADDRESS,		/* Bad EndPoint Address		 		0xc01	*/
	NWD_GTS_BAD_STACK,			/* Unknown Stack		 			0xc02	*/
	NWD_GTS_BLOCKING,			/* Transport is blocking	 		0xc03	*/
	NWD_GTS_BUF_FAULT,			/* User Mode Buffer EFAULT       	0xc04	*/
	NWD_GTS_CONNECTED,			/* EndPoint is already connected 	0xc05	*/
	NWD_GTS_CON_REFUSED,		/* Peer refused connect request	 	0xc06	*/
	NWD_GTS_MSG_OVERFLOW,		/* Receive Buffers to Small	 		0xc07	*/
	NWD_GTS_NO_CON_PEND,		/* No Con Pending to Accept	 		0xc08	*/
	NWD_GTS_NO_ENDPOINT,		/* GTS Endpoint not opened	 		0xc09	*/
	NWD_GTS_NO_MESSAGE,			/* No message availabe		 		0xc0a	*/
	NWD_GTS_NO_RESOURCE,		/* Resource can't be allocated		0xc0b	*/
	NWD_GTS_NO_STACK,			/* Stack not Configured		 		0xc0c	*/
	NWD_GTS_NOT_CONNECTED,		/* GTS Endpoint not connected	 	0xc0d	*/
	NWD_GTS_TIMED_OUT,			/* GTS Endpoint Timed Out	 		0xc0e	*/
	NWD_GTS_WRONG_MODE,			/* Requestor on Responder or vv	 	0xc0f	*/
	NWD_GTS_PB_ALL_FRAGS_RECEIVED,	/* All PB frags rcvd for reply	0xc10	*/
	NWD_GTS_REPLY_TIMED_OUT,	/* Reply timed out, re xmit req (PB) 0xc11	*/

	/*
	 * GIPC Diagnostics.
	 */

	NWD_GIPC_BAD_IPC = GIPC_RANGE,	/* Unknown IPC Mechanism	0x1000	*/
	NWD_GIPC_BAD_PEER, 		/* Peer process name unknown		0x1001	*/
	NWD_GIPC_BIG_MSG,		/* Send message exceeds IPC lim		0x1002	*/
	NWD_GIPC_BLOCKING,		/* IPC mechanism is blockinga		0x1003	*/
	NWD_GIPC_BUF_FAULT,		/* User Mode Buffer EFAULT			0x1004	*/
	NWD_GIPC_IOCTL_FAIL,	/* Private IOCTL not completed		0x1005	*/
	NWD_GIPC_NO_CHANNEL,	/* GIPC Channel not opened			0x1006	*/
	NWD_GIPC_NO_IPC	,		/* IPC Mechanism not configured		0x1007	*/
	NWD_GIPC_NO_MESSAGE,	/* No message availabe				0x1008	*/
	NWD_GIPC_NO_RESOURCE,	/* Resource can't be allocateda		0x1009	*/
	NWD_GIPC_PEER_REJECT	/* Peer process rejected open		0x100a	*/

};

#endif /* _NET_NUC_NUCERROR_H */
