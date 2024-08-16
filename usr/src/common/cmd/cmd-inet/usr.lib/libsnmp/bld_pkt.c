/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.lib/libsnmp/bld_pkt.c	1.1"
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
static char SNMPID[] = "@(#)bld_pkt.c	6.2 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

#include <stdio.h>
#include <errno.h>

#include <snmp/snmp.h>

/*
build_pdu

     This routine is called with the PDU pointer  being  used  to
     create  the  PDU.   It traces down the structure of varbinds
     that has been added to it and builds the ASN.1 packet in the
     *packlet  pointer  of  the PDU pointer's data structure.  At
     this point, PDU processing is complete and the structure  is
     ready to be passed onto the authentication layers.

     It is the main part of the packet builder.  It traverses the
     varbind list to determine the packet size and then calls the
     add_* routines to bind the item's constructs into the packet.
     The 'packlet' is an OctetString in pdu-ptr->packlet.
*/

short
build_pdu(pdu_ptr)
	Pdu *pdu_ptr;
{
	long varbindlen;
	long varbind_tot_len;
	long varbindlenlen;
	long datalen, temp_data_len;
	long lenlen;
	unsigned char *working_ptr;
	short cc;

	/* find out length of whole PDU */
	if ((varbindlen = find_len_varbind(pdu_ptr->var_bind_list)) == -1) {
		LIB_ERROR("build_pdu, varbindlen:\n");
		return (-1);	       /* abort */
	}
	if ((varbindlenlen = dolenlen(varbindlen)) == -1) {
		LIB_ERROR("build_pdu,varbindlenlen:\n");
		return (-1);
	}
	varbind_tot_len = 1 + varbindlenlen + varbindlen;

	switch (pdu_ptr->type) {
	case GET_REQUEST_TYPE:
	case GET_NEXT_REQUEST_TYPE:
	case GET_RESPONSE_TYPE:
	case SET_REQUEST_TYPE:
		datalen = find_len_signedinteger(pdu_ptr->u.normpdu.request_id) +
		    find_len_signedinteger(pdu_ptr->u.normpdu.error_status) +
		    find_len_signedinteger(pdu_ptr->u.normpdu.error_index) +
		    varbind_tot_len;
		break;
	case TRAP_TYPE:
		datalen = find_len_signedinteger(pdu_ptr->u.trappdu.generic_trap) +
		    find_len_signedinteger(pdu_ptr->u.trappdu.specific_trap) +
		    find_len_signedinteger(pdu_ptr->u.trappdu.time_ticks) +
		    varbind_tot_len;
		if ((temp_data_len = find_len_oid(pdu_ptr->u.trappdu.enterprise)) == -1) {
			LIB_ERROR("build_pdu, temp_data_len (1)\n");
			return (-1);
		}
		datalen = datalen + temp_data_len;
		if ((temp_data_len = find_len_octetstring(pdu_ptr->u.trappdu.agent_addr)) == -1) {
			LIB_ERROR("build_pdu, temp_data_len (2)\n");
			return (-1);
		}
		datalen = datalen + temp_data_len;
		break;
	default:
		LIB_ERROR1("build_pdu, bad pdu type: %x\n", pdu_ptr->type);
		return (-1);
		break;
	};			       /* end of switch */

	if ((lenlen = dolenlen(datalen)) == -1) {
		LIB_ERROR("build_pdu,lenlen:\n");
		return (-1);
	}
	/* now allocate memory for PDU packlet */
	if ((pdu_ptr->packlet = (OctetString *) malloc(sizeof (OctetString))) == NULL) {
		LIB_PERROR("pdu_ptr->packlet");
		return (-1);
	}
	pdu_ptr->packlet->length = 1 + lenlen + datalen;

	if ((pdu_ptr->packlet->octet_ptr =
	     (unsigned char *)malloc((int)pdu_ptr->packlet->length)) == NULL) {
		LIB_PERROR("pdu_ptr->packlet->octet_ptr");
		free(pdu_ptr->packlet);
		NULLIT(pdu_ptr->packlet);
		return (-1);
	}
	working_ptr = pdu_ptr->packlet->octet_ptr;

	*working_ptr++ = (unsigned char)(0xff & pdu_ptr->type);
	add_len(&working_ptr, lenlen, datalen);

	switch (pdu_ptr->type) {
	case GET_REQUEST_TYPE:
	case GET_NEXT_REQUEST_TYPE:
	case GET_RESPONSE_TYPE:
	case SET_REQUEST_TYPE:
		add_signedinteger(&working_ptr, INTEGER_TYPE, pdu_ptr->u.normpdu.request_id);
		add_signedinteger(&working_ptr, INTEGER_TYPE, pdu_ptr->u.normpdu.error_status);
		add_signedinteger(&working_ptr, INTEGER_TYPE, pdu_ptr->u.normpdu.error_index);
		break;
	case TRAP_TYPE:
		add_oid(&working_ptr, OBJECT_ID_TYPE, pdu_ptr->u.trappdu.enterprise);
		add_octetstring(&working_ptr, IP_ADDR_PRIM_TYPE, pdu_ptr->u.trappdu.agent_addr);
		add_signedinteger(&working_ptr, INTEGER_TYPE, pdu_ptr->u.trappdu.generic_trap);
		add_signedinteger(&working_ptr, INTEGER_TYPE, pdu_ptr->u.trappdu.specific_trap);
		add_signedinteger(&working_ptr, TIME_TICKS_TYPE, pdu_ptr->u.trappdu.time_ticks);
		break;
	default:
		LIB_ERROR1("build_pdu, bad pdu_ptr->type - II. Shouldn't happen!:%x\n",
			   pdu_ptr->type);
		free_octetstring(pdu_ptr->packlet);
		NULLIT(pdu_ptr->packlet);
		return (-1);
		break;
	};			       /* end of switch II */

	*working_ptr++ = SEQUENCE_TYPE;
	add_len(&working_ptr, varbindlenlen, varbindlen);

	if ((cc = add_varbind(&working_ptr, pdu_ptr->var_bind_list)) == -1) {
		free_octetstring(pdu_ptr->packlet);
		NULLIT(pdu_ptr->packlet);
		return (-1);
	}
	return (0);
}				       /* end of build_pdu() */

long
find_len_varbind(vbl_ptr)
	VarBindList *vbl_ptr;
{
	VarBindUnit *vb_ptr;
	long name_tot_len;
	long value_tot_len;
	long datalen;
	long lenlen;
	long tot_so_far;

	if (vbl_ptr == NULL)
		return (0);

	tot_so_far = find_len_varbind(vbl_ptr->next);

	if (tot_so_far == -1)
		return (-1);	       /* signal error backup */

	vb_ptr = vbl_ptr->vb_ptr;
	/* variable name stuff */
	if ((name_tot_len = find_len_oid(vb_ptr->name)) == -1) {
		LIB_ERROR("find_len_varbind, name_tot_len\n");
		return (-1);
	}
	/* do variable value stuff */
	switch (vb_ptr->value->type) {
	case COUNTER_TYPE:	       /* handle unsigned integers */
	case GAUGE_TYPE:
		value_tot_len = find_len_unsignedinteger(vb_ptr->value->ul_value);
		break;
	case INTEGER_TYPE:	       /* handle signed integers */
	case TIME_TICKS_TYPE:
		value_tot_len = find_len_signedinteger(vb_ptr->value->sl_value);
		break;
	case OBJECT_ID_TYPE:	       /* handle quasi-octet strings */
		value_tot_len = find_len_oid(vb_ptr->value->oid_value);
		break;
	case OCTET_PRIM_TYPE:
	case OCTET_CONSTRUCT_TYPE:
	case IP_ADDR_PRIM_TYPE:
	case IP_ADDR_CONSTRUCT_TYPE:
	case OPAQUE_PRIM_TYPE:
	case OPAQUE_CONSTRUCT_TYPE:
		if ((value_tot_len = find_len_octetstring(vb_ptr->value->os_value)) == -1) {
			LIB_ERROR("find_len_varbind, value_tot_len\n");
			return (-1);
		}
		break;
	case NULL_TYPE:
		value_tot_len = 2;
		break;
	default:
		LIB_ERROR1("find_len_varbind, value: Illegal type: 0x%x\n", vb_ptr->value->type);
		return (-1);
		break;
	};			       /* end of switch */

	datalen = name_tot_len + value_tot_len;

	vbl_ptr->data_length = datalen;

	if ((lenlen = dolenlen(datalen)) == -1) {
		LIB_ERROR("find_len_varbind, lenlen\n");
		return (-1);
	}
	return (1 + lenlen + datalen + tot_so_far);
}				       /* end of find_len_varbind */

long
find_len_octetstring(os_ptr)
	OctetString *os_ptr;
{
	long lenlen;

	if (os_ptr == NULL) {
		return (-1);
	}
	if ((lenlen = dolenlen(os_ptr->length)) == -1) {
		LIB_ERROR("find_len_octetstring, lenlen\n");
		return (-1);
	}
	return (1 + lenlen + os_ptr->length);
}

short
find_len_oid(oid_ptr)
	OID oid_ptr;
{
	long lenlen;
	short i;
	long encoded_len;

	encoded_len = 1;	       /* for first two SID's */

	for (i = 2; i < oid_ptr->length; i++) {
		if (oid_ptr->oid_ptr[i] < 0x80)			/* 0 - 0x7f */
			encoded_len += 1;
		else if (oid_ptr->oid_ptr[i] < 0x4000)		/* 0x80 - 0x3fff */
			encoded_len += 2;
		else if (oid_ptr->oid_ptr[i] < 0x200000)	/* 0x4000 - 0x1FFFFF */
			encoded_len += 3;
		else if (oid_ptr->oid_ptr[i] < 0x10000000)	/* 0x200000 - 0x0fffffff */
			encoded_len += 4;
		else
			encoded_len += 5;
	}

	if ((lenlen = dolenlen(encoded_len)) == -1) {
		LIB_ERROR("find_len_oid, lenlen\n");
		return (-1);
	}
	return (short)(1 + lenlen + encoded_len);
}

short
find_len_unsignedinteger(value)
	unsigned long value;
{
	long datalen;

	/* if high bit one, must use 5 octets (first with 00) */
	if (((value >> 24) & 0x0ff) != 0)
		datalen = 4;
	else if (((value >> 16) & 0x0ff) != 0)
		datalen = 3;
	else if (((value >> 8) & 0x0ff) != 0)
		datalen = 2;
	else
		datalen = 1;

	if (((value >> (8 * (datalen - 1))) & 0x080) != 0)
		datalen++;

	/* length of length  < 127 octet's, that's for sure! */

	return (short)(1 + 1 + datalen);
}

short
find_len_signedinteger(value)
	long value;
{
	long datalen;

	switch ((unsigned char)((value >> 24) & 0x0ff)) {
	case 0x00:
		if (((value >> 16) & 0x0ff) != 0)
			datalen = 3;
		else if (((value >> 8) & 0x0ff) != 0)
			datalen = 2;
		else
			datalen = 1;
		if (((value >> (8 * (datalen - 1))) & 0x080) != 0)
			datalen++;
		break;
	case 0xff:
		if (((value >> 16) & 0x0ff) != 0xFF)
			datalen = 3;
		else if (((value >> 8) & 0x0ff) != 0xFF)
			datalen = 2;
		else
			datalen = 1;
		if (((value >> (8 * (datalen - 1))) & 0x080) == 0)
			datalen++;
		break;
	default:
		datalen = 4;
	};			       /* end of switch */

	return (short)(1 + 1 + datalen);
}

short
dolenlen(len)
	long len;
{
	/* short form? */
	if (len < 128)
		return (1);
	if (len < 0x0100)
		return (2);
	if (len < 0x010000)
		return (3);
	if (len < 0x01000000)
		return (4);
	LIB_ERROR("Lenlen: Length greater than 0x01000000???\n");
	return (-1);
}

void
add_len(working_ptr, lenlen, data_len)
	unsigned char **working_ptr;
	long lenlen, data_len;
{
	long i;

	if (lenlen == 1) {	       /* short form? */
		*(*working_ptr)++ = (unsigned char)data_len;
		return;
	}
	/* oh well, long form time */
	*(*working_ptr)++ = ((unsigned char)0x80 + (unsigned char)lenlen - 1);
	for (i = 1; i < lenlen; i++)
		*(*working_ptr)++ = (unsigned char)((data_len >> (8 * (lenlen - i - 1))) & 0x0FF);
}

short
add_varbind(working_ptr, vbl_ptr)
	unsigned char **working_ptr;
	VarBindList *vbl_ptr;
{
	VarBindUnit *vb_ptr;
	long lenlen;

	if (vbl_ptr == NULL)
		return (0);

	vb_ptr = vbl_ptr->vb_ptr;
	if ((lenlen = dolenlen(vbl_ptr->data_length)) == -1) {
		LIB_ERROR("add_varbind, lenlen:\n");
		return (-1);
	}
	*(*working_ptr)++ = SEQUENCE_TYPE;
	add_len(working_ptr, lenlen, vbl_ptr->data_length);

	if (add_oid(working_ptr, OBJECT_ID_TYPE, vb_ptr->name) == -1) {
		LIB_ERROR("add_varbind, add_oid\n");
		return (-1);
	}
	switch (vb_ptr->value->type) {
	case COUNTER_TYPE:	       /* handle unsigned integers */
	case GAUGE_TYPE:
		add_unsignedinteger(working_ptr, vb_ptr->value->type, vb_ptr->value->ul_value);
		break;
	case INTEGER_TYPE:	       /* handle signed integers */
	case TIME_TICKS_TYPE:
		add_signedinteger(working_ptr, vb_ptr->value->type, vb_ptr->value->sl_value);
		break;
	case OBJECT_ID_TYPE:	       /* handle quasi-octet strings */
		add_oid(working_ptr, vb_ptr->value->type, vb_ptr->value->oid_value);
		break;
	case OCTET_PRIM_TYPE:
	case OCTET_CONSTRUCT_TYPE:
	case IP_ADDR_PRIM_TYPE:
	case IP_ADDR_CONSTRUCT_TYPE:
	case OPAQUE_PRIM_TYPE:
	case OPAQUE_CONSTRUCT_TYPE:
		add_octetstring(working_ptr, vb_ptr->value->type, vb_ptr->value->os_value);
		break;
	case NULL_TYPE:
		add_null(working_ptr);
		break;
	default:
		LIB_ERROR1("build_varbind, value: Illegal type: 0x%x\n", vb_ptr->value->type);
		return (-1);
		break;
	};			       /* end of switch */

	return (add_varbind(working_ptr, vbl_ptr->next));
}

short
add_octetstring(unsigned char **working_ptr, short type, OctetString *os_ptr)
{
	long i;
	long lenlen;

	if ((lenlen = dolenlen(os_ptr->length)) == -1) {
		LIB_ERROR("add_octetstring,lenlen\n");
		return (-1);
	}
	*(*working_ptr)++ = (unsigned char)(0xff & type);
	add_len(working_ptr, lenlen, os_ptr->length);

	for (i = 0; i < os_ptr->length; i++)
		*(*working_ptr)++ = os_ptr->octet_ptr[i];

	return (0);
}

short
add_oid(unsigned char **working_ptr, short type, OID oid_ptr)
{
	short i;
	long lenlen;
	long encoded_len;

	encoded_len = 1;	       /* for first two SID's */
	for (i = 2; i < oid_ptr->length; i++) {
		if (oid_ptr->oid_ptr[i] < 0x80)			/* 0 - 0x7f */
			encoded_len += 1;
		else if (oid_ptr->oid_ptr[i] < 0x4000)		/* 0x80 - 0x3fff */
			encoded_len += 2;
		else if (oid_ptr->oid_ptr[i] < 0x200000)	/* 0x4000 - 0x1FFFFF */
			encoded_len += 3;
		else if (oid_ptr->oid_ptr[i] < 0x10000000)	/* 0x200000 - 0x0fffffff */
			encoded_len += 4;
		else
			encoded_len += 5;
	}

	if ((lenlen = dolenlen(encoded_len)) == -1) {
		LIB_ERROR("add_oid,lenlen\n");
		return (-1);
	}
	*(*working_ptr)++ = (unsigned char)(0xff & type);
	add_len(working_ptr, lenlen, encoded_len);

	if (oid_ptr->length < 2)
		*(*working_ptr)++ = (unsigned char)(oid_ptr->oid_ptr[0] * 40);
	else
		*(*working_ptr)++ = (unsigned char)((oid_ptr->oid_ptr[0] * 40) +
						    oid_ptr->oid_ptr[1]);

	for (i = 2; i < oid_ptr->length; i++) {
		/* 0 - 0x7f */
		if (oid_ptr->oid_ptr[i] < 0x80) {
			*(*working_ptr)++ = (unsigned char)oid_ptr->oid_ptr[i];
		/* 0x80 - 0x3fff */
		} else if (oid_ptr->oid_ptr[i] < 0x4000) {
			*(*working_ptr)++ = (unsigned char)
			    (((oid_ptr->oid_ptr[i]) >> 7) | 0x80);
			*(*working_ptr)++ = (unsigned char)(oid_ptr->oid_ptr[i] & 0x7f);
		/* 0x4000 - 0x1FFFFF */
		} else if (oid_ptr->oid_ptr[i] < 0x200000) {
			*(*working_ptr)++ = (unsigned char)
			    (((oid_ptr->oid_ptr[i]) >> 14) | 0x80);
			*(*working_ptr)++ = (unsigned char)
			    (((oid_ptr->oid_ptr[i]) >> 7) | 0x80);
			*(*working_ptr)++ = (unsigned char)(oid_ptr->oid_ptr[i] & 0x7f);
		/* 0x200000 - 0x0fffffff */
		} else if (oid_ptr->oid_ptr[i] < 0x10000000) {
			*(*working_ptr)++ = (unsigned char)
			    (((oid_ptr->oid_ptr[i]) >> 21) | 0x80);
			*(*working_ptr)++ = (unsigned char)
			    (((oid_ptr->oid_ptr[i]) >> 14) | 0x80);
			*(*working_ptr)++ = (unsigned char)
			    (((oid_ptr->oid_ptr[i]) >> 7) | 0x80);
			*(*working_ptr)++ = (unsigned char)(oid_ptr->oid_ptr[i] & 0x7f);
		/* > 0x0fffffff */
		} else {
			*(*working_ptr)++ = (unsigned char)
			    (((oid_ptr->oid_ptr[i]) >> 28) | 0x80);
			*(*working_ptr)++ = (unsigned char)
			    (((oid_ptr->oid_ptr[i]) >> 21) | 0x80);
			*(*working_ptr)++ = (unsigned char)
			    (((oid_ptr->oid_ptr[i]) >> 14) | 0x80);
			*(*working_ptr)++ = (unsigned char)
			    (((oid_ptr->oid_ptr[i]) >> 7) | 0x80);
			*(*working_ptr)++ = (unsigned char)(oid_ptr->oid_ptr[i] & 0x7f);
		}
	}			       /* end of second for */

	return (0);
}				       /* end of add_oid */

short
add_unsignedinteger(unsigned char **working_ptr, short type, unsigned long value)
{
	long i;
	long datalen;
	long lenlen;

	/* if high bit one, must use 5 octets (first with 00) */
	if (((value >> 24) & 0x0ff) != 0)
		datalen = 4;

	else if (((value >> 16) & 0x0ff) != 0)
		datalen = 3;
	else if (((value >> 8) & 0x0ff) != 0)
		datalen = 2;
	else
		datalen = 1;

	if (((value >> (8 * (datalen - 1))) & 0x080) != 0)
		datalen++;

	lenlen = 1;		       /* < 127 octet's, that's for sure! */

	*(*working_ptr)++ = (unsigned char)(0xff & type);
	add_len(working_ptr, lenlen, datalen);

	if (datalen == 5) {	       /* gotta put a 00 in first octet */
		*(*working_ptr)++ = (unsigned char)0;
		for (i = 1; i < datalen; i++)	/* bug fix 4/24/89, change 0 -> 1 */
			*(*working_ptr)++
			    = (unsigned char)(value >> (8 * ((datalen - 1) - i) & 0x0ff));
	} else {
		for (i = 0; i < datalen; i++)
			*(*working_ptr)++
			    = (unsigned char)(value >> (8 * ((datalen - 1) - i) & 0x0ff));
	}

	return (0);
}

short
add_signedinteger(unsigned char **working_ptr, short type, long value)
{
	long i;
	long datalen;
	long lenlen;

	switch ((unsigned char)((value >> 24) & 0x0ff)) {
	case 0x00:
		if (((value >> 16) & 0x0ff) != 0)
			datalen = 3;
		else if (((value >> 8) & 0x0ff) != 0)
			datalen = 2;
		else
			datalen = 1;
		if (((value >> (8 * (datalen - 1))) & 0x080) != 0)
			datalen++;

		break;
	case 0xff:
		if (((value >> 16) & 0x0ff) != 0xFF)
			datalen = 3;
		else if (((value >> 8) & 0x0ff) != 0xFF)
			datalen = 2;
		else
			datalen = 1;
		if (((value >> (8 * (datalen - 1))) & 0x080) == 0)
			datalen++;
		break;

	default:
		datalen = 4;
	};			       /* end of switch */

	lenlen = 1;		       /* < 127 octet's, that's for sure! */

	*(*working_ptr)++ = (unsigned char)(0xff & type);
	add_len(working_ptr, lenlen, datalen);

	for (i = 0; i < datalen; i++)
		*(*working_ptr)++
		    = (unsigned char)(value >> (8 * ((datalen - 1) - i) & 0x0ff));

	return (0);
}

void
add_null(working_ptr)
	unsigned char **working_ptr;
{
	*(*working_ptr)++ = NULL_TYPE;
	*(*working_ptr)++ = 0x00;
}
