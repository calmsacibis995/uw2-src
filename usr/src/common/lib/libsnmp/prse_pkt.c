/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsnmp:prse_pkt.c	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libsnmp/prse_pkt.c,v 1.4.2.1 1994/10/25 17:41:41 cyang Exp $"
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991, 1992,    *
 *                 1993, 1994  Novell, Inc. All Rights Reserved.            *
 *                                                                          *
 ****************************************************************************
 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	    *
 *      The copyright notice above does not evidence any   	            *
 *      actual or intended publication of such source code.                 *
 ****************************************************************************/

#ifndef lint
static char TCPID[] = "@(#)prse_pkt.c	1.1 STREAMWare TCP/IP SVR4.2 source";
#endif /* lint */
/*      SCCS IDENTIFICATION        */
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
static char SNMPID[] = "@(#)prse_pkt.c	6.2 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "snmp.h" 

/*
parse_pdu

     This routine takes a PDU  from  fully  populated  Authheader
     structure  and  parses  the  information  into the library's
     internal PDU format, including all varbind instances.   This
     routine  is  usually  called  with the authentication header
     pointer returned by parse_authentication, which is the  same
     state  as  the  header pointer after build_authentication is
     called.  The PDU pointer returned from this call is the same
     state   as  the  PDU  pointer  on  a  building  phase  after
     build_pdu() has been called.   If  this  routine  fails,  it
     returns a NULL.
*/

Pdu *
parse_pdu(AuthHeader *auth_ptr)
{
	unsigned char *working_ptr, *end_ptr;
	Pdu *pdu_ptr;
	VarBindList *vbl_ptr;
	short type, pdu_type;
	long length;

	if ((auth_ptr->packlet == NULL) || (auth_ptr->packlet->octet_ptr == NULL) ||
	    (auth_ptr->packlet->length == 0)) {
		LIB_ERROR("parse_pdu, bad packlet in auth_ptr->packlet:\n");
		return (NULL);
	}
	working_ptr = auth_ptr->packlet->octet_ptr;
	end_ptr = working_ptr + auth_ptr->packlet->length;

	if ((pdu_ptr = (Pdu *) malloc(sizeof (Pdu))) == NULL) {
		LIB_PERROR("parse_pdu, pdu_ptr:");
		return (NULL);
	}
	pdu_ptr->packlet = NULL;
	pdu_ptr->u.trappdu.enterprise = NULL;
	pdu_ptr->u.trappdu.agent_addr = NULL;
	pdu_ptr->var_bind_list = NULL;
	pdu_ptr->var_bind_end_ptr = NULL;

	if ((pdu_type = parse_type(&working_ptr, end_ptr)) == -1) {
		LIB_ERROR("parse_pdu, pdu_type\n");
		free_pdu(pdu_ptr);
		NULLIT(pdu_ptr);
		snmpstat->inbadtypes++;
		return (NULL);
	}
	pdu_ptr->type = pdu_type;

	if ((length = parse_length(&working_ptr, end_ptr)) == -1) {
		LIB_ERROR("parse_pdu, length\n");
		free_pdu(pdu_ptr);
		NULLIT(pdu_ptr);
		return (NULL);
	}
	if (working_ptr + length > end_ptr) {
		LIB_ERROR("parse_pdu, bad length:\n");
		free_pdu(pdu_ptr);
		NULLIT(pdu_ptr);
		snmpstat->inasnparseerrs++;
		return (NULL);
	}
	switch ((int)pdu_type) {
	case GET_REQUEST_TYPE:
	case GET_NEXT_REQUEST_TYPE:
	case GET_RESPONSE_TYPE:
	case SET_REQUEST_TYPE:
		pdu_ptr->u.normpdu.request_id =
		    parse_signedinteger(&working_ptr, end_ptr, &type);
		if (type == -1) {
			LIB_ERROR("parse_pdu, request_id\n");
			free_pdu(pdu_ptr);
			NULLIT(pdu_ptr);
			return (NULL);
		}
		pdu_ptr->u.normpdu.error_status =
		    parse_signedinteger(&working_ptr, end_ptr, &type);
		if (type == -1) {
			LIB_ERROR("parse_pdu, error_status\n");
			free_pdu(pdu_ptr);
			NULLIT(pdu_ptr);
			return (NULL);
		}
		pdu_ptr->u.normpdu.error_index =
		    parse_signedinteger(&working_ptr, end_ptr, &type);
		if (type == -1) {
			LIB_ERROR("parse_pdu, error_index\n");
			free_pdu(pdu_ptr);
			NULLIT(pdu_ptr);
			return (NULL);
		}
		break;
	case TRAP_TYPE:
		if ((pdu_ptr->u.trappdu.enterprise =
		     parse_oid(&working_ptr, end_ptr)) == NULL) {
			LIB_ERROR("parse_pdu, enterprise\n");
			free_pdu(pdu_ptr);
			NULLIT(pdu_ptr);
			return (NULL);
		}
		if ((pdu_ptr->u.trappdu.agent_addr =
		   parse_octetstring(&working_ptr, end_ptr, &type)) == NULL) {
			LIB_ERROR("parse_pdu, agent_addr\n");
			free_pdu(pdu_ptr);
			NULLIT(pdu_ptr);
			return (NULL);
		}
		pdu_ptr->u.trappdu.generic_trap =
		    parse_signedinteger(&working_ptr, end_ptr, &type);
		if (type == -1) {
			LIB_ERROR("parse_pdu, generic_trap\n");
			free_pdu(pdu_ptr);
			NULLIT(pdu_ptr);
			return (NULL);
		}
		pdu_ptr->u.trappdu.specific_trap =
		    parse_signedinteger(&working_ptr, end_ptr, &type);
		if (type == -1) {
			LIB_ERROR("parse_pdu, specific_trap\n");
			free_pdu(pdu_ptr);
			NULLIT(pdu_ptr);
			return (NULL);
		}
		pdu_ptr->u.trappdu.time_ticks =
		    parse_signedinteger(&working_ptr, end_ptr, &type);
		if (type == -1) {
			LIB_ERROR("parse_pdu, time-ticks\n");
			free_pdu(pdu_ptr);
			NULLIT(pdu_ptr);
			return (NULL);
		}
		break;
	default:
		LIB_ERROR1("parse_pdu, bad pdu_type: %x\n", pdu_type);
		free_pdu(pdu_ptr);
		NULLIT(pdu_ptr);
		snmpstat->inbadtypes++;
		return (NULL);
	};			       /* end of switch*/

	/* now strip out the sequence of */
	if (parse_sequence(&working_ptr, end_ptr, &type) == -1) {
		LIB_ERROR("parse_pdu, parse_sequence failure\n");
		free_pdu(pdu_ptr);
		NULLIT(pdu_ptr);
		return (NULL);
	}
	/* now parse the varbind list */
	while (working_ptr < end_ptr) {
		if ((vbl_ptr = parse_varbind(&working_ptr, end_ptr)) == NULL) {
			LIB_ERROR("parse_pdu, vb_ptr\n");
			free_pdu(pdu_ptr);
			NULLIT(pdu_ptr);
			return (NULL);
		}
		/* is this first one? */
		if (pdu_ptr->var_bind_list == NULL) {	/* start list */
			pdu_ptr->var_bind_list = vbl_ptr;
			pdu_ptr->var_bind_end_ptr = vbl_ptr;
		} else {	       /* tack onto end of list */
			pdu_ptr->var_bind_end_ptr->next = vbl_ptr;
			pdu_ptr->var_bind_end_ptr = vbl_ptr;
		}

		/* DO NOT FREE vbl_ptr!  Just handed it off to pdu structure */
		vbl_ptr = NULL;
	};			       /* end of while */

	return (pdu_ptr);
}

VarBindList *
parse_varbind(unsigned char **working_ptr,
	      unsigned char *end_ptr)
{
	VarBindList *vbl_ptr;
	VarBindUnit *vb_ptr;
	short type;

	if (parse_sequence(working_ptr, end_ptr, &type) == -1) {
		LIB_ERROR("parse_varbind, parse_sequence failure\n");
		return (NULL);
	}
	if ((vbl_ptr = (VarBindList *) malloc(sizeof (VarBindList))) == NULL) {
		LIB_ERROR("parse_varbind, vbl_ptr\n");
		return (NULL);
	}
	if ((vb_ptr = (VarBindUnit *) malloc(sizeof (VarBindUnit))) == NULL) {
		LIB_ERROR("parse_varbind, vb_ptr\n");
		free_varbind_list(vbl_ptr);
		NULLIT(vbl_ptr);
		return (NULL);
	}
	vbl_ptr->vb_ptr = vb_ptr;
	vbl_ptr->next = NULL;

	if ((vb_ptr->value = (ObjectSyntax *)malloc(sizeof (ObjectSyntax))) == NULL) {
		LIB_ERROR("parse_varbind, vb_ptr->value\n");
		free_varbind_list(vbl_ptr);
		NULLIT(vbl_ptr);
		return (NULL);
	}
	vb_ptr->name = NULL;
	vb_ptr->value->os_value = NULL;
	vb_ptr->value->oid_value = NULL;
	vb_ptr->value->ul_value = 0;
	vb_ptr->value->sl_value = 0;

	if ((vb_ptr->name = parse_oid(working_ptr, end_ptr)) == NULL) {
		LIB_ERROR("parse_varbind, vb_ptr->name\n");
		free_varbind_list(vbl_ptr);
		NULLIT(vbl_ptr);
		return (NULL);
	}
	switch (*(*working_ptr)) {
	case COUNTER_TYPE:	       /* handle unsigned integers */
	case GAUGE_TYPE:
		vb_ptr->value->ul_value =
		    parse_unsignedinteger(working_ptr, end_ptr, &vb_ptr->value->type);
		if (vb_ptr->value->type == -1) {
			LIB_ERROR("parse_varbind, vb_ptr->value->ul_value\n");
			free_varbind_list(vbl_ptr);
			NULLIT(vbl_ptr);
			return (NULL);
		}
		break;
	case INTEGER_TYPE:	       /* handle signed integers */
	case TIME_TICKS_TYPE:
		vb_ptr->value->sl_value =
		    parse_signedinteger(working_ptr, end_ptr, &vb_ptr->value->type);
		if (vb_ptr->value->type == -1) {
			LIB_ERROR("parse_varbind, vb_ptr->value->sl_value\n");
			free_varbind_list(vbl_ptr);
			NULLIT(vbl_ptr);
			return (NULL);
		}
		break;
	case OBJECT_ID_TYPE:	       /* handle quasi-octet strings */
		vb_ptr->value->type = OBJECT_ID_TYPE;
		if ((vb_ptr->value->oid_value = parse_oid(working_ptr, end_ptr)) == NULL) {
			LIB_ERROR("parse_varbind, vb_ptr->value->oid_value\n");
			free_varbind_list(vbl_ptr);
			NULLIT(vbl_ptr);
			return (NULL);
		}
		break;
	case OCTET_PRIM_TYPE:
	case OCTET_CONSTRUCT_TYPE:
	case IP_ADDR_PRIM_TYPE:
	case IP_ADDR_CONSTRUCT_TYPE:
	case OPAQUE_PRIM_TYPE:
	case OPAQUE_CONSTRUCT_TYPE:
		if ((vb_ptr->value->os_value =
		     parse_octetstring(working_ptr, end_ptr, &vb_ptr->value->type)) == NULL) {
			LIB_ERROR("parse_varbind, vb_ptr->value->ul_value\n");
			free_varbind_list(vbl_ptr);
			NULLIT(vbl_ptr);
			return (NULL);
		}
		break;
	case NULL_TYPE:
		if (parse_null(working_ptr, end_ptr, &vb_ptr->value->type) == -1) {
			LIB_ERROR("parse_varbind, parse_null\n");
			free_varbind_list(vbl_ptr);
			NULLIT(vbl_ptr);
			return (NULL);
		}
		break;
	default:
		LIB_ERROR1("parse_varbind, value: Illegal type: 0x%x\n",
			   vb_ptr->value->type);
		free_varbind_list(vbl_ptr);
		NULLIT(vbl_ptr);
		snmpstat->inasnparseerrs++;
		return (NULL);
	};			       /* end of switch */

	if (*working_ptr > end_ptr) {
		LIB_ERROR("parse_varbind, past end of packet!\n");
		snmpstat->inasnparseerrs++;
		return (NULL);
	}
	return (vbl_ptr);
}

OctetString *
parse_octetstring(unsigned char **working_ptr,
		  unsigned char *end_ptr,
		  short *type)
{
	OctetString *os_ptr;
	long length;
	long i;

	if ((*type = parse_type(working_ptr, end_ptr)) == -1) {
		LIB_ERROR("parse_octetstring, type error!\n");
		return (NULL);
	}
	if ((length = parse_length(working_ptr, end_ptr)) == -1) {
		LIB_ERROR("parse_octetstring, length error!\n");
		return (NULL);
	}
	if ((os_ptr = make_octetstring(*working_ptr, length)) == NULL) {
		LIB_ERROR("parse_octetstring, os_ptr\n");
		*type = -1;
		return (NULL);
	}
	os_ptr->length = length;

	/*
	 * call to make_octetstring just copied - didn't advance working_ptr.
	 * gotta crank by hand ;-)  MSC barfed on += by doing weird castings
	 * of types (at least that's what codeview showed) and blowing up.
	 */

	for (i = 0; i < length; i++)
		(*working_ptr)++;

	if (*working_ptr > end_ptr) {
		LIB_ERROR("parse_octetstring, past end of packet!\n");
		*type = -1;
		snmpstat->inasnparseerrs++;
		return (NULL);
	}
	return (os_ptr);
}				       /* end of parse_octetstring() */

OID
parse_oid(unsigned char **working_ptr,
	  unsigned char *end_ptr)
{
	OID  oid_ptr;
	long i;
	long length;
	short type;

	if ((type = parse_type(working_ptr, end_ptr)) == -1) {
		LIB_ERROR("parse_oid, type error!\n");
		return (NULL);
	}
	if (type != OBJECT_ID_TYPE) {
		LIB_ERROR1("parse_oid, type %x not OBJECT_ID_TYPE\n", type);
		snmpstat->inasnparseerrs++;
		return (NULL);
	}
	if ((length = parse_length(working_ptr, end_ptr)) == -1) {
		LIB_ERROR("parse_oid, length error!\n");
		return (NULL);
	}
	if (length > MAX_SIZE) {       /* if it's that big its GOTTA be too big */
		LIB_ERROR1("parse_oid: Bad sid string length: %ld\n", length);
		snmpstat->inasnparseerrs++;
		return (NULL);
	}
	if ((oid_ptr = (OID) malloc(sizeof *oid_ptr)) == NULL) {
		LIB_PERROR("parse_oid, oid_ptr");
		return (NULL);
	}
	oid_ptr->oid_ptr = NULL;

	if (length == 0) {	       /* zero lengthed OID's are BRAIN-DAMAGED!!! */
		oid_ptr->length = 0;
		return (oid_ptr);
	}
	/*
	 * we can be assured that this will at LEAST be long enough. (and saves
	 * stack space in MS-DOS small memory model).
	 */
	if ((oid_ptr->oid_ptr = (unsigned int *)malloc((int)(length + 2) * sizeof (unsigned int))) == NULL) {
		LIB_PERROR("make_oid, oid_ptr:");
		free_oid(oid_ptr);
		NULLIT(oid_ptr);
		return (NULL);
	}
	oid_ptr->oid_ptr[0] = (unsigned long)(*(*working_ptr) / 40);
	oid_ptr->oid_ptr[1] = (unsigned long)(*(*working_ptr)++
					      - (oid_ptr->oid_ptr[0] * 40));
	oid_ptr->length = 2;

	oid_ptr->oid_ptr[2] = 0;
	for (i = 0; i < length - 1; i++) {

		oid_ptr->oid_ptr[oid_ptr->length] =
			(oid_ptr->oid_ptr[oid_ptr->length] << 7) + (*(*working_ptr) & 0x7F);

		if ((*(*working_ptr)++ & 0x80) == 0) {	/* is last octet */
			oid_ptr->length++;
			if (i < length - 2)	/* -1 - i++ (from the loop) */
				oid_ptr->oid_ptr[oid_ptr->length] = 0;
		}
	}

	if (*working_ptr > end_ptr) {
		LIB_ERROR("parse_oid, past end of packet!\n");
		free_oid(oid_ptr);
		NULLIT(oid_ptr);
		snmpstat->inasnparseerrs++;
		return (NULL);
	}
	return (oid_ptr);
}				       /* end of parse_oid() */

unsigned long
parse_unsignedinteger(unsigned char **working_ptr,
		      unsigned char *end_ptr,
		      short *type)
{
	unsigned long value;
	long length;
	long i;

	if ((*type = parse_type(working_ptr, end_ptr)) == -1) {
		LIB_ERROR("parsed_unsignedinteger, type error!\n");
		return (0);
	}
	if ((length = parse_length(working_ptr, end_ptr)) == -1) {
		LIB_ERROR("parse_unsignedinteger, length error!\n");
		return (0);
	}
	/* check length */
	if ((length > 5) || ((length > 4) && (*(*working_ptr) != 0x00))) {
		LIB_ERROR1("parsed_unsignedinteger, length error: %ld\n", length);
		*type = -1;	       /* signal error */
		snmpstat->inasnparseerrs++;
		return (0);
	}
	/* do we have that funky leading 0 octet 'cause first bit is 1? */
	if (*(*working_ptr) == 0x00) { /* if so, skip */
		(*working_ptr)++;
		length--;	       /* and adjust length accordingly */
	}
	value = 0;
	for (i = 0; i < length; i++)
		value = (value << 8) + (unsigned long)*(*working_ptr)++;

	if (*working_ptr > end_ptr) {
		LIB_ERROR("parse_unsignedinteger, past end of packet!\n");
		*type = -1;
		snmpstat->inasnparseerrs++;
		return (0);
	}
	return (value);
}				       /* end of parse_unsignedinteger() */

long
parse_signedinteger(unsigned char **working_ptr,
		    unsigned char *end_ptr,
		    short *type)
{
	long value;
	long length;
	long i;
	long sign;

	value = 0;

	if ((*type = parse_type(working_ptr, end_ptr)) == -1) {
		LIB_ERROR("parsed_signedinteger, type error!\n");
		return (0);
	}
	if ((length = parse_length(working_ptr, end_ptr)) == -1) {
		LIB_ERROR("parse_signedinteger, length error!\n");
		return (0);
	}
	if (length > 4) {
		LIB_ERROR1("parsed_signedinteger, length error: %ld\n", length);
		*type = -1;
		snmpstat->inasnparseerrs++;
		return (0);
	}
	sign = ((*(*working_ptr) & 0x80) == 0x00) ? 0x00 : 0xff;

	for (i = 0; i < length; i++)
		value = (value << 8) + (unsigned long)*(*working_ptr)++;

	/* now fill in the upper bits with the appropriate sign extension. */
	for (i = length; i < 4; i++)
		value = value + (sign << i * 8);

	if (*working_ptr > end_ptr) {
		LIB_ERROR("parse_signedinteger, past end of packet!\n");
		*type = -1;
		snmpstat->inasnparseerrs++;
		return (0);
	}
	return (value);
}				       /* end of parse_signedinteger() */

short
parse_null(unsigned char **working_ptr,
	   unsigned char *end_ptr,
	   short *type)
{
	long length;

	*type = parse_type(working_ptr, end_ptr);

	if (*type != NULL_TYPE) {
		LIB_ERROR1("parse_null, Unexpected type: %x\n", *type);
		*type = -1;
		snmpstat->inasnparseerrs++;
		return (-1);
	}
	if ((length = parse_length(working_ptr, end_ptr)) == -1) {
		LIB_ERROR("parse_null, length error!\n");
		return (-1);
	}
	if (*working_ptr > end_ptr) {
		LIB_ERROR("parse_null, past end of packet!\n");
		snmpstat->inasnparseerrs++;
		return (-1);
	}
	return (0);
}				       /* end of parse_null() */

long
parse_sequence(unsigned char **working_ptr,
	       unsigned char *end_ptr,
	       short *type)
{
	long length;

	if ((*type = parse_type(working_ptr, end_ptr)) != SEQUENCE_TYPE) {
/* MR # ul94-29041a0
 		LIB_ERROR1("parse_sequence, Unexpected type: %x\n", *type);*/
		if (*type != -1)
			snmpstat->inasnparseerrs++;
		*type = -1;
		return (-1);
	}
	if ((length = parse_length(working_ptr, end_ptr)) == -1) {
		LIB_ERROR("parse_sequence, length error!\n");
		return (-1);
	}
	if (*working_ptr > end_ptr) {
		LIB_ERROR("parse_sequence, past end of packet!\n");
		snmpstat->inasnparseerrs++;
		return (-1);
	}
	return (length);
}				       /* end of parse_sequence() */

short
parse_type(unsigned char **working_ptr,
	   unsigned char *end_ptr)
{
	short type;

	type = *(*working_ptr)++;

	if (*working_ptr > end_ptr) {
/* MR # ul94-29041a0
		LIB_ERROR("parse_type, past end of packet!\n");*/
		snmpstat->inasnparseerrs++;
		return (-1);
	}
	switch (type) {
	case INTEGER_TYPE:
	case OCTET_PRIM_TYPE:
	case NULL_TYPE:
	case OBJECT_ID_TYPE:
	case OCTET_CONSTRUCT_TYPE:
	case SEQUENCE_TYPE:
	case IP_ADDR_PRIM_TYPE:
	case COUNTER_TYPE:
	case GAUGE_TYPE:
	case TIME_TICKS_TYPE:
	case OPAQUE_PRIM_TYPE:
	case IP_ADDR_CONSTRUCT_TYPE:
	case OPAQUE_CONSTRUCT_TYPE:
	/* SNMP PDU types */
	case GET_REQUEST_TYPE:
	case GET_NEXT_REQUEST_TYPE:
	case GET_RESPONSE_TYPE:
	case SET_REQUEST_TYPE:
	case TRAP_TYPE:
	/* SMUX specific types */
	case SMUX__PDUs_registerRequest:
	/* These values are the same as the other ones, that's why ... */
	/* they are commented out ...
	case SMUX__PDUs_simple:
	case SMUX__PDUs_close:
	case SMUX__PDUs_registerResponse:
	case SMUX__PDUs_commitOrRollback:
	*/
		return (type);
	default:
		snmpstat->inasnparseerrs++;
		return (-1);
	}
	/* NOTREACHED */
}				       /* end of parse_type() */

long
parse_length(unsigned char **working_ptr,
	     unsigned char *end_ptr)
{
	long length;
	long lenlen;
	long i;

	length = (long)*(*working_ptr)++;

	if (length < 0x80)
		return (length);

	/* long form, sigh. */
	lenlen = length & 0x7f;

	if ((lenlen > 4) || (lenlen < 1)) {	/* not in standard, but I can't handle it */
		LIB_ERROR1("Choke, gag!  Are you trying to kill me(%d)?!?\n", lenlen);
		snmpstat->inasnparseerrs++;
		return (-1);
	}
	length = 0;
	for (i = 0; i < lenlen; i++)
		length = (length << 8) + *(*working_ptr)++;

	if (*working_ptr > end_ptr) {
		LIB_ERROR("parse_length, past end of packet!\n");
		snmpstat->inasnparseerrs++;
		return (-1);
	}
	return (length);
}				       /* end of parse_length() */
