/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.lib/libsnmp/auth_lib.c	1.1"
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
static char SNMPID[] = "@(#)auth_lib.c	6.2 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */


#include <stdio.h>

#include <snmp/snmp.h>

/*
 * Authentication is now an ASN.1 entity, so make it here.
 */

/*
make_authentication

     This routine is used to create a library format  authentica-
     tion  header data structure for use by build_authentication.
     This particular implementation of  the  library  creates  an
     authentication  header based on the 'trivial' authentication
     put forth by  RFC1065-66-67,  which  is  a  community  octet
     string  ususally  based  on  text.  The header and the octet
     string  associated  with  the  header   are   free'ed   when
     free_authentication()  is  called  with  the  authentication
     pointer.
*/

AuthHeader *
make_authentication(community)
	OctetString *community;
{
	AuthHeader *auth_ptr;

	if ((auth_ptr = (AuthHeader *) malloc(sizeof (AuthHeader))) == NULL) {
		LIB_PERROR("make_authentication, auth_ptr:");
		return (NULL);
	}
	auth_ptr->community = NULL;
	auth_ptr->version = VERSION;
	auth_ptr->packlet = NULL;

	auth_ptr->community = community;

	return (auth_ptr);
}				       /* end of make_authentication() */

/*
build_authentication

     This routine takes the PDU and the  authentication  informa-
     tion and builds the actual SNMP packet in the authentication
     data structure's  *packlet  octet  string  area.

     auth_ptr->packlet->octet_ptr  points  to  the  packet  and
     auth_ptr->packlet->length points to the packet's length.  This
     'packlet'  is free'ed when free_authentication() is called, so it
     should be bcopy()'ed to a holding area or the authentication
     should  not  be  free'ed  until the packet is actually sent.
     Once this has been done, the authentication can  be  free'ed
     with a call to free_authentication().
*/

short
build_authentication(auth_ptr, pdu_ptr)
	AuthHeader *auth_ptr;
	Pdu *pdu_ptr;
{
	long i;
	short cc;
	long lenlen;

	long pdudatalen;
	long datalen;
	unsigned char *working_ptr;

	/* make sure auth_ptr->packlet is clean and ready to roll */
	free_octetstring(auth_ptr->packlet);

	if ((auth_ptr->packlet = (OctetString *) malloc(sizeof (OctetString))) == NULL) {
		LIB_PERROR("build_authentication, packlet:");
		return (-1);
	}
	auth_ptr->packlet->octet_ptr = NULL;

	if (pdu_ptr == NULL) {
		LIB_ERROR("build_authentication, NO PDU!\n");
		free_octetstring(auth_ptr->packlet);
		auth_ptr->packlet = NULL;
		return (-1);
	}
	pdudatalen = pdu_ptr->packlet->length;
	datalen = find_len_unsignedinteger(auth_ptr->version) +
	    find_len_octetstring(auth_ptr->community) + pdudatalen;
	lenlen = dolenlen(datalen);

	auth_ptr->packlet->length = 1 + lenlen + datalen;

	if ((auth_ptr->packlet->octet_ptr =
	   (unsigned char *)malloc((int)auth_ptr->packlet->length)) == NULL) {
		LIB_PERROR("build_authentication:");
		free_octetstring(auth_ptr->packlet);
		auth_ptr->packlet = NULL;
		return (-1);
	}
	working_ptr = auth_ptr->packlet->octet_ptr;

	*working_ptr++ = SEQUENCE_TYPE;
	add_len(&working_ptr, lenlen, datalen);

	add_unsignedinteger(&working_ptr, INTEGER_TYPE, auth_ptr->version);
	add_octetstring(&working_ptr, OCTET_PRIM_TYPE, auth_ptr->community);

	for (i = 0; i < pdudatalen; i++)
		*working_ptr++ = pdu_ptr->packlet->octet_ptr[i];
	return (0);
}

/*
free_authentication

     This routine frees all memory associated  with  a  'trivial'
     authentication  header  data structure, including the actual
     SNMP packet that build_authentication creates and the  octet
     string  associated with make_authentication.  The PDU struc-
     ture  pointed  to   by   the   pdu_ptr   passed   to   make-
     authentication() is NOT touched.
*/

void
free_authentication(auth_ptr)
	AuthHeader *auth_ptr;
{
	if (auth_ptr != NULL) {
		free_octetstring(auth_ptr->community);
		free_octetstring(auth_ptr->packlet);
		free(auth_ptr);
	}
}

/*
parse_authentication

     This routine is used to create a library format  authentica-
     tion header data structure from an incoming SNMP packet.  If
     parsing errors occur, a message  is  outputted  to  standard
     error   and   the  routine  returns  NULL.   Otherwise,  the
     sesssion_ptr part of  the  structure  should  be  check  for
     authentication  and the pointer passed on to parse_pdu() for
     further ASN.1 parsing.  It should be noted that the state of
     the  authentication header created during the building phase
     after a call to  build_authentication  is  nearly  identical
     to the state of the authentication header after this call on
     the parsing side.
*/

AuthHeader *
parse_authentication(packet_ptr, length)
	unsigned char *packet_ptr;
	long length;
{
	AuthHeader *auth_ptr;
	unsigned char *working_ptr;
	unsigned char *end_ptr;
	unsigned char *new_end_ptr;
	short type;
	long seq_length;

	working_ptr = (unsigned char *)packet_ptr;
	end_ptr = working_ptr + length;

	if ((auth_ptr = (AuthHeader *) malloc(sizeof (AuthHeader))) == NULL) {
		LIB_PERROR("parse_authentication,auth_ptr:");
		return (NULL);
	}
	auth_ptr->packlet = NULL;
	auth_ptr->community = NULL;

	if ((seq_length = parse_sequence(&working_ptr, end_ptr, &type)) == -1) {
		LIB_ERROR("Parse_authentication, parse_sequence:\n");
		free_authentication(auth_ptr);
		auth_ptr = NULL;
		return (NULL);
	}
	/*
   * Re-calculate end_ptr, based on ASN.1 structure's internal length.
   * If they don't match up, post warning and continue with ASN.1's
   * internal number (or should I just abort.  just silently use new num?)
   */

	new_end_ptr = working_ptr + seq_length;

	if (new_end_ptr != end_ptr) {
		LIB_ERROR2("parse_authentication: end pointers do not match n:%x, o:%x\n",
			   new_end_ptr, end_ptr);
		end_ptr = new_end_ptr;
	}
	auth_ptr->version = parse_unsignedinteger(&working_ptr, end_ptr, &type);
	if (type == -1) {
		LIB_ERROR("Parse_authentication, version:\n");
		free_authentication(auth_ptr);
		auth_ptr = NULL;
		return (NULL);
	}
	if ((auth_ptr->community = parse_octetstring(&working_ptr, end_ptr, &type)) == NULL) {
		LIB_ERROR("parse_authentication, community\n");
		free_authentication(auth_ptr);
		auth_ptr = NULL;
		return (NULL);
	}
	/* copy what's left of data into auth_ptr->packlet */
	auth_ptr->packlet = make_octetstring(working_ptr, (unsigned long)(end_ptr - working_ptr));

	return (auth_ptr);
}
