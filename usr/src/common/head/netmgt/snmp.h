/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:netmgt/snmp.h	1.8"
#ident	"$Header: /SRCS/esmp/usr/src/nw/head/netmgt/snmp.h,v 1.9 1994/08/09 23:24:38 cyang Exp $"
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

/*      @(#)snmp.h	1.1 STREAMWare TCP/IP SVR4.2  source        */
/*      SCCS IDENTIFICATION        */
/*      @(#)snmp.h	6.7 INTERACTIVE SNMP  source        */
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
 * All rights reserved.
 */
#ifndef _SNMP_SNMP_H
#define _SNMP_SNMP_H
#include <stdio.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */


#define VERSION 0		       /* SNMP Version 1 (0) */

/* choke on ANYTHING over this size (cause it'll only get bigger later on). */
#define MAX_SIZE 484

/* Universal's */
#define INTEGER_TYPE           0x02
#define OCTET_PRIM_TYPE        0x04
#define DisplayString          OCTET_PRIM_TYPE
#define NULL_TYPE              0x05
#define OBJECT_ID_TYPE         0x06
#define OCTET_CONSTRUCT_TYPE   0x24
#define SEQUENCE_TYPE          0x30
#define Aggregate              0xFF

/* Application's */
#define IP_ADDR_PRIM_TYPE      0x40
#define COUNTER_TYPE           0x41
#define GAUGE_TYPE             0x42
#define TIME_TICKS_TYPE        0x43
#define OPAQUE_PRIM_TYPE       0x44
#define IP_ADDR_CONSTRUCT_TYPE 0x60
#define OPAQUE_CONSTRUCT_TYPE  0x64

/* Context's */
#define GET_REQUEST_TYPE       0xA0
#define GET_NEXT_REQUEST_TYPE  0xA1
#define GET_RESPONSE_TYPE      0xA2
#define SET_REQUEST_TYPE       0xA3
#define TRAP_TYPE              0xA4

/* Application's SMUX */
#define	SMUX__PDUs_simple		0x60 
#define	SMUX__PDUs_close		0x41
#define	SMUX__PDUs_registerRequest	0x62
#define	SMUX__PDUs_registerResponse	0x43
#define	SMUX__PDUs_get__request		0xA0
#define	SMUX__PDUs_get__next__request	0xA1
#define	SMUX__PDUs_get__response	0xA2
#define	SMUX__PDUs_set__request		0xA3
#define	SMUX__PDUs_trap			0xA4
#define	SMUX__PDUs_commitOrRollback	0x44

#define SMUX_SIMPLE_TYPE       0x60
#define SMUX_CLOSE_TYPE        0x41
#define SMUX_REG_REQUEST_TYPE  0x62
#define SMUX_REG_RESPONSE_TYPE 0x43
#define SMUX_SOUT_TYPE         0x44

#define SMUX_GET_REQUEST_TYPE       GET_REQUEST_TYPE
#define SMUX_GET_NEXT_REQUEST_TYPE  GET_NEXT_REQUEST_TYPE
#define SMUX_GET_RESPONSE_TYPE      GET_RESPONSE_TYPE
#define SMUX_SET_REQUEST_TYPE       SET_REQUEST_TYPE
#define SMUX_TRAP_TYPE              TRAP_TYPE

/* Error codes */
#define NO_ERROR            0
#define TOO_BIG_ERROR       1
#define NO_SUCH_NAME_ERROR  2
#define BAD_VALUE_ERROR     3
#define READ_ONLY_ERROR     4
#define GEN_ERROR           5

/* Function return values */
#define OK	 0
#define NOTOK	-1

typedef struct _OctetString {
	unsigned char *octet_ptr;
	long length;
} OctetString;

typedef struct _OID {
	short length;		  /* number of sub-identifiers */
	unsigned int *oid_ptr;	  /* ordered list of sub-identifiers */
#define	oid_nelem	length
#define	oid_elements	oid_ptr
} OIDentifier, *OID;
#define	NULLOID ((OID) 0)

typedef struct _ObjectSyntax {
	short type;
	unsigned long ul_value;	       /* Counter, Gauge */
	long sl_value;		       /* simple num., time_ticks */
	OctetString *os_value;	       /* for OS, IpAddress, Opaque */
	OID oid_value;	       /* Object Identifier */
} ObjectSyntax;

typedef struct _VarBindUnit {
	OID name;
	ObjectSyntax *value;
} VarBindUnit;

typedef struct _VarBindList {
	long data_length;
	struct _VarBindUnit *vb_ptr;
	struct _VarBindList *next;
#define	VarBind		vb_ptr
} VarBindList;

#define	type_SNMP_VarBind	_VarBindUnit
#define	type_SNMP_VarBindList	_VarBindList

#define	free_SNMP_VarBindList	free_varbind_list

#define	type_SNMP_ObjectName	OIDentifier
#define	free_SNMP_ObjectName	free_oid

#define	type_SNMP_ObjectSyntax	_ObjectSyntax
#define	free_SNMP_ObjectSyntax	free_value

#define	type_SNMP_ClnpAddress	_OctetString
#define	free_SNMP_ClnpAddress	free_octetstring

#define	type_SNMP_DisplayString	_OctetString
#define	free_SNMP_DisplayString	free_octetstring

#define	type_SNMP_IpAddress	_OctetString
#define	free_SNMP_IpAddress	free_octetstring

#define	type_SNMP_NetworkAddress	type_SNMP_IpAddress
#define	free_SNMP_NetworkAddress	free_SNMP_IpAddress

typedef struct _NormPdu {
	long request_id;
	long error_status;
	long error_index;
} NormPdu;

typedef struct _TrapPdu {
	OID  enterprise;
	OctetString *agent_addr;
	long generic_trap;
	long specific_trap;
	long time_ticks;
} TrapPdu;

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

typedef struct _AuthHeader {
	OctetString *packlet;
	unsigned long version;
	OctetString *community;
} AuthHeader;

/* SMUX PDUs */

typedef struct type_SNMP_SimpleOpen {
	int version;
#define SMUX_version_1			0
#define	SNMP_version_version__1		0	/* for ISODE */
	OID  identity;
	struct _OctetString *description;
	struct _OctetString *password;
} SMUX_SimpleOpen;

typedef struct type_SNMP_ClosePDU {
	int parm;
#define ClosePDU_goingDown              0
#define ClosePDU_unsupportedVersion     1
#define ClosePDU_packetFormat           2
#define ClosePDU_protocolError          3
#define ClosePDU_internalError          4
#define ClosePDU_authenticationFailure  5
} SMUX_ClosePDU;

#define	goingDown		ClosePDU_goingDown
#define	unsupportedVersion	ClosePDU_unsupportedVersion
#define	packetFormat		ClosePDU_packetFormat
#define	protocolError		ClosePDU_protocolError
#define	internalError		ClosePDU_internalError
#define	authenticationFailure	ClosePDU_authenticationFailure

typedef struct type_SNMP_RReqPDU {
	OID  subtree;
	int priority;
	int operation;

#define	operation_delete	0
#define	operation_readOnly	1
#define	operation_readWrite	2

#define register_op_delete    0
#define register_op_readOnly  1
#define register_op_readWrite 2
} SMUX_RReqPDU;

#define	delete		operation_delete
#define	readOnly	operation_readOnly
#define	readWrite	operation_readWrite

typedef struct type_SNMP_RRspPDU {
	int parm;
#define RRspPDU_failure                -1
} SMUX_RRspPDU;

typedef struct type_SNMP_SOutPDU {
	int parm;
#define SOutPDU_commit                  0
#define SOutPDU_rollback                1
} SMUX_SOutPDU;

typedef struct type_SNMP_PDU {
	int request__id;

	int error__status;
#define	error__status_noError		0
#define	error__status_tooBig		1
#define	error__status_noSuchName	2
#define	error__status_badValue		3
#define	error__status_readOnly		4
#define	error__status_genErr		5

	int error__index;
	struct type_SNMP_VarBindList *variable__bindings;
} SMUX_Data_PDU;

#define SMUX_GetRequest_PDU		SMUX_Data_PDU
#define SMUX_GetNextRequest_PDU		SMUX_Data_PDU
#define SMUX_GetResponse_PDU		SMUX_Data_PDU
#define SMUX_SetRequest_PDU		SMUX_Data_PDU

typedef long type_SNMP_TimeTicks;

typedef struct type_SNMP_Trap__PDU {
	OID enterprise;
	struct type_SNMP_NetworkAddress *agent__addr;

	int generic__trap;
#define	trap_coldStart			0
#define	trap_warmStart			1
#define	trap_linkDown			2
#define	trap_linkUp			3
#define	trap_authenticationFailure	4
#define	trap_egpNeighborLoss		5
#define	trap_enterpriseSpecific		6

	int specific__trap;
	type_SNMP_TimeTicks time__stamp;
	struct type_SNMP_VarBindList *variable__bindings;
} SMUX_Trap_PDU;

typedef struct type_SNMP_PDUs {
	int offset;
	union {
		SMUX_GetRequest_PDU *get__request;
		SMUX_GetNextRequest_PDU *get__next__request;
		SMUX_GetResponse_PDU *get__response;
		SMUX_GetResponse_PDU *set__request;
		SMUX_Trap_PDU *trap;
	} un;
} SNMP_PDU;

typedef struct type_SNMP_SMUX__PDUs {
	OctetString *packlet;
	int offset;

	union {
		SMUX_SimpleOpen *simple;
		SMUX_ClosePDU *close;
		SMUX_RReqPDU *registerRequest;
		SMUX_RRspPDU *registerResponse;
		SMUX_GetRequest_PDU *get__request;
		SMUX_GetNextRequest_PDU *get__next__request;
		SMUX_GetResponse_PDU *get__response;
		SMUX_SetRequest_PDU *set__request;
		SMUX_Trap_PDU *trap;
		SMUX_SOutPDU *commitOrRollback;
	} un;
} SNMP_SMUX_PDU;

/* statistics */
typedef struct _snmpstat {
	int inpkts;
	int outpkts;
	int inbadversions;
	int inbadcommunitynames;
	int inbadcommunityuses;
	int inasnparseerrs;
	int inbadtypes;
	int intoobigs;
	int innosuchnames;
	int inbadvalues;
	int inreadonlys;
	int ingenerrs;
	int intotalreqvars;
	int intotalsetvars;
	int ingetrequests;
	int ingetnexts;
	int insetrequests;
	int ingetresponses;
	int intraps;
	int outtoobigs;
	int outnosuchnames;
	int outbadvalues;
	int outreadonlys;
	int outgenerrs;
	int outgetrequests;
	int outgetnexts;
	int outsetrequests;
	int outgetresponses;
	int outtraps;
} snmpstat_type;

/* extern's */

/* snmp */
snmpstat_type * snmpstat;

/* make_lib */
#ifdef __STDC__
OctetString *make_octetstring(unsigned char *string, long length);
OID make_oid(unsigned int *sid, short length);
VarBindList *make_varbind(OID oid, short type, unsigned long ul_value,
			  long sl_value, OctetString *os_value, OID oid_value);
Pdu *make_pdu(short type, long request_id, long error_status,
	      long error_index, OID enterprise, OctetString * agent_addr,
	      long generic_trap, long specific_trap, long time_ticks);
short link_varbind(Pdu * pdu_ptr, VarBindList * vbl_ptr);
OctetString *make_octet_from_text(unsigned char *text_str);
OctetString *make_octet_from_hex(unsigned char *text_str);
OID make_obj_id_from_hex(unsigned char *text_str);
OID make_oid_from_dot(unsigned char *text_str);
#else
OctetString *make_octetstring();
OID make_oid();
VarBindList *make_varbind();
Pdu *make_pdu();
short link_varbind();
OctetString *make_octet_from_text();
OctetString *make_octet_from_hex();
OID make_obj_id_from_hex();
OID make_oid_from_dot();
#endif

/* free_lib */
#ifdef __STDC__
void free_octetstring(OctetString * os_ptr);
void free_oid(OID oid_ptr);
void free_varbind(VarBindUnit * vb_ptr);
void free_pdu(Pdu * pdu_ptr);
void free_varbind_list(VarBindList * vbl_ptr);
/* SMUX pdu */
void free_SMUX_SimpleOpen(SMUX_SimpleOpen *open_ptr);
void free_SMUX_ClosePDU(SMUX_ClosePDU *close_ptr);
void free_SMUX_GetPDU(SMUX_Data_PDU *get_ptr);
void free_SMUX_TrapPDU(SMUX_Trap_PDU *trap_ptr);
void free_SMUX_RReqPDU(SMUX_RReqPDU *rreq_ptr);
void free_SMUX_SOutPDU(SMUX_SOutPDU *sout_ptr);
void free_SNMP_SMUX_PDU(SNMP_SMUX_PDU *pdu_ptr);
#else
void free_octetstring();
void free_varbind();
void free_oid();
void free_pdu();
void free_varbind_list();
/* SMUX pdu */
void free_SMUX_SimpleOpen();
void free_SMUX_ClosePDU();
void free_SMUX_GetPDU();
void free_SMUX_TrapPDU();
void free_SMUX_RReqPDU();
void free_SMUX_SOutPDU();
void free_SNMP_SMUX_PDU();
#endif

/* build_packet */
#ifdef __STDC__
short build_pdu(Pdu * pdu_ptr);
long find_len_varbind(VarBindList * vbl_ptr);
long find_len_octetstring(OctetString * os_ptr);
short find_len_oid(OID oid_ptr);
short find_len_unsignedinteger(unsigned long value);
short find_len_signedinteger(long value);
short dolenlen(long len);
void add_len(unsigned char **working_ptr, long lenlen, long data_len);
short add_varbind(unsigned char **working_ptr, VarBindList * vbl_ptr);
short add_octetstring(unsigned char **working_ptr, short type,
		      OctetString * os_ptr);
short add_oid(unsigned char **working_ptr, short type, OID oid_ptr);
short add_unsignedinteger(unsigned char **working_ptr, short type,
			  unsigned long value);
short add_signedinteger(unsigned char **working_ptr, short type, long value);
void add_null(unsigned char **working_ptr);
#else
short build_pdu();
long find_len_varbind();
short find_len_oid();
long find_len_octetstring();
short find_len_unsignedinteger();
short find_len_signedinteger();
short dolenlen();
void add_len();
short add_varbind();
short add_octetstring();
short add_oid();
short add_unsignedinteger();
short add_signedinteger();
void add_null();
#endif

/* parse_packet */
#ifdef __STDC__
Pdu *parse_pdu(AuthHeader * auth_ptr);
VarBindList *parse_varbind(unsigned char **working_ptr, unsigned char *end_ptr);
OctetString *parse_octetstring(unsigned char **working_ptr,
			       unsigned char *end_ptr, short *type);
OID parse_oid(unsigned char **working_ptr, unsigned char *end_ptr);
unsigned long parse_unsignedinteger(unsigned char **working_ptr,
				    unsigned char *end_ptr, short *type);
long parse_signedinteger(unsigned char **working_ptr,
			 unsigned char *end_ptr, short *type);
short parse_null(unsigned char **working_ptr, unsigned char *end_ptr,
		 short *type);
long parse_sequence(unsigned char **working_ptr, unsigned char *end_ptr,
		    short *type);
short parse_type(unsigned char **working_ptr, unsigned char *end_ptr);
long parse_length(unsigned char **working_ptr, unsigned char *end_ptr);
#else
Pdu *parse_pdu();
VarBindList *parse_varbind();
OID parse_oid();
OctetString *parse_octetstring();
unsigned long parse_unsignedinteger();
long parse_signedinteger();
short parse_null();
long parse_sequence();		       /* returns length of sequence */
short parse_pdu_type();
short parse_type();
long parse_length();
#endif

/* auth_lib */
#ifdef __STDC__
AuthHeader *make_authentication(OctetString * community);
short build_authentication(AuthHeader * auth_ptr, Pdu * pdu_ptr);
void free_authentication(AuthHeader * auth_ptr);
AuthHeader *parse_authentication(unsigned char *packet_ptr, long length);
#else
AuthHeader *make_authentication();
short build_authentication();
void free_authentication();
AuthHeader *parse_authentication();
#endif

#if defined(SVR4)
#define bcopy(b1,b2,length)     (void) memcpy ((b2), (b1), (length))
#define bcmp(b1,b2,length)      memcmp ((b1), (b2), (length))
#define bzero(b,length)         (void) memset ((b), 0, (length))
#endif

#define LIB_ERROR(string)               fprintf(stderr, string)
#define LIB_ERROR1(string, val)         fprintf(stderr, string, val)
#define LIB_ERROR2(string, val1, val2)  fprintf(stderr, string, val1, val2)
#define LIB_PERROR(string)              perror(string)

#ifndef NONULL
#define NULLIT(pointer) pointer = NULL;
#else
#define NULLIT(pointer)
#endif

#ifdef __STDC__
SNMP_SMUX_PDU *decode_SMUX_PDU(unsigned char *packet_ptr, long length);
short encode_SMUX_PDU(SNMP_SMUX_PDU *pdu_ptr);
#else
SNMP_SMUX_PDU *decode_SMUX_PDU();
short encode_SMUX_PDU();
#endif


typedef int (*IFP) ();
#define		NULLIFP	((IFP) 0)
#define		NULLCP	((char *) 0)


#define	type_SNMP_OpenPDU		type_SNMP_SimpleOpen
#define	type_SNMP_GetRequest__PDU	type_SNMP_PDU
#define	type_SNMP_GetNextRequest__PDU	type_SNMP_PDU
#define	type_SNMP_GetResponse__PDU	type_SNMP_PDU
#define	type_SNMP_SetRequest__PDU	type_SNMP_PDU

#define	free_SNMP_OpenPDU		free_SMUX_SimpleOpen
#define	free_SNMP_GetRequest__PDU	free_SMUX_GetPDU
#define	free_SNMP_GetNextRequest__PDU	free_SMUX_GetPDU
#define	free_SNMP_GetResponse__PDU	free_SMUX_GetPDU
#define	free_SNMP_SetRequest__PDU	free_SMUX_GetPDU

/* SMUX API errors */

#define	invalidOperation	(-1)
#define	parameterMissing	(-2)
#define	systemError		(-3)
#define	youLoseBig		(-4)
#define	congestion		(-5)
#define	inProgress		(-6)

extern int smux_errno;
extern char smux_info[];

struct smuxEntry {
	char  *se_name;
	OIDentifier se_identity;
	char  *se_password;
	int   se_priority;
};

/* SMUX API functions */
#ifdef __STDC__
int smux_init(int debug);	                              /* INIT */
int smux_simple_open(OID identity, char *description,
		     char *commname,int commlen);             /* OPEN */
int smux_close(int reason);		                      /* CLOSE */
int smux_register(OID subtree, int priority, int operation);  /* REGISTER */
int smux_response(struct type_SNMP_GetResponse__PDU *event);  /* RESPONSE */
int smux_wait(struct type_SNMP_SMUX__PDUs **event, int secs); /* WAIT */
int smux_trap(int generic, int specific,
	      struct type_SNMP_VarBindList *bindings);	      /* TRAP */
char *smux_error(int i);		                    /* TEXTUAL ERROR */
/* SMUX entry functions */
struct smuxEntry *getsmuxEntry(FILE * fp);
struct smuxEntry *getsmuxEntrybyname(char * name);
struct smuxEntry *getsmuxEntrybyidentity(OID identity);
#else  /* __STDC__ */
int smux_init();		       /* INIT */
int smux_simple_open();		       /* OPEN */
int smux_close();		       /* CLOSE */
int smux_register();		       /* REGISTER */
int smux_response();		       /* RESPONSE */
int smux_wait();		       /* WAIT */
int smux_trap();		       /* TRAP */
char *smux_error();		       /* TEXTUAL ERROR */
/* SMUX entry functions */
struct smuxEntry *getsmuxEntry();
struct smuxEntry *getsmuxEntrybyname();
struct smuxEntry *getsmuxEntrybyidentity();
#endif  /* __STDC__ */

#ifdef SVR4
#define index(a,b)	strchr((a),(b))
#endif

#define	inaddr_copy(hp,sin)	bcopy ((hp) -> h_addr, (char *) &((sin) -> sin_addr), (hp) -> h_length)

/* Paths of transport devices and configuration files */
#define _PATH_IP	"/dev/ip"
#define _PATH_TCP	"/dev/tcp"
#define _PATH_UDP	"/dev/udp"
#define _PATH_ICMP	"/dev/icmp"
#define _PATH_ARP	"/dev/arp"
#define _PATH_COTS	"/dev/ticots"

#ifdef NETWARE
#define _PATH_IPX	"/dev/ipx"
#endif

#define SNMPD_PEERS_FILE  "/etc/netmgt/snmpd.peers"
#define SNMPD_COMM_FILE   "/etc/netmgt/snmpd.comm"
#define SNMPD_CONF_FILE   "/etc/netmgt/snmpd.conf"
#define SNMPD_TRAP_FILE   "/etc/netmgt/snmpd.trap"
#define SNMPD_PID_FILE    "/tmp/snmpd.pid"

#define SNMPD_SHMKEY      4096  /* Key to the snmpd shared memory */

/* Low level SMUX implementation functions (smuxio) */
#ifdef __STDC__
int start_smux_server(void);
int start_tcp_client(struct sockaddr_in *sock);
int join_tcp_client(int fd, struct sockaddr_in *sock);
int join_tcp_server(int fd, struct sockaddr_in *sock, int sent_connect);
int recv_tcp_packet(int fd, char *packet);
int send_tcp_packet(int fd, unsigned char *packet, unsigned int length);
int dispatch_smux_packet(int fd, unsigned char *packet, long amount);
int fetch_smux_packet(int fd, char *rem_data, int *rem_len, 
		      unsigned char *packet, long *amount);
int xselect(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, int secs);
#else /*__STDC__*/
int start_smux_server();
int start_tcp_client();
int join_tcp_client();
int join_tcp_server();
int recv_tcp_packet();
int send_tcp_packet();
int dispatch_smux_packet();
int fetch_smux_packet();
int xselect();
#endif /*__STDC__*/
/* Some low level functions */
#ifdef __STDC__
OID oid_extend(OID q, int howmuch);     /* extend an OID */

OID oid_normalize(); 

OID oid_cpy(OID q);                     /* copy an OID */ 

int oid_cmp(OID p, OID q);              /* compare two OIDs */

int elem_cmp(register unsigned int *ip, register int i, 
             register unsigned int *jp, register int j); /* compare elements */

char *sprintoid(register OID oid);      /* OID to string */
OctetString *os_cpy(OctetString *orig); /* copy an OctetString */
char *sprintoid(register OID oid);      /* OID to string */
#else /*__STDC__*/
OID oid_extend();                       /* extend an OID */
OID oid_normalize(); 
OID oid_cpy();                          /* copy an OID */
int oid_cmp();                          /* compare two OIDs */
int elem_cmp();                         /* compare elements */
OctetString *os_cpy();                  /* copy an OctetString */
char *sprintoid();                      /* OID to string */
#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif
#endif /* _SNMP_SNMP_H */


