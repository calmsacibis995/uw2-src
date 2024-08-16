/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/netmgt/include/nwsmux.h	1.1"
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
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/netmgt/include/nwsmux.h,v 1.1 1994/02/03 00:10:22 rbell Exp $
 */

/* filename: nwsmux.h */

#ifndef _NW_SMUX_H_
#define _NW_SMUX_H_

/* SMUX packets */
#define NWtype_SNMP_SMUX__PDUs \
  type_SNMP_SMUX__PDUs

/* SMUX packets offsets */
#define	NWtype_SNMP_SMUX__PDUs_simple \
  type_SNMP_SMUX__PDUs_simple	

#define	NWtype_SNMP_SMUX__PDUs_close \
  type_SNMP_SMUX__PDUs_close

#define	NWtype_SNMP_SMUX__PDUs_registerRequest	\
  type_SNMP_SMUX__PDUs_registerRequest   

#define	NWtype_SNMP_SMUX__PDUs_registerResponse \
  type_SNMP_SMUX__PDUs_registerResponse

#define	NWtype_SNMP_SMUX__PDUs_get__request \
  type_SNMP_SMUX__PDUs_get__request

#define	NWtype_SNMP_SMUX__PDUs_get__next__request \
  type_SNMP_SMUX__PDUs_get__next__request 
  
#define	NWtype_SNMP_SMUX__PDUs_get__response \
  type_SNMP_SMUX__PDUs_get__response

#define	NWtype_SNMP_SMUX__PDUs_set__request \
  type_SNMP_SMUX__PDUs_set__request 

#define	NWtype_SNMP_SMUX__PDUs_trap \
  type_SNMP_SMUX__PDUs_trap

#define	NWtype_SNMP_SMUX__PDUs_commitOrRollback \
  type_SNMP_SMUX__PDUs_commitOrRollback 

#define	NWfree_SNMP_SMUX__PDUs(parm) \
	(void) fre_obj((char *) parm, _ZSNMP_mod.md_dtab[_ZSMUX_PDUsSNMP], &_ZSNMP_mod, 1)

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



#endif 