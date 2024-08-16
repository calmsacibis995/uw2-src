/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/npsd/npsd.h	1.10"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: npsd.h,v 1.8 1994/09/22 21:27:16 vtag Exp $"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#ifndef _NPSD_NPSD_H_
#define _NPSD_NPSD_H_

#include "nwmsg.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <stropts.h>
#include <malloc.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <memory.h>
#include <errno.h>

#include <sys/nwportable.h>
#include "nps.h"
#include <sys/nwtdr.h>
#include <sys/ipx_app.h>
#include <sys/ripx_app.h>
#include <sys/spx_app.h>
#include <sys/sap_app.h>
#include "nwconfig.h"
#include "npsmsgtable.h"
#include "util_proto.h"

/*
 * Constants of General Utility
 */
#define DEFAULT_TIMEOUT 5	/* for I_STR */

/*
** Strings for Constructing Tokens
*/
#define LAN_TOKEN_LEADER	"lan_"
#define NETWORK_TOKEN		"_network"
#define ADAPTER_TOKEN		"_adapter"
#define ADAPTER_TYPE_TOKEN	"_adapter_type"
#define SPEED_TOKEN			"_kb_per_sec"
#define RIP_AGE_TOKEN		"_rip_ageout_intervals"
#define RIP_BCST_TOKEN		"_rip_bcast_intervals"
#define RIP_GAP_TOKEN		"_rip_inter_pkt_delay"
#define RIP_MSIZE_TOKEN		"_rip_max_pkt_size"
#define RIP_CHG_TOKEN		"_rip_send_changed_only"
#define SAP_AGE_TOKEN		"_sap_ageout_intervals"
#define SAP_BCST_TOKEN		"_sap_bcast_intervals"
#define SAP_GAP_TOKEN		"_sap_inter_pkt_delay"
#define SAP_MSIZE_TOKEN		"_sap_max_pkt_size"
#define SAP_CHG_TOKEN		"_sap_send_changed_only"
#define SAP_RPLY_GNS_TOKEN	"_sap_reply_to_gns"

#define DEFAULT_LAN_SPEED		10000	/* 10 meg */
#define DEFAULT_BCAST_INTVL		2		/* 2 30 sec intervals */
#define DEFAULT_AGE_INTVL		4		/* 4 DEFAULT_BCAST_INTVLs 4min */
#define DEFAULT_GAP				55		/* 55 millisec */
#define DEFAULT_SEND_CHG		0		/* Send Change ONLY inactive */
#define DEFAULT_RIP_MSIZE		432		/* Max packet size for RIP */
#define DEFAULT_SAP_MSIZE		480		/* Max packet size for SAP */
#define DEFAULT_SAP_RPLY_GNS	SAP_REPLY_GNS	/* Sap always replies to GNS */

extern int spxFd;
extern int nbFd;
extern int ipx0Fd;
extern int lipmxFd;
extern int ripxFd;


/*
 * Structure for map between adapter type keyword and function.
 */
typedef struct {
	int (*function)(int, lanInfo_t *, int, char *);
	char *name;
} a_functionMap_t;

/*
 * Globals
 */
extern char Verbose;
extern char titleStr[];
extern int	Cret;
extern int	etalkFd;
/*
**  DPRINTF prints only if -d option is set
*/
#ifdef DEBUG
extern int debugopt;
#define DPRINTF(arg) if(debugopt) printf arg
#else
#define DPRINTF(arg)
#endif

/*
 * Other Stuff
 */
#ifndef MIN
#define	MIN(a,b)	(((a)<(b))?(a):(b))
#endif

/*
**	Forward declarations
*/

extern int NVTFork( char *);
extern int NVTConfigure( char *);
extern int NVTDeconfigure( char *);
extern int doStrIoctl(int, int, char *, int, int);

extern void NPSExit(char *);
int doStrIoctl( int, int, char *, int, int);
#endif /* _NPSD_NPSD_H_ */
