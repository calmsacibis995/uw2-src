/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nm:common/cmd/cmd-nm/usr.sbin/snmp/in.snmpd/v_ifEntry.c	1.2"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nm/usr.sbin/snmp/in.snmpd/v_ifEntry.c,v 1.3 1994/05/27 16:46:15 rbell Exp $"

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
static char SNMPID[] = "@(#)v_ifEntry.c	2.15.1.2 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/*
 * Revision History:
 *  1/31/89 JDC
 *  Amended copyright notice
 *
 *  Added SUN3 support for ifPhysAddress via ioctl call
 * 
 *  2/4/89 JDC
 *  Changed references from "gotone" to "snmpd"
 *
 *  Added support for ifLastChange
 *
 *  2/6/89 JDC
 *  Added counter on strncmp 2 places where it was missing
 *
 *  4/24/89 JDC
 *  Turned off some of the verbose debug output to cut down on the noise
 *
 *  5/17/89 KWK
 *  changed SUN3 to SUNOS35 and SUN4 to SUNOS40, since that's what they really 
 *  meant and were causing confusion
 *
 *  11/8/89 JDC
 *  Make it print pretty via tgrind
 *
 *  12/08/89 JDC
 *  Fix bug in get_if_entry for sun os 3.5 reported by xxx
 *
 *  01/10/90 JDC
 *  Fix type on ifSpeed in var_ifEntry.c as reported by xxx
 *
 *  05/28/90 JDC
 *  Fix botched fix on ifSpeed in var_ifEntry.c [sigh]
 *
 */

/*
 *  if entries for snmpd
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
#include <stdio.h>
#if defined(SVR3) || defined(SVR4)
#include <fcntl.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/time.h>	/* added to support ifLastChange JDC 2/4/89 */
#include <net/if.h>
#if defined(M_UNIX) || defined(SVR4)
#include <sys/dlpi.h>
#ifndef SVR4
#include <sys/macstat.h>
#else
#include <sys/dlpi_ether.h>
#endif
#endif
#include <netinet/in.h>
#include <sys/ioctl.h>
#ifdef SUNOS35
#include <sys/time.h>
#include <net/nit.h>
#endif
#ifndef SUNOS35
#include <netinet/in_var.h>
#endif
#if defined(SVR3) || defined(SVR4)
#ifdef PSE
#include <common.h>
#endif
#include <sys/stream.h>
#include <sys/stropts.h>
#include <net/route.h>
#include <netinet/ip_str.h>
#endif
#include <string.h>
#include <syslog.h>

#include "snmp.h"
#include "snmpuser.h"
#include "snmpd.h"

#define FALSE 0
#define TRUE 1

VarBindList *get_next_class();
OctetString *get_physaddr_from_name();

#define IF_TYPE 0
#define IF_SPEED 1

#if defined(SVR3) && !defined(M_UNIX)
#define MACIOC(x)       	(('M' << 8) | (x)) 
#define MACIOC_GETADDR		MACIOC(8)	/*get Physical Address*/
#define MACIOC_GETIFSTAT		MACIOC(7)	/*dump statistics*/
#endif

#ifdef SVR4

#define MAX_MAPPINGS 64
struct n_map {
	char ifname[IFNAMSIZ];
	char devname[128];
};
struct n_map ifmap[MAX_MAPPINGS];
static int n_mappings = 0;

static char *
cskip(p)
	char *p;
{
	if (!p)
		return;
	while (*p && *p != ':' && *p != '\n')
		++p;
	if (*p)
		*p++ = '\0';
	return p;
}

char *
svr4_mapname(s)
	char *s;
{
	static FILE *iifp = (FILE *)0;
	char *prefix, *unit, *dev;
	static char buf[256];
	static char tmp[256];
	char *p;
	int i = 0;

	if (n_mappings) {
		while (i < n_mappings) {
			if (strcmp(ifmap[i].ifname, s) == 0)
				return ifmap[i].devname;
			i++;
		}
	}
	if (!iifp) {
		iifp = fopen("/etc/confnet.d/inet/interface", "r");
		if (!iifp)
			return s;
	}
	(void) rewind(iifp);
	while ((fgets(buf, sizeof(buf), iifp)) != (char *)0) {
		if (buf[0] == '#' || buf[0] == '\0')
			continue;

		p = prefix = buf;	/* prefix is first field */
		p = cskip(p);		/* unit is second */
		unit = p;
		p = cskip(p);		/* skip third */
		p = cskip(p);
		dev = p;		/* dev is fourth */
		p = cskip(p);		/* null terminate */

		strcpy(tmp, prefix);
		strcat(tmp, unit);
		if (strcmp(s, tmp) == 0) {
			if (n_mappings < MAX_MAPPINGS - 1) {
				strcpy(ifmap[n_mappings].devname, dev);
				n_mappings++;
			}
			return dev;
		}
	}
	return s;
}

get_ifstats(ifname, ifsp, ifnp)
	char *ifname;
	struct ifstats *ifsp;
	struct ifstats *ifnp;
{
	int fd, ret;
	char devname[32];
	DL_mib_t ms;
	struct strioctl strioc;
	strcpy(devname,svr4_mapname(ifname));
	if (strcmp(devname, "/dev/loop") == 0)	/* XXX fix llcloop and 
						 * take this out */
		goto oy;
	if ((fd = open(devname, O_RDWR)) >= 0) {
		strioc.ic_len = sizeof(ms);
		strioc.ic_timout = 0;
		strioc.ic_dp = (char *) &ms;
		strioc.ic_cmd = DLIOCGMIB;
		ret = ioctl(fd, I_STR, &strioc);
		close(fd);
		if ((ret < 0) || (strioc.ic_len != sizeof(ms)))
			goto oy;
		else {
			/*
			 * copy the stuff from the USL way to our way
			 */
			ifsp->iftype = ms.ifType;
			ifsp->ifspeed = ms.ifSpeed;
			ifsp->ifinoctets = ms.ifInOctets;
			ifsp->ifoutoctets = ms.ifOutOctets;
			ifsp->ifinucastpkts = ms.ifInUcastPkts;
			ifsp->ifinnucastpkts = ms.ifInNUcastPkts;
			ifsp->ifoutucastpkts = ms.ifOutUcastPkts;
			ifsp->ifoutnucastpkts = ms.ifOutNUcastPkts;
			ifsp->ifindiscards = ms.ifInDiscards;
			ifsp->ifs_ierrors = ms.ifInErrors;
			ifsp->ifs_oerrors = ms.ifOutErrors;
			ifsp->ifinunkprotos = ms.ifInUnknownProtos;
			ifsp->ifoutdiscards = ms.ifOutDiscards;
			return (1);
		}
	} else {
oy:
		if (ifnp) {
			lseek(kmem, (off_t)ifnp, 0);
			if (read(kmem, ifsp, sizeof(*ifsp)) < 0) {
				syslog(LOG_WARNING, gettxt(":168", "ifstats read: %m.\n"));
				return 0;
			}
			return (1);
		 }
	}
	return 0;
}

#endif

VarBindList *
var_if_num_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
#ifdef BSD
  struct ifnet ifnet_entry;
#endif
#if defined(SVR3) || defined(SVR4)
  struct ip_provider prov_entry;
#endif
  char ifname[16];
#if defined(SUNOS35) || defined(SVR3) || defined(SVR4)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif
  char buffer[256];
  
  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);
  
  /* see if get next and fully qualified */
  if ((type_search == NEXT) && (in_name_ptr->length >= (var_name_ptr->length + 1)))
    return(get_next_class(var_next_ptr));

  if_num = 255;  /* start with a large number of interfaces */
  /* will get back actual number (should be less than this */
  /* note:  ok not to check the returned value here */
#ifdef BSD
  cc = get_if_entry(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname, &ifaddr_entry);
#endif
#if defined(SVR3) || defined(SVR4)
  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname, &ifaddr_entry);
#endif

  /*
   * check that the OID exactly matches what we expect
   */
  if (type_search == EXACT && 
      *(in_name_ptr->oid_ptr + (in_name_ptr->length - 1)) != 0)
    return(NULL);
  sprintf(buffer,"ifNumber.0");
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, if_num, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_index_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
#ifdef BSD
  struct ifnet ifnet_entry;
#endif
#if defined(SVR3) || defined(SVR4)
  struct ip_provider prov_entry;
#endif
  char ifname[16];
#if defined(SUNOS35) || defined(SVR3) || defined(SVR4)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif
  char buffer[256];
  
  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);
  
  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
  
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;
  
#ifdef BSD
  cc = get_if_entry(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname,
		    &ifaddr_entry);
#endif
#if defined(SVR3) || defined(SVR4)
  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);
#endif
  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }
  
  sprintf(buffer,"ifIndex.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, if_num, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_name_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OctetString *os_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
#ifdef BSD
  struct ifnet ifnet_entry;
#endif
#if defined(SVR3) || defined(SVR4)
  struct ip_provider prov_entry;
#endif
  char ifname[16];
#if defined(SUNOS35) || defined(SVR3) || defined(SVR4)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif
  char buffer[256];
  
  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);
  
  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
  
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;
  
#ifdef BSD
  cc = get_if_entry(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname,
		    &ifaddr_entry);
#endif
#if defined(SVR3) || defined(SVR4)
  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);
#endif
  
  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }
  
  sprintf(buffer,"ifDescr.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
#ifdef BSD
  sprintf(buffer,"%s%d", ifname, ifnet_entry.if_unit);
  os_ptr = make_octet_from_text(buffer);
#endif
#if defined(SVR3) || defined(SVR4)
  os_ptr = make_octet_from_text((unsigned char *)ifname);
#endif
  vb_ptr = make_varbind(oid_ptr, DisplayString, 0, 0, os_ptr, NULL);
  oid_ptr = NULL;
  os_ptr = NULL;
  return(vb_ptr);
}


VarBindList
*var_if_mtu_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
#ifdef BSD
  struct ifnet ifnet_entry;
#endif
#if defined(SVR3) || defined(SVR4)
  struct ip_provider prov_entry;
#endif
  char ifname[16];
#if defined(SUNOS35) || defined(SVR3) || defined(SVR4)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

#ifdef BSD
  cc = get_if_entry(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname,
		    &ifaddr_entry);
#endif
#if defined(SVR3) || defined(SVR4)
  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);
#endif

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  sprintf(buffer,"ifMtu.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
#ifdef BSD
  vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, ifnet_entry.if_mtu, NULL, NULL);
#endif
#if defined(SVR3) || defined(SVR4)
  vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, prov_entry.if_maxtu, NULL, NULL);
#endif
  oid_ptr = NULL;
  return(vb_ptr);
}



#if defined(SUNOS35) || defined(SVR4) || defined(TCP40)
/*#ifdef BOGUS*/
VarBindList
*var_if_physaddr_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OctetString *os_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
#ifdef BSD
  struct ifnet ifnet_entry;
#endif
#if defined(SVR3) || defined(SVR4)
  struct ip_provider prov_entry;
#endif
  char ifname[16];
#if defined(SUNOS35) || defined(SVR3) || defined(SVR4)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif
  char buffer[256];
  unsigned long speed;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

#ifdef BSD
  cc = get_if_entry(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname,
		    &ifaddr_entry);
#endif
#if defined(SVR3) || defined(SVR4)
  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);
#endif
  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

  /* mach. dependent routine for each interface */
#ifdef SUNOS35
  os_ptr = get_physaddr_from_name(ifname, ifnet_entry.if_unit); 
#endif
#if defined(SVR4) || defined(TCP40)
  os_ptr = get_physaddr_from_name(ifname); 
#endif

  if (os_ptr == NULL) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);             /* Signal failure */
  }
  sprintf(buffer,"ifPhysAddress.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, OCTET_PRIM_TYPE, 0, 0, os_ptr, NULL);
  os_ptr = NULL;
  oid_ptr = NULL;
  return(vb_ptr);
}
#endif


OctetString *
get_physaddr_from_name(name)
    char *name;
{
  int fd, ret;
  char devname[32];
  struct strioctl strioc;
  unsigned char eaddr[6];

  strcpy(devname,svr4_mapname(name));
  if (strcmp(devname, "/dev/loop") == 0)	/* XXX fix loopback */
	goto oy;
  if ((fd = open(devname, O_RDWR)) >= 0) {
    strioc.ic_len = 6;
    strioc.ic_timout = 0;
    strioc.ic_dp = (char *)eaddr;
    strioc.ic_cmd = DLIOCGENADDR;
    ret  = ioctl(fd, I_STR, &strioc);
    close(fd);
    if (ret < 0)
	    return(make_octetstring((unsigned char *)"", 0));
    else
	    return(make_octetstring((unsigned char *)(&eaddr[0]), 6));
  } else
oy:
	  return(make_octetstring((unsigned char *)"", 0));
}

VarBindList
*var_if_adminstatus_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
#ifdef BSD
  struct ifnet ifnet_entry;
#endif
#if defined(SVR3) || defined(SVR4)
  struct ip_provider prov_entry;
#endif
  char ifname[16];
#if defined(SUNOS35) || defined(SVR3) || defined(SVR4)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif
  char buffer[256];
  unsigned long status;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

#ifdef BSD
  cc = get_if_entry(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname,
		    &ifaddr_entry);
#endif
#if defined(SVR3) || defined(SVR4)
  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);
#endif

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#ifdef BSD
  if ((ifnet_entry.if_flags & IFF_UP) == 0) 
    status = 2;			/* down */
  else if (((ifnet_entry.if_flags & IFF_RUNNING) != 0) || 
    (strncmp(ifname,"lo0",3) == 0)) /* mod 2/6/89 JDC */
    status = 1;
  else 
    status = 3;
#endif
#if defined(SVR3) || defined(SVR4)
  if ((prov_entry.if_flags & IFF_UP) == 0) 
    status = 2;			/* down */
  else 
    status = 1;			/* up */
#endif


  sprintf(buffer,"ifAdminStatus.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, status, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}


int
var_if_adminstatus_test(var_name_ptr, in_name_ptr, arg, value)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     ObjectSyntax *value;
{
  int if_num = 0;
  int cc;
#ifdef BSD
  struct ifnet ifnet_entry;
#endif
#if defined(SVR3) || defined(SVR4)
  struct ip_provider prov_entry;
#endif
  char ifname[16];
#if defined(SUNOS35) || defined(SVR3) || defined(SVR4)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif
  char buffer[256];
  unsigned long status;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if (in_name_ptr->length != (var_name_ptr->length + 1))
    return(FALSE);

  /* Now find out which interface they are interested in */
  if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  
#ifdef BSD
  cc = get_if_entry(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname,
		    &ifaddr_entry);
#endif
#if defined(SVR3) || defined(SVR4)
  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);
#endif

  if (cc == FALSE) {
    return(FALSE);		/* No such interface */
  }

  if ((value->sl_value < 1) || (value->sl_value > 2)) 
    return(FALSE);

  return(TRUE);
}

int
var_if_adminstatus_set(var_name_ptr, in_name_ptr, arg, value)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     ObjectSyntax *value;
{
  int if_num = 0;
  int cc;
#ifdef BSD
  struct ifnet ifnet_entry;
#endif
#if defined(SVR3) || defined(SVR4)
  struct ip_provider prov_entry;
#endif
  char ifname[16];
#if defined(SUNOS35) || defined(SVR3) || defined(SVR4)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif
  unsigned long status;
  struct ifreq ifr;
  int s;
#if defined(SVR3) || defined(SVR4)
  struct strioctl ioc;
#endif

  struct timeval tv;
  struct timezone tz;
  long timenow;
  int t1, t2;

extern struct timeval global_tv;
extern struct timezone global_tz;

  long old_state;
  long new_state;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if (in_name_ptr->length != (var_name_ptr->length + 1))
    return(FALSE);

  /* Now find out which interface they are interested in */
  if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  
#ifdef BSD
  cc = get_if_entry(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname,
		    &ifaddr_entry);
#endif
#if defined(SVR3) || defined(SVR4)
  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);
#endif

  if (cc == FALSE) 
    return(FALSE);		/* No such interface */

  if ((value->sl_value < 1) || (value->sl_value > 2)) 
    return(FALSE);

#ifdef BSD
  sprintf(ifr.ifr_name, "%s%d", ifname, ifnet_entry.if_unit);
#endif
#if defined(SVR3) || defined(SVR4)
  sprintf(ifr.ifr_name, "%s", ifname);
#endif

#ifdef BSD
  s = socket(AF_INET, SOCK_DGRAM, 0);
#endif
#ifdef SVR3
  s = t_open("/dev/inet/udp", O_RDWR, (struct t_info *)0);
#endif
#ifdef SVR4
  s = t_open("/dev/udp", O_RDWR, (struct t_info *)0);
#endif
  if (s < 0) {
    return(FALSE);
  }

#ifdef BSD
  if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0) {
    perror("snmpd:  siocgifflags");
    close(s);
    return(FALSE);
  }
#endif
#if defined(SVR3) || defined(SVR4)
  ioc.ic_cmd = SIOCGIFFLAGS;
  ioc.ic_timout = -1;
  ioc.ic_len = sizeof(ifr);
  ioc.ic_dp = (char *) &ifr;
  if (ioctl(s, I_STR, (char *) &ioc) < 0) {
    syslog(LOG_WARNING, gettxt(":169", "SIOCGIFFLAGS: %m.\n"));
    t_close(s);
    return(FALSE);
  }
#endif
  /* save the old state for later compare */
  old_state = ifr.ifr_flags & IFF_UP;

  if (value->sl_value == 1)
    ifr.ifr_flags |= IFF_UP;
  else
    ifr.ifr_flags &= ~IFF_UP;

  new_state = ifr.ifr_flags & IFF_UP;

#ifdef BSD
  if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&ifr) < 0) {
    close(s);
    perror("snmpd:  siocgifflags");
    return(FALSE);
  }
#endif
#if defined(SVR3) || defined(SVR4)
  ioc.ic_cmd = SIOCSIFFLAGS;
  ioc.ic_timout = -1;
  ioc.ic_len = sizeof(ifr);
  ioc.ic_dp = (char *) &ifr;
  if (ioctl(s, I_STR, (char *) &ioc) < 0) {
    syslog(LOG_WARNING, gettxt(":170", "SIOCSIFFLAGS: %m.\n"));
    t_close(s);
    return(FALSE);
  }
#endif

/*
	save the current time in timeticks into the array which holds
	the time of the last change 
	zero whereas interfaces are numbered from 1
*/
  /*  first, get the current time */
   gettimeofday(&tv, &tz);

  /*  now, convert to timeticks */
    t1 = ((tv.tv_sec - global_tv.tv_sec) * 100);
    t2 = ((tv.tv_usec - global_tv.tv_usec) / 10000);
    timenow = ((tv.tv_sec - global_tv.tv_sec) * 100) +
      ((tv.tv_usec - global_tv.tv_usec) / 10000);

#ifdef BSD
  close(s);
#endif
#if defined(SVR3) || defined(SVR4)
  t_close(s);
#endif
  return(TRUE);
}




VarBindList
*var_if_operstatus_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
#ifdef BSD
  struct ifnet ifnet_entry;
#endif
#if defined(SVR3) || defined(SVR4)
  struct ip_provider prov_entry;
#endif
  char ifname[16];
#if defined(SUNOS35) || defined(SVR3) || defined(SVR4)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif
  char buffer[256];
  unsigned long status;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

#ifdef BSD
  cc = get_if_entry(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname,
		    &ifaddr_entry);
#endif
#if defined(SVR3) || defined(SVR4)
  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);
#endif

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }


#ifdef BSD
  if ((ifnet_entry.if_flags & IFF_UP) == 0) 
    status = 2;			/* down */
  else if (((ifnet_entry.if_flags & IFF_RUNNING) != 0) || 
    (strncmp(ifname,"lo0",3) == 0)) /* mod 2/6/89 JDC */
    status = 1;
  else 
    status = 3;
#endif
#if defined(SVR3) || defined(SVR4)
  if ((prov_entry.if_flags & IFF_UP) == 0) 
    status = 2;			/* down */
  else 
    status = 1;
#endif


  sprintf(buffer,"ifOperStatus.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, status, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_up_time_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int number_of_interfaces_found;
  int cc;
#ifdef BSD
  struct ifnet ifnet_entry;
#endif
#if defined(SVR3) || defined(SVR4)
  struct ip_provider prov_entry;
#endif
  char ifname[16];
#if defined(SUNOS35) || defined(SVR3) || defined(SVR4)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif

  char buffer[256];
  unsigned long status;


  struct timeval tv;
  struct timezone tz;
  long lasttime;
  int t1, t2;

extern struct timeval global_tv;
extern struct timezone global_tz;

  /* Check that an exact search only has exactly one sub-id field */
  /* that field to communicate if number of probe */
  /* if it doesn't, it isn't exact */
  /* note that we are assuming that the number of interfaces will fit in */
  /* a single byte */

  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after
var_name */
   
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  /* now, find out how many interfaces we have */
  number_of_interfaces_found = 255;
#ifdef BSD
  /*  note:  ok not to check returned value here  */
  cc = get_if_entry(nl[N_IFNET].n_value, &number_of_interfaces_found, &ifnet_entry, ifname, &ifaddr_entry);
#endif
#if defined(SVR3) || defined(SVR4)
  cc = get_if_entry(nl[N_PROVIDER].n_value, &number_of_interfaces_found, &prov_entry, ifname, &ifaddr_entry);
#endif

  /* if we have too many, punt */
  if ((number_of_interfaces_found > MAXINTERFACES) || 
	(if_num <= 0) ||
	(if_num > number_of_interfaces_found)) {
    if (number_of_interfaces_found > MAXINTERFACES) {

	syslog(LOG_WARNING, gettxt(":171", "var_if_up_time_get: too many interfaces -- recompile with a larger number for MAXINTERFACES.\n"));
    }
    if (type_search == EXACT) {
       return(NULL);
    }
    else {
       return(get_next_class(var_next_ptr));
    }
  }
  else {
  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname, &ifaddr_entry);

     lasttime = prov_entry.if_lastchange;
     /*
      * lasttime is in HZ -- convert to hundredths and subtract global_tv
      */
     {
       struct timeval tv;
       tv.tv_sec = lasttime / HZ;
       tv.tv_usec = (lasttime % HZ) * (1000000 / HZ);
       lasttime = ((tv.tv_sec - global_tv.tv_sec) * 100) +
                  ((tv.tv_usec - global_tv.tv_usec) / 10000);
	if (lasttime < 0)
		lasttime = 0;
     }
    sprintf(buffer, "ifLastChange.%d", if_num);
    oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
    vb_ptr = make_varbind(oid_ptr, TIME_TICKS_TYPE, 0, lasttime, NULL, NULL);
    oid_ptr = NULL;
    return(vb_ptr);
  }
}



VarBindList
*var_if_inerrors_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
#ifdef BSD
  struct ifnet ifnet_entry;
#endif
#if defined(SVR3) || defined(SVR4)
  struct ip_provider prov_entry;
  struct ifstats ifstats;
#endif
  char ifname[16];
#if defined(SUNOS35) || defined(SVR3) || defined(SVR4)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

#ifdef BSD
  cc = get_if_entry(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname,
		    &ifaddr_entry);
#endif
#if defined(SVR3) || defined(SVR4)
  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);
#endif

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#if defined(SVR3) || defined(SVR4)
#if defined(M_UNIX) || defined(M_XENIX) || defined(TCP40) || defined(SVR4)
    if (!get_ifstats(ifname,&ifstats, prov_entry.ia.ia_ifa.ifa_ifs))
      ifstats.ifs_ierrors = 0;
#else
  if (prov_entry.ia.ia_ifa.ifa_ifs) {
    lseek(kmem, prov_entry.ia.ia_ifa.ifa_ifs, 0);
    if (read(kmem, &ifstats, sizeof(struct ifstats)) < 0) {
      perror("snmpd:  var_if_inerrors_get, ifstats");
      return(NULL);
    }
  }
  else {
    ifstats.ifs_ierrors = 0;
  }
#endif
#endif
  sprintf(buffer,"ifInErrors.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
#ifdef BSD
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ifnet_entry.if_ierrors, 0, NULL, NULL);
#endif
#if defined(SVR3) || defined(SVR4)
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ifstats.ifs_ierrors, 0, NULL, NULL);
#endif
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_outerrors_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
#ifdef BSD
  struct ifnet ifnet_entry;
#endif
#if defined(SVR3) || defined(SVR4)
  struct ip_provider prov_entry;
  struct ifstats ifstats;
#endif
  char ifname[16];
#if defined(SUNOS35) || defined(SVR3) || defined(SVR4)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

#ifdef BSD
  cc = get_if_entry(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname,
		    &ifaddr_entry);
#endif
#if defined(SVR3) || defined(SVR4)
  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);
#endif

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#if defined(SVR3) || defined(SVR4)
#if defined(M_UNIX) || defined(M_XENIX) || defined(TCP40) || defined(SVR4)
    if (!get_ifstats(ifname,&ifstats, prov_entry.ia.ia_ifa.ifa_ifs))
      ifstats.ifs_oerrors = 0;
#else
  if (prov_entry.ia.ia_ifa.ifa_ifs) {
    lseek(kmem, prov_entry.ia.ia_ifa.ifa_ifs, 0);
    if (read(kmem, &ifstats, sizeof(struct ifstats)) < 0) {
      perror("snmpd:  var_if_outerrors_get, ifstats");
      return(NULL);
    }
  }
  else {
    ifstats.ifs_oerrors = 0;
  }
#endif
#endif
  sprintf(buffer,"ifOutErrors.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
#ifdef BSD
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ifnet_entry.if_oerrors, 0, NULL, NULL);
#endif
#if defined(SVR3) || defined(SVR4)
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ifstats.ifs_oerrors, 0, NULL, NULL);
#endif
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_outqlen_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
#ifdef BSD
  struct ifnet ifnet_entry;
#endif
#if defined(SVR3) || defined(SVR4)
  struct ip_provider prov_entry;
  queue_t queue;
  mblk_t mb;
  mblk_t *mbp;
  int outqlen;
#endif
  char ifname[16];
#if defined(SUNOS35) || defined(SVR3) || defined(SVR4)
  struct sockaddr_in ifaddr_entry;
#else
  union {
    struct ifaddr ifa;
    struct in_ifaddr in;
  } ifaddr_entry;
#endif
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

#ifdef BSD
  cc = get_if_entry(nl[N_IFNET].n_value, &if_num, &ifnet_entry, ifname,
		    &ifaddr_entry);
#endif
#if defined(SVR3) || defined(SVR4)
  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);
#endif

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#if defined(SVR3) || defined(SVR4)
  outqlen = 0;
  lseek(kmem, (off_t)prov_entry.qbot, 0);
  if (read(kmem, &queue, sizeof(queue_t)) < 0) {
      syslog(LOG_WARNING, gettxt(":172", "var_if_outqlen_get, queue: %m.\n"));
      return(NULL);
  }
  mbp = queue.q_first;
  while (mbp != NULL) {
    lseek(kmem, (off_t)mbp, 0);
    if (read(kmem, &mb, sizeof(mblk_t)) < 0) {
      syslog(LOG_WARNING, gettxt(":173", "var_if_outqlen_get, mb: %m.\n"));
      return(NULL);
    }
    outqlen++;
    mbp = mb.b_next;
  }
#endif
  sprintf(buffer,"ifOutQLen.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
#ifdef BSD
  vb_ptr = make_varbind(oid_ptr, GAUGE_TYPE, ifnet_entry.if_snd.ifq_len, 0, NULL, NULL);
#endif
#if defined(SVR3) || defined(SVR4)
  vb_ptr = make_varbind(oid_ptr, GAUGE_TYPE, outqlen, 0, NULL, NULL);
#endif
  oid_ptr = NULL;
  return(vb_ptr);
}

#ifdef BSD
/* Return the generic if stats for the IP interface listed. */
get_if_entry(ifnetaddr, if_num, ifnet_entry, ifname, ifaddr_entry)
     off_t ifnetaddr;
     int *if_num;
     struct ifnet *ifnet_entry; 
     char *ifname;
#ifdef SUNOS35
     struct sockaddr_in *ifaddr_entry;
#else
     struct ifaddr *ifaddr_entry;
#endif
{
  int i;
  off_t ifaddraddr;
  
  if (ifnetaddr == 0) {
    syslog(LOG_WARNING, gettxt(":174", "ifnet: Symbol not defined.\n");
    return(FALSE);
  }
  
  lseek(kmem, ifnetaddr, 0);
  if (read(kmem, &ifnetaddr, sizeof(ifnetaddr)) < 0) {
    syslog(LOG_WARNING, gettxt(":175", "get_if_entry: ifnetaddr: %m.\n"));
    return(FALSE);
  }
  
  if (ifnetaddr == NULL)  
    return(FALSE);
  
  i = 0;
  do {
    if (ifnetaddr == NULL) {
      *if_num = i;  /* cheating for determining number of interfaces*/
      return(FALSE);
    }
    
    lseek(kmem, ifnetaddr, 0);
    if (read(kmem, ifnet_entry, sizeof (struct ifnet)) < 0) {
    	syslog(LOG_WARNING, gettxt(":175", "get_if_entry: ifnetaddr: %m.\n"));
      return(FALSE);
    }
    
    lseek(kmem, (off_t)ifnet_entry->if_name, 0);
    if (read(kmem, ifname, 16) < 0) {
      syslog(LOG_WARNING, gettxt(":176", "get_if_entry: ifnet_entry->if_name: %m.\n"));
      return(FALSE);
    }
    ifname[15] = '\0';
    
#ifdef SUNOS35
    /* copy the sockaddr_in structure */
    bcopy(&ifnet_entry->if_addr, ifaddr_entry, sizeof(struct sockaddr_in));
    if (ifnet_entry->if_addr.sa_family == AF_INET)
      i++;
#else
    ifaddraddr = (off_t) ifnet_entry->if_addrlist;
    
    if (ifaddraddr) {
      /* now find an internet address on the interface, if any */
      do {
	lseek(kmem, ifaddraddr, 0);
	if (read(kmem, ifaddr_entry, sizeof(struct ifaddr)) < 0) {
	  syslog(LOG_WARNING, gettxt(":177", "get_if_entry: ifaddr_entry: %m.\n");
	  return(FALSE);
	}
	ifaddraddr = (off_t)ifaddr_entry->ifa_next;
      } while ((ifaddraddr) && (ifaddr_entry->ifa_addr.sa_family != AF_INET));
      
      /* If this has an IP addr, then this counts as an IP interface.  */
      if (ifaddr_entry->ifa_addr.sa_family == AF_INET)
	i++; /* We've got an IP interface */
    }
#endif
    
    ifnetaddr = (off_t)ifnet_entry->if_next;
  } while (i < *if_num);

  return(TRUE);
}
#endif /* BSD */
#if defined(SVR3) || defined(SVR4)
/* Return the generic if stats for the IP interface listed. */
get_if_entry(provaddr, if_num, prov_entry, ifname, ifaddr_entry)
     off_t provaddr;
     int *if_num;
     struct ip_provider *prov_entry; 
     char *ifname;
     struct sockaddr_in *ifaddr_entry;
{
  int i, n;
  off_t lastprov;
  
  if (provaddr == 0) {
    syslog(LOG_WARNING, gettxt(":178", "get_if_entry: provider: Symbol not defined.\n"));
    return(FALSE);
  }
  
  if (nl[N_LASTPROV].n_value == 0) {
    syslog(LOG_WARNING, gettxt(":179", "get_if_entry: lastprov: Symbol not defined.\n"));
    return(FALSE);
  }
  lseek(kmem, nl[N_LASTPROV].n_value, 0);
  if (read(kmem, &lastprov, sizeof(lastprov)) < 0) {
    syslog(LOG_WARNING, gettxt(":180", "get_if_entry: lastprov: %m.\n"));
    return(FALSE);
  }
  n = (lastprov - provaddr) / sizeof(struct ip_provider) + 1;
  if (*if_num == 0) { /* 0 is invalid interface number - wmv */
    *if_num = n;  /* cheating for determining number of interfaces*/
    return(FALSE);
  }

  i = 0;
  lseek(kmem, provaddr, 0);
  do {
    if (i == n) {
      *if_num = i;  /* cheating for determining number of interfaces*/
      return(FALSE);
    }
    
    if (read(kmem, prov_entry, sizeof(struct ip_provider)) < 0) {
      syslog(LOG_WARNING, gettxt(":181", "get_if_entry: provaddr: %m.\n"));
      return(FALSE);
    }
    
    memcpy(ifname, prov_entry->name, 16);
    ifname[15] = '\0';
    
    /* copy the sockaddr_in structure */
    memcpy(ifaddr_entry, &prov_entry->ia.ia_ifa.ifa_addr,
	   sizeof(*ifaddr_entry));
    if (prov_entry->ia.ia_ifa.ifa_addr.sa_family == AF_INET)
      i++;
    
  } while (i < *if_num);

  if (prov_entry->qbot == NULL)
    return(FALSE);

  return(TRUE);
}
#endif /* SVR3 || SVR4 */

VarBindList
*var_if_indiscards_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  struct ip_provider prov_entry;
  struct ifstats ifstats;
  char ifname[16];
  struct sockaddr_in ifaddr_entry;
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#if defined(M_UNIX) || defined(M_XENIX) || defined(TCP40) || defined(SVR4)
    if (!get_ifstats(ifname,&ifstats, prov_entry.ia.ia_ifa.ifa_ifs))
      ifstats.ifindiscards = 0;
#else
  if (prov_entry.ia.ia_ifa.ifa_ifs) {
    lseek(kmem, prov_entry.ia.ia_ifa.ifa_ifs, 0);
    if (read(kmem, &ifstats, sizeof(struct ifstats)) < 0) {
      perror("snmpd:  var_if_indiscards_get, ifstats");
      return(NULL);
    }
  }
  else {
    ifstats.ifindiscards = 0;
  }
#endif
  sprintf(buffer,"ifInDiscards.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ifstats.ifindiscards, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_innucast_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  struct ip_provider prov_entry;
  struct ifstats ifstats;
  char ifname[16];
  struct sockaddr_in ifaddr_entry;
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#if defined(M_UNIX) || defined(M_XENIX) || defined(TCP40) || defined(SVR4)
    if (!get_ifstats(ifname,&ifstats, prov_entry.ia.ia_ifa.ifa_ifs))
      ifstats.ifinnucastpkts = 0;
#else
  if (prov_entry.ia.ia_ifa.ifa_ifs) {
    lseek(kmem, prov_entry.ia.ia_ifa.ifa_ifs, 0);
    if (read(kmem, &ifstats, sizeof(struct ifstats)) < 0) {
      perror("snmpd:  var_if_innucast_get, ifstats");
      return(NULL);
    }
  }
  else {
    ifstats.ifinnucastpkts = 0;
  }
#endif
  sprintf(buffer,"ifInNUcastPkts.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ifstats.ifinnucastpkts, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}


VarBindList
*var_if_inoctets_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  struct ip_provider prov_entry;
  struct ifstats ifstats;
  char ifname[16];
  struct sockaddr_in ifaddr_entry;
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#if defined(M_UNIX) || defined(M_XENIX) || defined(TCP40) || defined(SVR4)
    if (!get_ifstats(ifname,&ifstats, prov_entry.ia.ia_ifa.ifa_ifs))
      ifstats.ifinoctets = 0;
#else
  if (prov_entry.ia.ia_ifa.ifa_ifs) {
    lseek(kmem, prov_entry.ia.ia_ifa.ifa_ifs, 0);
    if (read(kmem, &ifstats, sizeof(struct ifstats)) < 0) {
      perror("snmpd:  var_if_inoctets_get, ifstats");
      return(NULL);
    }
  }
  else {
    ifstats.ifinoctets = 0;
  }
#endif
  sprintf(buffer,"ifInOctets.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ifstats.ifinoctets, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_inucast_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  struct ip_provider prov_entry;
  struct ifstats ifstats;
  char ifname[16];
  struct sockaddr_in ifaddr_entry;
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#if defined(M_UNIX) || defined(M_XENIX) || defined(TCP40) || defined(SVR4)
    if (!get_ifstats(ifname,&ifstats, prov_entry.ia.ia_ifa.ifa_ifs))
      ifstats.ifinucastpkts = 0;
#else
  if (prov_entry.ia.ia_ifa.ifa_ifs) {
    lseek(kmem, prov_entry.ia.ia_ifa.ifa_ifs, 0);
    if (read(kmem, &ifstats, sizeof(struct ifstats)) < 0) {
      perror("snmpd:  var_if_inucast_get, ifstats");
      return(NULL);
    }
  }
  else {
    ifstats.ifinucastpkts = 0;
  }
#endif
  sprintf(buffer,"ifInUcastPkts.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ifstats.ifinucastpkts, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_inunkprotos_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  struct ip_provider prov_entry;
  struct ifstats ifstats;
  char ifname[16];
  struct sockaddr_in ifaddr_entry;
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#if defined(M_UNIX) || defined(M_XENIX) || defined(TCP40) || defined(SVR4)
    if (!get_ifstats(ifname,&ifstats, prov_entry.ia.ia_ifa.ifa_ifs))
      ifstats.ifinunkprotos = 0;
#else
  if (prov_entry.ia.ia_ifa.ifa_ifs) {
    lseek(kmem, prov_entry.ia.ia_ifa.ifa_ifs, 0);
    if (read(kmem, &ifstats, sizeof(struct ifstats)) < 0) {
      perror("snmpd:  var_if_inunkprotos_get, ifstats");
      return(NULL);
    }
  }
  else {
    ifstats.ifinunkprotos = 0;
  }
#endif
  sprintf(buffer,"ifInUnknownProtos.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ifstats.ifinunkprotos, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_outdiscards_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  struct ip_provider prov_entry;
  struct ifstats ifstats;
  char ifname[16];
  struct sockaddr_in ifaddr_entry;
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#if defined(M_UNIX) || defined(M_XENIX) || defined(TCP40) || defined(SVR4)
    if (!get_ifstats(ifname,&ifstats, prov_entry.ia.ia_ifa.ifa_ifs))
      ifstats.ifoutdiscards = 0;
#else
  if (prov_entry.ia.ia_ifa.ifa_ifs) {
    lseek(kmem, prov_entry.ia.ia_ifa.ifa_ifs, 0);
    if (read(kmem, &ifstats, sizeof(struct ifstats)) < 0) {
      perror("snmpd:  var_if_outdiscards_get, ifstats");
      return(NULL);
    }
  }
  else {
    ifstats.ifoutdiscards = 0;
  }
#endif
  sprintf(buffer,"ifOutDiscards.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ifstats.ifoutdiscards, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_outnucast_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  struct ip_provider prov_entry;
  struct ifstats ifstats;
  char ifname[16];
  struct sockaddr_in ifaddr_entry;
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#if defined(M_UNIX) || defined(M_XENIX) || defined(TCP40) || defined(SVR4)
    if (!get_ifstats(ifname,&ifstats, prov_entry.ia.ia_ifa.ifa_ifs))
      ifstats.ifoutnucastpkts = 0;
#else
  if (prov_entry.ia.ia_ifa.ifa_ifs) {
    lseek(kmem, prov_entry.ia.ia_ifa.ifa_ifs, 0);
    if (read(kmem, &ifstats, sizeof(struct ifstats)) < 0) {
      perror("snmpd:  var_if_outnucast_get, ifstats");
      return(NULL);
    }
  }
  else {
    ifstats.ifoutnucastpkts = 0;
  }
#endif
  sprintf(buffer,"ifOutNUcastPkts.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ifstats.ifoutnucastpkts, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_outoctets_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  struct ip_provider prov_entry;
  struct ifstats ifstats;
  char ifname[16];
  struct sockaddr_in ifaddr_entry;
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#if defined(M_UNIX) || defined(M_XENIX) || defined(TCP40) || defined(SVR4)
    if (!get_ifstats(ifname,&ifstats, prov_entry.ia.ia_ifa.ifa_ifs))
      ifstats.ifoutoctets = 0;
#else
  if (prov_entry.ia.ia_ifa.ifa_ifs) {
    lseek(kmem, prov_entry.ia.ia_ifa.ifa_ifs, 0);
    if (read(kmem, &ifstats, sizeof(struct ifstats)) < 0) {
      perror("snmpd:  var_if_outoctets_get, ifstats");
      return(NULL);
    }
  }
  else {
    ifstats.ifoutoctets = 0;
  }
#endif
  sprintf(buffer,"ifOutOctets.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ifstats.ifoutoctets, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_outucast_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  struct ip_provider prov_entry;
  struct ifstats ifstats;
  char ifname[16];
  struct sockaddr_in ifaddr_entry;
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#if defined(M_UNIX) || defined(M_XENIX) || defined(TCP40) || defined(SVR4)
    if (!get_ifstats(ifname,&ifstats, prov_entry.ia.ia_ifa.ifa_ifs))
      ifstats.ifoutucastpkts = 0;
#else
  if (prov_entry.ia.ia_ifa.ifa_ifs) {
    lseek(kmem, prov_entry.ia.ia_ifa.ifa_ifs, 0);
    if (read(kmem, &ifstats, sizeof(struct ifstats)) < 0) {
      perror("snmpd:  var_if_outucast_get, ifstats");
      return(NULL);
    }
  }
  else {
    ifstats.ifoutucastpkts = 0;
  }
#endif
  sprintf(buffer,"ifOutUcastPkts.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, COUNTER_TYPE, ifstats.ifoutucastpkts, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_speed_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  struct ip_provider prov_entry;
  struct ifstats ifstats;
  char ifname[16];
  struct sockaddr_in ifaddr_entry;
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#if defined(M_UNIX) || defined(M_XENIX) || defined(TCP40) || defined(SVR4)
    if (!get_ifstats(ifname,&ifstats, prov_entry.ia.ia_ifa.ifa_ifs))
      ifstats.ifspeed = 0;
#else
  if (prov_entry.ia.ia_ifa.ifa_ifs) {
    lseek(kmem, prov_entry.ia.ia_ifa.ifa_ifs, 0);
    if (read(kmem, &ifstats, sizeof(struct ifstats)) < 0) {
      perror("snmpd:  var_if_speed_get, ifstats");
      return(NULL);
    }
  }
  else {
    ifstats.ifspeed = 0;
  }
#endif
  sprintf(buffer,"ifSpeed.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, GAUGE_TYPE, ifstats.ifspeed, 0, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

VarBindList
*var_if_type_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  int if_num = 0;
  int cc;
  struct ip_provider prov_entry;
  struct ifstats ifstats;
  char ifname[16];
  struct sockaddr_in ifaddr_entry;
  char buffer[256];

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }

#if defined(M_UNIX) || defined(M_XENIX) || defined(TCP40) || defined(SVR4)
    if (!get_ifstats(ifname,&ifstats, prov_entry.ia.ia_ifa.ifa_ifs))
	ifstats.iftype = IFOTHER;
#else
  if (prov_entry.ia.ia_ifa.ifa_ifs) {
    lseek(kmem, prov_entry.ia.ia_ifa.ifa_ifs, 0);
    if (read(kmem, &ifstats, sizeof(struct ifstats)) < 0) {
      perror("snmpd:  var_if_type_get, ifstats");
      return(NULL);
    }
  }
  else {
    ifstats.iftype = 0;
  }
#endif
  sprintf(buffer,"ifType.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  vb_ptr = make_varbind(oid_ptr, INTEGER_TYPE, 0, ifstats.iftype, NULL, NULL);
  oid_ptr = NULL;
  return(vb_ptr);
}

struct ziftype {
	int type;
	char *oid;
};

struct ziftype iftypes[] = {
	IFOTHER, "0.0",	/* really other */
	IFOTHER, "0.0", 	/* 1822 */
	IFOTHER, "0.0",	/* hdh1822 */
	IFDDN_X25,"0.0",	
	IFRFC877_X25,"0.0",
	IFETHERNET_CSMACD, "transmission.7",
	IFISO88023_CSMACD, "transmission.7",
#if !defined(IFISO88025_TOKENRING)
#define	IFISO88025_TOKENRING	9
#endif
	IFISO88025_TOKENRING, "transmission.9",
	IFPPP, "0.0",
	IFLOOPBACK, "0.0",
	IFSLIP, "0.0"
};
int n_iftypes = sizeof(iftypes) / sizeof(struct ziftype);

VarBindList
*var_if_specific_get(var_name_ptr, in_name_ptr, arg, var_next_ptr, type_search)
     OID var_name_ptr;
     OID in_name_ptr;
     int arg;
     VarEntry *var_next_ptr;
     int type_search;
{
  VarBindList *vb_ptr;
  OID oid_ptr;
  OID oidvalue_ptr;
  int if_num = 0;
  int cc;
  struct ip_provider prov_entry;
  struct ifstats ifstats;
  char ifname[16];
  struct sockaddr_in ifaddr_entry;
  char buffer[256];
  int i;
  char *mibp;

  /* Check that an exact search only has one sub-id field for if number - else not exact */
  if ((type_search == EXACT) && (in_name_ptr->length != (var_name_ptr->length + 1)))
    return(NULL);

  /* Now find out which interface they are interested in */
  if (in_name_ptr->length > var_name_ptr->length)
    if_num = in_name_ptr->oid_ptr[var_name_ptr->length]; /* oid sub-field after var_name */
    
  /* If a get next, then get the NEXT interface (+1)  */
  if (type_search == NEXT)
    if_num++;

  cc = get_if_entry(nl[N_PROVIDER].n_value, &if_num, &prov_entry, ifname,
		    &ifaddr_entry);

  if (cc == FALSE) {
    if (type_search == NEXT)
      return(get_next_class(var_next_ptr)); /* get next variable */
    if (type_search == EXACT)
      return(NULL);		/* Signal failure */
  }
#if defined(SVR3) || defined(SVR4)
#if defined(M_UNIX) || defined(M_XENIX)  || defined(TCP40) || defined(SVR4)
    if (!get_ifstats(ifname,&ifstats, prov_entry.ia.ia_ifa.ifa_ifs))
      ifstats.ifs_ipackets = 0;
#else
  if (prov_entry.ia.ia_ifa.ifa_ifs) {
    lseek(kmem, prov_entry.ia.ia_ifa.ifa_ifs, 0);
    if (read(kmem, &ifstats, sizeof(struct ifstats)) < 0) {
      perror("snmpd:  var_if_specific_get, ifstats");
      return(NULL);
    }
  }
  else {
    ifstats.iftype = IFOTHER;
  }
#endif
#endif
  if (ifstats.iftype < IFOTHER)
	ifstats.iftype = IFOTHER;
  for (i = 0; i < n_iftypes; i++) {
	if (iftypes[i].type == ifstats.iftype) {
		mibp = iftypes[i].oid;
		break;
	}
  }
  if (i > n_iftypes)
	mibp = iftypes[0].oid;
  sprintf(buffer,"ifSpecific.%d", if_num);
  oid_ptr = make_obj_id_from_dot((unsigned char *)buffer);
  oidvalue_ptr = make_obj_id_from_dot((unsigned char *)mibp);
  vb_ptr = make_varbind(oid_ptr, OBJECT_ID_TYPE, 0, 0, NULL, oidvalue_ptr);
  oid_ptr = NULL;
  oidvalue_ptr = NULL;
  return(vb_ptr);
}

