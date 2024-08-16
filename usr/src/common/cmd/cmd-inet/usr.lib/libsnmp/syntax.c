/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.lib/libsnmp/syntax.c	1.1"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/* syntax.c - SMI syntax handling */

/*
 *
 * Contributed by NYSERNet Inc. This work was partially supported by
 * the U.S. Defense Advanced Research Projects Agency and the Rome
 * Air Development Center of the U.S. Air Force Systems Command under
 * contract number F30602-88-C-0016.
 *
 */

/*
 * All contributors disclaim all warranties with regard to this
 * software, including all implied warranties of mechantibility
 * and fitness. In no event shall any contributor be liable for
 * any special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in action of contract, negligence or other tortuous action,
 * arising out of or in connection with, the use or performance
 * of this software.
 */

/*
 * As used above, "contributor" includes, but not limited to:
 * NYSERNet, Inc.
 * Marshall T. Rose
 */

#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <snmp/objects.h>

#define	MAXSYN	 50

OID oid_cpy();
void free_oid();
void free_octetstring();
OctetString *make_octetstring();

static struct syntax syntaxes[MAXSYN + 1];
static OS synlast = syntaxes;

/* INTEGER */
static int 
integer_decode(x, value)
	int **x;
	ObjectSyntax *value;
{
	int i = value->sl_value;

	if ((*x = (int *)malloc(sizeof **x)) == NULL)
		return NOTOK;
	**x = i;

	return OK;
}

static
integer_free(x)
	int *x;
{
	free((char *)x);
}

static 
add_integer()
{
	(void)add_syntax("INTEGER", INTEGER_TYPE, integer_decode, integer_free);
	(void)add_syntax("Services", INTEGER_TYPE, integer_decode, integer_free);
	(void)add_syntax("Privileges", INTEGER_TYPE, integer_decode, integer_free);
}

/* OCTET STRING */
static int 
string_decode(x, value)
	OctetString **x;
	ObjectSyntax *value;
{
	OctetString *octet_value = value->os_value;

	if (octet_value == NULL)
		return NOTOK;
	*x = octet_value;

	return OK;
}

static 
add_string()
{
	(void)add_syntax("OctetString", OCTET_PRIM_TYPE, string_decode, free_octetstring);
	(void)add_syntax("DisplayString", OCTET_PRIM_TYPE, string_decode, free_octetstring);
	(void)add_syntax("PhysAddress", OCTET_PRIM_TYPE, string_decode, free_octetstring);
	(void)add_syntax("ClnpAddress", OCTET_PRIM_TYPE, string_decode, free_octetstring);
}

/* OBJECT IDENTIFIER */
static int 
object_decode(x, value)
	OID *x;
	ObjectSyntax *value;
{
	OID oid = value->oid_value;

	if (oid == NULLOID || (*x = oid_cpy(oid)) == NULLOID)
		return NOTOK;

	return OK;
}

static 
add_object()
{
	(void)add_syntax("ObjectID", OBJECT_ID_TYPE, object_decode, free_oid);
}

/* NULL */
static int 
null_decode(x, value)
	char **x;
	ObjectSyntax *value;
{
	if ((*x = (char *)calloc(1, sizeof **x)) == NULL)
		return NOTOK;

	return OK;
}

static
null_free(x)
	char *x;
{
	free((char *)x);
}

static 
add_nul()
{
	(void)add_syntax("NULL", NULL_TYPE, null_decode, null_free);
}

/* IpAddress */
static int 
ipaddr_decode(x, value)
	struct sockaddr_in **x;
	ObjectSyntax *value;
{
	struct type_SNMP_IpAddress *ip;
	struct sockaddr_in *isock;

	ip = value->os_value;

	if ((isock = (struct sockaddr_in *)calloc(1, sizeof *isock)) == NULL) {
		free_SNMP_IpAddress(ip);
		return NOTOK;
	}
	if (ip->length != 4) {
		free((char *)isock);
		free_SNMP_IpAddress(ip);
		return NOTOK;
	}
	isock->sin_family = AF_INET;
	bcopy(ip->octet_ptr, (char *)&isock->sin_addr,
	      sizeof isock->sin_addr);

	*x = isock;

	free_SNMP_IpAddress(ip);
	return OK;
}

static
ipaddr_free(x)
	struct sockaddr_in *x;
{
	free((char *)x);
}

static 
add_ipaddr()
{
	(void)add_syntax("IpAddress", IP_ADDR_PRIM_TYPE, ipaddr_decode, ipaddr_free);
}

/* NetworkAddress */
static 
add_netaddr()
{
	(void)add_syntax("NetworkAddress", IP_ADDR_PRIM_TYPE, ipaddr_decode, ipaddr_free);
}

/* Counter */
static int 
counter_decode(x, value)
	unsigned long **x;
	ObjectSyntax *value;
{
	unsigned long i = value->ul_value;

	if ((*x = (unsigned long *)malloc(sizeof **x)) == NULL)
		return NOTOK;
	**x = i;

	return OK;
}

static
counter_free(x)
	unsigned long *x;
{
	free((char *)x);
}

static 
add_counter()
{
	(void)add_syntax("Counter", COUNTER_TYPE, counter_decode, counter_free);
}

/* Gauge */
static 
add_gauge()
{
	(void)add_syntax("Gauge", GAUGE_TYPE, counter_decode, counter_free);
}

/* TimeTicks */
static 
add_timeticks()
{
	(void)add_syntax("TimeTicks", TIME_TICKS_TYPE, counter_decode, counter_free);
}

int 
readsyntax()
{
	add_integer();
	add_string();
	add_object();
	add_nul();
	add_ipaddr();
	add_netaddr();
	add_counter();
	add_gauge();
	add_timeticks();
}

int 
add_syntax(name, type, f_decode, f_free)
	char *name;
	int type;
	IFP f_decode, f_free;
{
	int i;
	register OS os = synlast++;

	if ((i = synlast - syntaxes) >= MAXSYN)
		return NOTOK;

	bzero((char *)os, sizeof *os);
	os->os_name = name;
	os->os_type = type;
	os->os_decode = f_decode;
	os->os_free = f_free;

	return i;
}

OS 
text2syn(name)
	char *name;
{
	register OS os;

	for (os = syntaxes; os < synlast; os++)
		if (strcmp(os->os_name, name) == 0)
			return os;

	return NULLOS;
}

int 
o_number(oi, v, number)
	OI oi;
	struct type_SNMP_VarBind *v;
	caddr_t number;
{
	int status;
	unsigned long *temp_val1;
	long *temp_val2;
	OS os;

	if ((v->value = (struct type_SNMP_ObjectSyntax *)malloc(sizeof (struct type_SNMP_ObjectSyntax))) == NULL) {
		fprintf(stderr, "o_number, Cannot allocate space for value \n");
		return (status = NOTOK);
	}
	os = oi->oi_type->ot_syntax;
	switch (os->os_type) {
	case COUNTER_TYPE:
	case GAUGE_TYPE:
		v->value->type = os->os_type;
		temp_val1 = (unsigned long *)number;
		v->value->ul_value = *temp_val1;
		break;
	case INTEGER_TYPE:
	case TIME_TICKS_TYPE:
		v->value->type = os->os_type;
		temp_val2 = (long *)number;
		v->value->sl_value = *temp_val2;
		break;
	default:
		return (status = NOTOK);
	}

	return (status = OK);
}

int 
o_string(oi, v, base, len)
	OI oi;
	struct type_SNMP_VarBind *v;
	char *base;
	int len;
{
	int status;
	OS os;
	OctetString *octet_value;

	octet_value = make_octetstring((unsigned char *)base, (long)len);

	if ((v->value = (struct type_SNMP_ObjectSyntax *)malloc(sizeof (struct type_SNMP_ObjectSyntax))) == NULL) {
		fprintf(stderr, "o_string, Cannot allocate space for value \n");
		return (status = NOTOK);
	}
	os = oi->oi_type->ot_syntax;
	switch (os->os_type) {
	case OCTET_PRIM_TYPE:
	case OCTET_CONSTRUCT_TYPE:
	case IP_ADDR_PRIM_TYPE:
	case IP_ADDR_CONSTRUCT_TYPE:
	case OPAQUE_PRIM_TYPE:
	case OPAQUE_CONSTRUCT_TYPE:
		v->value->type = os->os_type;
		v->value->os_value = octet_value;
		break;
	default:
		return (status = NOTOK);
	}

	return (status = OK);
}

int 
o_specific(oi, v, value)
	OI oi;
	struct type_SNMP_VarBind *v;
	caddr_t value;
{
	int status;
	OS os;
	OID oid_value;

	oid_value = oid_cpy((OID) value);

	if ((v->value = (struct type_SNMP_ObjectSyntax *)malloc(sizeof (struct type_SNMP_ObjectSyntax))) == NULL) {
		fprintf(stderr, "o_specific, Cannot allocate space for value \n");
		return (status = NOTOK);
	}
	os = oi->oi_type->ot_syntax;
	switch (os->os_type) {
	case OBJECT_ID_TYPE:
		v->value->type = os->os_type;
		v->value->oid_value = oid_value;
		break;
	default:
		return (status = NOTOK);
	}

	return (status = OK);
}

int 
o_ipaddr(oi, v, value)
	OI oi;
	struct type_SNMP_VarBind *v;
	caddr_t value;
{
	int status;
	OS os;
	unsigned long temp_ip_addr1;
	unsigned char temp_ip_addr2[4];
	OctetString *octet_value;

	temp_ip_addr1 = ntohl(((struct sockaddr_in *)value)->sin_addr.s_addr);
	temp_ip_addr2[0] = ((temp_ip_addr1 >> 24) & 0xFF);
	temp_ip_addr2[1] = ((temp_ip_addr1 >> 16) & 0xFF);
	temp_ip_addr2[2] = ((temp_ip_addr1 >> 8) & 0xFF);
	temp_ip_addr2[3] = (temp_ip_addr1 & 0xFF);

	octet_value = make_octetstring(temp_ip_addr2, 4);

	if ((v->value = (struct type_SNMP_ObjectSyntax *)malloc(sizeof (struct type_SNMP_ObjectSyntax))) == NULL) {
		fprintf(stderr, "o_ipaddr, Cannot allocate space for value \n");
		return (status = NOTOK);
	}
	os = oi->oi_type->ot_syntax;
	switch (os->os_type) {
	case IP_ADDR_PRIM_TYPE:
	case IP_ADDR_CONSTRUCT_TYPE:
		v->value->type = os->os_type;
		v->value->os_value = octet_value;
		break;
	default:
		return (status = NOTOK);
	}

	return (status = OK);
}

int 
o_longword(oi, v, number)
	OI oi;
	struct type_SNMP_VarBind *v;
	int number;
{
	return o_number(oi, v, (caddr_t) & number);
}

int 
mediaddr2oid(ip, addr, len, islen)
	register unsigned int *ip;
	register u_char *addr;
	int len, islen;
{
	register int i;

	if (islen)
		*ip++ = len & 0xff;

	for (i = len; i > 0; i--)
		*ip++ = *addr++ & 0xff;

	return (len + (islen ? 1 : 0));
}

/* inaddr2oid - internetaddress to object identifier */
OID 
inaddr2oid(addr)
	unsigned long addr;
{
	register OID oid;
	register unsigned int i, *ip;
	register u_char *ap;

	if ((oid = (OID) malloc(sizeof *oid)) == NULLOID)
		return NULLOID;

	if ((oid->oid_elements = (unsigned int *)malloc((unsigned)4))
	    == NULL) {
		free((char *)oid);
		return NULLOID;
	}
	oid->oid_nelem = 4;
	ap = (u_char *) & addr;
	for (ip = oid->oid_elements, i = 4; i > 0; i--)
		*ip++ = *ap++ & 0xff;

	return oid;
}
