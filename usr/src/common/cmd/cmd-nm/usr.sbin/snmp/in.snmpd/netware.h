/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nm:common/cmd/cmd-nm/usr.sbin/snmp/in.snmpd/netware.h	1.2"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nm/usr.sbin/snmp/in.snmpd/netware.h,v 1.4 1994/05/24 17:42:02 cyang Exp $"

/*
 * This header contains various constants and data structures needed 
 * for using the NetWare protocol (IPX).
 */

#ifndef _SNMP_NETWARE_H
#define _SNMP_NETWARE_H

#include "sys/ipx_app.h"

#define IPX_PACKET_TYPE		IPX_PEP_PACKET_TYPE
#define IPX_MAX_DATA		546

#endif /* _SNMP_NETWARE_H */
