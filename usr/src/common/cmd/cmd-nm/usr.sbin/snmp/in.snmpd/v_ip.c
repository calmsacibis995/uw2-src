/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nm:common/cmd/cmd-nm/usr.sbin/snmp/in.snmpd/v_ip.c	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nm/usr.sbin/snmp/in.snmpd/v_ip.c,v 1.6 1994/08/11 15:51:13 cyang Exp $"

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
static char SNMPID[] = "@(#)v_ip.c	4.4 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */
#include <sys/param.h>
#if !defined(SVR3) && !defined(SVR4)
#include <sys/vmmac.h>
#include <machine/pte.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stropts.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#ifndef SUNOS35
#include <netinet/in_var.h>
#endif
#ifdef SUNOS35
#include <sys/ioctl.h>
#endif
#if defined(SVR3) || defined(SVR4)
#ifdef PSE
#include <common.h>
#endif
#include <sys/stream.h>
#endif
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_var.h>
#if defined(SVR3) || defined(SVR4)
#include <net/route.h>
#include <netinet/ip_str.h>
#endif
#include <syslog.h>
#include <unistd.h>

#include "snmp.h"
#include "snmpuser.h"
#include "snmpd.h"

#define FALSE 0
#define TRUE 1

#ifdef NEW_MIB
extern int ip_fd, nfds;
extern fd_set ifds;
#endif

#if !defined(SVR3) && !defined(SVR4)
extern int global_gateway;
#endif

VarBindList *get_next_class();

VarBindList
*var_ip_stat_get(OID var_name_ptr,
		 OID in_name_ptr,
		 unsigned int arg,
		 VarEntry *var_next_ptr,
		 int type_search)
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  struct ipstat ipstat;
#ifdef NEW_MIB
  struct ip_stuff ip_stuff;
#endif
  int total;
  int cc;

  /* see if explicitly determined */
  if ((type_search == EXACT) &&
      ((in_name_ptr->length != (var_name_ptr->length + 1)) ||
       (in_name_ptr->oid_ptr[var_name_ptr->length] != 0)))
    return(NULL);

  if ((type_search == NEXT) && (cmp_oid_class(in_name_ptr, var_name_ptr) == 0) && (in_name_ptr->length >= (var_name_ptr->length + 1)))
    return(get_next_class(var_next_ptr));

#ifdef NEW_MIB
  cc = get_ip_stat(&ip_stuff);

  bcopy ((char *)&(ip_stuff.ip_stat), (char *)&ipstat,
	sizeof (struct ipstat));
#else
  cc = get_ip_stat(nl[N_IPSTAT].n_value, &ipstat);
#endif


  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  switch (arg) 
    {
    case 1:
      if (nl[N_IPFORWARDING].n_value == 0) 
	{
	  syslog(LOG_WARNING, 
		 gettxt(":141", "var_ip_stat_get: ipforwarding: Symbol not defined.\n"));
	  if (type_search == EXACT)
	    return (NULL);
	  if (type_search == NEXT)
	    return(get_next_class(var_next_ptr));
	}
      lseek(kmem, nl[N_IPFORWARDING].n_value, 0);
      if (read(kmem, &total, sizeof(total)) < 0) 
	{
	  syslog(LOG_WARNING, 
		 gettxt(":142", "var_ip_stat_get: ipforwarding: %m.\n"));
	  if (type_search == EXACT)
	    return (NULL);
	  if (type_search == NEXT)
	    return(get_next_class(var_next_ptr));
	}
      if (total == 0)
	total = 2;

      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipForwarding.0");
      vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, total, NULL, NULL);
      oid_ptr = NULL;
      break;

    case 2:
      if (nl[N_IP_TTL].n_value == 0) 
	{
	  syslog(LOG_WARNING,
		 gettxt(":143", "var_ip_stat_get: ip_ttl: Symbol not defined.\n"));
	  if (type_search == EXACT)
	    return(NULL);
	  if (type_search == NEXT)
	    return(get_next_class(var_next_ptr));
	}
      lseek(kmem, nl[N_IP_TTL].n_value, 0);
      if (read(kmem, &total, sizeof(total)) < 0) 
	{
	  syslog(LOG_WARNING, 
		 gettxt(":144", "var_ip_stat_get: ip_ttl: %m.\n"));
	  if (type_search == EXACT)
	    return(NULL);
	  if (type_search == NEXT)
	    return(get_next_class(var_next_ptr));
	}
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipDefaultTTL.0");
      vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, total, NULL, NULL);
      oid_ptr = NULL;
      break;

    case 3:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipInReceives.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ipstat.ips_total, 0, 
			    NULL, NULL);
      oid_ptr = NULL;
      break;

    case 4:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipInHdrErrors.0");
      total = ipstat.ips_badsum + ipstat.ips_tooshort +  
	ipstat.ips_toosmall + ipstat.ips_badhlen + ipstat.ips_badlen;
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, total, 0, NULL, NULL);
      oid_ptr = NULL;
      break;
      
    case 5:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipInAddrErrors.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ipstat.ips_cantforward, 
			    0, NULL, NULL);
      oid_ptr = NULL;
      break;

    case 6:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipForwDatagrams.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ipstat.ips_forward, 
			    0, NULL, NULL);
      oid_ptr = NULL;
      break;
      
    case 7:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipInUnknownProtos.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ipstat.ips_unknownproto, 
			    0, NULL, NULL);
      oid_ptr = NULL;
      break;

    case 8:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipInDiscards.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ipstat.ips_inerrors, 
			    0, NULL, NULL);
      oid_ptr = NULL;
      break;

    case 9:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipInDelivers.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ipstat.ips_indelivers, 
			    0, NULL, NULL);
      oid_ptr = NULL;
      break;

    case 10:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipOutRequests.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ipstat.ips_outrequests, 
			    0, NULL, NULL);
      oid_ptr = NULL;
      break;

    case 11:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipOutDiscards.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ipstat.ips_outerrors, 
			    0, NULL, NULL);
      oid_ptr = NULL;
      break;

    case 12:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipOutNoRoutes.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ipstat.ips_noroutes, 
			    0, NULL, NULL);
      oid_ptr = NULL;
      break;

    case 13:
    if (nl[N_IPQ_TTL].n_value == 0) 
      {
	syslog(LOG_WARNING, 
	       gettxt(":145", "var_ip_stat_get: ipq_ttl: Symbol not defined.\n"));
	if (type_search == EXACT)
	  return(NULL);
	if (type_search == NEXT)
	  return(get_next_class(var_next_ptr));
      }
      lseek(kmem, nl[N_IPQ_TTL].n_value, 0);
      if (read(kmem, &total, sizeof(total)) < 0) 
	{
	  syslog(LOG_WARNING, 
		 gettxt(":146", "var_ip_stat_get: ipq_ttl: %m.\n"));
	  if (type_search == EXACT)
	    return(NULL);
	  if (type_search == NEXT)
	    return(get_next_class(var_next_ptr));
	}
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipReasmTimeout.0");
      vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, total, NULL, NULL);
      oid_ptr = NULL;
      break;

    case 14:  /* ipReasmReqds */
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipReasmReqds.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ipstat.ips_fragments, 
			    0, NULL, NULL);
      oid_ptr = NULL;
      break;
      
    case 15:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipReasmOKs.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ipstat.ips_reasms, 
			    0, NULL, NULL);
      oid_ptr = NULL;
      break;

  case 16:
    oid_ptr = make_obj_id_from_dot((unsigned char *)"ipReasmFails.0");
    total = ipstat.ips_fragdropped + ipstat.ips_fragtimeout;
    vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, total, 0, NULL, NULL);
    oid_ptr = NULL;
    break;

    case 17:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipFragOKs.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ipstat.ips_pfrags, 
			    0, NULL, NULL);
      oid_ptr = NULL;
      break;

    case 18:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipFragFails.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ipstat.ips_fragfails, 
			    0, NULL, NULL);
      oid_ptr = NULL;
      break;

    case 19:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipFragCreates.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ipstat.ips_frags, 
			    0, NULL, NULL);
      oid_ptr = NULL;
      break;
    case 23:
      oid_ptr = make_obj_id_from_dot((unsigned char *)"ipRoutingDiscards.0");
      vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, 0, 0, NULL, NULL);
      oid_ptr = NULL;
      break;

    default:			/* should never happen */
      if (type_search == EXACT)
	return(NULL);
      else
	return(get_next_class(var_next_ptr));
    }
  return(vb_ptr);
} 

VarBindList
*var_ip_addr_get(OID var_name_ptr,
		 OID in_name_ptr,
		 unsigned int arg,
		 VarEntry *var_next_ptr,
		 int type_search)
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  OctetString *os_ptr;
  int if_num = 0;
  int cc;
#if defined(BSD)  || defined(NEW_MIB)
  struct ifnet ifnet_entry;
#endif
#if (defined(SVR3) || defined(SVR4)) & !defined(NEW_MIB)
  struct ip_provider prov_entry;
#endif
  char ifname[IFNAMSIZ];
#if (defined(SUNOS35) || defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif
  char buffer[256];
  struct sockaddr_in *sin;
  unsigned long test_addr;
  unsigned long final_ip_addr;
  unsigned long temp_ip_addr;
  unsigned char netaddr[4];
  int i;
  int broad_type;
#if defined(SVR3) || defined(SVR4)
  struct in_addr *in;
#endif

  /* see if explicitly determined */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 4)))
    return(NULL);

  /* determine ip address */
  test_addr = 0;
  if (in_name_ptr->length > var_name_ptr->length ) {
    for (i=var_name_ptr->length; 
	 ((i < in_name_ptr->length) && (i < var_name_ptr->length + 4)); i++)
      test_addr = (test_addr << 8) + in_name_ptr->oid_ptr[i];  /* first sub-field after name */
    for (; i < var_name_ptr->length + 4; i++)
      test_addr = test_addr << 8;
  }

  if ((type_search == NEXT) && (in_name_ptr->length >= var_name_ptr->length+4))
    test_addr++;

  /*printf("nl[N_IFNET].n_value %x\n", nl[N_IFNET].n_value);*/
  
  /*printf("test_addr = %x\n", test_addr);*/

#ifdef BSD
  cc = get_ip_addr(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname,
		   &ifaddr_entry, test_addr);
#endif
#if (defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
  cc = get_ip_addr(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		   &ifaddr_entry, test_addr);
#endif
#ifdef NEW_MIB
  cc = get_ip_addr(&if_num, &ifnet_entry, ifname, &ifaddr_entry,
		   test_addr);
#endif
  
  /*printf(" cc = %x\n", cc);*/

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr));
    if (type_search == EXACT)
      return(NULL);
  }

#if (defined(SUNOS35) || defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
  sin = &ifaddr_entry;
#else
  sin = (struct sockaddr_in *) &ifaddr_entry.in.ia_addr;
#endif
  final_ip_addr = ntohl(sin->sin_addr.s_addr);
  /*printf(" final_ip_addr %x \n", final_ip_addr);*/

  if ((type_search == EXACT) && (final_ip_addr != test_addr))
    return(NULL);
  
  switch (arg) {
  case 1:
    sprintf(buffer,"ipAdEntAddr.%d.%d.%d.%d",
	    ((final_ip_addr>>24) &0xFF),
	    ((final_ip_addr>>16) &0xFF),
	    ((final_ip_addr>>8) &0xFF),
	    (final_ip_addr &0xFF));
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    netaddr[0] = ((final_ip_addr>>24) & 0xFF);
    netaddr[1] = ((final_ip_addr>>16) & 0xFF);
    netaddr[2] = ((final_ip_addr>>8) & 0xFF);
    netaddr[3] = (final_ip_addr & 0xFF);
    os_ptr = make_octetstring(netaddr, 4);
    vb_ptr = make_varbind(oid_ptr, IP_ADDR_PRIM_TYPE, 0, 0, os_ptr, NULL);
    oid_ptr = NULL;
    break;
  case 2:
    sprintf(buffer,"ipAdEntIfIndex.%d.%d.%d.%d",
	    ((final_ip_addr>>24) &0xFF),
	    ((final_ip_addr>>16) &0xFF),
	    ((final_ip_addr>>8) &0xFF),
	    (final_ip_addr &0xFF));
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, if_num, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 3:
    sprintf(buffer,"ipAdEntNetMask.%d.%d.%d.%d",
	    ((final_ip_addr>>24) &0xFF),
	    ((final_ip_addr>>16) &0xFF),
	    ((final_ip_addr>>8) &0xFF),
	    (final_ip_addr &0xFF));
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
#ifdef BSD
#ifndef SUNOS35
    temp_ip_addr = ifaddr_entry.in.ia_subnetmask;
#else
    temp_ip_addr = get_subnet_mask(ifname, ifnet_entry.if_unit);
#endif
#endif
#if (defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
    temp_ip_addr = prov_entry.ia.ia_subnetmask;
#endif
#ifdef NEW_MIB
    temp_ip_addr = ifaddr_entry.in.ia_sockmask.sin_addr.s_addr;
#endif
    netaddr[0] = ((temp_ip_addr>>24) & 0xFF);
    netaddr[1] = ((temp_ip_addr>>16) & 0xFF);
    netaddr[2] = ((temp_ip_addr>>8) & 0xFF);
    netaddr[3] = (temp_ip_addr & 0xFF);
    os_ptr = make_octetstring(netaddr, 4);
    vb_ptr = make_varbind(oid_ptr, IP_ADDR_PRIM_TYPE, 0, 0, os_ptr, NULL);
    oid_ptr = NULL;
    break;
  case 4:
    sprintf(buffer,"ipAdEntBcastAddr.%d.%d.%d.%d",
	    ((final_ip_addr>>24) &0xFF),
	    ((final_ip_addr>>16) &0xFF),
	    ((final_ip_addr>>8) &0xFF),
	    (final_ip_addr &0xFF));
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
#ifdef BSD
#ifdef SUNOS35
    sin = (struct sockaddr_in *) &ifnet_entry.if_broadaddr;
#else
    sin = (struct sockaddr_in *) &ifaddr_entry.in.ia_broadaddr;
#endif
    temp_ip_addr = ntohl(sin->sin_addr.s_addr);
#endif
#if defined(SVR3) && !defined(TCP40) && !defined(NEW_MIB)
    in = &prov_entry.ia.ia_broadaddr;
    temp_ip_addr = ntohl(in->s_addr);
#endif
#if (defined(SVR4) || defined(TCP40)) && !defined(NEW_MIB)
    sin = (struct sockaddr_in *) &prov_entry.ia.ia_broadaddr;
    temp_ip_addr = ntohl(sin->sin_addr.s_addr);
#endif
#ifdef NEW_MIB
    sin = (struct sockaddr_in *) &ifaddr_entry.in.ia_broadaddr;
    temp_ip_addr = ntohl(sin->sin_addr.s_addr);
#endif
    broad_type = temp_ip_addr & 0x01;
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, broad_type, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 5:
    sprintf(buffer,"ipAdEntReasmMaxSize.%d.%d.%d.%d",
	    ((final_ip_addr>>24) &0xFF),
	    ((final_ip_addr>>16) &0xFF),
	    ((final_ip_addr>>8) &0xFF),
	    (final_ip_addr &0xFF));
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
#ifdef BSD
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, 65535, NULL, NULL);
#endif
#if defined(SVR3) || defined(SVR4)
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, IP_MAXPACKET, NULL, NULL);
#endif
    oid_ptr = NULL;
    break;
  default:
    if (type_search == EXACT)
      return(NULL);
    else
      return(get_next_class(var_next_ptr));
  };

  return(vb_ptr);
} 
  
#ifdef SUNOS35
get_subnet_mask(ifname, unit_number)
   char *ifname;
   short unit_number;
{
  int s;
  struct ifreq ifr;
  struct sockaddr_in *sin;

  /*printf(" ifname = %s%d\n", ifname, unit_number);*/
  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0) {
     return(0);
  }
  sprintf(ifr.ifr_name, "%s%d\0", ifname, unit_number);
  if (ioctl(s, SIOCGIFNETMASK, &ifr) < 0) {
    close(s);
    return(0);
  }
  else {
    close(s);
    sin = (struct sockaddr_in *) &ifr.ifr_addr;
    return(ntohl(sin->sin_addr.s_addr));
  }
}
#endif

#ifdef NEW_MIB
int	get_ip_stat(ip_stuff)
struct	ip_stuff *ip_stuff; 
{
    struct strioctl strioc;

    if (ip_fd < 0) {
	if ((ip_fd = open(_PATH_IP, O_RDONLY)) < 0) {
	    syslog(LOG_WARNING, gettxt(":167", "get_ip_stat: Open of %s failed: %m.\n"), _PATH_IP);
	    return (FALSE);
	}
	else {
	    /* Set up to receive link-up/down traps */
	    strioc.ic_cmd = SIOCSIPTRAP;
	    strioc.ic_dp = (char *)0;
	    strioc.ic_len = 0;
	    strioc.ic_timout = -1;

	    if (ioctl(ip_fd, I_STR, &strioc) < 0) {
		syslog(LOG_WARNING, gettxt(":151", "var_ip_stat_get: SIOCSIPTRAP: %m.\n"));
		(void) close (ip_fd);
		ip_fd = -1;
		return (FALSE);
	    }

	    if (ip_fd >= nfds)
		nfds = ip_fd + 1;
	    FD_SET(ip_fd, &ifds);
	}
    }

    strioc.ic_cmd = SIOCGIPSTUFF;
    strioc.ic_dp = (char *)ip_stuff;
    strioc.ic_len = sizeof(struct ip_stuff);
    strioc.ic_timout = -1;

    if (ioctl(ip_fd, I_STR, &strioc) < 0) {
	syslog(LOG_WARNING, gettxt(":152", "get_ip_stat: ioctl: SIOCGIPSTUFF: %m.\n"));
	(void) close (ip_fd);
	ip_fd = -1;
	return(FALSE);
    }
 
    return(TRUE);
}
#else
get_ip_stat(off_t ipstataddr,
	    struct ipstat *ipstat)
{
  if (ipstataddr == 0) {
    syslog(LOG_WARNING, gettxt(":153", "ipstataddr: Symbol not defined.\n"));
    return(FALSE);
  }

  lseek(kmem, ipstataddr, 0);
  if (read(kmem, ipstat, sizeof(struct ipstat)) < 0) {
    syslog(LOG_WARNING, gettxt(":154", "get_ip_stat: %m.\n"));
    return(FALSE);
  }

  return (TRUE);
}
#endif

#ifdef BSD
/* Return the generic if stats for the IP interface listed. */
get_ip_addr(ifnetaddr, if_num, ifnet_entry, ifname, ifaddr_entry, test_addr)
     off_t ifnetaddr;
     int *if_num;
     struct ifnet *ifnet_entry; 
     char *ifname;
#ifdef SUNOS35
     struct sockaddr_in *ifaddr_entry;
#else
     struct in_ifaddr *ifaddr_entry;
#endif
     unsigned long test_addr;
{
  int i;
  off_t ifaddraddr;
  struct ifnet temp_ifnet_entry;
#ifndef SUNOS35
  struct in_ifaddr temp_ifaddr_entry;
#endif
  char temp_ifname[IFNAMSIZ];
  struct sockaddr_in *temp_addr;
  unsigned long best_addr;
  int is_ip;

  if (ifnetaddr == 0) {
    syslog(LOG_WARNING, gettxt(":155", "ifnet: Symbol not defined.\n"));
    return(FALSE);
  }

  lseek(kmem, ifnetaddr, 0);
  if (read(kmem, &ifnetaddr, sizeof(ifnetaddr)) < 0) {
    syslog(LOG_WARNING, gettxt(":156", "get_ip_addr, ifnetaddr: %m.\n"));
    return(FALSE);
  }

  if (ifnetaddr == NULL)  
    return(FALSE);

  i = 0;
  best_addr = 0xFFFFFFFF;

  while (ifnetaddr) {
    is_ip = FALSE;
    
    lseek(kmem, ifnetaddr, 0);
    if (read(kmem, &temp_ifnet_entry, sizeof (struct ifnet)) < 0) {
      syslog(LOG_WARNING, gettxt(":156", "get_ip_addr, ifnetaddr: %m.\n"));
      return(FALSE);
    }
    
    lseek(kmem, (off_t)temp_ifnet_entry.if_name, 0);
    if (read(kmem, temp_ifname, IFNAMSIZ) < 0) {
      syslog(LOG_WARNING, gettxt(":157", "get_ip_addr, temp_ifnet_entry.if_name: %m.\n"));
      return(FALSE);
    }
    temp_ifname[IFNAMSIZ - 1] = '\0';

#ifdef SUNOS35
    if (temp_ifnet_entry.if_addr.sa_family == AF_INET) {
      is_ip = TRUE;
      temp_addr = (struct sockaddr_in *) &temp_ifnet_entry.if_addr;
      if ((ntohl(temp_addr->sin_addr.s_addr) < best_addr) && 
	  (ntohl(temp_addr->sin_addr.s_addr) >= test_addr)) {
	bcopy((char *) &temp_ifnet_entry, (char *) ifnet_entry, sizeof(struct ifnet));
	bcopy((char *) &temp_ifnet_entry.if_addr, (char *) ifaddr_entry, sizeof(struct sockaddr));
	strncpy(ifname, temp_ifname, IFNAMSIZ);
	*if_num = i + 1;		/* will hold best if number */
	best_addr = ntohl(temp_addr->sin_addr.s_addr);
      }
    }
#else    
    ifaddraddr = (off_t) temp_ifnet_entry.if_addrlist;

    /* now find an internet address on the interface, if any */
    while(ifaddraddr) {
      lseek(kmem, ifaddraddr, 0);
      if (read(kmem, &temp_ifaddr_entry, sizeof(struct in_ifaddr)) < 0) {
	syslog(LOG_WARNING, gettxt(":158", "get_ip_addr, temp_ifaddr_entry: %m.\n"));
	return(FALSE);
      }
      /* If this has an IP addr, then this counts as an IP interface.  */
      if (temp_ifaddr_entry.ia_ifa.ifa_addr.sa_family == AF_INET) {
	is_ip = TRUE; /* We've got an IP interface */
	temp_addr = (struct sockaddr_in *) &temp_ifaddr_entry.ia_ifa.ifa_addr; /* put in sockaddr_in form */
	if ((ntohl(temp_addr->sin_addr.s_addr) < best_addr) && 
	    (ntohl(temp_addr->sin_addr.s_addr) >= test_addr)) {
	  bcopy((char *) &temp_ifnet_entry, (char *) ifnet_entry, sizeof(struct ifnet));
	  bcopy((char *) &temp_ifaddr_entry, (char *) ifaddr_entry, sizeof(struct in_ifaddr));
	  strncpy(ifname, temp_ifname, IFNAMSIZ);
	  *if_num = i + 1;		/* will hold best if number */
	  best_addr = ntohl(temp_addr->sin_addr.s_addr);
	}
      }
      ifaddraddr = (off_t)temp_ifaddr_entry.ia_ifa.ifa_next;
/* or is it     ifaddraddr = (off_t)temp_ifaddr_entry.ia_next; ?*/
    } /* end of while */
#endif
    if (is_ip)
      i++;
    ifnetaddr = (off_t)temp_ifnet_entry.if_next;
  }

  /* If still 0xFF..., then end of list of if's */
  if (best_addr == 0xFFFFFFFF)
    return(FALSE);
  /* 
   * Now return the best one found.  Well, it's already in the structures
   *  that were passed, so just return.
   */
  
  return(TRUE);
}
#endif /* BSD */
#if (defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
/* Return the generic if stats for the IP interface listed. */
get_ip_addr(off_t provaddr,
	    int *if_num,
	    struct ip_provider *prov_entry,
	    char *ifname,
	    struct sockaddr_in *ifaddr_entry,
	    unsigned long test_addr)
{
  int i, n;
  off_t lastprov;
  struct ip_provider temp_prov_entry;
  struct sockaddr_in *temp_addr;
  unsigned long best_addr;

  if (provaddr == 0) {
    syslog(LOG_WARNING, gettxt(":159", "get_ip_addr: provider: Symbol not defined.\n"));
    return(FALSE);
  }

  if (nl[N_LASTPROV].n_value == 0) {
    syslog(LOG_WARNING, gettxt(":160", "get_ip_addr: lastprov: Symbol not defined.\n"));
    return(FALSE);
  }
  lseek(kmem, nl[N_LASTPROV].n_value, 0);
  if (read(kmem, &lastprov, sizeof(lastprov)) < 0) {
    syslog(LOG_WARNING, gettxt(":161", "get_ip_addr: lastprov: %m.\n"));
    return(FALSE);
  }
  n = (lastprov - provaddr) / sizeof(struct ip_provider) + 1;

  i = 0;
  best_addr = 0xFFFFFFFF;
  lseek(kmem, provaddr, 0);

  while (i < n) {
    if (read(kmem, &temp_prov_entry, sizeof (struct ip_provider)) < 0) {
      syslog(LOG_WARNING, gettxt(":162", "get_ip_addr, temp_prov_entry: %m.\n"));
      return(FALSE);
    }
    
    temp_prov_entry.name[IFNAMSIZ - 1] = '\0';

    if (temp_prov_entry.ia.ia_ifa.ifa_addr.sa_family == AF_INET) {
      temp_addr = (struct sockaddr_in *) &temp_prov_entry.ia.ia_ifa.ifa_addr;
      if ((ntohl(temp_addr->sin_addr.s_addr) < best_addr) && 
	  (ntohl(temp_addr->sin_addr.s_addr) >= test_addr)) {
	bcopy((char *) &temp_prov_entry, (char *) prov_entry,
		sizeof(struct ip_provider));
	bcopy((char *) &temp_prov_entry.ia.ia_ifa.ifa_addr,
		(char *) ifaddr_entry, sizeof(struct sockaddr_in));
	strncpy(ifname, temp_prov_entry.name, IFNAMSIZ);
	*if_num = i + 1;                /* will hold best if number */
	best_addr = ntohl(temp_addr->sin_addr.s_addr);
      }
    }
    i++;
  }

  /* If still 0xFF..., then end of list of if's */
  if (best_addr == 0xFFFFFFFF)
    return(FALSE);
  /* 
   * Now return the best one found.  Well, it's already in the structures
   *  that were passed, so just return.
   */
  
  return(TRUE);
}
#endif /* (SVR3 || SVR4) && !(NEW_MIB) */

#ifdef NEW_MIB
/* Return the generic if stats for the IP interface listed. */
int
get_ip_addr(if_num, ifnet_entry, ifname, ifaddr_entry, test_addr)
int *if_num;
struct ifnet *ifnet_entry; 
char *ifname;
struct in_ifaddr *ifaddr_entry;
unsigned long test_addr;
{
    int  i, cc, if_index, if_max;
    struct  ifreq_all if_all;
    struct  sockaddr_in *temp_addr;
    unsigned long best_addr;

    if_all.if_number = -1;
    cc = get_if_all(&if_all);

    if (cc == FALSE)
	return (FALSE);

    best_addr = 0xFFFFFFFF;
    if_max = if_all.if_number;
    if_index = 1;

    while (if_index <= if_max) {
	if_all.if_number = if_index;
	cc = get_if_all(&if_all);

	for (i = 0; i < if_all.if_naddr; i++) {
	    temp_addr = (struct sockaddr_in *) &(if_all.addrs[i].addr);

	    if ((ntohl(temp_addr->sin_addr.s_addr) < best_addr) &&
		(ntohl(temp_addr->sin_addr.s_addr) >= test_addr)) {

		bcopy((char *) &(if_all.if_entry), (char *) ifnet_entry,
		    sizeof(struct ifnet));

		bcopy((char *) &(if_all.addrs[i].addr),
		     (char *) &(ifaddr_entry->ia_addr),
		     sizeof(struct sockaddr_in));
		bcopy((char *) &(if_all.addrs[i].dstaddr),
		     (char *) &(ifaddr_entry->ia_broadaddr),
		     sizeof(struct sockaddr_in));
		bcopy((char *) &(if_all.addrs[i].netmask),
		     (char *) &(ifaddr_entry->ia_sockmask),
		     sizeof(struct sockaddr_in));

		strncpy(ifname, if_all.if_entry.if_name, IFNAMSIZ);
		ifname[IFNAMSIZ - 1] = '\0';
		*if_num = if_index;
		best_addr = ntohl(temp_addr->sin_addr.s_addr);
	    }
	} /* for loop */

	if_index++;
    } /* while loop */

    /* If still 0xFF..., then end of list of if's */
    if (best_addr == 0xFFFFFFFF)
	return (FALSE);

    return (TRUE);
}
#endif

int var_ip_stat_test(OID var_name_ptr,
		     OID in_name_ptr,
		     unsigned int arg,
		     ObjectSyntax *value)
{
  int forwarding;

  if ((in_name_ptr->length != (var_name_ptr->length + 1)) ||
      (in_name_ptr->oid_ptr[var_name_ptr->length] != 0))
    return(FALSE);

#if (defined(SVR3) || defined(SVR4))
  switch (arg) {
  case 1:
#ifndef NEW_MIB
    if (nl[N_IPFORWARDING].n_value == 0) {
      syslog(LOG_WARNING, gettxt(":163", "var_ip_stat_test: ipforwarding: Symbol not defined.\n"));
      return(FALSE);
    }
#endif
    forwarding = value->sl_value;
    if (forwarding != 1 && forwarding != 2) {
      return(FALSE);
    }
    break;
  case 2:
    if (nl[N_IP_TTL].n_value == 0) {
      syslog(LOG_WARNING, gettxt(":164", "var_ip_stat_test: ip_ttl: Symbol not defined.\n"));
      return(FALSE);
    }
    break;
  default:
    return(FALSE);
  }
  return(TRUE);
#endif
#if !defined(SVR3) && !defined(SVR4)
  return(FALSE);
#endif
}

#ifdef NEW_MIB
int	var_ip_stat_set(var_name_ptr, in_name_ptr, arg, value)
OID	*var_name_ptr;
OID	*in_name_ptr;
unsigned int arg;
ObjectSyntax *value;
{
    struct strioctl strioc;
    struct ip_misc ip_misc;

    if (ip_fd < 0) {
	if ((ip_fd = open(_PATH_IP, O_RDONLY)) < 0) {
	    syslog(LOG_WARNING, gettxt(":165", "var_ip_stat_set: Open of %s failed: %m.\n"),
		_PATH_IP);
	    return (FALSE);
	}
	else {
	    /* Set up to receive link-up/down traps */
	    strioc.ic_cmd = SIOCSIPTRAP;
	    strioc.ic_dp = (char *)0;
	    strioc.ic_len = 0;
	    strioc.ic_timout = -1;

	    if (ioctl(ip_fd, I_STR, &strioc) < 0) {
		syslog(LOG_WARNING, gettxt(":166", "var_ip_stat_set: SIOCSIPTRAP: %m.\n"));
		(void) close (ip_fd);
		ip_fd = -1;
		return (FALSE);
	    }

	    if (ip_fd >= nfds)
		nfds = ip_fd + 1;
	    FD_SET(ip_fd, &ifds);
	}
    }

    strioc.ic_cmd = SIOCSIPMISC;
    strioc.ic_dp = (char *) &ip_misc; 
    strioc.ic_len = sizeof(struct ip_misc);
    strioc.ic_timout = -1;
    ip_misc.arg = arg;

    switch (arg) {
    case 1:
	ip_misc.ip_forwarding = value->sl_value;
	break;

    case 2:
	ip_misc.ip_default_ttl = value->sl_value;
	break;

    default:
	return(FALSE);
    }

    if (ioctl(ip_fd, I_STR, &strioc) < 0) {
	perror ("snmpd: var_ip_stat_set: ioctl: SIOCSIPMISC");
	(void) close(ip_fd);
	ip_fd = -1;
	return(FALSE);
    }

    return(TRUE);

}
#else
int var_ip_stat_set(OID var_name_ptr,
		    OID in_name_ptr,
		    unsigned int arg,
		    ObjectSyntax *value)
{
  int forwarding;
  int ttl;
 
#if defined(SVR3) || defined(SVR4)
  switch (arg) {
  case 1:
    forwarding = value->sl_value;
    lseek(kmem, nl[N_IPFORWARDING].n_value, 0);
    if (write(kmem, (char *)&forwarding, sizeof(forwarding)) != sizeof(forwarding)) {
      return(FALSE);
    }
    break;
  case 2:
    ttl = value->sl_value;
    lseek(kmem, nl[N_IP_TTL].n_value, 0);
    if (write(kmem, (char *)&ttl, sizeof(ttl)) != sizeof(ttl)) {
      return(FALSE);
    }
    break;
  default:
    return(FALSE);
  }
  return(TRUE);
#endif
#if !defined(SVR3) && !defined(SVR4)
  return(FALSE);
#endif
}
#endif
