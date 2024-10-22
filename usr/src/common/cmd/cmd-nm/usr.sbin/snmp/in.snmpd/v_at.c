/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nm:common/cmd/cmd-nm/usr.sbin/snmp/in.snmpd/v_at.c	1.4"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nm/usr.sbin/snmp/in.snmpd/v_at.c,v 1.5 1994/07/25 20:12:03 cyang Exp $"

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
static char SNMPID[] = "@(#)v_at.c	4.3 INTERACTIVE SNMP source";
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
#if !defined(SVR3) && !defined(SVR4)
#include <sys/mbuf.h>
#endif
#include <net/if.h>
#include <net/af.h>
#include <netinet/in.h>
#ifndef SUNOS35
#include <netinet/in_var.h>
#endif
#if defined(SVR3) || defined(SVR4)
#include <net/if_arp.h>
#ifdef PSE
#include <common.h>
#endif
#include <sys/stream.h>
#include <sys/stropts.h>
#include <net/route.h>
#include <netinet/ip_str.h>
#endif
#include <netinet/if_ether.h>
#ifdef SVR4
#include <sys/sockio.h>
#endif
#ifdef BSD
#include <sys/ioctl.h>
#endif
#include <syslog.h>
#include <unistd.h>

#include "snmp.h"
#include "snmpuser.h"
#include "snmpd.h"

#define FALSE 0
#define TRUE 1


VarBindList *get_next_class();

VarBindList
*var_at_get(OID var_name_ptr,
	    OID in_name_ptr,
	    unsigned int arg,
	    VarEntry *var_next_ptr,
	    int type_search)
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  OctetString *os_ptr;
  int i;
  unsigned long ip_addr;
  int ip_addr_len;
  struct arptab arptab_entry;
  int if_num = 0;
  int cc;
#if defined(BSD) || defined(NEW_MIB)
  struct ifnet ifnet_entry;
#endif
#if (defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
  struct ip_provider prov_entry;
#endif
  char ifname[16];
#if (defined(SUNOS35) || defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif
  int interface, in_interface;
  char buffer[256];
  unsigned char netaddr[4];
  unsigned long new_ip_addr;

  int tempi;

  if ((type_search == EXACT) && (in_name_ptr->length != 
				 (var_name_ptr->length + 6)))
    return(NULL);

  /* determine interface */
  interface = 1;
  if (in_name_ptr->length > var_name_ptr->length)
    interface = in_name_ptr->oid_ptr[var_name_ptr->length];  /* first sub-field after name */

  /* check that the interface exists */
  /*  
   *  bug fix for arp table entries on loopback interface
   *
   *  changed 255 to interface in the line below
   *  1/28/89 JDC
   */

  in_interface = interface;
again: 
  if_num = interface;
  /* end of fix 1/28/89 JDC */
#ifdef BSD
  get_if_entry(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname,
	      &ifaddr_entry);
#endif
#if (defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
  get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
	      &ifaddr_entry);
#endif
#ifdef NEW_MIB
  get_if_entry(&if_num, &ifnet_entry, ifname, &ifaddr_entry);
#endif

  if (interface > if_num) {
    if (type_search == EXACT)
      return(NULL);
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr));
  }
  /*
   *  fix bug where get arp table entries for the loopback interface
   *  JDC 1/28/89
   */ 
#ifdef BSD
  if (strcmp(ifname, "lo") == 0) {
#endif
#if defined(SVR3) || defined(SVR4)
  if (strcmp(ifname, "lo0") == 0) {
#endif
    if (type_search == EXACT)
      return(NULL);
    if (type_search == NEXT) {
      interface++;
      goto again;
    }
  }
   /*
    *  end bug fix
    */

  /* determine ip address */
  ip_addr = 0;
  if (in_name_ptr->length > var_name_ptr->length + 2 && interface == in_interface) {
    for (i=var_name_ptr->length + 2; i < var_name_ptr->length + 6; i++) {
      if (i < in_name_ptr->length)
	ip_addr = (ip_addr << 8) + in_name_ptr->oid_ptr[i];  /* first sub-field after name */
      else
	ip_addr = ip_addr << 8;
    }
  }

  if (type_search == NEXT)
    ip_addr++;

  ip_addr_len = 4;

#ifdef NEW_MIB
  cc = get_arp_entry(ip_addr, ip_addr_len, &arptab_entry);
#else
  cc = get_arp_entry(nl[N_ARPTAB].n_value, nl[N_ARPTAB_SIZE].n_value, 
		     ip_addr, ip_addr_len, &arptab_entry);
#endif

  /* If past end of arp table and exact, we fail */
  if ((cc == FALSE) && (type_search == EXACT))
    return(NULL);

  /* if failed (and type=NEXT implied */
  if (cc == FALSE) {
    interface++;
    goto again;
  }

  new_ip_addr = ntohl(arptab_entry.at_iaddr.s_addr);

  if ((type_search == EXACT) && (ip_addr != new_ip_addr))
    return(NULL);

  switch (arg) {
  case 1:
    sprintf(buffer,"atIfIndex.%d.1.%d.%d.%d.%d", interface,
	    ((new_ip_addr>>24) & 0xff), ((new_ip_addr>>16) & 0xff),
	    ((new_ip_addr>>8) & 0xff), (new_ip_addr & 0xff));
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, interface, NULL, NULL);
    oid_ptr = NULL;
    break;
  case 2:
    sprintf(buffer,"atPhysAddress.%d.1.%d.%d.%d.%d", interface,
	    ((new_ip_addr>>24) & 0xff), ((new_ip_addr>>16) & 0xff),
	    ((new_ip_addr>>8) & 0xff), (new_ip_addr & 0xff));
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
#if defined(SUNOS35) || defined(SUNOS40)
    os_ptr = make_octetstring(&arptab_entry.at_enaddr, 6);
#else
    os_ptr = make_octetstring(arptab_entry.at_enaddr, 6);
#endif
    vb_ptr = make_varbind(oid_ptr, OCTET_PRIM_TYPE, 0, 0, os_ptr, NULL);
    os_ptr = NULL;
    oid_ptr = NULL;
    break;
  case 3:
    sprintf(buffer,"atNetAddress.%d.1.%d.%d.%d.%d", interface,
	    ((new_ip_addr>>24) & 0xff), ((new_ip_addr>>16) & 0xff),
	    ((new_ip_addr>>8) & 0xff), (new_ip_addr & 0xff));
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    netaddr[0] = ((new_ip_addr >> 24) & 0xff);
    netaddr[1] = ((new_ip_addr >> 16) & 0xff);
    netaddr[2] = ((new_ip_addr >> 8) & 0xff);
    netaddr[3] = (new_ip_addr & 0xff);
    os_ptr = make_octetstring(netaddr, 4);
    vb_ptr = make_varbind(oid_ptr, IP_ADDR_PRIM_TYPE, 0, 0, os_ptr, NULL);
    os_ptr = NULL;
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

#ifdef NEW_MIB
extern int arp_fd;

int
get_arp_entry(unsigned long ip_addr,
	      int ip_addr_len,
	      struct arptab *arptab_entry)
{
    register struct arptab *at;
    int i, arptabsize, best_arptab_index;
    struct in_addr test_addr;
    struct in_addr best_ip_addr;
    struct gi_arg  gi_arg, *gp;

    test_addr.s_addr = htonl(ip_addr);

    if (arp_fd < 0) {
	if ((arp_fd = open(_PATH_ARP, O_RDONLY)) < 0) {
	    syslog(LOG_WARNING, gettxt(":129", "get_arp_entry: Open of %s failed: %m.\n"),
		_PATH_ARP);
	    return(FALSE);
	}
    }

    gi_arg.gi_size = 0;
    gi_arg.gi_where = (caddr_t)&gi_arg;

    if ((arptabsize = ioctl(arp_fd, STIOCGARPTAB, &gi_arg)) < 0) {
	syslog(LOG_WARNING, gettxt(":130", "get_arp_entry: STIOCGARPTAB: %m.\n"));
	(void) close(arp_fd);
	arp_fd = -1;
	return(FALSE);
    }
    arptabsize = (11 * arptabsize) / 10;
    at = (struct arptab *) malloc(arptabsize * sizeof(struct arptab));

    if (at == NULL) {
	syslog(LOG_WARNING, gettxt(":131", "get_arp_entry: Malloc failed: %m.\n"));
	return(FALSE);
    }

    gp = (struct gi_arg *)at;
    gp->gi_size = arptabsize;
    gp->gi_where = (caddr_t)at;

    if ((arptabsize = ioctl(arp_fd, STIOCGARPTAB, (char *)at)) < 0) {
	   syslog(LOG_WARNING, gettxt(":130", "get_arp_entry: STIOCGARPTAB: %m.\n"));
	   (void) free(at);
	   (void) close(arp_fd);
	   arp_fd = -1;
	   return(FALSE);
      }

    best_ip_addr.s_addr = 0xffffffff;
    best_arptab_index = -1;

    for (i = 0; i < arptabsize; i++) {

	if ((at[i].at_iaddr.s_addr != 0) && (at[i].at_flags & ATF_COM) &&
	    (((at[i].at_iaddr.s_addr >= test_addr.s_addr) && 
	    (at[i].at_iaddr.s_addr < best_ip_addr.s_addr) &&
	    (ip_addr_len != 5)) || 
	    ((at[i].at_iaddr.s_addr > test_addr.s_addr) && 
	    (at[i].at_iaddr.s_addr < best_ip_addr.s_addr)))) {

	    best_ip_addr.s_addr = at[i].at_iaddr.s_addr;
	    best_arptab_index = i;
	}
    } /* end of for i < hashsize loop */

    /* must've gotten last entry, hop to next class */
    if (best_ip_addr.s_addr == 0xffffffff) {
	(void) free(at);
	return(FALSE);
    }

    /* Copy found entry */
    i = best_arptab_index;
    arptab_entry->at_iaddr = at[i].at_iaddr;
#if defined(SUNOS35) || defined(SUNOS40) 
    bcopy(&at[i].at_enaddr, &arptab_entry->at_enaddr, 6);
#else
    bcopy(at[i].at_enaddr, arptab_entry->at_enaddr, 6);
#endif
    arptab_entry->at_timer = at[i].at_timer;
    arptab_entry->at_flags = at[i].at_flags;
    arptab_entry->at_hold = at[i].at_hold; /* don't use! */
    free(at);

    /* send back TRUE */
    return(TRUE);

}

#else
int get_arp_entry(off_t arptabaddr, 
		  off_t arptabsizeaddr,
		  unsigned long ip_addr,
		  int ip_addr_len,
		  struct arptab *arptab_entry)
{
  register struct arptab *at;
  int arptabsize;
  int i;
  struct in_addr test_addr;
  int best_arptab_index;
  struct in_addr best_ip_addr;

  test_addr.s_addr = htonl(ip_addr);
  
  if (arptabaddr == 0) {
    syslog(LOG_WARNING, gettxt(":132", "arptabaddr: symbol not in namelist.\n"));
    return(FALSE);
  }
  if (arptabsizeaddr == 0) {
    syslog(LOG_WARNING, gettxt(":133", "snmpd: arptabsizeaddr: symbol not in namelist.\n"));
    return(FALSE);
  }
  
  lseek(kmem, arptabsizeaddr, 0);
  if (read(kmem, &arptabsize, sizeof (arptabsize)) < 0) {
    syslog(LOG_WARNING, gettxt(":134", "get_arp_entry, arptabsize: %m.\n"));
    return(FALSE);
  }

  at = (struct arptab *)malloc( arptabsize*sizeof (struct arptab) );
  if (at == NULL) {
    syslog(LOG_WARNING, gettxt(":131", "get_arp_entry: Malloc failed: %m.\n"));
    return(FALSE);
  }

  lseek(kmem, arptabaddr, 0); /* just doing netaddr for now, add hostaddr when get chance */
  if (read(kmem, (char *) at, arptabsize*sizeof (struct arptab)) < 0) {
    syslog(LOG_WARNING, gettxt(":135", "get_arp_entry, arptab: %m.\n"));
    free(at);
    return(FALSE);
  }
  
  best_ip_addr.s_addr = 0xffffffff;
  best_arptab_index = -1;
  
  for (i = 0; i < arptabsize; i++) {
    if ((at[i].at_iaddr.s_addr != 0) &&
	(at[i].at_flags & ATF_COM) &&
	(((at[i].at_iaddr.s_addr >= test_addr.s_addr) && 
	  (at[i].at_iaddr.s_addr < best_ip_addr.s_addr) && (ip_addr_len != 5)) || 
	 ((at[i].at_iaddr.s_addr > test_addr.s_addr) && 
	  (at[i].at_iaddr.s_addr < best_ip_addr.s_addr)))) {
      best_ip_addr.s_addr = at[i].at_iaddr.s_addr;
      best_arptab_index = i;
    }
  } /* end of for i < hashsize loop */

  if (best_ip_addr.s_addr == 0xffffffff) { /* must've gotten last entry, hop to next class */
    free(at);
    return(FALSE);
  }

  /* Copy found entry */
  i = best_arptab_index;
  arptab_entry->at_iaddr = at[i].at_iaddr;
#if defined(SUNOS35) || defined(SUNOS40) 
  bcopy(&at[i].at_enaddr, &arptab_entry->at_enaddr, 6);
#else
  bcopy(at[i].at_enaddr, arptab_entry->at_enaddr, 6);
#endif
  arptab_entry->at_timer = at[i].at_timer;
  arptab_entry->at_flags = at[i].at_flags;
  arptab_entry->at_hold = at[i].at_hold; /* don't use! */
  free(at);

  /* send back TRUE */
  return(TRUE);
}
#endif

/* Operations to change the ARP table */

static struct atstate_s {
  int flags;
  int in_index;
  unsigned long in_ip_addr;
  int set_index;
  struct arptab arp;
} atstate;

#define GOTTEN		0x1
#define DELETE		0x2
#define NOTEXIST	0x4
#define PHYSADDR	0x8

#define clear(x)	bzero((char *)&(x), sizeof(x))

int var_at_test(OID var_name_ptr,
		OID in_name_ptr,
		unsigned int arg,
		ObjectSyntax *value)
{
  int interface;
  int i, cc;
  unsigned long ip_addr;
  int ip_addr_len;

  if (in_name_ptr->length != var_name_ptr->length + 6) {
    goto bad;
  }

  interface = in_name_ptr->oid_ptr[var_name_ptr->length];

  if (arpcheckinterface(interface) == FALSE) {
    goto bad;
  }

  if ((atstate.flags & GOTTEN) && atstate.in_index != interface) {
    goto bad;
  }

  atstate.in_index = interface;

  ip_addr = 0;
  for (i=var_name_ptr->length + 2; i < var_name_ptr->length + 6; i++) {
    ip_addr = (ip_addr << 8) + in_name_ptr->oid_ptr[i];
  }
  ip_addr_len = 4;

  if ((atstate.flags & GOTTEN) && ip_addr != atstate.in_ip_addr) {
    goto bad;
  }

  if (!(atstate.flags & GOTTEN)) {
#ifdef NEW_MIB
    cc = get_arp_entry(ip_addr, ip_addr_len, &atstate.arp);
#else
    cc = get_arp_entry(nl[N_ARPTAB].n_value, nl[N_ARPTAB_SIZE].n_value,
		       ip_addr, ip_addr_len, &atstate.arp);
#endif
    if (cc == FALSE ||
        ntohl(atstate.arp.at_iaddr.s_addr) != ip_addr) {
      atstate.flags |= NOTEXIST;
      clear(atstate.arp);
      atstate.arp.at_iaddr.s_addr = htonl(ip_addr);
    }
    atstate.flags |= GOTTEN;
    atstate.in_ip_addr = ip_addr;
  }

  switch (arg) {
  case 1:
    if (value->type == NULL_TYPE) {
      atstate.flags |= DELETE;
      break;
    }
    if (arpcheckinterface(value->sl_value) == FALSE) {
      goto bad;
    }
    atstate.set_index = value->sl_value;
    break;
  case 2:
    if (value->type == NULL_TYPE) {
      atstate.flags |= DELETE;
      break;
    }
    if (value->os_value->length != sizeof(atstate.arp.at_enaddr)) {
      goto bad;
    }
    bcopy((char *)value->os_value->octet_ptr, atstate.arp.at_enaddr,
      value->os_value->length);
    atstate.flags |= PHYSADDR;
    break;
  case 3:
    if (value->type == NULL_TYPE) {
      atstate.flags |= DELETE;
      break;
    }
    if (value->os_value->length != sizeof(atstate.arp.at_iaddr)) {
      goto bad;
    }
    bcopy((char *)value->os_value->octet_ptr, &atstate.arp.at_iaddr,
          value->os_value->length);
    break;
  default:
    goto bad;
  }
  return(TRUE);

bad:
  clear(atstate);
  return(FALSE);
}

int var_at_set(OID var_name_ptr,
	       OID in_name_ptr,
	       unsigned int arg,
	       ObjectSyntax *value)
{
  int cc;
  
  if (atstate.flags == 0) {
    return(TRUE);
  }
  if (!(atstate.flags & DELETE)) {
    if ((atstate.flags & (NOTEXIST|PHYSADDR)) == NOTEXIST) {
      clear(atstate);
      return(FALSE);
    }
  }

  cc = atsettable();  
  clear(atstate);

  return(cc);
}

int atsettable(void)
{
#ifdef BSD
  struct arpreq ar;
  struct sockaddr_in *sin;
  int s;
  int ret;

  bzero((caddr_t)&ar, sizeof(struct arpreq));
  sin = (struct sockaddr_in *)&ar.arp_pa;
  sin->sin_family = AF_INET;
  bcopy((caddr_t)atstate.arp.at_iaddr, (caddr_t)&sin->sin_addr,
        sizeof(sin->sin_addr));
  ar.arp_ha.sa_family = AF_UNSPEC;
  bcopy((caddr_t)atstate.arp.at_enaddr, (caddr_t)ar.arp_ha.sa_data,
        sizeof(atstate.arp.at_enaddr));
  ar.arp_flags = 0;

  if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    clear(atstate);
    return(FALSE);
  }
  ret = ioctl(s, (atstate.flags & DELETE) ? SIOCDARP : SIOCSARP, (caddr_t)&ar);
  close(s);
  clear(atstate);
  if (ret < 0) {
    return(FALSE);
  }
  return(TRUE);
#endif
#if defined(SVR3) || defined(SVR4)
  struct strioctl strioc;
  struct sockaddr_in *sin;
  struct arpreq ar;
  int fd;
  int ret;

  memset((caddr_t)&ar, '\0', sizeof(struct arpreq));
  sin = (struct sockaddr_in *)&ar.arp_pa;
  sin->sin_family = AF_INET;
  bcopy((caddr_t)&atstate.arp.at_iaddr, (caddr_t)&sin->sin_addr,
         sizeof(sin->sin_addr));
  ar.arp_ha.sa_family = AF_UNSPEC;
  bcopy((caddr_t)atstate.arp.at_enaddr, (caddr_t)ar.arp_ha.sa_data,
         sizeof(atstate.arp.at_enaddr));
  ar.arp_flags = 0;

  if ((fd = open(_PATH_ARP, 0)) < 0) {
    clear(atstate);
    return(FALSE);
  }
  strioc.ic_cmd = (atstate.flags & DELETE) ? SIOCDARP : SIOCSARP;
  strioc.ic_timout = -1;
  strioc.ic_len = sizeof(struct arpreq);
  strioc.ic_dp = (caddr_t)&ar;
  ret = ioctl(fd, I_STR, (caddr_t) & strioc);
  close(fd);
  clear(atstate);
  if (ret < 0) {
    return(FALSE);
  }
  return(TRUE);
#endif
#if !defined(BSD) && !defined(SVR3) && !defined(SVR4)
  clear(atstate);
  return(FALSE);
#endif
}

arpcheckinterface(interface)
    int interface;
{
  int cc;
  int if_num;
#if defined(BSD) || defined(NEW_MIB)
  struct ifnet ifnet_entry;
#endif
#if (defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
  struct ip_provider prov_entry;
#endif
  char ifname[16];
#if (defined(SUNOS35) || defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif

  if_num = interface;
#ifdef BSD
  cc = get_if_entry(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname,
	      &ifaddr_entry);
#endif
#if (defined(SVR3) || defined(SVR4)) && !defined(NEW_MIB)
  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
	      &ifaddr_entry);
#endif
#ifdef NEW_MIB
  cc = get_if_entry(&if_num, &ifnet_entry, ifname, &ifaddr_entry);
#endif

  if (cc == FALSE) {
    return(FALSE);
  }
  if (interface != if_num) {
    return(FALSE);
  }
  /*
   *  fix bug where get arp table entries for the loopback interface
   *  JDC 1/28/89
   */ 
#ifdef BSD
  if (strcmp(ifname, "lo") == 0) {
#endif
#if defined(SVR3) || defined(SVR4)
  if (strcmp(ifname, "lo0") == 0) {
#endif
    return(FALSE);
  }
  return(TRUE);
}
