/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsnmp:make_lib.c	1.4"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libsnmp/make_lib.c,v 1.5 1994/08/09 23:33:04 cyang Exp $"
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
static char TCPID[] = "@(#)make_lib.c	1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)make_lib.c	6.2 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

#include "snmp.h" 
#include "snmpuser.h"

/*
make_octetstring

     This routine produces a library OctetString  construct  from
     a  passed  byte  string  and length.  This construct is usu-
     ally passed to other library calls, such  as  make_varbind()
     or  make_authentication(),  and linked into a larger library
     construct of an ASN.1 entity.   free_octetstring()  recovers
     all  memory malloc'ed by make_octetstring, but should not be
     used if the octetstring is passed to  another  library  rou-
     tine.   The  free_*()  counterparts to those routine free up
     all memory that has been linked to the  higher  level  ASN.1
     structure.
*/

OctetString *
make_octetstring(unsigned char *string,	long length)
{
	OctetString *os_ptr;

	if (length > MAX_SIZE) {
		LIB_ERROR1("make_octetstring: Bad octetstring length: %d\n", length);
		LIB_ERROR1("Bad String: %s\n", string);
		return (NULL);
	}
	if ((os_ptr = (OctetString *) malloc(sizeof (OctetString))) == NULL) {
		LIB_PERROR("make_octetstring, os_ptr");
		return (NULL);
	}
	os_ptr->octet_ptr = NULL;

	/* malloc length + 1 in case length = 0 and we are on a SVID compliant box */

	if ((os_ptr->octet_ptr = (unsigned char *)malloc((int)length + 1)) == NULL) {
		LIB_PERROR("make_octetstring, octet_ptr:");
		free_octetstring(os_ptr);
		NULLIT(os_ptr);
		return (NULL);
	}
	bcopy(string, os_ptr->octet_ptr, (int)length);
	os_ptr->length = length;

	return (os_ptr);
}				       /* end of make_octetstring */

/*
make_oid

     This routine produces a library Object Identifier  construct
     from  a  passed  sub-identifier  array and length.  The sub-
     identifier array is an array of unsigned long integers, with
     each element of the array corresponding to the value of each
     integer at each position in the dot-notation display  of  an
     object  identifier.   For  example,  the  Object  Identifier
     1.3.6.1.2.1.1 would have a value 1  in  sid_array[0],  3  in
     sid_array[1],  etc.   length is the number of sub-identifier
     elements present in the array.  The construct  returned  can
     be  free'ed  by  passing the pointer to free_oid().  This is
     usually unnecessary as the construct is most often passed to
     another  library routine for inclusion in a large ASN.1 con-
     struct and that library routine's free'ing counterpart  will
     do the memory recovery.
*/

OID
make_oid(unsigned int *sid, short length)
{
	OID oid_ptr;
	int i;

	if (length > MAX_SIZE) {       /* if it's that big its GOTTA be too big */
		LIB_ERROR1("make_oid: Bad sid string length: %d\n", length);
		return (NULL);
	}
	if ((oid_ptr = (OID) malloc(sizeof (OIDentifier))) == NULL) {
		LIB_PERROR("make_oid, oid_ptr");
		return (NULL);
	}
	oid_ptr->oid_ptr = NULL;

	if ((oid_ptr->oid_ptr = (unsigned int *)malloc(length * sizeof (unsigned int))) == NULL) {
		LIB_PERROR("make_oid, oid_ptr:");
		free_oid(oid_ptr);
		NULLIT(oid_ptr);
		return (NULL);
	}
	for (i = 0; i < length; i++)
		oid_ptr->oid_ptr[i] = (unsigned long)sid[i];
	oid_ptr->length = length;

	return (oid_ptr);
}				       /* end of make_oid */

/*
make_varbind

     This routine is called to create  a  var_bind  entry  to  be
     strung onto a PDU.  It returns a pointer to a malloc'ed data
     structure of type VarBindList.  This pointer is usually then
     used  in a call to link_varbind() to associate this var_bind
     with a PDU.  The  structure  is  free'ed  when  the  PDU  is
     free'ed with a call to free_pdu().

     A var_bind is a variable-value binding.  It binds the  name,
     the  type,  and  the value of a variable into one construct.
     The current list of SNMP types are and which of the  calling
     variables to set the value of:
     COUNTER_TYPE:  uses ul_value
     GAUGE_TYPE:  uses ul_value
     INTEGER_TYPE: uses sl_value
     TIME_TICKS_TYPE: uses sl_value
     OBJECT_ID_TYPE: uses oid_value
     OCTET_PRIM_TYPE: uses os_value
     OCTET_CONSTRUCT_TYPE: uses os_value
     IP_ADDR_PRIM_TYPE: uses os_value
     IP_ADDR_CONSTRUCT_TYPE: uses os_value
     OPAQUE_PRIM_TYPE: uses os_value
     OPAQUE_CONSTRUCT_TYPE: uses os_value
     NULL_TYPE:  no passed value needed
*/

VarBindList *
make_varbind(
	OID oid,
	short type,
	unsigned long ul_value,
	long sl_value,
	OctetString *os_value,
	OID oid_value)
{
	VarBindList *vbl_ptr;

	if ((vbl_ptr = (VarBindList *) malloc(sizeof (VarBindList))) == NULL) {
		LIB_PERROR("make_varbind, vbl_ptr");
		return (NULL);
	}
	if ((vbl_ptr->vb_ptr = (VarBindUnit *) malloc(sizeof (VarBindUnit))) == NULL) {
		LIB_PERROR("make_varbind, vb_ptr");
		free_varbind_list(vbl_ptr);
		NULLIT(vbl_ptr);
		return (NULL);
	}
	if ((vbl_ptr->vb_ptr->value = (ObjectSyntax *) malloc(sizeof (ObjectSyntax))) == NULL) {
		LIB_PERROR("make_varbind, vb_ptr->value");
		free_varbind_list(vbl_ptr);
		NULLIT(vbl_ptr);
		return (NULL);
	}
	vbl_ptr->vb_ptr->name = oid;	       /* name */
	vbl_ptr->vb_ptr->value->type = type;
	vbl_ptr->vb_ptr->value->ul_value = 0;
	vbl_ptr->vb_ptr->value->sl_value = 0;
	vbl_ptr->vb_ptr->value->oid_value = NULL;
	vbl_ptr->vb_ptr->value->os_value = NULL;

	vbl_ptr->next = NULL;
	vbl_ptr->data_length = 0;

	/* combine the Choice's from simple and application */
	switch (type) {
	case COUNTER_TYPE:
	case GAUGE_TYPE:
		vbl_ptr->vb_ptr->value->ul_value = ul_value;
		break;
	case INTEGER_TYPE:
	case TIME_TICKS_TYPE:
		vbl_ptr->vb_ptr->value->sl_value = sl_value;
		break;
	case OBJECT_ID_TYPE:
		vbl_ptr->vb_ptr->value->oid_value = oid_value;
		break;
	case OCTET_PRIM_TYPE:
	case OCTET_CONSTRUCT_TYPE:
	case IP_ADDR_PRIM_TYPE:
	case IP_ADDR_CONSTRUCT_TYPE:
	case OPAQUE_PRIM_TYPE:
	case OPAQUE_CONSTRUCT_TYPE:
		vbl_ptr->vb_ptr->value->os_value = os_value;
		break;
	case NULL_TYPE:
		break;
	default:
		LIB_ERROR1("make_varbind: Illegal type: 0x%x\n", type);
		free_varbind(vbl_ptr->vb_ptr);
		free_varbind_list(vbl_ptr);
		NULLIT(vbl_ptr);
		return (NULL);
	};			       /* end of switch */

	return (vbl_ptr);
}				       /* end of  make_varbind() */

/*
make_pdu

     This routine is called to create the  initial  header  block
     for  building the SNMP ASN.1 data structure, which upon com-
     pletion is used to build the actual SNMP packet.  It returns
     a pointer to a malloc'ed data structure of type PDU:
     typedef struct _Pdu {
       OctetString *packlet;
       short type;
       union {
         NormPdu normpdu;
         TrapPdu trappdu;
       } u;
       VarBindList *var_bind_list;
       VarBindList *var_bind_end_ptr;
     } Pdu;

     The type is one of GET_REQUEST_TYPE,  GET_NEXT_REQUEST_TYPE,
     GET_RESPONSE_TYPE,   SET_REQUEST_TYPE,  or  TRAP_TYPE.   The
     request ID is the  id  number  assigned  to  the  particular
     packet  by  the  application.   Since the application is UDP
     based, retry is solely controlled by the network  management
     application.   The  error_status  is  set to other than 0 in
     GET_RESPONSE_TYPE only to indicate that this response is  in
     reply  to  a  bad  request.  The error_index is used only by
     GET_RESPONSE_TYPE and points to the var-bind  entry  in  the
     PDU  that  offends  the  agent.   The  enterprise is used by
     TRAP_TYPE pdund is an object identifier associated  with
     the  trap  generating entity.  The agent_addr is used by the
     TRAP_TYPE pdu and consists of an octet string containing the
     IP  address of the trap generating entity.  The generic_trap
     and specific_trap are used by the TRAP_TYPE pdu and  consist
     of  integers  that  indicate  which  type  of  trap this PDU
     represents.   The  time_ticks  is  the   TRAP_TYPE   emiting
     entity's sense of time since the agent has restarted.

     This routine is called once for each packet to be generated.
     The  pdu  pointer is then passed to the routine link_varbind
     repeatedly to string var_binds into the packet.  Build_pdu()
     is  then called to perform the ASN.1 encoding of the pdu and
     place that result  in  the  pdu  pointer's  *packlet  field.
     After  the  packlet  has  been  wrapped in an authentication
     envelope,  it  is  free'ed  by  passing   the   pointer   to
     free_pdu().
*/

Pdu *
make_pdu(
	short type,
	long request_id,	       /* norm stuff */
	long error_status,	       /* norm stuff */
	long error_index,	       /* norm stuff */
	OID enterprise,	       /* trap stuff */
	OctetString *agent_addr,       /* trap stuff *//* an IP addr */
	long generic_trap,	       /* trap stuff */
	long specific_trap,	       /* trap stuff */
	long time_ticks		       /* trap stuff */
	)
{
	Pdu *pdu_ptr;

	if ((pdu_ptr = (Pdu *) malloc(sizeof (Pdu))) == NULL) {
		LIB_PERROR("make_pdu, pdu_ptr:");
		return (NULL);
	}
	pdu_ptr->type = type;
	pdu_ptr->packlet = NULL;
	pdu_ptr->u.trappdu.enterprise = NULL;
	pdu_ptr->u.trappdu.agent_addr = NULL;
	pdu_ptr->var_bind_list = NULL;
	pdu_ptr->var_bind_end_ptr = NULL;

	/* sanity check type */
	switch (type) {
	case GET_REQUEST_TYPE:
	case GET_NEXT_REQUEST_TYPE:
	case GET_RESPONSE_TYPE:
	case SET_REQUEST_TYPE:
		pdu_ptr->u.normpdu.request_id = request_id;
		pdu_ptr->u.normpdu.error_status = error_status;
		pdu_ptr->u.normpdu.error_index = error_index;
		if (type == GET_REQUEST_TYPE)
			snmpstat->outgetrequests++;
		else if (type == GET_NEXT_REQUEST_TYPE)
			snmpstat->outgetnexts++;
		else if (type == SET_REQUEST_TYPE)
			snmpstat->outsetrequests++;
		else if (type == GET_RESPONSE_TYPE)
			snmpstat->outgetresponses++;
		break;
	case TRAP_TYPE:
		/* sanity check octet strings */
		if (enterprise == NULL) {
			LIB_ERROR("make_pdu: Trap type but no Enterprise!\n");
			free_pdu(pdu_ptr);
			NULLIT(pdu_ptr);
			return (NULL);
		}
		if (agent_addr == NULL) {
			LIB_ERROR("make_pdu: Trap type but no Agent_addr!\n");
			free_pdu(pdu_ptr);
			NULLIT(pdu_ptr);
			return (NULL);
		}
		pdu_ptr->u.trappdu.enterprise = enterprise;
		pdu_ptr->u.trappdu.agent_addr = agent_addr;
		pdu_ptr->u.trappdu.generic_trap = generic_trap;
		pdu_ptr->u.trappdu.specific_trap = specific_trap;
		pdu_ptr->u.trappdu.time_ticks = time_ticks;
		snmpstat->outtraps++;
		break;
	default:
		LIB_ERROR1("make_pdu: illegal type: 0x%x\n", type);
		free_pdu(pdu_ptr);
		NULLIT(pdu_ptr);
		return (NULL);
	};			       /* end of switch */

	return (pdu_ptr);
}				       /* end of make_normpdu */

/*
link_varbind

     This routine adds the varbind entry created  by  a  call  to
     make_varbind()  to  a  PDU  previously  started by a call to
     make_pdu() to flesh out the PDU.  This should only be called
     once  for  each varbind_ptr being associated with the PDU as
     it is this association that is used to reclaim  memory  when
     the PDU is free'ed.
*/

/*
 * Thread a vb_ptr onto the end of the list
 */
short
link_varbind(Pdu *pdu_ptr,
	     VarBindList *vbl_ptr)
{
	if (vbl_ptr == NULL) {
		LIB_ERROR("link_varbind: Trying to add Null Variable\n");
		return (-1);
	}
	/* first variable? */
	if (pdu_ptr->var_bind_end_ptr == NULL) {
		pdu_ptr->var_bind_list = vbl_ptr;
		pdu_ptr->var_bind_end_ptr = vbl_ptr;
		return (0);
	}
	/* nope, so just add to end of list and update end ptr */
	pdu_ptr->var_bind_end_ptr->next = vbl_ptr;
	pdu_ptr->var_bind_end_ptr = vbl_ptr;
	return (0);
}				       /* end of link_varbind */

/*
make_octet_from_text

     This routine is used to create a library format octet string
     data  structure  for  use  by  calls  to  make_varbind() and
     make_authentication() from text strings.
*/

OctetString *
make_octet_from_text(unsigned char *text_str)
{
	long i;
	OctetString *os_ptr;

	if ((os_ptr = (OctetString *) malloc(sizeof (OctetString))) == NULL) {
		LIB_PERROR("make_octet_from_text, os_ptr");
		return (NULL);
	}
	os_ptr->octet_ptr = NULL;
	os_ptr->length = strlen((char *)text_str);

	if ((os_ptr->octet_ptr = (unsigned char *)malloc((int)os_ptr->length + 1)) == NULL) {
		LIB_PERROR("make_octet_from_text, octet_ptr:");
		free_octetstring(os_ptr);
		NULLIT(os_ptr);
		return (NULL);
	}
	for (i = 0; i < os_ptr->length; i++)
		os_ptr->octet_ptr[i] = text_str[i];
	return (os_ptr);
}

/*
make_octet_from_hex

     This routine is used to create a library format octet string
     data  structure  for  use  by  calls  to  make_varbind() and
     make_authentication() from hex  text strings.  The format of
     these strings is a hex value (1 or more hex digits, upper or
     lower case) followed by a space, with more  hex  values  and
     spaces  to finish out the string.  For example, to create an
     octet string or length two with a hex 15 in the first  octet
     and  a  hex 3D in the second octet, the text string could be
     "15 3d" or "0x15 0x3D".  The OctetString construct  returned
     by  this  call can be free'ed by a call to free_octetstring.
     This is usually unnecessary as the construct is  most  often
     passed  to another library routine for inclusion in a larger
     ASN.1 construct and that  library's  free  counterpart  will
     take care of memory recovery.
*/

OctetString *
make_octet_from_hex(unsigned char *text_str)
{
  OctetString *os_ptr;
  int value;
  short i, cc;
  unsigned char *temp_ptr;
  int temp_len;

  if ((os_ptr = (OctetString *) malloc(sizeof (OctetString))) == NULL) 
    {
      LIB_PERROR("make_octet_from_hex, os_ptr");
      return (NULL);
    }
  os_ptr->octet_ptr = NULL;
  
  /*
   * reducing local variables saves stack which wins big on PC's
   * using small memory model.  That's why, OK?
   */

  temp_len = ((int)strlen((char *)text_str) / 2) + 1;

  if ((os_ptr->octet_ptr = (unsigned char *)malloc(temp_len)) == NULL) 
    {
      LIB_PERROR("make_octet_from_hex, octet_ptr:");
      free_octetstring(os_ptr);
      NULLIT(os_ptr);
      return (NULL);
    }
  for (i = 0, temp_ptr = text_str; ((i < 40) && (temp_ptr != NULL) &&
				    (*temp_ptr != '\0')); i++) 
    {
      if ((cc = sscanf((char *)temp_ptr, "%x", &value)) != 1)
	return (NULL);

      /* sets temp_ptr to null when we hit end of string */
      os_ptr->octet_ptr[i] = (unsigned char)value;

      temp_ptr = (unsigned char *)strchr((char *)temp_ptr, ' ');
      while ((temp_ptr != NULL) &&
	     ((*temp_ptr == ' ') || 
	      (!isxdigit(*temp_ptr))) && (*temp_ptr != '\0'))
	temp_ptr++;
    }
  os_ptr->length = i;

  return (os_ptr);
}

/*
make_obj_id_from_hex

     This routine is used to create a library format Object Iden-
     tifier data structure for use by calls to make_varbind() and
     make_authentication() from hex text strings.  The format  of
     these strings is a hex value (1 or more hex digits, upper or
     lower case) followed by a space, with more  hex  values  and
     spaces  to finish out the string.  For example, to create an
     Object Identifier consisting of three sub-identifiers  (say,
     1.21.51)  text string could be "1 15 3d" or "0x1 0x15 0x3D".
     The Object Identifier construct returned by this call can be
     free'ed  by a call to free_oid.  This is usually unnecessary
     as the construct is most often  passed  to  another  library
     routine  for  inclusion in a larger ASN.1 construct and that
     library's  free  counterpart  will  take  care   of   memory
     recovery.
*/

OID
make_obj_id_from_hex(unsigned char *text_str)
{
	OID oid_ptr;
	int i, cc;
	unsigned char *temp_ptr;
	int temp_value;

	if ((oid_ptr = (OID) malloc(sizeof (OIDentifier))) == NULL) {
		LIB_PERROR("make_obj_id_from_hex, oid_ptr");
		return (NULL);
	}
	oid_ptr->oid_ptr = NULL;

	if ((oid_ptr->oid_ptr = (unsigned int *)malloc((((int)strlen((char *)text_str) / 2) + 1) * sizeof (unsigned int))) == NULL) {
		LIB_PERROR("make_obj_id_from_hex, oid_ptr");
		free(oid_ptr);
		NULLIT(oid_ptr);
		return (NULL);
	}
	for (i = 0, temp_ptr = text_str;
	     ((i < 40) && (temp_ptr != NULL) && (*temp_ptr != '\0') &&
	      ((cc = sscanf((char *)temp_ptr, "%x", &temp_value)) == 1));
	     i++) {

		oid_ptr->oid_ptr[i] = temp_value;

		temp_ptr = (unsigned char *)strchr((char *)temp_ptr, ' ');
		while ((temp_ptr != NULL) &&
		       ((*temp_ptr == ' ') || (!isxdigit(*temp_ptr))) && (*temp_ptr != '\0'))
			temp_ptr++;
	}

	oid_ptr->length = i;
	return (oid_ptr);
}

/*
make_oid_from_dot

     This routine is called to create a library form object identifier
    from a character string. The string input is usually in the format
    of "integer.integer.integer..." (i.e. "1.3.6.1.2.1.0"). It returns
    a pointer to a malloc'd data structure containing internal library
    representation  for an object identifier.
*/

OID 
make_oid_from_dot(unsigned char *text_str)
{
	OID oid_ptr;
	int i, dot_count;
	unsigned char temp_buf[256];
	unsigned char *temp_ptr;

	strcpy((char *)temp_buf, (char *)text_str);

	dot_count = 0;
	for (i = 0; temp_buf[i] != '\0'; i++) {
		if (temp_buf[i] == '.')
			dot_count++;
	}

	if ((oid_ptr = (OID) malloc(sizeof *oid_ptr)) == NULL) {
		LIB_PERROR("make_oid_from_dot, oid_ptr");
		return (NULL);
	}
	oid_ptr->oid_nelem = dot_count + 1;
	oid_ptr->oid_elements = NULL;

	if ((oid_ptr->oid_elements =
	     (unsigned int *)calloc((unsigned int)(dot_count + 1), sizeof (unsigned int))) == NULL) {
		LIB_PERROR("make_oid_from_dot, oid_ptr->oid_elements");
		free_oid(oid_ptr);
		NULLIT(oid_ptr);
		return (NULL);
	}
	temp_ptr = temp_buf;

	for (i = 0; i < dot_count + 1; i++) {
		if (isdigit(*temp_ptr)) {
			if ((oid_ptr->oid_elements[i] = parse_sub_id_decimal(&temp_ptr)) < 0) {
				LIB_ERROR("make_oid_from_dot, decimal:\n");
				free_oid(oid_ptr);
				NULLIT(oid_ptr);
				return (NULL);
			}
		} else {
			if (*temp_ptr == '.')
				temp_ptr++;	/* to skip over dot */
			else {
				LIB_ERROR2("make_oid_from_dot, bad character: %d, %s \n",
					   *temp_ptr, temp_ptr);
				free_oid(oid_ptr);
				NULLIT(oid_ptr);
				return (NULL);
			}
		}
	}
	return (oid_ptr);

}				       /* end of make_oid_from_dot */
