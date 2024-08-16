/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/headstrconf.h	1.10"
#ifndef _NET_NUC_STREAMS_HEADSTRCONF_H
#define _NET_NUC_STREAMS_HEADSTRCONF_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/headstrconf.h,v 2.51.2.1 1994/12/12 01:22:39 stevbam Exp $"

/*
 *  Netware Unix Client 
 *
 *	MODULE:
 *   	 headstrconf.h -The NUC STREAMS Mechanism IPC Head
 *			Service Package	configuration.  Component of the NUC
 *			Core Services Device.
 *
 *	ABSTRACT:
 *		The headstrconf.h is included with the NW STREAMS
 *		Head Service Package.  It defines the NW STREAMS
 *		Head manifest constants and data structures.  It is included
 *		in all variant NW STREAMS Head modules.
 *
 */

/*
 * The Release level of the NUC STREAMS Head
 */
#define	NWSTR_MAJOR_VERSION	3
#define	NWSTR_MINOR_VERSION	12

/*
 * Private NW Stream Head Constants
 */
#define	NOT_A_MODULE	0			/* Module ID of NW Head	*/
#define	READ_HEAD_NAME	"NWstrReadHead"		/* Name of NW Read Head	*/
#define	WRITE_HEAD_NAME	"NWstrWriteHead"	/* Name of NW Write Head*/

#define IPXHANGUP_TIMEOUT	HZ	 
#define	IOCTL_TIMEOUT		HZ	/* Second IOCTL timeout	*/
#define	MAX_ALLOC_RETRY		3		/* allocMsgBlock(3K) lim*/
#define	DEFAULT_MAX_MSG_SIZE	0xFFFFL		/* 64K if no strmsgsz	*/

#define	PEER_IS_OPEN	1		/* Driver open on qdetach()	*/
#define	NO_FLAGS	0		/* Flags on qdetach()		*/

/*
 * NAME
 *	NWstrHeadTune -	The tuneable parameters of the NUC STREAMS Head
 *			Package.
 *
 * DESCRIPTION
 *	This data structure defines the Tuneable Parameters of the NUC
 *	STREAMS Head Package.  This strucutre is initialized with
 *	the tuneable parameters found in "nwstr_tune.h", which can be
 *	modified by the system administrator when a new kernel is built.  Once
 *	initialized, this strucutre conveys the tune of the NUC STREAMS Head
 *	for the active UNIX kernel.
 */
typedef	struct {
	uint32	servers;		/* Servers reached by STREAMS	*/
	uint32	virtualClients;		/* Core Service Virtual Clients	*/
	uint32	channels;		/* Channel per Client		*/
	uint32	memSize;		/* NUC STREAM Head Region Size	*/
} STR_HEAD_TUNE_T;



#ifdef ALLOCATE_STREAMS_CONF	/* Declared in NWstrSpace.c */

/*
 * Declare and intialize NW STREAMS HEAD data structures
 */

/*
 * Declare the Queue Initializtion Structures for use with NW STREAM HEAD
 * Read and Write Queue Instatiations.
 */
static	struct	module_info	NWreadHeadInfo = 
	{NOT_A_MODULE, READ_HEAD_NAME, 0, INFPSZ, STRHIGH, STRLOW};

static	struct	module_info	NWwriteHeadInfo =
	{NOT_A_MODULE, WRITE_HEAD_NAME, 0, INFPSZ, STRHIGH, STRLOW};

extern	int	NWstrReadHeadPut();
extern	int	NWstrReadHeadService();

struct	qinit	NWreadHeadInit =
	{NWstrReadHeadPut, NWstrReadHeadService, NULL, NULL, NULL,
		&NWreadHeadInfo, NULL};

extern	int	NWstrWriteHeadService();

struct	qinit	NWwriteHeadInit =	
	{NULL, NWstrWriteHeadService, NULL, NULL, NULL, &NWwriteHeadInfo, NULL};

/*
 * Allocate & Initialize NUC STREAMS Head Global Variables
 */
int32		NWstrInitialized = FALSE;
opaque_t	*NWstrChannelList;

/*
 * Allocate & Initialize NUC STREAM Head Tune Parameters
 */
STR_HEAD_TUNE_T	NWstrHeadTune = {
	EST_STR_SERVERS,
	EST_VIRTUAL_CLIENTS,
	CHANNELS_PER_CLIENT,
	NWSTR_REGION_SIZE
};

#else

/*
 * Reference NW STREAMS HEAD data strucutures
 */
extern	struct	qinit	NWreadHeadInit;
extern	struct	qinit	NWwriteHeadInit;

/*
 * Also Reference the CLONE device for we need to test for it!!
 */
extern	int	cloneopen();

/*
 * Reference NUC STREAMS Head Global Variables
 */
extern	int32		NWstrInitialized;	/* Is Head Running	*/
extern	opaque_t	*NWstrChannelList;	/* List Handle		*/
extern	STR_HEAD_TUNE_T	NWstrHeadTune;

#endif                       /* ALLOCATE_STREAMS_CONF	*/

#endif /* _NET_NUC_STREAMS_HEADSTRCONF_H */
