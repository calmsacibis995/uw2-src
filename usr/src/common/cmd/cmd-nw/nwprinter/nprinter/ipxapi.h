/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/ipxapi.h	1.3"
/*
 * Copyright 1989, 1991 Unpublished Work of Novell, Inc. All Rights Reserved.
 * 
 * THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL, 
 * PROPRIETARY AND TRADE SECRET INFORMATION OF NOVELL, INC. ACCESS
 * TO THIS WORK IS RESTRICTED TO (I) NOVELL EMPLOYEES WHO HAVE A
 * NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
 * ASSIGNMENTS AND (II) ENTITIES OTHER THAN NOVELL WHO HAVE
 * ENTERED INTO APPROPRIATE AGREEMENTS. 
 * NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 *
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/ipxapi.h,v 1.4 1994/08/18 16:27:10 vtag Exp $
 */
#ifndef __IPXAPI_H__
#define __IPXAPI_H__

#include <sys/nwportable.h>

/*
 * The following are for general purpose IPX use
 */

#define IPX_DRIVER_NAME			"/dev/ipx"
#define IPX_ADDRESS_LENGTH		12

#define IPXET_ERRNO			0
#define IPXET_T_ERRNO		1
#define IPXET_TLOOK			2
#define IPXET_USER			3

typedef uint8	IPXAddress_ta[IPX_ADDRESS_LENGTH];

typedef struct {
	int		  	  fd;
	int		  	  errnoType;
	int		  	  errno;
	IPXAddress_ta ipxAddress;
} IPXHandle_t;


/*
 * The following are used with IPXIsServerAdvertising
 */
#define IPXMAX_SERVER_NAME_LENGTH		48

#define IPX_SERVER_TYPE_WILD_CARD		0xFFFF

typedef struct {
	uint16		  serverType;
	char		  serverName[IPXMAX_SERVER_NAME_LENGTH];
	IPXAddress_ta ipxAddress;
	uint16		  hops;
} IPXsapServerInfo_t;

int IPXOpenTransport(
	IPXHandle_t	*ipxHandle,
	int16		 socket);
int IPXCloseTransport( IPXHandle_t	*ipxHandle);
int IPXIsServerAdvertising(
	IPXHandle_t			*ipxHandle,
	IPXsapServerInfo_t	*sapInfo);
void IPXGetSocketFromAddress(
	IPXAddress_ta ipxAddress,
	uint16		 *socket);
char * IPXDisplayAddress(
	IPXAddress_ta ipxAddress);
char * IPXDisplayErrno(
	IPXHandle_t	*ipxHandle);

#endif

