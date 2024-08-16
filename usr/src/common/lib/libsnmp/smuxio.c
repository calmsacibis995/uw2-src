/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsnmp:smuxio.c	1.5"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libsnmp/smuxio.c,v 1.7 1994/08/02 23:36:54 cyang Exp $"
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
static char TCPID[] = "@(#)smuxio.c 1.2 STREAMWare TCP/IP SVR4.2 source";
#endif /* lint */
#ifndef lint
static char SNMPID[] = "@(#)smuxio.c   6.1 INTERACTIVE SNMP source";
#endif /* lint */
/*      SCCS IDENTIFICATION        */
/* smuxio.c - I/O abstractions */

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
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "snmp.h"
#include "snmpio.h"
#ifdef SVR4
#include <sys/stream.h>
#include <sys/tiuser.h>
extern int t_errno;
#endif

#ifdef SVR4
#define block_signals()   sighold(SIGHUP)
#define unblock_signals() sigrelse(SIGHUP)
#endif


#define OK      0
#define NOTOK     -1
#define MAXCONN       5
#define SMUX_PORT  199
#define BUFSIZE       2048

extern int errno;

/* prototypes */
int block_fd(int fd);
int unblock_fd(int fd);

int start_smux_server(void)
   {
   int fd;
   struct sockaddr_in sock;
   struct sockaddr_in *sin = &sock;
   struct servent *SimpleServ;
#ifdef SVR4
   struct t_bind *req, *ret;
#endif

   /* start up the smux service */
   if((SimpleServ = getservbyname("smux", "tcp")) == NULL) 
      {
      LIB_ERROR("smux: add smux 199/tcp to /etc/services.\n");
      }

   bzero((char *)sin, sizeof(*sin));
   sin->sin_family = AF_INET;
   sin->sin_port = SimpleServ ? SimpleServ->s_port : htons(SMUX_PORT);

#ifdef BSD
   if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
      {
      LIB_PERROR("smux: socket");
      return(NOTOK);
      }

   if(bind(fd, sin, sizeof(*sin)) < 0) 
      {
      LIB_PERROR("smux: socket");
      close(fd);
      return(NOTOK);
      }
#endif

#ifdef SVR4
#ifdef USING_TCP_LOOPBACK
   if((fd = t_open(_PATH_TCP, O_RDWR | O_NDELAY, (struct t_info *)0)) < 0) 
      {
#else
   if((fd = t_open(_PATH_COTS, O_RDWR | O_NDELAY, (struct t_info *)0)) < 0) 
      {
#endif

      t_error("smux: t_open");
      return(NOTOK);
      }

   if((req = (struct t_bind *)t_alloc(fd, T_BIND, 0)) == NULL) 
      {
      t_error("smux: t_alloc");
      t_close(fd);
      return(NOTOK);
      }

   if((ret = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR)) == NULL) 
      {
      t_error("smux:t_alloc");
      t_close(fd);
      return(NOTOK);
      }

   req->addr.buf = (char *)sin;
   req->addr.len = sizeof(*sin);
   req->qlen = MAXCONN;

   if(t_bind(fd, req, ret) < 0) 
      {
      t_error("smux:t_bind");
      req->addr.buf = (char *)0;
      t_free((char *)req, T_BIND);
      t_free((char *)ret, T_BIND);
      t_close(fd);
      return(NOTOK);
      }

   if(bcmp(req->addr.buf, ret->addr.buf, req->addr.len) != 0) 
      {
      LIB_ERROR("smux: Couldn't bind to the requested address");
      req->addr.buf = (char *)0;
      t_free((char *)req, T_BIND);
      t_free((char *)ret, T_BIND);
      t_close(fd);
      return(NOTOK);
      }

   t_free((char *)req, T_BIND);
   t_free((char *)ret, T_BIND);
#endif

   return(fd);
   }

int start_tcp_client(struct sockaddr_in *sock)
   {
   int fd, i;

#ifdef BSD
   if((fd = socket(AF_INET, SOCK_STREAM, 0)) == NOTOK) 
      {
      LIB_PERROR("smux: start_tcp_client: socket");
      return(NOTOK);
      }

   for(i = 0; i < 5; i++) 
      {
      if(bind(fd, (struct sockaddr_in *)sock, sizeof *sock) != NOTOK)
         break;

      switch(errno) 
         {
         case EADDRINUSE:
            continue;

         case EADDRNOTAVAIL:

         default:
            LIB_PERROR("smux: start_tcp_client: bind");
            (void)close_up(fd);
         return(fd = NOTOK);
         }
      }
#endif

#ifdef SVR4
#ifdef USING_TCP_LOOPBACK
   if((fd = t_open(_PATH_TCP, O_RDWR, (struct t_info *)0)) < 0) 
      {
#else
   if((fd = t_open(_PATH_COTS, O_RDWR, (struct t_info *)0)) < 0) 
      {
#endif

      t_error("smux: start_tcp_client: t_open");
      return(NOTOK);
      }

   if(t_bind(fd, (struct t_bind *)0, (struct t_bind *)0) < 0) 
      {
      (void)close_up(fd);
      t_error("smux: start_tcp_client: t_bind");
      return(fd = NOTOK);
      }
#endif

   return(fd);
   }

#ifdef BSD
int join_tcp_client(int fd, struct sockaddr_in *sock)
   {
   int new_fd;

   if((new_fd = accept(fd, (struct sockaddr_in *)sock, &len)) == NOTOK) 
      {
      LIB_PERROR("smux: accept");
      return(NOTOK);
      }

   return(new_fd);
   }

int join_tcp_server(int fd, struct sockaddr_in *sock)
   {
   int error_index;
   int result;

   if((result = connect(fd, (struct sockaddr_in *)sock, sizeof *sock)) == NOTOK) 
      {
      error_index = errno;
      LIB_PERROR("smux: join_tcp_server: connect");
      errno = error_index;
      }

   return(result);
   }
#endif   /* BSD */

#ifdef SVR4
int join_tcp_client(int fd, struct sockaddr_in *sock)
   {
   int new_fd;
   struct t_call *callptr;

#ifdef USING_TCP_LOOPBACK
   if((new_fd = t_open(_PATH_TCP, O_RDWR, (struct t_info *)0)) < 0) 
      {
#else
   if((new_fd = t_open(_PATH_COTS, O_RDWR, (struct t_info *)0)) < 0) 
      {
#endif

      t_error("smux: t_open");
      return(NOTOK);
      }

   if(t_bind(new_fd, (struct t_bind *)0, (struct t_bind *)0) < 0) 
      {
      t_error("smux: t_bind");
      t_close(new_fd);
      return(NOTOK);
      }

   if((callptr = (struct t_call *)t_alloc(fd, T_CALL, T_ADDR)) == NULL) 
      {
      t_error("smux: t_alloc");
      t_close(new_fd);
      return(NOTOK);
      }

   if(t_listen(fd, callptr) < 0) 
      {
      t_error("smux: t_listen");
      t_free((char *)callptr, T_CALL);
      t_close(new_fd);
      return(NOTOK);
      }

   if(t_accept(fd, new_fd, callptr) < 0) 
      {
      if(t_errno == TLOOK) 
         {
         if(t_rcvdis(fd, (struct t_discon *)0) < 0) 
            {
            t_error("smux: t_listen");
            t_free((char *)callptr, T_CALL);
            t_close(new_fd);
            return(NOTOK);
            }

         t_free((char *)callptr, T_CALL);
         t_close(new_fd);
         return(NOTOK);
         }

      t_error("smux: t_accept");
      t_close(new_fd);
      t_free((char *)callptr, T_CALL);
      return(NOTOK);
      }

   /* fill in the sock structure with the peer's address */
   bcopy((char *)callptr->addr.buf, (char *)sock,
         callptr->addr.len);
   t_free((char *)callptr, T_CALL);
   return(new_fd);
   }

int join_tcp_server(int fd, struct sockaddr_in *sock, int sent_connect)
   {
   int error_index, result;
   struct t_call *callptr;

   if(!sent_connect) 
      {
      if((callptr = (struct t_call *)t_alloc(fd, T_CALL, 0)) == NULL) 
         {
         t_error("smux: join_tcp_server: t_alloc");
         return(NOTOK);
         }

      callptr->addr.maxlen = sizeof(*sock);
      callptr->addr.len = sizeof(*sock);
      callptr->addr.buf = (char *)sock;
/*
      bcopy((char *)sock, (char *)callptr->addr.buf, callptr->addr.len);   
*/
      callptr->opt.len = 0;
      callptr->udata.len = 0;

      if((result = t_connect(fd, callptr, (struct t_call *)0)) < 0) 
         {
         callptr->addr.buf = NULL;
         t_free((char *)callptr, T_CALL);
         error_index = t_errno;
         t_error("smux: join_tcp_server: t_connect");
         t_errno = error_index;
         return(NOTOK);
         }

      callptr->addr.buf = NULL;
      t_free((char *)callptr, T_CALL);
      } 
   else if((result = t_rcvconnect(fd, (struct t_call *)0)) < 0) 
      {
      error_index = t_errno;
      t_error("smux: join_tcp_server: t_rcvconnect");
      t_errno = error_index;
      return(NOTOK);
      }

   return(result);
   }
#endif   /* SVR4 */

int recv_tcp_packet(int fd, char *packet)
   {
   long recv_so_far;
   int flags;

#ifdef BSD
   if((recv_so_far = recv(fd, packet, BUFSIZE, flags)) < 0) 
      {
      LIB_PERROR("smux: recv_tcp_socket: recv");
      return(NOTOK);
      }
#endif

#ifdef SVR4
   if((recv_so_far = t_rcv(fd, packet, BUFSIZE, &flags)) < 0) 
      {
      t_error("smux: recv_tcp_socket: t_rcv");
      return(NOTOK);
      }
#endif

   return(recv_so_far);
   }

int send_tcp_packet(int fd, unsigned char *packet, unsigned int length)
   {
   int sent_so_far;

#ifdef BSD
   if((sent_so_far = send(fd, packet, length, 0)) < 0) 
      {
      LIB_PERROR("smux: send_tcp_packet: send");
      return(NOTOK);
      }
#endif

#ifdef SVR4
   if((sent_so_far = t_snd(fd, (char *)packet, length, 0)) < 0) 
      {
      t_error("smux: send_tcp_packet: t_snd");
      return(NOTOK);
      }
#endif

   return(sent_so_far);
   }

int dispatch_smux_packet(int fd, unsigned char *packet, long amount)
   {
   long data_sent = 0;
   long remain = amount;
   unsigned char *curr_pkt_ptr;

   block_signals();
   if(block_fd(fd) < 0)
      return(NOTOK);

   curr_pkt_ptr = packet;
   while(remain > 0) 
      {
      if((data_sent = send_tcp_packet(fd, curr_pkt_ptr, remain)) < 0)
         return(NOTOK);

      remain -= data_sent;
      curr_pkt_ptr += data_sent;
      }

   if(unblock_fd(fd) < 0)
      return(NOTOK);

   unblock_signals();

   return(OK);
   }

int fetch_smux_packet(int fd, char *rem_data, int *rem_len, 
		      unsigned char *packet, long *amount)
   {
   unsigned char *work_ptr, *end_ptr;
   char temp_buf[BUFSIZE];
   int i, new_avail = 0;
   int got_one = 0;
   int lenlen, length = -1;

   block_signals();
   if(block_fd(fd) < 0)
      return(NOTOK);

   if(*rem_len == 0) 
      {
      if((*rem_len = recv_tcp_packet(fd, temp_buf)) < 0) 
         {
         LIB_ERROR("recv_tcp_packet failed -1 \n");
         return(NOTOK);
         }

      for(i = 0; i < *rem_len; i++)
         rem_data[i] = temp_buf[i];

      rem_data[i] = '\0';
      }

   while(!got_one) 
      {
      work_ptr = (unsigned char *)rem_data;
      end_ptr = work_ptr + *rem_len;
      work_ptr++;        /* Bypass the pdu_type octet */

      if(*rem_len >= 3)
         length = parse_length(&work_ptr, end_ptr);

      if(length > 0) 
         {
         /* because packet length contains length and pdu_type */
         lenlen = dolenlen(length);
         length = length + lenlen + 1;
         }

      if((length > 0) && (length <= *rem_len)) 
         {
         for(i = 0; i < length; i++)
            *packet++ = rem_data[i];

         *amount = length;
         got_one = 1;

         for(i = 0; i < (*rem_len - length); i++)
            temp_buf[i] = rem_data[length + i];

         *rem_len = *rem_len - length;

         for(i = 0; i < *rem_len; i++)
            rem_data[i] = temp_buf[i];

         rem_data[i] = '\0';
         } 
      else 
         {
         if((new_avail = recv_tcp_packet(fd, temp_buf)) < 0) 
            {
            LIB_ERROR("recv_tcp_packet failed -2 \n");
            return(NOTOK);
            }

         for(i = 0; i < new_avail; i++)
            rem_data[i + *rem_len] = temp_buf[i];

         rem_data[i + *rem_len] = '\0';
         *rem_len = *rem_len + new_avail;
         }
      }

   if(unblock_fd(fd) < 0)
      return(NOTOK);

   unblock_signals();

   return(OK);
   }

int block_fd(int fd)
   {
   int flags, junk;

   if((flags = fcntl(fd, F_GETFL, &junk)) < 0)
      return(NOTOK);

#ifdef BSD
   flags &= ~FNDELAY;
#elif SVR4
   flags &= ~O_NDELAY;
#endif

   if(fcntl(fd, F_SETFL, flags) < 0)
      return(NOTOK);

   return(fd);
   }

int unblock_fd(int fd)
   {
   int flags, junk;

   if((flags = fcntl(fd, F_GETFL, &junk)) < 0)
      return(NOTOK);

#ifdef BSD
   flags |= FNDELAY;
#elif SVR4
   flags |= O_NDELAY;
#endif

   if(fcntl(fd, F_SETFL, flags) < 0)
      return(NOTOK);

   return(fd);
   }

#ifdef BSD
#include <sys/time.h>

int selsocket(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, int secs)
   {
   int n;
   fd_set ifds, ofds, xfds;
   struct timeval tvs;
   register struct timeval *tv = &tvs;

   if(secs != NOTOK)
      tv->tv_sec = secs, tv->tv_usec = 0;
   else
      tv = NULL;

   if(rfds)
      ifds = *rfds;

   if(wfds)
      ofds = *wfds;

   if(efds)
      xfds = *efds;

   for(;;) 
      {
      switch(n = select(nfds, rfds, wfds, efds, tv)) 
         {
         case OK:
            if(secs == NOTOK)
               break;
         return OK;

         case NOTOK:
            if(errno == EINTR && secs == NOTOK)
               continue;
         /* else fall... */

         default:
         return n;
         }

      if(rfds)
         *rfds = ifds;

      if(wfds)
         *wfds = ofds;

      if(efds)
         *efds = xfds;
      }
   }
#endif   /* BSD */

#ifdef SVR4
#include <poll.h>

int selsocket(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, int secs)
   {
   int i, j, n;
   struct pollfd pollfds[128];

   for(i = j = 0; i < nfds; i++) 
      {
      pollfds[j].fd = NOTOK;
      pollfds[j].events = 0;

      if(rfds && FD_ISSET(i, rfds)) 
         {
         pollfds[j].fd = i;
         pollfds[j].events |= POLLIN | POLLPRI;
         }

      if(wfds && FD_ISSET(i, wfds)) 
         {
         pollfds[j].fd = i;
         pollfds[j].events |= POLLOUT;
         }

      if(efds && FD_ISSET(i, efds)) 
         {
         pollfds[j].fd = i;
         /* one always gets notified of exceptions */
         }

      if(pollfds[j].fd == i)
         j++;
      }

   if(rfds)
      FD_ZERO(rfds);

   if(wfds)
      FD_ZERO(wfds);

   if(efds)
      FD_ZERO(efds);

   if(secs != 0 && secs != NOTOK)
      secs *= 1000;

again:
   n = poll(pollfds, (unsigned long)j, secs);

   if(n == NOTOK) 
      {
      if(errno == EAGAIN)
         goto again;

      if(errno != EINTR)
         LIB_PERROR("Poll failed");

      return NOTOK;
      }

   for(i = 0; i < j; i++) 
      {
      if(rfds && (pollfds[i].revents & (POLLIN | POLLPRI)))
         FD_SET(pollfds[i].fd, rfds);

      if(wfds && (pollfds[i].revents & POLLOUT))
         FD_SET(pollfds[i].fd, wfds);

      if(efds && (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)))
         FD_SET(pollfds[i].fd, efds);
      }

   return n;
   }
#endif   /* SVR4 */


int xselect(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, int secs)
   {
   register int fd;
   int n;

#ifdef BSD
   fd_set ifds, ofds, xfds;
#endif

   static int nsysfds = NOTOK;

   if(nsysfds == NOTOK)

#ifdef SVR4
      nsysfds = FD_SETSIZE;
#else
      nsysfds = getdtablesize();
#endif

   if(nfds > FD_SETSIZE)
      nfds = FD_SETSIZE;

   if(nfds > nsysfds + 1)
      nfds = nsysfds + 1;

   n = 0;

#ifdef BSD
   FD_ZERO(&ifds);

   if(rfds)
      ifds = *rfds;         /* struct copy */

   if(wfds)
      ofds = *wfds;         /* struct copy */

   if(efds)
      xfds = *efds;         /* struct copy */
#endif

   if((n = selsocket(nfds, rfds, wfds, efds, secs)) != NOTOK)
      return n;

#ifdef BSD
   if(errno == EBADF) 
      {
      struct stat st;

      if(rfds)
         FD_ZERO(rfds);

      if(wfds)
         FD_ZERO(wfds);

      if(efds)
         FD_ZERO(efds);

      n = 0;

      for(fd = 0; fd < nfds; fd++)
         if(((rfds && FD_ISSET(fd, &ifds))
               || (wfds && FD_ISSET(fd, &ofds))
               || (efds && FD_ISSET(fd, &xfds)))
               && fstat(fd, &st) == NOTOK) 
            {
            if(rfds && FD_ISSET(fd, &ifds))
               FD_SET(fd, rfds);

            if(wfds && FD_ISSET(fd, &ofds))
               FD_SET(fd, wfds);

            if(efds && FD_ISSET(fd, &xfds))
               FD_SET(fd, efds);

            LIB_ERROR1("fd %d has gone bad.\n", fd);
            n++;
            }
      if(n)
         return n;

      errno = EBADF;
      }
#endif   /* BSD */

   return NOTOK;
   }

static char *empty = NULL;

#ifdef  h_addr
static char *addrs[2] = {NULL};
#endif

struct hostent *gethostbystring(char *s)
   {
   register struct hostent *h;
   static unsigned long iaddr;
   static struct hostent hs;

   iaddr = inet_addr(s);
   if(iaddr == NOTOK && strcmp(s, "255.255.255.255"))
      return gethostbyname(s);

   h = &hs;
   h->h_name = s;
   h->h_aliases = &empty;
   h->h_addrtype = AF_INET;
   h->h_length = sizeof(iaddr);

#ifdef h_addr
   h->h_addr_list = addrs;
   bzero((char *)addrs, sizeof addrs);
#endif

   h->h_addr = (char *)&iaddr;

   return(h);
   }