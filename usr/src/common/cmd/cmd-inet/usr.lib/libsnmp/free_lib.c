/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.lib/libsnmp/free_lib.c	1.1"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ifndef lint
static char SNMPID[] = "@(#)free_lib.c	6.2 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */


#include <stdio.h>

#include <snmp/snmp.h>


/* free the data structures allocated and built by make_octetstring  */
void
free_octetstring(os_ptr)
	OctetString *os_ptr;
{
	if (os_ptr != NULL) {
		if (os_ptr->octet_ptr != NULL) {
			free(os_ptr->octet_ptr);
			NULLIT(os_ptr->octet_ptr);
		}
		free(os_ptr);
	}
}

/* free the data structures allocated and built by make_obj_id_from_dot etc  */
void
free_oid(oid_ptr)
	OID oid_ptr;
{
	if (oid_ptr != NULL) {
		if (oid_ptr->oid_ptr != NULL) {
			free((char *)oid_ptr->oid_ptr);
			NULLIT(oid_ptr->oid_ptr);
		}
		free((char *)oid_ptr);
	}
}

/*  free the data structures allocated and built by make_varbind  */
void
free_varbind(vb_ptr)
	VarBindUnit *vb_ptr;
{
	if (vb_ptr != NULL) {
		free_oid(vb_ptr->name);
		free_oid(vb_ptr->value->oid_value);
		free_octetstring(vb_ptr->value->os_value);
		free((char *)vb_ptr);
	}
}

/*  free the data structures allocated and built by make_pdu  */
void
free_pdu(pdu_ptr)
	Pdu *pdu_ptr;
{
	if (pdu_ptr != NULL) {
		free_octetstring(pdu_ptr->packlet);
		if (pdu_ptr->type == TRAP_TYPE) {
			free_oid(pdu_ptr->u.trappdu.enterprise);
			free_octetstring(pdu_ptr->u.trappdu.agent_addr);
		}
		free_varbind_list(pdu_ptr->var_bind_list);
		free(pdu_ptr);
	}
}

/*  free the date structures allocated and built by make_varbind  */
void
free_varbind_list(vbl_ptr)
	VarBindList *vbl_ptr;
{
	if (vbl_ptr != NULL) {
		free_varbind_list(vbl_ptr->next);
		free_varbind(vbl_ptr->vb_ptr);
		free(vbl_ptr);
	}
}

/*  free data structures allocated for the values of the objects */
void 
free_value(val_ptr)
	ObjectSyntax *val_ptr;
{
	if (val_ptr != NULL) {
		switch (val_ptr->type) {
		case OBJECT_ID_TYPE:
			free_oid(val_ptr->oid_value);
			break;
		case OCTET_PRIM_TYPE:
		case OCTET_CONSTRUCT_TYPE:
		case IP_ADDR_PRIM_TYPE:
		case IP_ADDR_CONSTRUCT_TYPE:
		case OPAQUE_PRIM_TYPE:
		case OPAQUE_CONSTRUCT_TYPE:
			free_octetstring(val_ptr->os_value);
			break;
		default:
			break;
		}
		free((char *)val_ptr);
	}
}

/* Routines for freeing the SMUX pdus */
void 
free_SMUX_SimpleOpen(open_ptr)
	SMUX_SimpleOpen *open_ptr;
{
	if (open_ptr != NULL) {
		free_oid(open_ptr->identity);
		free_octetstring(open_ptr->description);
		free_octetstring(open_ptr->password);
		free((char *)open_ptr);
	}
}

void 
free_SMUX_ClosePDU(close_ptr)
	SMUX_ClosePDU *close_ptr;
{
	if (close_ptr != NULL)
		free((char *)close_ptr);
}

void 
free_SMUX_RReqPDU(rreq_ptr)
	SMUX_RReqPDU *rreq_ptr;
{
	if (rreq_ptr != NULL) {
		free_oid(rreq_ptr->subtree);
		free((char *)rreq_ptr);
	}
}

void 
free_SMUX_RRspPDU(rrsp_ptr)
	SMUX_RRspPDU *rrsp_ptr;
{
	if (rrsp_ptr != NULL)
		free((char *)rrsp_ptr);
}

void 
free_SMUX_GetPDU(get_ptr)
	SMUX_Data_PDU *get_ptr;
{
	if (get_ptr != NULL) {
		free_varbind_list(get_ptr->variable__bindings);
		free((char *)get_ptr);
	}
}

void 
free_SMUX_TrapPDU(trap_ptr)
	SMUX_Trap_PDU *trap_ptr;
{
	if (trap_ptr != NULL) {
		free_oid(trap_ptr->enterprise);
		free_octetstring(trap_ptr->agent__addr);
		free_varbind_list(trap_ptr->variable__bindings);
		free((char *)trap_ptr);
	}
}

void 
free_SMUX_SOutPDU(sout_ptr)
	SMUX_SOutPDU *sout_ptr;
{
	if (sout_ptr != NULL)
		free((char *)sout_ptr);
}

void 
free_SNMP_SMUX_PDU(pdu_ptr)
	SNMP_SMUX_PDU *pdu_ptr;
{
	if (pdu_ptr != NULL) {
		if (pdu_ptr->packlet != NULL)
			free_octetstring(pdu_ptr->packlet);

		switch (pdu_ptr->offset) {

		case SMUX__PDUs_simple:
			free_SMUX_SimpleOpen(pdu_ptr->un.simple);
			break;

		case SMUX__PDUs_close:
			free_SMUX_ClosePDU(pdu_ptr->un.close);
			break;

		case SMUX__PDUs_registerRequest:
			free_SMUX_RReqPDU(pdu_ptr->un.registerRequest);
			break;

		case SMUX__PDUs_registerResponse:
			free_SMUX_RRspPDU(pdu_ptr->un.registerResponse);
			break;

		case SMUX__PDUs_get__request:
		case SMUX__PDUs_get__next__request:
		case SMUX__PDUs_get__response:
		case SMUX__PDUs_set__request:
			free_SMUX_GetPDU(pdu_ptr->un.get__request);
			break;

		case SMUX__PDUs_trap:
			free_SMUX_TrapPDU(pdu_ptr->un.trap);
			break;

		case SMUX__PDUs_commitOrRollback:
			free_SMUX_SOutPDU(pdu_ptr->un.commitOrRollback);
			break;

		default:
			break;
		}		       /* end of switch */

		free((char *)pdu_ptr);
	}
}
