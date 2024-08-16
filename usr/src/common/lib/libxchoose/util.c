/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libxchoose:util.c	1.5"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libxchoose/util.c,v 1.5 1994/05/09 13:36:22 plc Exp $"

#include "util.h"
#include <sys/ipx_app.h>

/*
 *  tabstops are set to 4
 *	This is a quiet file, meaning no debug messages are generated
 *	for normal operations.  Also no strings in this file are in
 *	need of internationalization. Lucky me :-)
 */

/*-----------------------------------------------------------------------*/
/*  Routine: BindAndConnect
/*
/*  Purpose: Do all necessary step to connect to a transport endpoint.
/*			 Binding is done to a transport provided port number.
/*			 Port number and remote system address for connection are
/*           determined from the argumented netbuf structure.
/*
/*  Entry:	 netconfigp	pointer to transport provider information.
/*			 netbufp	Address to connect to.
/*
/*	Exit:	 >0		alls well, fd is valid
/*			 -1		Can't open device,can't bind
/*					can't connect.               
/*-----------------------------------------------------------------------*/
int
BindAndConnect(netconfigp,netbufp)
    struct	netconfig	*netconfigp;
    struct	netbuf		*netbufp;
{
	int		fd;
	struct	t_call		*callptr;

	if ((fd = t_open(netconfigp->nc_device,  O_RDWR, NULL)) < 0)
	{
		syslog(LOG_DEBUG,"BindAndConnect():t_open %s",netconfigp->nc_device);
		return(-1);
	}

	if(t_bind(fd,NULL,NULL) < 0 ) 
	{
		tli_error("BindAndConnect():t_bind:",TO_SYSLOG,LOG_DEBUG);
		t_close(fd);
		return(-1);
	}
	/*
 	*  Allocate a library structure for this endpoint. All fields.
 	*/
   	if((callptr = (struct t_call *) t_alloc(fd,T_CALL,T_ALL)) == NULL)
   	{
		tli_error("BindAndConnect():t_alloc:",TO_SYSLOG,LOG_DEBUG);
		t_unbind(fd);
		t_close(fd);
		return(-1);
   	}
	callptr->addr.buf = netbufp->buf;
	callptr->addr.len = netbufp->len;
	callptr->addr.maxlen = netbufp->maxlen;

	callptr->opt.buf = NULL;
	callptr->opt.len = 0;
	callptr->opt.maxlen = 0;

	callptr->udata.buf = NULL;
	callptr->udata.len = 0;
	callptr->udata.maxlen = 0;
	if ( t_connect(fd,callptr,NULL) < 0 )
	{
		tli_error("BindAndConnect():t_connect:",TO_SYSLOG,LOG_DEBUG);
		(void) t_free((char *)callptr,T_CALL);
		t_close(fd);
		return(-1);
	}
	(void) t_free((char *)callptr,T_CALL);
	return(fd);
}
/*-----------------------------------------------------------------------*/
/*  Routine: BindPortAndConnect
/*
/*  Purpose: Do all necessary step to connect to a transport endpoint
/*			 on a specific port number on the remote machine.
/*			 Binding is done to a transport provided port number.
/*			 Connection is done to the argumented hostname on the
/*			 specified port number.
/*
/*  Entry:	 netconfigp	pointer to transport provider information.
/*			 port   	connection must be done to this number.
/*			 hostname	name of remote host to connect to.
/*
/*	Exit:	 >0		alls well, fd is valid
/*			 -1		Can't open device,Space not available, can't bind
/*					bound2 not equal to bind2
/*-----------------------------------------------------------------------*/
int
BindPortAndConnect(netconfigp,port,hostname)
    struct	netconfig	*netconfigp;
    int 				*port;
	char		*hostname;
{
	int		fd;
	char	port_num[16];
    struct	netbuf		*netbufp;
	struct	nd_hostserv nd_hostserv;		
	struct	nd_addrlist *nd_addrlistp = NULL;

	/*
	 * Have to specify a port number 
	 */
	if ( *port == 0 )
	{
		return(-1);
	}
	sprintf(port_num,"%4.4d",*port);
	nd_hostserv.h_host = hostname;
	nd_hostserv.h_serv = port_num;
	if(netdir_getbyname(netconfigp, &nd_hostserv, &nd_addrlistp) == 0)
	{
		netbufp = nd_addrlistp->n_addrs;
		if (( fd = BindAndConnect(netconfigp,netbufp)) < 0)
		{
			(void) netdir_free((char *)nd_addrlistp, ND_ADDRLIST);
			return(-1);
		}
		(void) netdir_free((char *)nd_addrlistp, ND_ADDRLIST);
		return(fd);
	}
	return(-1);
}
/*-----------------------------------------------------------------------*/
/*  Routine: BindReservedAndConnect
/*
/*  Purpose: Do all necessary step to connect to a transport endpoint
/*			 using a transport specific reserved port number.
/*			 Binding is done to a transport provided reserved port number.
/*			 Port number and remote system address for connection are
/*           determined from the argumented netbuf structure.
/*
/*  Entry:	 netconfigp	pointer to transport provider information.
/*			 netbufp	Address create transport endpoint on. 
/*
/*	Exit:	 >0		alls well, fd is valid
/*			 -1		Cand't open device,Space not available, can't bind
/*					bound2 not equal to bind2
/*-----------------------------------------------------------------------*/
int
BindReservedAndConnect(netconfigp,netbufp)
    struct	netconfig	*netconfigp;
    struct	netbuf		*netbufp;
{
	int		fd;
	struct	t_call	*callptr;
	extern int _nderror;

	if ((fd = t_open(netconfigp->nc_device, O_RDWR, NULL)) < 0)
	{
		syslog(LOG_DEBUG,"BindReservedAndConnect():t_open %s",
														netconfigp->nc_device);
		return (-1);
	}
	/*
	 *	Need a reserved port, don't care which one.
	 */
	if (netdir_options(netconfigp,ND_SET_RESERVEDPORT,fd,NULL) < 0)
	{
		syslog(LOG_DEBUG,"BindReservedAndConnect():netdir_options: %s",
														netconfigp->nc_netid);
		syslog(LOG_DEBUG,"                        :_nderror = %d",_nderror);
		t_close(fd);
		return(-1);
	}
   	if((callptr = (struct t_call *) t_alloc(fd,T_CALL,T_ALL)) == NULL)
   	{
		tli_error("BindReservedAndConnect():t_alloc:",TO_SYSLOG,LOG_DEBUG);
		t_unbind(fd);
		t_close(fd);
		return(-1);
   	}
	callptr->addr.buf = netbufp->buf;
	callptr->addr.len = netbufp->len;
	callptr->addr.maxlen = netbufp->maxlen;

	callptr->opt.buf = NULL;
	callptr->opt.len = 0;
	callptr->opt.maxlen = 0;

	callptr->udata.buf = NULL;
	callptr->udata.len = 0;
	callptr->udata.maxlen = 0;

	if (t_connect(fd, callptr, NULL) < 0) 
	{
		tli_error("BindReservedAndConnect():t_connect:",TO_SYSLOG,LOG_DEBUG);
		t_close(fd);
		return (-1);
	}
	(void) t_free((char *)callptr,T_CALL);
	return (fd);
}
/*-----------------------------------------------------------------------*/
/*  Routine: BindRSAndConnect
/*
/*  Purpose: Do all necessary step to connect to a transport endpoint
/*			 on a specific port number on the remote machine.
/*			 Binding is done to a transport provided reserved port number.
/*			 Connection is done to the argumented hostname on the
/*			 specified port number.
/*			 port is not tested for inclusion in the reserved set.
/*
/*  Entry:	 netconfigp	pointer to transport provider information.
/*			 port   	if pointed to value is non-zero then connection
/*						must be done using this number. 
/*			 hostname	name of remote host to connect to.
/*
/*	Exit:	 >0		alls well, fd is valid
/*			 -1		Can't open device,Space not available, can't bind
/*					bound2 not equal to bind2
/*-----------------------------------------------------------------------*/
int
BindRSAndConnect(netconfigp,port,hostname)
    struct	netconfig	*netconfigp;
    int 				*port;
	char				*hostname;
{
	int		fd;
	char	port_num[16];
    struct	netbuf		*netbufp;
	struct	nd_hostserv nd_hostserv;		/* used for verification */
	struct	nd_addrlist *nd_addrlistp = NULL;

	if ( *port == 0 )
	{
		return(-1);
	}
	sprintf(port_num,"%4.4d",*port);
	nd_hostserv.h_host = hostname;
	nd_hostserv.h_serv = port_num;
	if(netdir_getbyname(netconfigp, &nd_hostserv, &nd_addrlistp) == 0)
	{
		netbufp = nd_addrlistp->n_addrs;
		if ((fd = BindReservedAndConnect(netconfigp,netbufp)) < 0 )
		{
			(void) netdir_free((char *)nd_addrlistp, ND_ADDRLIST);
			return(-1);
		}
		(void) netdir_free((char *)nd_addrlistp, ND_ADDRLIST);
		return(fd);
	}
	return(-1);
}
/*-----------------------------------------------------------------------*/
/*  Routine: BindAndAccept
/*
/*  Purpose: Do all necessary step to create a new transport endpoint onto
/*			 which we accept a connection request.
/*			 Meat of this routine is in xtli.c.  SetupTliCallBuf must have 
/*			 been called for this fd.
/*
/*  Entry:	 conn_fd	fd that the conection request was recieved on
/*			 callptr	were to stick the connection information
/*
/*	Exit:	 >0		alls well, connection accepted.
/*			 -1		Can't open device,can't get a reserved port      
/*					couldn't accept the connection or
/*					errno set to EINTR is t_accept returns with TLOOK
/*-----------------------------------------------------------------------*/
int
BindAndAccept(conn_fd,callptr )
	int		conn_fd;
	struct	t_call	**callptr;
{
	int		fd;
	extern int MoreConnections;

	fd = ConnectTliClient(conn_fd,&MoreConnections,callptr);
	return(fd);
}
/*-----------------------------------------------------------------------*/
/*  Routine: BindAndListenOnWellKnown
/*
/*  Purpose: Do all necessary step to create a transport endpoint on
/*			 a well known specified address.
/*			 Should only be used for creating endpoints for services that
/*			 are in /etc/services.
/*			 
/*
/*  Entry:	 netconfigp	pointer to transport provider information.
/*			 netbufp	Address create transport endpoint on. 
/*			 callptr	Filled in by this routine.
/*
/*	Exit:	 >0		alls well, fd is valid
/*			 -1		Can't open device,Space not available, can't bind
/*					bound2 not equal to bind2
/*-----------------------------------------------------------------------*/
int
BindAndListenOnWellKnown(netconfigp,netbufp,callptr)
    struct	netconfig	*netconfigp;
    struct	netbuf		*netbufp;
    struct	t_call		**callptr;
{
	int		fd;
	struct  t_bind *bind2;
	struct  t_bind *bound2;

	if ((fd = t_open(netconfigp->nc_device,  O_RDWR, NULL)) < 0)
	{
		syslog(LOG_DEBUG,"BindAndListenOnWellKnown():t_open: %s:",
													netconfigp->nc_device);
		return(-1);
	}

	/*
	 * Be conservative only get what we need.
	 */
	if ((bind2 = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR)) == NULL )
	{
		tli_error("BindAndListenOnWellKnown():t_alloc:",TO_SYSLOG,LOG_DEBUG);
		t_close(fd);
		return (-1);
	}
	if ((bound2 = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR)) == NULL )
	{
		tli_error("BindAndListenOnWellKnown():t_alloc:",TO_SYSLOG,LOG_DEBUG);
		(void) t_free( (char *)bind2, T_BIND);
		t_close(fd);
		return (-1);
	}
	(void) memcpy(bind2->addr.buf, netbufp->buf,
								 (int) bind2->addr.maxlen);
	bind2->addr.len = bind2->addr.maxlen;

	/*
	 *  Set the number of connections requests that can be accepted
	 *	without performing a t_accept.  ( This variable seems
	 *	to have different meanings. On USL SVR5 TCP a qlen of 1 allows
	 *	2 connections requests to be accepted by the transport, all
	 * 	other connection requests are dropped until the transport queue 
	 *	has dropped below the qlen level. On Novell's SPX this setting 
	 *	allows only 1 connection request in at a time. This avoids the
	 *	asynchronous event warning in t_accept, at least for multiple
	 *	connect requests. )
	 */
	bind2->qlen = 2;
	if(t_bind (fd, bind2, bound2) < 0)
	{
		tli_error("BindAndListenOnWellKnown():t_bind:",TO_SYSLOG,LOG_DEBUG);
		(void) t_free( (char *)bind2, T_BIND);
		(void) t_free( (char *)bound2, T_BIND);
		t_unbind(fd);
		t_close(fd);
		return(-1);
	}
	/*
	 *  Need to verify that we have been bound to the correct address 
	 *	since TLI has the option to assign a different one. One would
	 *	think there would be an option to suppress this FEATURE.
	 */
	if ( verify_bound2(bind2,bound2,netconfigp) != 0)
	{
		(void) t_free( (char *)bind2, T_BIND);
		(void) t_free( (char *)bound2, T_BIND);
		t_unbind(fd);
		t_close(fd);
		return (-1);
	}
	/*
	 * Don't need these guys anymore.
	 */
	(void) t_free( (char *)bind2, T_BIND);
	(void) t_free( (char *)bound2, T_BIND);
	
	/*
	 *  Allocate a library structure for this endpoint. All fields.
	 */
    if((*callptr = (struct t_call *) t_alloc(fd,T_CALL,T_ALL)) == NULL)
    {
		tli_error("BindAndListenOnWellKnown():t_alloc:",TO_SYSLOG,LOG_DEBUG);
		t_unbind(fd);
		t_close(fd);
		return(-1);
    }
	return(fd);
}
/*-----------------------------------------------------------------------*/
/*  Routine: AcceptConnection
/*
/*  Purpose: Do all necessary step to accept a connection on an endpoint.
/*			 Use this only when a SINGLE connection request can be 
/*			 generated for this endpoint.  We do not support the
/*			 asynchronize event problem here.
/*
/*  Entry:	 fd			fd that the connection request is to be received on
/*			 callptr	were to stick the connection information
/*
/*	Exit:	 >0		alls well, connection accepted.
/*			 -1		couldn't accept the connection
/*					connection transport endpoint CLOSED.
/*-----------------------------------------------------------------------*/
int 
AcceptConnection(fd,callptr)
	int 			fd;
	struct t_call	**callptr;
{
	int t;

   	if((*callptr = (struct t_call *) t_alloc(fd,T_CALL,T_ALL)) == NULL)
   	{
		tli_error("AcceptConnection():t_alloc:",TO_SYSLOG,LOG_DEBUG);
		return(-1);
   	}
	t_listen(fd, *callptr);
	if (t_accept(fd,fd, *callptr) < 0) 
	{
		tli_error("AcceptConnection():t_accept:",TO_SYSLOG,LOG_DEBUG);
		if (t_errno == TLOOK) 
		{
	        t = t_look(fd);
        	switch(t)
        	{
        		case T_LISTEN	  :
	        		syslog(LOG_DEBUG, "T_LISTEN");	    
					break;
        		case T_CONNECT	  :
	        		syslog(LOG_DEBUG, "T_CONNECT");	    
					break;
        		case T_DATA	  :
	        		syslog(LOG_DEBUG, "T_DATA");	    
					break;
  				case T_EXDATA	  :
	        		syslog(LOG_DEBUG, "T_EXDATA");	    
					break;
  				case T_DISCONNECT :
	        		t_rcvdis(fd, NULL);
	        		syslog(LOG_DEBUG, "T_DISCONNECT");	    
					break;
  				case T_UDERR	  :
	        		syslog(LOG_DEBUG, "T_UNDER");	    
					break;
  				case T_ORDREL	  :
	        		syslog(LOG_DEBUG, "T_ORDEL");	    
					break;
  			}
		    errno = EINTR;
			t_close(fd);
			return(0);
		}
		t_close(fd);
		return (-1);
	}
	return(0);
}
/*-----------------------------------------------------------------------*/
/*  Routine: BindRSAndListen
/*
/*  Purpose: 
/*	
/*
/*  Entry:
/*	
/*
/*
/*
/*	Exit:	
/*
/*-----------------------------------------------------------------------*/
int
BindRSAndListen(netconfigp,port,hostname)
    struct	netconfig	*netconfigp;
    int 				*port;
	char				*hostname;
{

	int 	portmax,portmin;
	char	port_num[16];
	int		fd,i,x;
	struct  t_bind *bind2;
	struct  t_bind *bound2;
    struct	netbuf		*netbufp;
	struct	nd_hostserv nd_hostserv;		/* used for verification */
	struct	nd_addrlist *nd_addrlistp = NULL;
	int		bound		= FALSE;

	/*
	 *	Find the correct range of reserved ports for this transport
	 *	type.
	 */
	if ( strcmp(netconfigp->nc_netid,"tcp") == 0 )
	{
		portmax = IPPORT_RESERVED - 1;
		portmin = IPPORT_RESERVED/2;
	}
	else if ( strcmp(netconfigp->nc_netid,"spx") == 0 )
	{
		portmax = IPXPORT_RESERVED;
		portmin = STARTPORT;
	}
	else
	{
		syslog(LOG_DEBUG, "BindRSAndListen(): %s != (TCP || SPX)",
														netconfigp->nc_netid);
		return(-1);
	}
	if ((fd = t_open(netconfigp->nc_device,  O_RDWR, NULL)) < 0)
	{
		syslog(LOG_DEBUG,"BindRSAndListen():t_open: %s",netconfigp->nc_device);
		return(-1);
	}
	if ((bind2 = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR)) == NULL )
	{
		tli_error("BindRSAndListen:t_alloc ",TO_SYSLOG,LOG_DEBUG);
		return (-1);
	}
	if ((bound2 = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR)) == NULL )
	{
		tli_error("BindRSAndListen:t_alloc ",TO_SYSLOG,LOG_DEBUG);
		(void) t_free( (char *)bind2, T_BIND);
		return (-1);
	}
	/*
	 *	Bind to a reserved port
	 */
	for(i = portmin; i < portmax && bound == FALSE;i++ )
	{
		sprintf(port_num,"%4.4d",i);
		*port = i;						/* set the bound port number */
		nd_hostserv.h_host = hostname;
		nd_hostserv.h_serv = port_num;
		if(netdir_getbyname(netconfigp, &nd_hostserv, &nd_addrlistp) == 0)
		{
            netbufp = nd_addrlistp->n_addrs;
			/*
			 *  Keep trying to bind until all host address exhausted
			 *  or success.
			 */ 
            for(x=0; x< nd_addrlistp->n_cnt; x++)
            {
				(void) memcpy(bind2->addr.buf, netbufp->buf, 
											(int) bind2->addr.maxlen);
				bind2->addr.len = bind2->addr.maxlen;
				bind2->qlen = 8;

				if(t_bind(fd,bind2,bound2) < 0 ) 
				{
					netbufp++;
					continue;
				}
				/*
				 *  Verify the address we were bound to.
				 */
				if ( verify_bound2(bind2,bound2,netconfigp) == 0 )
				{
					bound = TRUE;
					break;
				}
				t_unbind(fd);
			}
			(void) netdir_free((char *)nd_addrlistp, ND_ADDRLIST);
		}
	}
	(void) t_free( (char *)bind2, T_BIND);
	(void) t_free( (char *)bound2, T_BIND);
	if ( bound == TRUE )
	{
		return(fd);
	}
	return(-1);
}
/*-----------------------------------------------------------------------*/
/*  Routine: BindAndListen
/*
/*  Purpose: 
/*	
/*
/*  Entry:
/*	
/*
/*
/*
/*	Exit:	
/*
/*-----------------------------------------------------------------------*/
int
BindAndListen(netconfigp,port)
    struct	netconfig	*netconfigp;
    int 				*port;
{

	int		fd;
	struct  t_bind *bound2;
	struct  t_bind *bind2;
	int		bound		= FALSE;

	if ((fd = t_open(netconfigp->nc_device,  O_RDWR, NULL)) < 0)
	{
		syslog(LOG_DEBUG,"BindAndListen():t_open: %s",netconfigp->nc_device);
		return(-1);
	}
	if ((bind2 = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR)) == NULL )
	{
		tli_error("BindAndListen():t_alloc ",TO_SYSLOG,LOG_DEBUG);
		return (-1);
	}
	if ((bound2 = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR)) == NULL )
	{
		tli_error("BindAndListen():t_alloc ",TO_SYSLOG,LOG_DEBUG);
		return (-1);
	}

	bind2->qlen=1;
	/*
	 * addr.len must not be zero. Otherwise kernel may set qlen to 0
	 */
	bind2->addr.len = bind2->addr.maxlen;
	if(t_bind(fd,bind2,bound2) < 0 ) 
	{
		tli_error("BindAndListen():t_bind ",TO_SYSLOG,LOG_DEBUG);
		(void) t_free( (char *)bind2, T_BIND);
		(void) t_free( (char *)bound2, T_BIND);
		t_close(fd);
		return (-1);
	}
	/*
	 *  Get the port number we were bound to.
	 */
	if ( get_port_number(bound2,netconfigp,port) == 0 )
	{
		bound = TRUE;
	}
	(void) t_free( (char *)bound2, T_BIND);
	(void) t_free( (char *)bind2, T_BIND);
	if ( bound == TRUE )
	{
		return(fd);
	}
	return(-1);
}
/*-----------------------------------------------------------------------*/
/*  Routine: get_port_number
/*
/*  Purpose: Get the port number the TLI transport actually bound us
/*			 to.
/*
/*  Entry:	 bound2		pointer to a t_bind structure containing the
/*						assigned port number.
/*			 netconfigp pointer to the transport information.
/*			 port		pointer for port number
/*
/*	Exit:	 0		port number valid
/*			 -1		unable to determine port number
/*	
/*-----------------------------------------------------------------------*/
int
get_port_number(bound2,netconfigp,port)
	struct  t_bind *bound2;
    struct	netconfig	*netconfigp;
	int * port;
{

	unsigned short *t;

	if (netconfigp == NULL)
	{
		syslog(LOG_DEBUG,"get_port_number(): netconfigp == NULL");
		return(-1);
	}

	if (strcmp(netconfigp->nc_proto, "spx") == 0)
		t = (unsigned short *)
			&(((struct ipxAddress *)bound2->addr.buf)->sock[0]);
	else if (strcmp(netconfigp->nc_proto, NC_TCP) == 0)
		t = (unsigned short *)
			&(((struct sockaddr_in *)bound2->addr.buf)->sin_port);
	else
	{
		syslog(LOG_DEBUG,"get_port_number(): ! (spx || tcp)");
		return(-1);
	}

	*port = ntohs(*t);
	return(0);
}
/*-----------------------------------------------------------------------*/
/*  Routine: verify_bound2
/*
/*  Purpose: Check that the port the TLI transport actually bound us
/*			 to is the same as the requested port number.
/*
/*  Entry:	 bind2 		pointer to a t_bind structure containing the 
/*						requested bind address and port
/*			 bound2		pointer to a t_bind structure containing the
/*						assigned port number.
/*			 netconfigp pointer to the transport information.
/*
/*	Exit:	 0		alls well, assigned equals requested port
/*			 -1		miscompare on port numbers
/*	
/*-----------------------------------------------------------------------*/
int
verify_bound2(bind2,bound2,netconfigp)
	struct  t_bind *bind2;
	struct  t_bind *bound2;
    struct	netconfig	*netconfigp;
{
	if (bind2->addr.len && bound2->addr.len)
	{
		if (bind2->addr.len == bound2->addr.len)
		{
			if(memcmp(bind2->addr.buf, bound2->addr.buf, bind2->addr.len) == 0 )
				return(0);
			else
			{
				char 	*received_uaddr;
				char 	*requested_uaddr;

				received_uaddr = taddr2uaddr(netconfigp,&(bound2->addr));
				requested_uaddr = taddr2uaddr(netconfigp,&(bind2->addr));
				if (requested_uaddr == NULL || received_uaddr == NULL )
				{
					syslog(LOG_DEBUG,"verify_bound2():uaddr == NULL");
					return(-1);
				}
				syslog(LOG_DEBUG, "verify_bound2()");
				syslog(LOG_DEBUG, "       bound2 =%s",received_uaddr);
				syslog(LOG_DEBUG, "       bind2  =%s",requested_uaddr);
				free(requested_uaddr);
				free(received_uaddr);
				return(-1);
			}
		}
	}
	syslog(LOG_DEBUG,"verify_bound2(): bound2->addr.len || bind2->addr.len");
	syslog(LOG_DEBUG,"       bound2->addr.len = %d",bound2->addr.len);
	syslog(LOG_DEBUG,"       bind2->addr.len = %d",bind2->addr.len);
	return(-1);
}
/*-----------------------------------------------------------------------*/
/*  Routine: releaseNcloseTransport
/*
/*  Purpose: Release and close the transport. Sends either t_sndrel if the 
/*			 provider supports orderly otherwise sends t_snddis.
/*
/*  Entry:	 fd 		valid file descriptor for a tli transport
/*			 dir		TO_STDERR = error messages to stderr
/*						TO_SYSLOG = error messages to syslog
/*			 flags		contains priority flags for syslog
/*
/*	Exit:	  -1		failed ( error sent to stdout or syslog )
/*	
/*-----------------------------------------------------------------------*/
int
releaseNcloseTransport(int fd,int dir,int flags)
{
	struct	t_info	info;
	int 	result = 0;

	/*
	 * Get the transport information so we can determine the 
	 * correct release command.
	 */
	if ( t_getinfo(fd,&info) < 0 )
	{
		tli_error("releaesNcloseTransport():",dir,flags);
		/*
		 * Couldn't get the info, assume disorderly and procede.
		 */
		info.servtype = T_COTS;
		result = -1;
	}
	/*
	 * Release the connection.  Send t_sndrel for T_COTS_ORD. All
	 * others, only T_COTS for now, send t_snddis.
	 */
	switch ( info.servtype )
	{
		case T_COTS_ORD:			/* Orderly */
			if ( t_sndrel(fd) < 0 )
			{
				tli_error("releaesNcloseTransport():",dir,flags);
				result = -1;
			}
			break;
		case T_COTS:				/* Disorderly  FALL THRU */
		default:
			sleep(1);
			if ( t_snddis(fd,NULL) < 0 )
			{
				tli_error("releaesNcloseTransport():",dir,flags);
				result = -1;
			}
			break;
	}
	if ( t_unbind(fd) < 0 )
	{
		tli_error("releaesNcloseTransport():",dir,flags);
		result = -1;
	}
	if ( t_close(fd) < 0 )
	{
		tli_error("releaesNcloseTransport():",dir,flags);
		result = -1;
	}
	return(result);
}
/*-----------------------------------------------------------------------*/
/*  Routine: tli_error
/*
/*  Purpose: Send the system generated tli error message to the
/*			 syslog or stderr.  Deamons would normally go to syslog
/*			 while clients would send to stderr.
/*
/*  Entry:	 msg 		routine identifier string pointer.
/*			 dir		TO_STDERR = error messages to stderr
/*						TO_SYSLOG = error messages to syslog
/*			 flags		contains priority flags for syslog
/*
/*	Exit:	 
/*	
/*-----------------------------------------------------------------------*/
void
tli_error(char *msg,int dir,int flags)
{
	extern int	t_nerr;
	extern char	*t_errlist[];

	if (dir == TO_STDERR )
	{
		t_error(msg);
	}
	else if ( dir == TO_SYSLOG )
	{
		if ( t_errno > 0 && t_errno < t_nerr )
			syslog(flags,"%s %s",msg,t_errlist[t_errno]);
		else
			syslog(flags,"%s t_errno = %d",msg,t_errno);
	}
}
	

/*-----------------------------------------------------------------------*/
/* Berkley support routines
/*-----------------------------------------------------------------------*/
void
bzero (b, length)
char *b;
int length;
{
	memset(b,0,(size_t)length);
}
void
bcopy (b1, b2, length)
char *b1, *b2;
int length;
{
#ifdef EASY 
	memcpy(b2,b1,length);
#else
	if ( b1 < b2 ) {
		b2 += length;
		b1 += length;
		while ( length-- )
			*--b2 = *--b1;
	} else {
		while (length-- )
			*b2++ = *b1++;
	}
#endif
}

/*-----------------------------------------------------------------------*/
/*  Routine:  shutdown
/*
/*  Purpose:  Emulate the socket shutdown function using TLI functions
/*
/*  Entry:    fd  	file descriptor
/*			  how	ignored
/*
/*  Exit:     
/*-----------------------------------------------------------------------*/
int
shutdown(int fd,int how)
{
	t_unbind(fd);
	t_close(fd);
	return(0);
}
/*-----------------------------------------------------------------------*/
/*  Routine:  sigblock
/*
/*  Purpose:  Emulate the Berkley sigblock function using SVR4 functions
/*
/*  Restrictions:   Assumes signals values 1 - 31
/*
/*  Entry:    mask  	int containing masks to be added ( see sigmask )
/*
/*  Exit:     retmask   int containing the previous set of mask values
/*-----------------------------------------------------------------------*/
int
sigblock (unsigned int mask )
{
	sigset_t 	cset; 			/* contains current set of blocked signals */
    int maskval;
    unsigned int i,x;
    unsigned int	retmask = 0;

    
	sigprocmask(0,NULL,&cset);	/*-- get the current set of signals */
	/*
	 *	Create a mask of the current set of blocked
	 *	signals.  We need to return it.
	 */
	for(i=1;i <= (sizeof(int) * 8) - 1 ;i++ )
	{
		x = 1;
		if ( sigismember(&cset,i))
		{
			retmask |= x << (i - 1);
		}
	}
	/*
	 *	Add the specified masks to the current set
	 */
	i = 1;
	while( mask )
	{
		if ( x = i & mask )
		{
            maskval = 0;
            while(x)
            {
				maskval += 1;
                x >>= 1;
            }
			sigaddset(&cset,maskval);
        }
        mask &= ~i;
        i <<= 1;
    }
	/*
	 *	Install the modified mask set.
	 */
    sigprocmask(SIG_BLOCK,&cset,NULL);
    return(retmask);
}
/*-----------------------------------------------------------------------*/
/*  Routine:  sigsetmask
/*
/*  Purpose:  Emulate the Berkley sigsetmask function using SVR4 functions
/*
/*  Restrictions:   Assumes signals values 1 - 31
/*
/*  Entry:    mask  	int containing masks to be set ( see sigmask )
/*
/*  Exit:     retmask   int containing the previous set of mask values
/*-----------------------------------------------------------------------*/

sigsetmask (unsigned int mask )
{
	
	sigset_t 	cset; 			/* contains current set of blocked signals */
    int i,maskval;
    unsigned int x;
    unsigned int	retmask = 0;

    
	sigprocmask(0,NULL,&cset);	/*-- get the current set of signals */
	/*
	 *	Create a mask of the current set of blocked
	 *	signals.  We need to return it.
	 */
	for(i=1;i <= (sizeof(int) * 8) - 1 ;i++ )
	{
		x = 1;
		if ( sigismember(&cset,i))
		{
			retmask |= x << (i - 1);
		}
	}
	/*
	 *	Delete all current masks and set the specified mask bits
	 */
	sigemptyset(&cset);
	i = 1;
	while( mask )
	{
		if ( x = i & mask )
		{
            maskval = 0;
            while(x)
            {
				maskval += 1;
                x >>= 1;
            }
			sigaddset(&cset,maskval);
        }
        mask &= ~i;
        i <<= 1;
    }
	/*
	 *	Install the modified mask set.
	 */
    sigprocmask(SIG_SETMASK,&cset,NULL);
    return(retmask);
}
sigvec(signo, sa, osa)
int signo;
struct sigaction *sa;
struct sigaction *osa; 
{
	sigaction(signo,sa,osa);
	return(0);
}

/*-----------------------------------------------------------------------*/
/*  Routine: tstNsetsig
/*
/*  Purpose: Test whether or not the argumented signal number 
/*			 has it's handler set to the argumented handler, sigstate. 
/*			 If not then set it to the new handler.       
/*
/*  Entry:	 signo		signal number to test handler of.
/*			 sigstate	Handler to test for.
/*			 handler   	Handler to set to.
/*
/*	Exit: 	 EXIT if not able to acquire current signal actions.
/*-----------------------------------------------------------------------*/
void
tstNsetsig(int signo,void (*sigstate)(), void (*handler)())
{
	struct sigaction sa;

	if ( sigaction(signo,NULL,&sa) == -1 )
	{	
		syslog(LOG_DEBUG,"tstNsetsig():errno = %d",errno);
		exit(1);
	}
	if ( sa.sa_handler != sigstate)
		(void)sigset(signo, handler);
}

/*-----------------------------------------------------------------------*/
/*
/*  The next two routines, create_netbuf_arg and decode_netbuf_arg
/*  are used to pass TLI connection information between programs
/*  during an exec.  Currently nwnetd and nrexec.  The connection 
/*  information is recieved by nwnetd during connection establishment
/*  ( t_accept).  The execed program , nrexec, needs this information 
/*  to do peer validation, what port number the connection came in on
/*  and who it is from.
/*
/*  create_netbuf_arg is called by the child of nwnetd.
/*  decode_netbuf_arg is called by the execed program. 
/*
/*  I could't find a way to get this information from the transport
/*  endpoint in the execed program.  Need something like getpeername(). 
/*  
/*-----------------------------------------------------------------------*/
/*  Routine: create_netbuf_arg
/*
/*  Purpose: Create universal address from a netbuf structure.
/*
/*  Entry:	 argv	array of char pointers to store string pointers in.
/*			 argc	index into first empty slot in argv array
/*			 netbufp	pointer to netbuf structure to be converted
/*			 netconfigp	so we do transport specific encoding
/*
/*	Exit:	 0		alls well
/*			 -1		malloc of string storage failed
/*-----------------------------------------------------------------------*/
int
create_netbuf_arg(char *argv[],int argc,struct netbuf *netbufp, \
    				struct	netconfig	*netconfigp)
{
	char	*cp;			/* 	working char pointer */

	cp  = taddr2uaddr(netconfigp,netbufp);
	if ( cp == NULL )
	{
		syslog(LOG_DEBUG,"create_netbuf_arg():cp == NULL");
		return(-1);
	}
	if ((argv[argc] = (char *)malloc(strlen(cp) +1)) == NULL)
	{
		free(cp);
		return(-1);
	}
	strcpy(argv[argc],cp);
	free(cp);
	return(0);
}
/*-----------------------------------------------------------------------*/
/*  Routine: decode_netbuf_arg
/*
/*  Purpose: Decode universal address into a netbuf 
/*			 structure elements.
/*
/*  Entry:	 argv	array of char pointers to get string pointers from.
/*					Strings must be upper case only.
/*			 argc	index into first element in argv array
/*			 netbufp	pointer to netbuf structure to be filled
/*			 proto		transport provider net_id
/*
/*	Exit:	 0		alls well
/*			 -1		malloc of address storage space failed
/*-----------------------------------------------------------------------*/
int
decode_netbuf_arg(char *argv[], int argc,struct netbuf *netbufp,char * proto)
{
	struct	netconfig *netconfigp = NULL;
	struct netbuf *tnetbufp;

	if ((netconfigp = getnetconfigent(proto)) == NULL )
	{
		syslog(LOG_DEBUG,"decode_netbuf_arg():netconfigp == NULL");
		syslog(LOG_DEBUG,"                   :proto %s",proto);
		return(-1);
	}
	if (( tnetbufp = uaddr2taddr(netconfigp,argv[argc])) == NULL )
	{
		syslog(LOG_DEBUG,"decode_netbuf_arg():tnetbufp == NULL");
		freenetconfigent(netconfigp);
		return(-1);
	}
	freenetconfigent(netconfigp);
	netbufp->len  = tnetbufp->len;
	netbufp->maxlen  = tnetbufp->maxlen;
	
	if (( netbufp->buf =  (char *)malloc(netbufp->maxlen)) == 0 )
	{
		syslog(LOG_DEBUG,"decode_netbuf_arg():netbufp->buf == NULL");
		return(-1);
	}
	/*
	 *	Load the address into netbufp->buf.
	 */
	memcpy(netbufp->buf,tnetbufp->buf,netbufp->maxlen);
	free(tnetbufp);
	return(0);
}
#if 0
/* static in rexec.c because new sap library also defines this functions */
/*-----------------------------------------------------------------------*/
/*  Routine: strcmpi
/*
/*  Purpose: Case insensitive string compare 
/*
/*-----------------------------------------------------------------------*/
strcmpi(const char *string1, const char *string2)
{
	do
	{
		if ( 	(*string1 != (*string2)) &&
				(*string1 != toupper(*string2)) &&
				(*string2 != toupper(*string1)))
		{
			return(*string1 - *string2);
		}
		string2++;
	}
	while(*string1++ != '\0');
	return(0);
}
#endif
