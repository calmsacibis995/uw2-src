/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nuc.cf/Space.c	1.13"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nuc.cf/Space.c,v 2.52.2.3 1995/02/12 23:39:58 hashem Exp $"

/*
 *
 *  Netware Unix Client 
 *
 *  MODULE:  space.c (nwmp) Netware Unix Client management portal
 *
 *  ABSTRACT:  Include any tuneable parameters
 *
 */

#define	_SPACE_C
#define ALLOCATE_GTS_CONF
#define ALLOCATE_GIPC_CONF
#define	ALLOCATE_STREAMS_CONF
#define FS_ONLY


#include <sys/types.h>
#include <sys/param.h>
#include <sys/cred.h>
#include <sys/time.h>
#include <sys/stream.h>

#include <config.h>

#include <sys/nwctypes.h>
#include <sys/nwncptune.h>
#include <sys/nwncpconf.h>

#include <sys/nuctool.h>
#include <sys/nwspiswitch.h>
#include <sys/nwncpspi.h>	/* formerly included by nwspiconf.h */
#include <sys/nwspi.h>

#include <sys/ipxengtune.h>

#include <sys/nwstr_tune.h>
#include <sys/strchannel.h>

#include <sys/nwmpdev.h>	/* formerly included by nwmptune.h */
#include <sys/nwmptune.h>

#include <sys/ddi.h>

long nwmpTunedMinorDevices = NUCNWMPOPENS;


struct ncpConfStruct ncpConf = {
	NCP_MIN_MEM_SIZE,
	NCP_MAX_MEM_SIZE,
	NCP_DEF_MEM_SIZE,
	NCP_MEM_SIZE,
	NCP_MAX_SERVERS,
	NCP_MAX_TASKS_PER_SERVER,
	NCP_IOBUFFERS_PER_TASK
};

char transportDevicePath[]="/dev/ipx";


/*
 *	if the kit has been configured for dynamic allocation,
 *	only declare a pointer
 */

#define ENABLED	 (int)-1
#define DISABLED (int)0
#define NULL 0

/*
 *	Define the possible service protocol modules
 */
#ifdef NCP_MODULE
extern SPI_OPS_T		ncp_spiops;
#endif

#ifdef NCPII_MODULE
extern SPI_OPS_T		ncpII_spiops;
#endif

#ifdef ALT_MODULE
extern SPI_OPS_T 	alt_spiops;
#endif


/*
 *	Assign the switch table based upon the service protocol modules that
 *	are installed
 */
SPI_SWITCH_T SPISwitch[] = {

#ifdef NCP_MODULE
	ENABLED, &ncp_spiops,
#else
	DISABLED, (SPI_OPS_T *)NULL,
#endif

	/*
	 *	Add NCPII module to the switch table if it's configured in...
	 */
#ifdef NCPII_MODULE
	ENABLED, &ncpII_spiops,
#else
	DISABLED, (SPI_OPS_T *)NULL,
#endif

	/*
	 *	Add an ALTERNATE PROTOCOL module to the switch table if 
	 *	it's configured in...
	 */
#ifdef ALT_MODULE
	ENABLED, &alt_spiops;
#else
	DISABLED, (SPI_OPS_T *)NULL,
#endif

};


/*
 * Estimated bytes of tool kit overhead for memory allocation & linked lists
 */
#define	NWSTR_TL_OVERHEAD	64

/*
 * Calculate the number of STREAMS Channel Objects
 */
#define	NWSTR_OBJECTS		((EST_STR_SERVERS * CHANNELS_PER_CLIENT) \
				 * EST_VIRTUAL_CLIENTS)

/*
 * Calculate the space requirements of the STREAMS Channels & tool kit overhead
 * per object
 */
#define	NWSTR_OBJECT_SPACE	(NWSTR_OBJECTS * sizeof(STR_CHANNEL_T))

#define	NWSTR_TL_SPACE		((1+NWSTR_OBJECTS) * NWSTR_TL_OVERHEAD)

/*
 * Calculate the total region needed by NWSTR layer
 */
#define	NWSTR_REGION_SIZE	(NWSTR_OBJECT_SPACE + NWSTR_TL_SPACE)

/*
 * Define the ALLOCATE_STREAMS_CONF, so when headstrconf.h is included,
 * the NetWare STREAM Head Memory Region Space NWstrMemSpace[] is allocated, and
 * the NWstrHeadTune structure is allocated and initialzed.  Also when the
 * streamsconf.h is included, the STREAMS Operations structure NWstreamsOps
 * is allocated and inititalized, which pluges it into the Generic Inter Process
 * Communication Operations Switch NWgipcOpsSw[RITCHITE_STREAMS].
 */


/*
 * Allocate & Initialize data structures in "headstrconf.h"
 */
#include <sys/headstrconf.h>

/*
 * Allocate & Initialize data structures in "streamsconf.h"
 */
#include <sys/streamsconf.h>

#include <sys/gtsconf.h>
#include <sys/gipcconf.h>


/*
 *	Semaphore space and tuneables
 */
#define NUC_SEMAPHORES (NUCLOGINS * NUCUSERS * 9 + 32)
long nucSemaphoreCount = NUC_SEMAPHORES;

nucSema_t semaTable[NUC_SEMAPHORES];

/*
 * Define the default security level
 */
uint32 defaultSecurityLevel = 0x01;
uint32 defaultChecksumLevel = 0x01;

uint8  maxSignatureRetries  = 0x03;

uint32			doPacketBurst = B_FALSE;
/*
 *	Packet Burst Inter Packet Gap ( 100 usec. ).
 */
int PacketBurstIPG = PB_IPG;
