/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsnmp:prnt_lib.c	1.4"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libsnmp/prnt_lib.c,v 1.9 1994/08/02 23:36:48 cyang Exp $"
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
static char TCPID[] = "@(#)prnt_lib.c	1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)prnt_lib.c	6.2 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */


#include <stdio.h>
#include <ctype.h>

#include "snmp.h"
#include "snmpuser.h"

/*
print_packet_out

     This routine prints out the contents of a buffer in  hex  at
     16  bytes  per  line.   It  is  called with a pointer to the
     buffer to be displayed and the number of  bytes  to  display
     out  of  the buffer.  This call is frequently used in debug-
     ging code to display the actual SNMP message that  has  been
     received  to  allow hand parsing of the message.  It is gen-
     erally unsuitable for a production user interface.
*/

void
print_packet_out(unsigned char *ptr, long len)
{
  OctetString oct;

  printf("\nPacket Dump:");

  oct.octet_ptr = ptr;
  oct.length = len;

  print_octet_string_out(&oct, 16);
}

/*
print_octet_string_out

     This routine prints out the contents  of  a  Octet  String's
     value  in  hex.   It  is called with a pointer to the Octet-
     String construct and the number of bytes to display  on  one
     line  (the  variable  'wrap').   This  call  is  used by the
     print_varbind() routine to actually print out  octet  string
     values.
*/

void
print_octet_string_out(OctetString *oct_ptr, short wrap)
{
  long i;

  for (i = 0; i < oct_ptr->length;) 
    {
      printf("%2.2x ", oct_ptr->octet_ptr[i]);
      i++;
      if ((i % wrap) == 0)
	printf("\n");
    }
  printf("\n");
}

/*
print_var_bind_list

     This routine prints out the contents of a varbind list in  a
     human  readable  form.   This  is a quick user interface for
     printing out the SNMP responses in  simple  SNMP  utilities.
     If  the PDU structure is pointed to by *pdu_ptr, the call is
     print_varbind_list(pdu_ptr->var_bind_list).
*/

short
print_varbind_list(VarBindList *vbl_ptr)
{
  VarBindUnit *vb_ptr;
  char buffer[128];
  short cc;

  buffer[0] = '\0';
  if (vbl_ptr == NULL)
    return (1);

  vb_ptr = vbl_ptr->vb_ptr;
  if ((cc = make_dot_from_obj_id(vb_ptr->name, buffer)) == -1) 
    {
      printf("print_varbind_list, vb_ptr->name:\n");
      return (-1);
    }
  printf("\nName: %s\n", buffer);

  switch (vb_ptr->value->type) 
    {
    case COUNTER_TYPE:	       /* handle unsigned integers */
      printf("Type: Counter\nValue: %lu\n", vb_ptr->value->ul_value);
      break;

    case GAUGE_TYPE:
      printf("Type: Gauge\nValue: %lu\n", 
		 vb_ptr->value->ul_value);
      break;

    case INTEGER_TYPE:	       /* handle signed integers */
      printf("Type: Integer\nValue: %ld\n", vb_ptr->value->sl_value);
      break;

    case TIME_TICKS_TYPE:
      printf("Type: Time Ticks\nValue: %ld\n", vb_ptr->value->sl_value);
      break;

    case IP_ADDR_PRIM_TYPE:
    case IP_ADDR_CONSTRUCT_TYPE:
      printf("Type: IP Address\nValue: %d.%d.%d.%d\n",
	      vb_ptr->value->os_value->octet_ptr[0],
	      vb_ptr->value->os_value->octet_ptr[1],
	      vb_ptr->value->os_value->octet_ptr[2],
	      vb_ptr->value->os_value->octet_ptr[3]);
      break;

    case OBJECT_ID_TYPE:
      if ((cc = make_dot_from_obj_id(vb_ptr->value->oid_value, buffer)) == -1) 
	{
	  printf("print_varbind_list, vb_ptr->value->oid_value:\n");
	  return (-1);
	}
      printf("Type: Object Identifier\nValue: %s\n", buffer);
      break;

    case OCTET_PRIM_TYPE:	       /* handle quasi-octet strings */
    case OCTET_CONSTRUCT_TYPE:
    case OPAQUE_PRIM_TYPE:
    case OPAQUE_CONSTRUCT_TYPE:
      printf("Type: Octet String\nValue: ");
      if (print_ascii(vb_ptr->value->os_value) < 0) 
	{	/* if can't print ascii */
	  print_octet_string_out(vb_ptr->value->os_value, 30);
	}
      break;
    
    case NULL_TYPE:
      printf("Type: NULL\nValue: NULL\n");
      break;

    default:
      LIB_ERROR1("print_varbind: Illegal type: 0x%x\n", vb_ptr->value->type);
      return (-1);
    };			       /* end of switch */

  /* do next one now */
  return (print_varbind_list(vbl_ptr->next));
}				       /* end of print_varbind_list() */

/*
print_ascii

     This routine prints out the contents  of  a  Octet  String's
     value  as  an ascii string if the value only contains print-
     able characters.  It is called with a pointer to the  Octet-
     String  construct and checks if the string is printable.  If
     it is not it returns a -1 value, otherwise it returns a 1.
*/
short
print_ascii(OctetString *os_ptr)
{
  long i;

  for (i = 0; i < os_ptr->length; i++) 
    {
      /* ASCII space is not a printable character in MSC */
      if (((!isprint(os_ptr->octet_ptr[i])) && 
	   (os_ptr->octet_ptr[i] != 0x20) &&
	   (os_ptr->octet_ptr[i] != 0x0a) && 
	   (os_ptr->octet_ptr[i] != 0x0d)) ||
	  ((os_ptr->octet_ptr[i] == 0x00) && 
	   (i != os_ptr->length - 1))) 
	{
	  return (-1);
	}
    }

  for (i = 0; i < os_ptr->length; i++) 
    {
      switch (os_ptr->octet_ptr[i]) 
	{
	case 0x00:
	case 0x0d:
	case 0x0a:
	  printf("\n");
#if 0
	  return (1);    /* SCA -- removed to support Cisco sysDescr.0 */
#endif
	  break;
	default:
	  printf("%c", os_ptr->octet_ptr[i]);
	}
    }
  printf("\n");
  return (1);
}
