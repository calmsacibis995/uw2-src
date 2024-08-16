/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/novell/novell.c	1.1.1.11"
#ident	"$Id: novell.c,v 1.24 1994/09/14 19:10:48 vtag Exp $"

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<xti.h>
#include	<netconfig.h>
#include	<netdir.h>
#include	<string.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<sys/utsname.h>
#include	<net/if.h>
#include	<stropts.h>
#include	<sys/byteorder.h>
#include	<sys/sap_app.h>
#include	<mt.h>
#ifdef _REENTRANT
#include	"novell_mt.h"
#endif

#ifndef N_CONST
#define N_CONST const
#endif

#define NWOT_WILD				0xFFFF

#define IPXPORT_RESERVED		0x4CFF
#define	STARTPORT				0x4C00
#define	ENDPORT					(IPXPORT_RESERVED - 1)
#define	NPORTS					(ENDPORT - STARTPORT + 1)

#define	END_DYNM_RESERVED	 	IPXPORT_RESERVED
#define	START_DYNM_RESERVED 	STARTPORT
#define	START_STATIC_RESERVED 	36917
#define	END_STATIC_RESERVED 	36985

static struct utsname systemNameStruct;
static unsigned char firstTime = 1;

struct servent *_novell_getservbyname();
struct servent *_novell_getservbysocket();
struct servent *_novell_getsapnumberbyname();
struct servent *_novell_getsapnamebynumber();

static struct nd_addrlist *
addAddressToList ( struct nd_addrlist	*result,
	const unsigned char	*address,
	unsigned short	port )
{
	struct netbuf *curNetbuf_p;
	char *p, *b;
	unsigned short *port_p;

	if (result == NULL)
		return (NULL);

	if ((b = malloc(sizeof(struct ipxAddress))) == NULL)
	{
		netdir_free(result, ND_ADDRLIST);
		return(NULL);
	}

	if ((p = realloc ( result->n_addrs, (result->n_cnt + 1) * sizeof(struct netbuf))) == NULL)
	{
		free(b);
		netdir_free(result, ND_ADDRLIST);
		return(NULL);
	}

	result->n_cnt++;
	result->n_addrs = (struct netbuf *)p;
	curNetbuf_p = &result->n_addrs[result->n_cnt - 1];
	curNetbuf_p->len = sizeof(struct ipxAddress);
	curNetbuf_p->maxlen = sizeof(struct ipxAddress);
	curNetbuf_p->buf = b;
	port_p = (unsigned short *) &curNetbuf_p->buf[10];
	*port_p = port;
	memcpy(curNetbuf_p->buf, address, 10);

	return(result);
}

struct nd_addrlist *
_netdir_getbyname ( N_CONST struct netconfig	*tp,
    		    N_CONST struct nd_hostserv	*serv )
{
	SAPI serverEntry;

	struct servent *serviceEntry_p;

	struct nd_addrlist *result;

	unsigned short server_port;
	unsigned short sap_type = ALL_SERVER_TYPE;

#ifdef CHECK_BINDERY
	unsigned char propertyType;

	char *p, *q, host[64], segmentData[256];
#else
	char *p, *q, host[64];
#endif

	int serverIndex = 0;

	/*
	**  const -> read-only, therefore not subject to contention
	*/
	static const unsigned char broadcastAddress[] =
	{
		0x00, 0x00, 0x00, 0x00,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};

	/*
	**  For efficiency, we check firstTime first, and lock only
	**  if we may have to modify firstTime and systemNameStruct.
	**  After locking, we check again to make sure that another
	**  thread didn't change firstTime in the meantime.
	*/
	if(firstTime)
	    {
	    MUTEX_LOCK(&_ntoa_novell_firstTime_lock);
	    if(firstTime)
		{
		uname(&systemNameStruct);
		for ( p = systemNameStruct.nodename; *p != '\0'; p++)
		    {
		    *p = toupper(*p);
		    }

		firstTime = 0;
		}
	    MUTEX_UNLOCK(&_ntoa_novell_firstTime_lock);
	    }

	if (!serv || !tp)
	    {
	    set_nderror(ND_BADARG);
	    return (NULL);
	    }

	set_nderror(ND_OK);       /* assume success */

	/* NULL is not allowed, that returns no answer */
	if (! (serv->h_host))
	    {
	    set_nderror(ND_NOHOST);
	    return (NULL);
	    }

	/*
	    Convert the name to upper case.
	*/
	for ( p = serv->h_host, q = host; *p != '\0'; p++, q++)
	    {
	    *q = toupper(*p);
	    }

	*q = '\0';

	if (strlen(host) == 32)
	{
		host[32] = '*';
		host[33] = '\0';
	}

	/*
	**  Find the port number for the service. If the service is
	**  not found, check whether the service name is really a port number.
	*/
	if (serviceEntry_p = _novell_getsapnumberbyname (serv->h_serv))
	{
	    sap_type = serviceEntry_p->s_port;

	}
	else if (serviceEntry_p = _novell_getservbyname (serv->h_serv, tp->nc_proto))
	{
	    server_port = serviceEntry_p->s_port;

	}
	else if (strspn(serv->h_serv, "0123456789") == strlen(serv->h_serv))
	{
	    /* It's a port number */
	    server_port = atoi(serv->h_serv);
	    server_port = htons(server_port);

	} else
	{
	    set_nderror(ND_NOSERV);
	    return (NULL);
	}

	result = (struct nd_addrlist *)(malloc(sizeof(struct nd_addrlist)));
	if (!result)
	    {
	    set_nderror(ND_NOMEM);
	    return (NULL);
	    }

	result->n_cnt = 0;
	result->n_addrs = (struct netbuf *)(calloc(0, sizeof(struct netbuf)));

	if (!result->n_addrs)
	    {
	    set_nderror(ND_NOMEM);
	    return (NULL);
	    }

	if ((strcmp(host, HOST_SELF) == 0) ||
	    (strcmp(host, systemNameStruct.nodename) == 0))
	{
		char address[10];
		struct strioctl ioc;   
		int fd;

		if ( (fd = open("/dev/ipx", O_RDWR)) < 0)
		{
		}
		else {
			ioc.ic_cmd = IPX_GET_NET;
			ioc.ic_timout = 5;
			ioc.ic_len = IPX_NET_SIZE;
			ioc.ic_dp = address;

			if (ioctl(fd, I_STR, &ioc) == -1)
			{
				close(fd);
			}
			else {

				ioc.ic_cmd = IPX_GET_NODE_ADDR;
				ioc.ic_timout = 5;
				ioc.ic_len = IPX_NODE_SIZE;
				ioc.ic_dp = &address[4];

				if (ioctl(fd, I_STR, &ioc) == -1)
				{
					close(fd);
				}
				else {

					/*
					**  Have resolved host name to network and node number.
					**  Now try to resolve service name into socket number.
					*/

					if ((SAPGetServerByName((uint8 *)systemNameStruct.nodename,
						(uint16)sap_type, &serverIndex, &serverEntry, 1) > 0 ))
					{
						uint8 address[10];

						address[0] = serverEntry.serverAddress.net[0];
						address[1] = serverEntry.serverAddress.net[1];
						address[2] = serverEntry.serverAddress.net[2];
						address[3] = serverEntry.serverAddress.net[3];
						address[4] = serverEntry.serverAddress.node[0];
						address[5] = serverEntry.serverAddress.node[1];
						address[6] = serverEntry.serverAddress.node[2];
						address[7] = serverEntry.serverAddress.node[3];
						address[8] = serverEntry.serverAddress.node[4];
						address[9] = serverEntry.serverAddress.node[5];

						if (sap_type == ALL_SERVER_TYPE)
						{
							result = addAddressToList(result, address, server_port);

						} else {

							result = addAddressToList(result, address,
								*((unsigned short *)&serverEntry.serverAddress.sock[0]));
						}

					} else {

						if (sap_type == ALL_SERVER_TYPE)
						{
							result = addAddressToList (result,
								(const unsigned char *)address, server_port);
						}
					}
					close(fd);
				}
			}
		}
	}
	else if((strcmp(host, HOST_BROADCAST)) == 0)
	    {
	    result = addAddressToList (result, broadcastAddress, server_port);
	    }
	else {

		while ((SAPGetServerByName((uint8 *)host, (uint16)sap_type,
			&serverIndex, &serverEntry, 1) > 0 ))
		{
			uint8 address[10];

			address[0] = serverEntry.serverAddress.net[0];
			address[1] = serverEntry.serverAddress.net[1];
			address[2] = serverEntry.serverAddress.net[2];
			address[3] = serverEntry.serverAddress.net[3];
			address[4] = serverEntry.serverAddress.node[0];
			address[5] = serverEntry.serverAddress.node[1];
			address[6] = serverEntry.serverAddress.node[2];
			address[7] = serverEntry.serverAddress.node[3];
			address[8] = serverEntry.serverAddress.node[4];
			address[9] = serverEntry.serverAddress.node[5];

			/*
			**  This comparison is useful 'cause it prevents
			**  ambiguous requests such as "HOST*" from working.
			*/
			if ((strcasecmp(serv->h_host, serverEntry.serverName) == 0) ||
				serverEntry.serverType == 0x278)
			{
				if (sap_type == ALL_SERVER_TYPE)
				{
					result = addAddressToList(result, address, server_port);

				} else {
					result = addAddressToList(result, address,
						*((unsigned short *)&serverEntry.serverAddress.sock[0]));
				}
			}
		}

#ifdef CHECK_BINDERY

		if (result && result->n_cnt == 0)
		{
			/*
			**  Check primary connection's bindery.
			*/
			uint32	objectID;
			uint8	objectName[48];
			uint16	objectType;

			uint8	propertySequence;

			unsigned char more;

			uint32 connectionReference;
			uint32 connHandle;

			/*
			**  If NetWare requester has been initialized,
			**  then we can open a connection to the primary
			**  server and check the server's bindery.
			*/

			if (get_primary_conn_reference(&connectionReference) != SUCCESS)
			{
			}
			else if (open_conn_by_reference(connectionReference, &connHandle, 0) != SUCCESS)
			{
			}
			else {
				for (objectID = (uint32)-1;
					NWScanObject(connHandle, host, htons(sap_type),
					&objectID, objectName, &objectType, NULL, NULL, NULL) == 0;)
				{
					if ((strcasecmp(serv->h_host, objectName) == 0) || htons(objectType) == 0x278)
					{
						for (propertySequence = 1, more = TRUE;
							more && (NWReadPropertyValue(connHandle, objectName,
							objectType, "NET_ADDRESS", propertySequence, segmentData,
							&more, &propertyType) == 0);)
						{
							if (sap_type == ALL_SERVER_TYPE)
							{
								result = addAddressToList (result,
								(const unsigned char  *)segmentData, server_port);

							} else {
								result = addAddressToList(result,
								(const unsigned char *)segmentData,
								*((unsigned short *)&segmentData[10]));
							}
						}
					}
				}
				close_conn(connHandle);
			}
		}
#endif /* CHECK_BINDERY */
	}

	if (result == NULL)
	{
	    set_nderror(ND_NOMEM);
	}
	else if (result->n_cnt == 0)
	{
	    free(result->n_addrs);
	    free(result);
	    set_nderror(ND_NOHOST);
	    result = NULL;
	}

	return (result);
}

static char *
getsapname(unsigned short saptype)
{
	struct servent *s = NULL;

	/*
	**  Look up sap number in /etc/netware/saptypes file.
	*/

	if (s = _novell_getsapnamebynumber(saptype))
	{
		return(strdup(s->s_name));
	}

	return(NULL);
}

static void
fix_socket_name(struct nd_hostservlist *result, unsigned short socket, char *proto)
{
	struct servent *s = NULL;
	char buf[6];
	char *p;
	int i, j;

	if (s = _novell_getservbysocket(socket, proto))
	{
		p = s->s_name;

	} else {

		sprintf(buf, "%d", htons(socket));
		p = buf;
	}

	for (i = 0; i < result->h_cnt; i++)
	{
		/*
		**  If service name is non NULL, then the service
		**  name is really the name of a sap type.
		**  If the service name is NULL, then the user specified
		**  socket number has yet to be converted into a name,
		**  and that's what we're going to do now.
		*/

		if (result->h_hostservs[i].h_serv == NULL)
		{
			result->h_hostservs[i].h_serv = strdup(p);

		} else {

			if (i != 0)
			{
				free(result->h_hostservs[0].h_host);
				free(result->h_hostservs[0].h_serv);
				result->h_hostservs[0].h_host = result->h_hostservs[i].h_host;
				result->h_hostservs[0].h_serv = result->h_hostservs[i].h_serv;
				result->h_hostservs[i].h_host = NULL;
				result->h_hostservs[i].h_serv = NULL;
			}
			for (j = 1; j < result->h_cnt; j++)
			{
				if (result->h_hostservs[j].h_host)
					free(result->h_hostservs[j].h_host);
				if (result->h_hostservs[j].h_serv)
					free(result->h_hostservs[j].h_serv);
			}
			result->h_cnt = 1;
			return;
		}
	}
}

struct nd_hostservlist *
_netdir_getbyaddr ( N_CONST struct netconfig	*tp,
    		    N_CONST struct netbuf	*addr )
{
	int serverIndex = 0;
	SAPI serverEntry;
	struct nd_hostservlist *result;
	struct nd_hostserv *host1, *host2;
	struct strioctl ioc;   
	ipxAddr_t serverAddress;

#ifdef CHECK_BINDERY
	char address[10];
	char segmentData[256];
#else
	char address[10];
#endif

	int fd, r;

#ifdef CHECK_BINDERY
	uint32	objectID;
	uint8	objectName[48];
	uint16	objectType;

	uint8	propertySequence;

	uint16	typeOfFirstEntry = 0xffff;

	unsigned char more, propertyType;

	uint32 connectionReference;
	uint32 connHandle;
#else
	uint16	typeOfFirstEntry = 0xffff;
#endif

	char *p;

	if (!addr || !tp || (addr->len != 12))
	{
		set_nderror(ND_BADARG);
		return (NULL);
	}

	set_nderror(ND_OK);       /* assume success */

	if ((result = (struct nd_hostservlist *)calloc(1, sizeof(*result))) == NULL)
	{
		set_nderror(ND_NOMEM);
		return(NULL);
	}

	serverAddress.net[0]  = addr->buf[0];
	serverAddress.net[1]  = addr->buf[1];
	serverAddress.net[2]  = addr->buf[2];
	serverAddress.net[3]  = addr->buf[3];
	serverAddress.node[0] = addr->buf[4];
	serverAddress.node[1] = addr->buf[5];
	serverAddress.node[2] = addr->buf[6];
	serverAddress.node[3] = addr->buf[7];
	serverAddress.node[4] = addr->buf[8];
	serverAddress.node[5] = addr->buf[9];

	while (SAPGetServerByAddr(&serverAddress, (uint16)ALL_SERVER_TYPE,
		&serverIndex, &serverEntry, 1 ))
	{
		p = realloc ( result->h_hostservs,
			(result->h_cnt + 1) * sizeof(struct nd_hostserv));

		if (p == NULL)
		{
			netdir_free(result, ND_HOSTSERVLIST);
			set_nderror(ND_NOMEM);
			return(NULL);
		}

		result->h_cnt++;
		result->h_hostservs = (struct nd_hostserv *)p;

		/*
		**  Make sure first entry has lowest type.
		**  Remaining entries are not sorted.
		*/
		host1 = &result->h_hostservs[0];
		host2 = &result->h_hostservs[result->h_cnt - 1];
		if (serverEntry.serverType < typeOfFirstEntry)
		{
			host2->h_host = host1->h_host;
			host2->h_serv = host1->h_serv;
			host1->h_host = strdup((char *)serverEntry.serverName);

			if ((addr->buf[10] || addr->buf[11]) &&
				((serverEntry.serverAddress.sock[0] == (uint8)addr->buf[10]) &&
				(serverEntry.serverAddress.sock[1] == (uint8)addr->buf[11])))
			{
				host1->h_serv = getsapname(serverEntry.serverType);
			} else {
				host1->h_serv = NULL;
			}

			typeOfFirstEntry = serverEntry.serverType;
		} else {
			host2->h_host = strdup((char *)serverEntry.serverName);

			if ((addr->buf[10] || addr->buf[11]) &&
				((serverEntry.serverAddress.sock[0] == (uint8)addr->buf[10]) &&
				(serverEntry.serverAddress.sock[1] == (uint8)addr->buf[11])))
			{
				host2->h_serv = getsapname(serverEntry.serverType);
			} else {
				host2->h_serv = NULL;
			}
		}
	}

	if (result->h_cnt)
	{
		fix_socket_name(result, *((unsigned short *)&addr->buf[10]),
			tp->nc_proto);
		return(result);
	}

	/*
	**  For efficiency, we check firstTime first, and lock only
	**  if we may have to modify firstTime and systemNameStruct.
	**  After locking, we check again to make sure that another
	**  thread didn't change firstTime in the meantime.
	*/
	if (firstTime)
	{
		MUTEX_LOCK(&_ntoa_novell_firstTime_lock);
		if (firstTime)
		{
			uname(&systemNameStruct);
			for ( p = systemNameStruct.nodename; *p != '\0'; p++)
			{
				*p = toupper(*p);
			}

			firstTime = 0;
		}
		MUTEX_UNLOCK(&_ntoa_novell_firstTime_lock);
	}

	/*
	**  Is it our own address?
	*/
	if ( (fd = open("/dev/ipx", O_RDWR)) < 0)
	{
	}
	else {
		ioc.ic_cmd = IPX_GET_NET;
		ioc.ic_timout = 5;
		ioc.ic_len = IPX_NET_SIZE;
		ioc.ic_dp = address;

		if (ioctl(fd, I_STR, &ioc) == -1)
		{
			close(fd);
		}
		else {

			ioc.ic_cmd = IPX_GET_NODE_ADDR;
			ioc.ic_timout = 5;
			ioc.ic_len = IPX_NODE_SIZE;
			ioc.ic_dp = &address[4];

			if (ioctl(fd, I_STR, &ioc) == -1)
			{
				close(fd);
			}
			else {

				close(fd);
				if (!memcmp(address, addr->buf, 10))
				{
					p = realloc ( result->h_hostservs,
						(result->h_cnt + 1) * sizeof(struct nd_hostserv));

					if (p == NULL)
					{
						netdir_free(result, ND_HOSTSERVLIST);
						set_nderror(ND_NOMEM);
						return(NULL);
					}
					result->h_cnt++;
					result->h_hostservs = (struct nd_hostserv *)p;

					host1 = &result->h_hostservs[result->h_cnt - 1];
					host1->h_host = strdup((char *)(systemNameStruct.nodename));
					host1->h_serv = NULL;

					return(result);
				}
			}
		}
	}

#ifdef CHECK_BINDERY

	/*
	**  Check primary connection's bindery.
	*/

	if (get_primary_conn_reference(&connectionReference) != SUCCESS)
	{
	}
	else if (open_conn_by_reference(connectionReference, &connHandle, 0) != SUCCESS)
	{
	}
	else {
		for (objectID = (uint32)-1;
			NWScanObject(connHandle, "*", NWOT_WILD, &objectID, objectName,
			&objectType, NULL, NULL, NULL) == 0;)
		{
			for (propertySequence = 1, more = TRUE;
				more && (NWReadPropertyValue(connHandle, objectName,
				objectType, "NET_ADDRESS", propertySequence,
				segmentData, &more, &propertyType) == 0);)
			{
				if (!memcmp(segmentData, addr->buf, 10))
				{
					p = realloc ( result->h_hostservs,
						(result->h_cnt + 1) * sizeof(struct nd_hostserv));

					if (p == NULL)
					{
						netdir_free(result, ND_HOSTSERVLIST);
						close_conn(connHandle);
						set_nderror(ND_NOMEM);
						return(NULL);
					}
					result->h_cnt++;
					result->h_hostservs = (struct nd_hostserv *)p;

					/*
					**  Make sure first entry has lowest type.
					**  Remaining entries are not sorted.
					*/
					host1 = &result->h_hostservs[0];
					host2 = &result->h_hostservs[result->h_cnt - 1];
					if (htons(objectType) < typeOfFirstEntry)
					{
						host2->h_host = host1->h_host;
						host2->h_serv = host1->h_serv;
						host1->h_host = strdup((char *)objectName);
						if ((addr->buf[10] || addr->buf[11]) &&
							((segmentData[10] == addr->buf[10]) &&
							(segmentData[11] == addr->buf[11])))
						{
							host1->h_serv = getsapname(htons(objectType));
						} else {
							host1->h_serv = NULL;
						}
						typeOfFirstEntry = htons(objectType);
					} else {
						host2->h_host = strdup((char *)objectName);
						if ((addr->buf[10] || addr->buf[11]) &&
							((segmentData[10] == addr->buf[10]) &&
							(segmentData[11] == addr->buf[11])))
						{
							host2->h_serv = getsapname(htons(objectType));
						} else {
							host2->h_serv = NULL;
						}
					}
				}
			}
		}
		close_conn(connHandle);
	}
#endif /* CHECK_BINDERY */

	if (result->h_cnt)
	{
		fix_socket_name(result, *((unsigned short *)&addr->buf[10]),
			tp->nc_proto);
	}
	return(result);
}

char *
_taddr2uaddr ( N_CONST struct netconfig	*tp,
    	       N_CONST struct netbuf		*addr )
{
	unsigned char *address;

	char buffer[64];
	
	if (!addr || !tp)
	{
		set_nderror(ND_BADARG);
		return (NULL);
	}

	address = (unsigned char *)addr->buf;

	sprintf
	    (
	    buffer,
	    "%.2x%.2x%.2x%.2x.%.2x%.2x%.2x%.2x%.2x%.2x.%.2x%.2x",
	    address[0],
	    address[1],
	    address[2],
	    address[3],
	    address[4],
	    address[5],
	    address[6],
	    address[7],
	    address[8],
	    address[9],
	    address[10],
	    address[11]
	    );

	return(strdup(buffer));
}


struct netbuf *
_uaddr2taddr ( N_CONST struct netconfig *tp,
    	       N_CONST char *addr )
{
	struct netbuf *result = NULL;

	if (!addr || !tp)
	{
		set_nderror(ND_BADARG);
	}
	else if ((result = (struct netbuf *)malloc(sizeof(*result))) == NULL)
	{
		set_nderror(ND_NOMEM);
	}
	else if((result->buf = calloc(1, sizeof(struct ipxAddress))) == NULL)
	{
		free((char *)result);
		result = NULL;
		set_nderror(ND_NOMEM);
	}
	else {

		result->len = sizeof(struct ipxAddress);
		result->maxlen = sizeof(struct ipxAddress);

		if(sscanf(addr, "%8x.%8x%4hx.%4hx",
			&result->buf[0], &result->buf[4],
			&result->buf[8], &result->buf[10]) != 4)
		{
			free(result->buf);
			free((char *)result);
			result = NULL;
			set_nderror(ND_BADARG);
		}

		*((unsigned int *)&result->buf[0]) =
			htonl(*((unsigned int *)&result->buf[0]));

		*((unsigned int *)&result->buf[4]) =
			htonl(*((unsigned int *)&result->buf[4]));

		*((unsigned short *)&result->buf[8]) =
			htons(*((unsigned short *)&result->buf[8]));

		*((unsigned short *)&result->buf[10]) =
			htons(*((unsigned short *)&result->buf[10]));
	}
	return(result);
}

static
ifioctl ( int		s,
	  int		cmd,
    	  char		*arg,
	  int		len )
{
        struct strioctl
	    ioc;

        ioc.ic_cmd = cmd;
        ioc.ic_timout = 0;
        if (len)
	    {
	    ioc.ic_len = len;
	    }
        else
	    {
	    ioc.ic_len = sizeof (struct ifreq);
	    }

        ioc.ic_dp = arg;
        return (ioctl(s, I_STR, (char *) &ioc) < 0 ? -1 : ioc.ic_len);
}

/*
 * Access functions to set and get the initial guess of the next
 * reserved port to which to bind.
 */
static short guess;

static short
_set_resvport_guess(port)
	short port;
{
	guess = port;
	MUTEX_UNLOCK(&_ntoa_novell_bindresvport_lock);
	return (0);
}

static short
_get_resvport_guess()
{
	MUTEX_LOCK(&_ntoa_novell_bindresvport_lock);
	if (guess == 0)
		guess = (getpid() % NPORTS) + STARTPORT;
	return (guess);
}

/*
 * This function is called for netdir_options(ND_SETRESERVEDPORT)
 * to bind to a reserved port.
 *
 * If the caller passes an address, then try to bind to the port
 * specified in that address. Ignore whether that port is reserved or not.
 *
 * If the caller passes a null addr, then use an algorithm to pick
 * a reserved port, and try to bind to it.  If t_bind fails
 * because the port is in use, keep trying until you have tried
 * all reserved ports (above STARTPORT).
 */

static
bindresvport(fd, addr)
	int	fd;
	struct	netbuf *addr;
{
	short	port;			/* port number that we assign */
	struct	ipxAddress *ipxa;	/* a pointer into addr.buf */
	struct	t_bind	*tbindp;	/* argument to t_bind */
	int	ret;			/* general purpose return var */
	int	i;			/* counter */

	set_nderror(ND_OK);
	if ((ret = t_getstate(fd)) < 0) {
		set_nderror(ND_XTIERROR);
		return (-1);
	}

	if (ret != T_UNBND) {
		set_nderror(ND_BADSTATE);
		return (-1);
	}

	if (addr) {
		if ( addr->len != 0xc ) {
			set_nderror(ND_FAILCTRL);
			errno = EPFNOSUPPORT;   /*leftover from sockets code ?*/
			return (-1);
		}
	}

	/* allocate structure for tbindp */
	if ((tbindp = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR)) == NULL) {
		set_nderror(ND_XTIERROR);
		return (-1);
	}

	if (tbindp->addr.maxlen < sizeof(struct ipxAddress)) {
		t_free((char *)tbindp, T_BIND);
		set_nderror(ND_NOMEM);
		return (-1);
	}
	tbindp->qlen = 0;	/* user should reset qlen if needed */

	if (!addr) {
		/*
		 * this is when the caller does not care which reserved
		 * port we bind to
		 *
		 * set up in tbindp->addr an IPX address that
		 * includes a reserved port.  We pick the reserved
		 * port using _get_resvport_guess().
		 */
		port = _get_resvport_guess();
			/*
			 * _get_resvport_guess grabs _ntoa_novell_bindresvport_lock.
			 * MUST BE RELEASED BEFORE RETURNING!
			 * _set_resvport_guess releases it.
			 */
		tbindp->addr.len = sizeof(struct ipxAddress);
		ipxa = (struct ipxAddress *)tbindp->addr.buf;
		(void)memset((char *)ipxa, 0, sizeof (struct ipxAddress));

		/*
		 * if t_binds fails with TADDRBUSY, we try to bind to
		 * all ports in the range STARTPORT..ENDPORT
		 */
		for (i = 0; i < NPORTS; i++) {
			ipxa->sock[0] = (port >> 8) & 0xff;
			ipxa->sock[1] = port & 0xff;
			port++;

			/* Port has been incremented (above).
			 * Reset port if not in the reserved range.
			 */
			if (port > ENDPORT)
				port = STARTPORT;

			if(t_bind(fd, tbindp, (struct t_bind *)NULL) == 0)
				break;

			if (get_t_errno() != TADDRBUSY) {
				t_free((char *)tbindp, T_BIND);
				set_nderror(ND_XTIERROR);
				_set_resvport_guess(port);
				return (-1);
			}
			/* This port is busy.
			 * Keep on trying.
			 */
		}                                                          

		if(i == NPORTS) {
			t_free((char *)tbindp, T_BIND);
			set_nderror(ND_FAILCTRL);
			_set_resvport_guess(port);
			return(-1);
		}

		_set_resvport_guess(port);
				/* Released _ntoa_novell_bindresvport_lock */

	} else {

		/* user supplied address, including port */
		memcpy((void *)tbindp->addr.buf, addr->buf, addr->len);
		tbindp->addr.len = addr->len;

		if (t_bind(fd, tbindp, (struct t_bind *)NULL) != 0) {
			ret = get_t_errno(); /* save original error */
			t_free((char *)tbindp, T_BIND);
			set_t_errno(ret); /* in case t_free overrides */
			set_nderror(ND_XTIERROR);
			return (-1);
		}
	}

	t_free((char *)tbindp, T_BIND);
	return (0);
}

static
checkresvport ( struct netbuf *addr )
{
	struct ipxAddress	*ipxa;
	unsigned short		port;

	if (addr == NULL) {
		set_nderror(ND_FAILCTRL);
		return (-1);
	}
	ipxa = (struct ipxAddress *)addr->buf;
	port = (short)((ipxa->sock[0] << 8) | ipxa->sock[1]);
	if (port >= START_DYNM_RESERVED && port <= END_DYNM_RESERVED)
		return (0);
	if (port >= START_STATIC_RESERVED && port <= END_STATIC_RESERVED)
		return (0);
	/*
	**  The tcpip resolver incorrectly sets _nderror to FAILCTRL.
	*/
	return (-1);
}

int
_netdir_options ( struct netconfig *tp,
	int	opts,
	int	fd,
	char	*par )
{
	struct nd_mergearg *ma;

	set_nderror(ND_OK);
	switch(opts)
	{
		case ND_SET_RESERVEDPORT:
		{
			return (bindresvport(fd, (struct netbuf *)par));
		}

		case ND_CHECK_RESERVEDPORT:
		{
			return (checkresvport((struct netbuf *)par));
		}

		case ND_MERGEADDR:
		{
			ma = (struct nd_mergearg *)(par);
			if ((ma->m_uaddr = strdup(ma->s_uaddr)) == NULL)
			{
 				set_nderror(ND_NOMEM);
				return(-1);
			}
			return(0);
		}

		case ND_SET_BROADCAST:
		{
			return(0);
		}

		default:
		{
			set_nderror(ND_NOCTRL);
			return(-1);
		}
	}
}
