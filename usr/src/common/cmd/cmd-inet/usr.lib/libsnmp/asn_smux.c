/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.lib/libsnmp/asn_smux.c	1.1"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/* asn_smux.c - ASN.1 encoding/decoding of SMUX PDUs */

#include <stdio.h>
#include <snmp/snmp.h>

short
encode_SMUX_PDU(pdu_ptr)
	SNMP_SMUX_PDU *pdu_ptr;
{
	long varbindlen;
	long varbind_tot_len;
	long varbindlenlen;
	long datalen, temp_data_len;
	long lenlen;
	long len_len;
	unsigned char *working_ptr;
	short cc;

	switch (pdu_ptr->offset) {
	case SMUX__PDUs_simple:
		datalen =
		    find_len_unsignedinteger(pdu_ptr->un.simple->version) +
		    find_len_oid(pdu_ptr->un.simple->identity) +
		    find_len_octetstring(pdu_ptr->un.simple->description) +
		    find_len_octetstring(pdu_ptr->un.simple->password);
		break;

	case SMUX__PDUs_close:
		datalen =
		    find_len_signedinteger(pdu_ptr->un.close->parm);
		break;

	case SMUX__PDUs_registerRequest:
		datalen =
		    find_len_oid(pdu_ptr->un.registerRequest->subtree) +
		    find_len_signedinteger(pdu_ptr->un.registerRequest->priority) +
		    find_len_signedinteger(pdu_ptr->un.registerRequest->operation);
		break;

	case SMUX__PDUs_registerResponse:
		datalen =
		    find_len_signedinteger(pdu_ptr->un.registerResponse->parm);
		break;

	case SMUX__PDUs_commitOrRollback:
		datalen =
		    find_len_signedinteger(pdu_ptr->un.commitOrRollback->parm);
		break;

	case SMUX__PDUs_get__request:
	case SMUX__PDUs_get__next__request:
	case SMUX__PDUs_get__response:
	case SMUX__PDUs_set__request:
		if ((varbindlen = find_len_varbind(pdu_ptr->un.get__request->variable__bindings)) == -1) {
			LIB_ERROR("encode_SMUX_PDU, varbindlen:\n");
			return (-1);   /* abort */
		}
		if ((varbindlenlen = dolenlen(varbindlen)) == -1) {
			LIB_ERROR("encode_SMUX_PDU, varbindlenlen:\n");
			return (-1);   /* abort */
		}
		varbind_tot_len = 1 + varbindlenlen + varbindlen;

		datalen =
		    find_len_signedinteger(pdu_ptr->un.get__request->request__id) +
		    find_len_signedinteger(pdu_ptr->un.get__request->error__status) +
		    find_len_signedinteger(pdu_ptr->un.get__request->error__index) +
		    varbind_tot_len;
		break;

	case SMUX__PDUs_trap:
		if ((varbindlen = find_len_varbind(pdu_ptr->un.trap->variable__bindings)) == -1) {
			LIB_ERROR("encode_SMUX_PDU, varbindlen:\n");
			return (-1);   /* abort */
		}
		if ((varbindlenlen = dolenlen(varbindlen)) == -1) {
			LIB_ERROR("encode_SMUX_PDU, varbindlenlen:\n");
			return (-1);   /* abort */
		}
		varbind_tot_len = 1 + varbindlenlen + varbindlen;

		datalen =
		    find_len_signedinteger(pdu_ptr->un.trap->generic__trap) +
		    find_len_signedinteger(pdu_ptr->un.trap->specific__trap) +
		    find_len_signedinteger(pdu_ptr->un.trap->time__stamp) +
		    find_len_oid(pdu_ptr->un.trap->enterprise) +
		    find_len_octetstring(pdu_ptr->un.trap->agent__addr) +
		    varbind_tot_len;
		break;

	default:
		LIB_ERROR1("encode_SMUX_PDU, bad pdu type: %x\n", pdu_ptr->offset);
		return (-1);
		break;
	}			       /* end of switch */

	if ((lenlen = dolenlen(datalen)) == -1) {
		LIB_ERROR("encode_SMUX_PDU, lenlen:\n");
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
	/*
	add_signedinteger (&working_ptr, INTEGER_TYPE, pdu_ptr->offset);
	add_len(&working_ptr, lenlen, datalen);
	*/

	*working_ptr++ = (unsigned char)(0xff & pdu_ptr->offset);
	add_len(&working_ptr, lenlen, datalen);

	switch (pdu_ptr->offset) {
	case SMUX__PDUs_simple:
		add_unsignedinteger(&working_ptr, INTEGER_TYPE,
				    pdu_ptr->un.simple->version);
		add_oid(&working_ptr, OBJECT_ID_TYPE,
			pdu_ptr->un.simple->identity);
		add_octetstring(&working_ptr, OCTET_PRIM_TYPE,
				pdu_ptr->un.simple->description);
		add_octetstring(&working_ptr, OCTET_PRIM_TYPE,
				pdu_ptr->un.simple->password);
		break;

	case SMUX__PDUs_close:
		add_signedinteger(&working_ptr, INTEGER_TYPE,
				  pdu_ptr->un.close->parm);
		break;

	case SMUX__PDUs_registerRequest:
		add_oid(&working_ptr, OBJECT_ID_TYPE,
			pdu_ptr->un.registerRequest->subtree);
		add_signedinteger(&working_ptr, INTEGER_TYPE,
				  pdu_ptr->un.registerRequest->priority);
		add_signedinteger(&working_ptr, INTEGER_TYPE,
				  pdu_ptr->un.registerRequest->operation);
		break;

	case SMUX__PDUs_registerResponse:
		add_signedinteger(&working_ptr, INTEGER_TYPE,
				  pdu_ptr->un.registerResponse->parm);
		break;

	case SMUX__PDUs_commitOrRollback:
		add_signedinteger(&working_ptr, INTEGER_TYPE,
				  pdu_ptr->un.commitOrRollback->parm);
		break;

	case SMUX__PDUs_get__request:
	case SMUX__PDUs_get__next__request:
	case SMUX__PDUs_get__response:
	case SMUX__PDUs_set__request:
		add_signedinteger(&working_ptr, INTEGER_TYPE,
				pdu_ptr->un.get__request->request__id);
		add_signedinteger(&working_ptr, INTEGER_TYPE,
				pdu_ptr->un.get__request->error__status);
		add_signedinteger(&working_ptr, INTEGER_TYPE,
				pdu_ptr->un.get__request->error__index);

		*working_ptr++ = SEQUENCE_TYPE;
		add_len(&working_ptr, varbindlenlen, varbindlen);
		if ((cc = add_varbind(&working_ptr,
			     pdu_ptr->un.get__request->variable__bindings)) == -1) {
			free_octetstring(pdu_ptr->packlet);
			NULLIT(pdu_ptr->packlet);
			return (-1);
		}
		break;

	case SMUX__PDUs_trap:
		add_oid(&working_ptr, OBJECT_ID_TYPE,
			pdu_ptr->un.trap->enterprise);
		add_octetstring(&working_ptr, IP_ADDR_PRIM_TYPE,
				pdu_ptr->un.trap->agent__addr);
		add_signedinteger(&working_ptr, INTEGER_TYPE,
				pdu_ptr->un.trap->generic__trap);
		add_signedinteger(&working_ptr, INTEGER_TYPE,
				pdu_ptr->un.trap->specific__trap);
		add_signedinteger(&working_ptr, TIME_TICKS_TYPE,
				pdu_ptr->un.trap->time__stamp);

		*working_ptr++ = SEQUENCE_TYPE;
		add_len(&working_ptr, varbindlenlen, varbindlen);
		if ((cc = add_varbind(&working_ptr,
				    pdu_ptr->un.trap->variable__bindings)) == -1) {
			free_octetstring(pdu_ptr->packlet);
			NULLIT(pdu_ptr->packlet);
			return (-1);
		}
		break;

	default:
		LIB_ERROR1("encode_SMUX_PDU, bad pdu_ptr->offset - II. Shouldn't happen!:%x\n",
			   pdu_ptr->offset);
		free_octetstring(pdu_ptr->packlet);
		NULLIT(pdu_ptr->packlet);
		return (-1);
		break;
	}			       /* end of switch II */

	return (0);
}				       /* end of encode_SMUX_PDU */

SNMP_SMUX_PDU *
decode_SMUX_PDU(packet_ptr, length)
	unsigned char *packet_ptr;
	long length;
{
	unsigned char *working_ptr, *end_ptr;
	SNMP_SMUX_PDU *pdu_ptr;
	VarBindList *vbl_ptr;
	VarBindList *var_bind_list;
	VarBindList *var_bind_end_ptr;
	short pdu_type, type;
	long  encoded_len;

	working_ptr = packet_ptr;
	end_ptr = working_ptr + length;

	if ((pdu_ptr = (SNMP_SMUX_PDU *) malloc(sizeof (SNMP_SMUX_PDU))) == NULL) {
		LIB_PERROR("decode_SMUX_PDU, pdu_ptr:");
		return (NULL);
	}
	pdu_ptr->packlet = NULL;

	if ((pdu_type = parse_type(&working_ptr, end_ptr)) == -1) {
		LIB_ERROR("decode_SMUX_PDU, pdu_type\n");
		free_SNMP_SMUX_PDU(pdu_ptr);
		NULLIT(pdu_ptr);
		return (NULL);
	}
	pdu_ptr->offset = pdu_type;

	if ((encoded_len = parse_length(&working_ptr, end_ptr)) == -1) {
		LIB_ERROR("parse_pdu, encoded_len\n");
		free_SNMP_SMUX_PDU(pdu_ptr);
		NULLIT(pdu_ptr);
		return (NULL);
	}
	if (working_ptr + encoded_len > end_ptr) {
		LIB_ERROR("decode_pdu, bad encoded_len:\n");
		free_SNMP_SMUX_PDU(pdu_ptr);
		NULLIT(pdu_ptr);
		return (NULL);
	}
	switch ((int)pdu_type) {
	case SMUX__PDUs_simple:
		{
			SMUX_SimpleOpen *simple;

			if ((simple = (SMUX_SimpleOpen *) malloc(sizeof (SMUX_SimpleOpen))) == NULL) {
				LIB_PERROR("decode_SMUX_PDU, simple:");
				return (NULL);
			}
			pdu_ptr->un.simple = simple;
			pdu_ptr->un.simple->version =
			    parse_unsignedinteger(&working_ptr, end_ptr, &type);
			if (type == -1) {
				LIB_ERROR("decode_SMUX_PDU, version \n");
				goto failed;
			}
			pdu_ptr->un.simple->identity = NULL;
			pdu_ptr->un.simple->identity =
			    parse_oid(&working_ptr, end_ptr);
			if (pdu_ptr->un.simple->identity == NULL) {
				LIB_ERROR("decode_SMUX_PDU, identity \n");
				goto failed;
			}
			pdu_ptr->un.simple->description = NULL;
			pdu_ptr->un.simple->description =
			    parse_octetstring(&working_ptr, end_ptr, &type);
			if (type == -1) {
				LIB_ERROR("decode_SMUX_PDU, description \n");
				goto failed;
			}
			pdu_ptr->un.simple->password = NULL;
			pdu_ptr->un.simple->password =
			    parse_octetstring(&working_ptr, end_ptr, &type);
			if (type == -1) {
				LIB_ERROR("decode_SMUX_PDU, password \n");
				goto failed;
			}
			break;
		}

	case SMUX__PDUs_close:
		{
			SMUX_ClosePDU *close;

			if ((close = (SMUX_ClosePDU *) malloc(sizeof (SMUX_ClosePDU))) == NULL) {
				LIB_PERROR("decode_SMUX_PDU, close:");
				return (NULL);
			}
			pdu_ptr->un.close = close;
			pdu_ptr->un.close->parm =
			    parse_signedinteger(&working_ptr, end_ptr, &type);
			if (type == -1) {
				LIB_ERROR("decode_SMUX_PDU, close.parm \n");
				goto failed;
			}
			break;
		}

	case SMUX__PDUs_registerRequest:
		{
			SMUX_RReqPDU *rreq;

			if ((rreq = (SMUX_RReqPDU *) malloc(sizeof (SMUX_RReqPDU))) == NULL) {
				LIB_PERROR("decode_SMUX_PDU, rreq:");
				return (NULL);
			}
			pdu_ptr->un.registerRequest = rreq;
			pdu_ptr->un.registerRequest->subtree = NULL;
			pdu_ptr->un.registerRequest->subtree =
			    parse_oid(&working_ptr, end_ptr);
			if (pdu_ptr->un.registerRequest->subtree == NULL) {
				LIB_ERROR("decode_SMUX_PDU, subtree \n");
				goto failed;
			}
			pdu_ptr->un.registerRequest->priority =
			    parse_signedinteger(&working_ptr, end_ptr, &type);
			if (type == -1) {
				LIB_ERROR("decode_SMUX_PDU, priority \n");
				goto failed;
			}
			pdu_ptr->un.registerRequest->operation =
			    parse_signedinteger(&working_ptr, end_ptr, &type);
			if (type == -1) {
				LIB_ERROR("decode_SMUX_PDU, operation \n");
				goto failed;
			}
			break;
		}

	case SMUX__PDUs_registerResponse:
		{
			SMUX_RRspPDU *rrsp;

			if ((rrsp = (SMUX_RRspPDU *) malloc(sizeof (SMUX_RRspPDU))) == NULL) {
				LIB_PERROR("decode_SMUX_PDU, rrsp:");
				return (NULL);
			}
			pdu_ptr->un.registerResponse = rrsp;
			pdu_ptr->un.registerResponse->parm =
			    parse_signedinteger(&working_ptr, end_ptr, &type);
			if (type == -1) {
				LIB_ERROR("decode_SMUX_PDU, registerResponse.parm \n");
				goto failed;
			}
			break;
		}

	case SMUX__PDUs_commitOrRollback:
		{
			SMUX_SOutPDU *sout;

			if ((sout = (SMUX_SOutPDU *) malloc(sizeof (SMUX_SOutPDU))) == NULL) {
				LIB_PERROR("decode_SMUX_PDU, sout:");
				return (NULL);
			}
			pdu_ptr->un.commitOrRollback = sout;
			pdu_ptr->un.commitOrRollback->parm =
			    parse_signedinteger(&working_ptr, end_ptr, &type);
			if (type == -1) {
				LIB_ERROR("decode_SMUX_PDU, commitOrRollback.parm \n");
				goto failed;
			}
			break;
		}

	case SMUX__PDUs_get__request:
	case SMUX__PDUs_get__next__request:
	case SMUX__PDUs_get__response:
	case SMUX__PDUs_set__request:
	case SMUX__PDUs_trap:
		{
			SMUX_GetRequest_PDU *get;
			SMUX_Trap_PDU *trap;

			if (pdu_type != SMUX__PDUs_trap) {
				if ((get = (SMUX_GetRequest_PDU *) malloc(sizeof (SMUX_GetRequest_PDU))) == NULL) {
					LIB_PERROR("decode_SMUX_PDU, get:");
					return (NULL);
				}
				pdu_ptr->un.get__request = get;
				pdu_ptr->un.get__request->request__id =
				    parse_signedinteger(&working_ptr, end_ptr, &type);
				if (type == -1) {
					LIB_ERROR("decode_SMUX_PDU, request__id \n");
					goto failed;
				}
				pdu_ptr->un.get__request->error__status =
				    parse_signedinteger(&working_ptr, end_ptr, &type);
				if (type == -1) {
					LIB_ERROR("decode_SMUX_PDU, error__status \n");
					goto failed;
				}
				pdu_ptr->un.get__request->error__index =
				    parse_signedinteger(&working_ptr, end_ptr, &type);
				if (type == -1) {
					LIB_ERROR("decode_SMUX_PDU, error__index \n");
					goto failed;
				}
			} else {
				if ((trap = (SMUX_Trap_PDU *) malloc(sizeof (SMUX_Trap_PDU))) == NULL) {
					LIB_PERROR("decode_SMUX_PDU, trap:");
					return (NULL);
				}
				pdu_ptr->un.trap = trap;
				pdu_ptr->un.trap->enterprise = NULL;
				pdu_ptr->un.trap->enterprise =
				    parse_oid(&working_ptr, end_ptr);
				if (pdu_ptr->un.trap->enterprise ==
					NULL) {
					LIB_ERROR("decode_SMUX_PDU, enterprise \n");
					goto failed;
				}
				pdu_ptr->un.trap->agent__addr = NULL;
				pdu_ptr->un.trap->agent__addr =
				    parse_octetstring(&working_ptr, end_ptr, &type);
				if (type == -1) {
					LIB_ERROR("decode_SMUX_PDU, agent__addr \n");
					goto failed;
				}
				pdu_ptr->un.trap->generic__trap =
				    parse_signedinteger(&working_ptr, end_ptr, &type);
				if (type == -1) {
					LIB_ERROR("decode_SMUX_PDU, generic__trap \n");
					goto failed;
				}
				pdu_ptr->un.trap->specific__trap =
				    parse_signedinteger(&working_ptr, end_ptr, &type);
				if (type == -1) {
					LIB_ERROR("decode_SMUX_PDU, specific__trap \n");
					goto failed;
				}
				pdu_ptr->un.trap->time__stamp =
				    parse_signedinteger(&working_ptr, end_ptr, &type);
				if (type == -1) {
					LIB_ERROR("decode_SMUX_PDU, time__stamp \n");
					goto failed;
				}
			}
			var_bind_list = NULL;
			var_bind_end_ptr = NULL;

			/* now strip out the sequence of */
			if (parse_sequence(&working_ptr, end_ptr, &type) == -1) {
				LIB_ERROR("decode_SMUX_PDU, parse_sequence failure\n");
				goto failed;
			}
			/* now parse the varbind list */
			while (working_ptr < end_ptr) {
				if ((vbl_ptr = parse_varbind(&working_ptr, end_ptr)) == NULL) {
					LIB_ERROR("decode_SMUX_PDU, vbl_ptr\n");
					goto failed;
				}
				/* is this first one? */
				if (var_bind_list == NULL) {	/* start list */
					var_bind_list = vbl_ptr;
					var_bind_end_ptr = vbl_ptr;
				} else {	/* tack onto end of list */
					var_bind_end_ptr->next = vbl_ptr;
					var_bind_end_ptr = vbl_ptr;
				}

				/* DON'T FREE vbl_ptr! Just hand it off to the pdu */
				vbl_ptr = NULL;
			};	       /* end of while */

			if (pdu_type != SMUX__PDUs_trap)
				pdu_ptr->un.get__request->variable__bindings = var_bind_list;
			else
				pdu_ptr->un.trap->variable__bindings = var_bind_list;

			break;
		}

	default:
		LIB_ERROR1("decode_SMUX_PDU, bad pdu_type: %x\n", pdu_type);
failed:;
		free_pdu((Pdu *)pdu_ptr);
		NULLIT(pdu_ptr);
		return (NULL);
	}

	return (pdu_ptr);

}				       /* end of decode_SMUX_PDU */
