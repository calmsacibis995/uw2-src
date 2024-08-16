/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/netmgt/include/nwsmuxsvr4.h	1.1"
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
 *
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/netmgt/include/nwsmuxsvr4.h,v 1.1 1994/02/03 00:10:23 rbell Exp $
 */

/* filename: nwsmuxsvr4.h  for UNIXWARE */

#ifndef _NW_SMUX_H_
#define _NW_SMUX_H_

/* SMUX packets */
#define NWtype_SNMP_SMUX__PDUs \
  type_SNMP_SMUX__PDUs

/* SMUX packets offsets */
#define	NWtype_SNMP_SMUX__PDUs_simple \
  SMUX__PDUs_simple	

#define	NWtype_SNMP_SMUX__PDUs_close \
  SMUX__PDUs_close

#define	NWtype_SNMP_SMUX__PDUs_registerRequest	\
  SMUX__PDUs_registerRequest   

#define	NWtype_SNMP_SMUX__PDUs_registerResponse \
  SMUX__PDUs_registerResponse

#define	NWtype_SNMP_SMUX__PDUs_get__request \
  SMUX__PDUs_get__request

#define	NWtype_SNMP_SMUX__PDUs_get__next__request \
  SMUX__PDUs_get__next__request 
  
#define	NWtype_SNMP_SMUX__PDUs_get__response \
  SMUX__PDUs_get__response

#define	NWtype_SNMP_SMUX__PDUs_set__request \
  SMUX__PDUs_set__request 

#define	NWtype_SNMP_SMUX__PDUs_trap \
  SMUX__PDUs_trap

#define	NWtype_SNMP_SMUX__PDUs_commitOrRollback \
  SMUX__PDUs_commitOrRollback 

#define	NWfree_SNMP_SMUX__PDUs \
	free_SNMP_SMUX_pdu

/*****************************************************************************/

#define NWsmux_init \
        smux_init

/*****************************************************************************/
#define NWsmux_simple_open \
        smux_simple_open

/*****************************************************************************/
#define NWsmux_close(reason) smux_close((reason))

/*****************************************************************************/
#define NWsmux_register(subtree,priority,operation) \
  smux_register((subtree), (priority), (operation))

/*****************************************************************************/
#define NWsmux_response(event) smux_response((event))

/*****************************************************************************/

#define NWsmux_wait(event,secs) smux_wait((event),(secs))

/*****************************************************************************/
#define NWsmux_trap(generic, specific, bindings) \
  smux_trap((generic),(specific),(bindings))

/*****************************************************************************/
#define NWsmux_error(i) smux_error((i))

/*****************************************************************************/
/* smuxEntry structure */
#define NWsmuxEntry smuxEntry

/*****************************************************************************/
#define NWsetsmuxEntry(f)  setsmuxEntry((f))

/*****************************************************************************/
#define NWendsmuxEntry()     endsmuxEntry()

/*****************************************************************************/
#define NWgetsmuxEntry()               getsmuxEntry()

/*****************************************************************************/
#define NWgetsmuxEntrybyname(name) getsmuxEntrybyname((name))

/*****************************************************************************/

#define NWgetsmuxEntrybyidentity(oid)  getsmuxEntrybyidentity((oid))


#define int_SNMP_error__status_noSuchName error__status_noSuchName
#define int_SNMP_error__status_noError error__status_noError
#define int_SNMP_error__status_genErr  error__status_genErr

#define int_SNMP_RRspPDU_failure RRspPDU_failure

#define int_SNMP_generic__trap_coldStart trap_coldStart

#define int_SNMP_generic__trap_enterpriseSpecific trap_enterpriseSpecific

#define int_SNMP_SOutPDU_commit SOutPDU_commit

#endif 

