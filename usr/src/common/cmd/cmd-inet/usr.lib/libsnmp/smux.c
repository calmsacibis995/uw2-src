/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.lib/libsnmp/smux.c	1.1"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/* smux.c - SMUX initiator library */

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

#include <stdio.h>
#include <varargs.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

extern int errno;
#ifdef SYSV
#include <sys/stream.h>
#include <sys/tiuser.h>
extern int t_errno;
#endif

#include <snmp/snmp.h>

#define	NULLFD	 ((fd_set *) 0)

static int sd = NOTOK;
static struct sockaddr_in in_socket;
static int sent_connect = 0;
static OID smux_enterprise = NULL;
static struct type_SNMP_NetworkAddress *smux_addr = NULL;
static long smux_stamp = 0;
static struct timeval my_boottime;

int smux_debug = 0;
int smux_errno;
int seconds;	/* Dummy variable ??? */
char smux_info[BUFSIZ];
unsigned char buffer[1024];
static char data_buf[2048];
static int data_len = 0;

struct	hostent *gethostbystring();

/*    INIT */

int 
smux_init(debug)
	int debug;
{
	int onoff;
	register struct sockaddr_in *isock = &in_socket;
	register struct hostent *hp;
	register struct servent *sp;

	smux_debug = debug;

	bzero((char *)isock, sizeof *isock);
	if ((hp = gethostbystring("127.0.0.1")) == NULL) {
		smux_errno = youLoseBig;
		perror("smux_init: Unknown host 127.0.0.1");
		return NOTOK;
	}
	isock->sin_family = hp->h_addrtype;
	isock->sin_port = (sp = getservbyname("smux", "tcp"))
	    ? sp->s_port : htons((unsigned short)199);
	inaddr_copy(hp, isock);

	if ((sd = start_tcp_client((struct sockaddr_in *)NULL)) == NOTOK) {
		smux_errno = systemError;
		fprintf(stderr, "smux_init: start_tcp_client failed");
		return NOTOK;
	}
#ifdef BSD
	(void)ioctl(sd, FIONBIO, (onoff = 1, (char *)&onoff));
	if (join_tcp_server(sd, isock) == NOTOK)
		switch (errno) {
		case EINPROGRESS:
			return sd;

		case EISCONN:
			break;

		default:
			smux_errno = systemError;
			fprintf(stderr, "smux_init: join_tcp_server failed");
			(void)close_up(sd);
			return (sd = NOTOK);
		}
#endif
#ifdef SYSV
	if (join_tcp_server(sd, isock, sent_connect) == NOTOK)
		switch (t_errno) {
		case TNODATA:
			LIB_ERROR("Sent connect -1 \n");
			sent_connect = 1;
			return sd;

		default:
			LIB_ERROR("Could not send connect \n");
			smux_errno = systemError;
			fprintf(stderr, "smux_init: join_tcp_server failed");
			sent_connect = 0;
			(void)close_up(sd);
			return (sd = NOTOK);
		}
#endif

	if (smuxalloc() == NOTOK)
		return NOTOK;

	(void)gettimeofday(&my_boottime, (struct timezone *)0);

	return sd;
}

/*  */

static int 
smuxalloc()
{
	int len;

	if (getsockname(sd, (struct sockaddr *)&in_socket,
			(len = sizeof in_socket, &len)) == NOTOK)
		bzero((char *)&in_socket.sin_addr, 4);

	if ((smux_addr = make_octetstring((u_char *)&in_socket.sin_addr, 4)) == NULL) {
		smux_errno = youLoseBig;
		fprintf(stderr, "smuxalloc: make_octetstring: failed");
		(void)close_up(sd);
		return (sd = NOTOK);
	}
	if (smux_debug)
		print_octet_string_out (smux_addr, 16);

	return OK;
}

/*    SIMPLE OPEN */

int 
smux_simple_open(identity, description, commname, commlen)
	OID identity;
	char *description, *commname;
	int commlen;
{
	int result;
	struct type_SNMP_SMUX__PDUs pdu;
	register struct type_SNMP_SimpleOpen *simple;

	if ((identity == NULL)
	    || (description == NULL)
	    || ((commname == NULL) && (commlen != 0))) {
		smux_errno = parameterMissing;
		return NOTOK;
	}
	if (sd == NOTOK) {
		LIB_ERROR("Connect failed initially  \n");
		smux_errno = invalidOperation;
		return NOTOK;
	}
	if (smux_addr == NULL) {
		fd_set mask;
		register struct sockaddr_in *isock = &in_socket;

		FD_ZERO(&mask);
		FD_SET(sd, &mask);
		if (xselect(sd + 1, NULL, &mask, NULL, 0) < 1)
			goto not_yet;
#ifdef BSD
		if (join_tcp_server(sd, isock) == NOTOK)
			switch (errno) {
			case EINPROGRESS:
not_yet:;
				smux_errno = inProgress;
				return NOTOK;

			case EISCONN:
				break;

			default:
				smux_errno = systemError;
				fprintf(stderr, "smux_simple_open: join_tcp_server failed");
				(void)close_up(sd);
				return (sd = NOTOK);
			}
#endif

#ifdef SYSV
		if (join_tcp_server(sd, isock, sent_connect) == NOTOK)
			switch (t_errno) {
			case TNODATA:
not_yet:;
				sent_connect = 1;
				smux_errno = inProgress;
				return NOTOK;

			default:
				smux_errno = systemError;
				fprintf(stderr, "smux_simple_open: join_tcp_server failed");
				sent_connect = 0;
				(void)close_up(sd);
				return (sd = NOTOK);
			}
#endif

		if (smuxalloc() == NOTOK)
			return NOTOK;
	}
	bzero((char *)&pdu, sizeof pdu);

	if ((simple = (struct type_SNMP_SimpleOpen *)calloc(1, sizeof *simple))
	    == NULL) {
no_mem:;
		smux_errno = congestion;
		fprintf(stderr, "smux_simple_open: out of memory");
		if (simple)
			free_SMUX_SimpleOpen(simple);

		(void)close_up(sd);
		return (sd = NOTOK);
	}
	pdu.offset = SMUX__PDUs_simple;
	pdu.un.simple = simple;

	if ((smux_enterprise = oid_cpy(identity)) == NULL)
		goto no_mem;

	simple->version = SNMP_version_version__1;
	if ((simple->identity = oid_cpy(identity)) == NULL
	    || (simple->description = make_octetstring((u_char *) description,
					   (long)strlen(description))) == NULL
	  || (simple->password = make_octetstring((u_char *) commname, commlen)) == NULL)
		goto no_mem;

	result = smuxsend(&pdu);

	free_SMUX_SimpleOpen(simple);

	return result;
}

/*  */

static int 
smuxsend(pdu)
	struct type_SNMP_SMUX__PDUs *pdu;
{
	int result;
	unsigned char *packet = buffer;
	long length;

	if (encode_SMUX_PDU(pdu) == NOTOK) {
		smux_errno = youLoseBig;
		fprintf(stderr, "smuxsend: encode_SMUX_PDU");
		result = NOTOK;
		goto out;
	}
	length = pdu->packlet->length;
	bcopy((char *)pdu->packlet->octet_ptr, packet, length);

	result = OK;

	if (smux_debug) {
		print_packet_out (packet, length);
	}
	if (dispatch_smux_packet(sd, packet, length) == NOTOK) {
		fprintf(stderr, "smuxsend: Error sending an SMUX pdu \n");
		result = NOTOK;
		goto out;
	}
out:;
	if (result == NOTOK) {
		(void)close_up(sd);
		return (sd = NOTOK);
	}
	return OK;
}

/*    CLOSE */

int 
smux_close(reason)
	int reason;
{
	int result;
	struct type_SNMP_SMUX__PDUs pdu;
	register struct type_SNMP_ClosePDU *close;

	if (smux_addr == NULL) {
		smux_errno = invalidOperation;
		fprintf(stderr, "smux_close: SMUX not opened");
		return NOTOK;
	}
	bzero((char *)&pdu, sizeof pdu);

	if ((close = (struct type_SNMP_ClosePDU *)calloc(1, sizeof *close))
	    == NULL) {
		smux_errno = congestion;
		fprintf(stderr, "smux_close: out of memory");
		result = NOTOK;
		if (close)
			free_SMUX_ClosePDU(close);

		(void)close_up(sd);
		return (sd = NOTOK);
	}
	pdu.offset = SMUX__PDUs_close;
	pdu.un.close = close;
	close->parm = reason;

	result = smuxsend(&pdu);

	free_SMUX_ClosePDU(close);
	(void)close_up(sd);
	sd = NOTOK;

	if (smux_enterprise)
		free_oid(smux_enterprise), smux_enterprise = NULL;
	if (smux_addr)
		free_octetstring(smux_addr), smux_addr = NULL;

	return result;
}

/*    REGISTER */

int 
smux_register(subtree, priority, operation)
	OID subtree;
	int priority, operation;
{
	int result;
	struct type_SNMP_SMUX__PDUs pdu;
	register struct type_SNMP_RReqPDU *rreq;

	if (subtree == NULL) {
		smux_errno = parameterMissing;
		fprintf(stderr, "smux_register: missing parameter");
		return NOTOK;
	}
	if (smux_addr == NULL) {
		smux_errno = invalidOperation;
		fprintf(stderr, "smux_register: SMUX not opened");
		return NOTOK;
	}
	bzero((char *)&pdu, sizeof pdu);

	if ((rreq = (struct type_SNMP_RReqPDU *)calloc(1, sizeof *rreq))
	    == NULL) {
no_mem:;
		result = NOTOK;
		smux_errno = congestion;
		fprintf(stderr, "smux_register: out of memory");
		if (rreq)
			free_SMUX_RReqPDU(rreq);

		(void)close_up(sd);
		return (sd = NOTOK);
	}
	pdu.offset = SMUX__PDUs_registerRequest;
	pdu.un.registerRequest = rreq;

	if ((rreq->subtree = oid_cpy(subtree)) == NULLOID)
		goto no_mem;
	rreq->priority = priority;
	rreq->operation = operation;

	result = smuxsend(&pdu);

	free_SMUX_RReqPDU(rreq);

	return result;
}

/*    WAIT */

int 
smux_wait(event, secs)
	struct type_SNMP_SMUX__PDUs **event;
	int secs;
{
	fd_set mask;
	unsigned char *packet = buffer;
	long length;

	if (event == NULL) {
		smux_errno = parameterMissing;
		fprintf(stderr, "smux_wait: missing parameter");
		return NOTOK;
	}
	if (smux_addr == NULL) {
		smux_errno = invalidOperation;
		fprintf(stderr, "smux_wait: SMUX not opened");
		return NOTOK;
	}
	FD_ZERO(&mask);
	FD_SET(sd, &mask);
	if (xselect(sd + 1, &mask, NULLFD, NULLFD, secs) <= OK) {
		errno = EWOULDBLOCK;
		fprintf(stderr, "smux_wait: xselect failed \n");
		smux_errno = inProgress;
		return NOTOK;
	}
	if (FD_ISSET(sd, &mask)) {
		smux_errno = inProgress;
		if (smux_debug)
			fprintf(stderr, "smux_wait: Incoming stuff on - %d \n", sd);
	}
	if (fetch_smux_packet(sd, data_buf, &data_len, packet, &length) == NOTOK) {
		smux_errno = youLoseBig;
		fprintf(stderr, "smux_wait: Error fetching an SMUX pdu");
		goto out;
	}
	if (smux_debug) {
		print_packet_out (packet, length);
	}
	if ((*event = decode_SMUX_PDU(packet, length)) == NULL) {
		smux_errno = youLoseBig;
		fprintf(stderr, "smux_wait: decode_SMUX_PDU");
		goto out;
	}
	if ((*event)->offset == SMUX__PDUs_close) {
		(void)close_up(sd);
		sd = NOTOK;
	}
	return OK;

out:;
	(void)close_up(sd);
	return (sd = NOTOK);
}

/*    RESPONSE */

int 
smux_response(event)
	struct type_SNMP_GetResponse__PDU *event;
{
	struct type_SNMP_SMUX__PDUs pdu;

	if (event == NULL) {
		smux_errno = parameterMissing;
		fprintf(stderr, "smux_response: missing parameter");
		return NOTOK;
	}
	if (smux_addr == NULL) {
		smux_errno = invalidOperation;
		fprintf(stderr, "smux_response: SMUX not opened");
		return NOTOK;
	}
	bzero((char *)&pdu, sizeof pdu);

	pdu.offset = SMUX__PDUs_get__response;
	pdu.un.get__response = event;

	return smuxsend(&pdu);
}

/*    TRAP */

int 
smux_trap(generic, specific, bindings)
	int generic, specific;
	struct type_SNMP_VarBindList *bindings;
{
	int result;
	struct timeval now;
	struct type_SNMP_SMUX__PDUs pdu;
	register struct type_SNMP_Trap__PDU *trap;

	if (smux_addr == NULL) {
		smux_errno = invalidOperation;
		fprintf(stderr, "smux_response: SMUX not opened");
		return NOTOK;
	}
	bzero((char *)&pdu, sizeof pdu);

	if ((trap = (struct type_SNMP_Trap__PDU *)calloc(1, sizeof *trap))
	    == NULL) {
		smux_errno = congestion;
		fprintf(stderr, "smux_response: out of memory");
		result = NOTOK;
		if (trap)
			free_SMUX_TrapPDU(trap);

		(void)close_up(sd);
		return (sd = NOTOK);
	}
	pdu.offset = SMUX__PDUs_trap;
	pdu.un.trap = trap;

	trap->enterprise = smux_enterprise;
	trap->agent__addr = smux_addr;
	trap->generic__trap = generic;
	trap->specific__trap = specific;
	trap->time__stamp = smux_stamp;
	(void)gettimeofday(&now, (struct timezone *)0);
	trap->time__stamp = (now.tv_sec - my_boottime.tv_sec) * 100 +
	    ((now.tv_usec - my_boottime.tv_usec) / 10000);
	trap->variable__bindings = bindings;

	result = smuxsend(&pdu);

	trap->enterprise = NULL;
	trap->agent__addr = NULL;
	trap->variable__bindings = NULL;

	free_SMUX_TrapPDU(trap);

	return result;
}

/*  */

static char *errors_up[] =
{
	"goingDown",
	"unsupportedVersion",
	"packetFormat",
	"protocolError",
	"internalError",
	"authenticationFailure"
};

static char *errors_down[] =
{
	"SMUX error 0",
	"invalidOperation",
	"parameterMissing",
	"systemError",
	"youLoseBig",
	"congestion",
	"inProgress"
};

char *
smux_error(i)
	int i;
{
	int j;
	char **ap;
	static char buffer[BUFSIZ];

	if (i < 0) {
		ap = errors_down, j = sizeof errors_down / sizeof errors_down[0];
		i = -i;
	} else
		ap = errors_up, j = sizeof errors_up / sizeof errors_up[0];
	if (0 <= i && i < j)
		return ap[i];

	(void)sprintf(buffer, "SMUX error %s%d", ap == errors_down ? "-" : "", i);

	return buffer;
}
