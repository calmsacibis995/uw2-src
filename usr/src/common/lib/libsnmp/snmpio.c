/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsnmp:snmpio.c	1.8"
#ident   "$Header: /SRCS/esmp/usr/src/nw/lib/libsnmp/snmpio.c,v 1.11 1994/09/15 21:50:32 cyang Exp $"
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991, 1992,    *
 *                 1993, 1994  Novell, Inc. All Rights Reserved.            *
 *                                                                          *
 ****************************************************************************
 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.       *
 *      The copyright notice above does not evidence any                *
 *      actual or intended publication of such source code.                 *
 ****************************************************************************/

#ifndef lint
static char TCPID[] = "@(#)snmpio.c 1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)snmpio.c   6.3 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/*
 *   snmpio.c : contains routines which are commonly called by SNMP
 *    applications for network input and output.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "snmp.h"
#include "snmpio.h"

/* initialize_io does initialization and opens udp connection */
void
initialize_io(char *program_name, char *name)
   {

#if defined(BSD) || defined(SVR3) || defined(SVR4)
   struct servent *SimpleServ;
   u_short snmp_port;
#endif

   /*  first, copy program name to the save area for use with error messages */
   strncpy(imagename, program_name, Min(sizeof(imagename) - 1,
            strlen(program_name)));

   /* make sure terminated properly in case it was long */
   imagename[sizeof(imagename) - 1] = '\0';

#ifdef BSD
   /*  set up timer for timeout */
   (void)signal(SIGALRM, time_out);

   /*  now, set up UDP connection */
   if((SimpleServ = getservbyname("snmp", "udp")) == NULL) 
      {
      LIB_ERROR1("%s:  Add snmp 161/udp  to /etc/services.\n", imagename);
      exit(-1);
      }

   snmp_port = SimpleServ->s_port;

   if((fd = socket(AF_INET, SOCK_DGRAM, 0, 0)) < 0) 
      {
      LIB_ERROR1("%s:  unable to connect to socket.\n", imagename);
      exit(-1);
      }

   sin.sin_addr.s_addr = inet_addr(name);

   if(sin.sin_addr.s_addr == -1) 
      {
      hp = gethostbyname(name);
      if(hp)
         bcopy(hp->h_addr, &sin.sin_addr, hp->h_length);
      else 
         {
         LIB_ERROR1("%s:  host unknown.\n", name);
         exit(-1);
         }
      }

   sin.sin_family = AF_INET;
   sin.sin_port = snmp_port;
#endif

#if defined(SVR3) || defined(SVR4)
   /*  now, set up UDP connection */
   if((SimpleServ = getservbyname("snmp", "udp")) == NULL) 
      {
      LIB_ERROR1("%s:  Add snmp 161/udp  to /etc/services.\n", imagename);
      exit(-1);
      }

   snmp_port = SimpleServ->s_port;

   if((fd = t_open(_PATH_UDP, O_RDWR, (struct t_info *)0)) < 0) 
      {
      LIB_ERROR1("%s: Unable to connect to transport endpoint.\n", imagename);
      exit(-1);
      }

   if(t_bind(fd, (struct t_bind *)0, (struct t_bind *)0) < 0) 
      {
      LIB_ERROR1("%s: Unable to bind transport endpoint.\n", imagename);
      exit(-1);
      }

   sin.sin_addr.s_addr = inet_addr(name);

   if(sin.sin_addr.s_addr == -1) 
      {
      hp = gethostbyname(name);
      if(hp)
         memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
      else 
         {
         LIB_ERROR1("%s:  host unknown.\n", name);
         exit(-1);
         }
      }

   sin.sin_family = htons(AF_INET);
   sin.sin_port = snmp_port;
#endif

   /* attach to the in.snmpd shared memory */
   {
     int shmid;
     shmid=shmget(SNMPD_SHMKEY, sizeof(snmpstat_type),0);
     snmpstat=(snmpstat_type *)shmat(shmid,0,0);
     if (snmpstat==(snmpstat_type *)(-1))
       snmpstat=(snmpstat_type *)malloc(sizeof(snmpstat_type));
   }
 }
int send_request(int socket, AuthHeader *auth_pointer)
   {
#ifdef BSD
   long packet_len;
   unsigned char out_pkt[2048];

   packet_len = auth_pointer->packlet->length;
   bcopy(auth_pointer->packlet->octet_ptr, out_pkt, packet_len);

   /* for debug
    * print_packet_out(auth_pointer->packlet->octet_ptr, packet_len);
    */

   if(sendto(socket, out_pkt, packet_len, 0, &sin, sizeof(sin)) < 0) 
      {
      LIB_ERROR1("%s: Send.\n", imagename);
      return(FALSE);
      }

   snmpstat->outpkts++;

   return(TRUE);
#endif

#ifdef SVR4
   struct t_unitdata unitdata;

   unitdata.addr.buf = (char *)&sin;
   unitdata.addr.len = sizeof(sin);
   unitdata.opt.len = 0;
   unitdata.udata.buf = (char *)auth_pointer->packlet->octet_ptr;
   unitdata.udata.len = auth_pointer->packlet->length;

   /* for debug
    * print_packet_out(auth_pointer->packlet->octet_ptr, packet_len);
    */

   if(t_sndudata(socket, &unitdata) < 0) 
      {
      LIB_ERROR1("%s:  send.\n", imagename);
      return(FALSE);
      }

   snmpstat->outpkts++;

   return(TRUE);
#endif
   }

int get_response(int seconds)
{
#ifdef BSD
  int fromlen = sizeof(from);
#endif

#ifdef SVR4
  int timeout;
  struct t_unitdata unitdata;
  int flags;
  extern int t_errno;
  struct pollfd mypollfd;
#endif

#ifdef BSD
   alarm(seconds);
   packet_len = (long)recvfrom(fd, packet, sizeof(packet), 0, &from, &fromlen);
   alarm(0);

   if(packet_len <= 0)
      return(TIMEOUT);

   snmpstat->inpkts++;

   return(RECEIVED);
#endif

#ifdef SVR4
  mypollfd.fd=fd;
  mypollfd.events=POLLIN;
  mypollfd.revents=0;
  
  timeout = seconds * 1000;

  if ((poll(&mypollfd, 1, timeout))==-1)
    {
      return (ERROR);
    }
  if (mypollfd.revents==POLLIN)
    {
      unitdata.addr.buf = (char *)&from;
      unitdata.addr.maxlen = sizeof(from);
      unitdata.opt.maxlen = 0;
      unitdata.udata.buf = (char *)packet;
      unitdata.udata.maxlen = sizeof(packet);
      
      packet_len = t_rcvudata(fd, &unitdata, &flags);
      
      packet_len = unitdata.udata.len;
      snmpstat->inpkts++;

      return(RECEIVED);
    }
  else
    return(TIMEOUT);
#endif
}

void close_up(int fd)
   {
#ifdef BSD
   close(fd);
#endif

#ifdef SVR4
   t_close(fd);
#endif
   }

/*
 *  Handle timing out
 */
#ifdef SVR4
void time_out(int signo)
#else
int time_out()
#endif
   {
#ifndef SVR4
   return(0);
#endif
   }

long make_req_id(void)
   {
   long ltime;

   time(&ltime);

   return(long)(ltime & 0x7fff);
   }
