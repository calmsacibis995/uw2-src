/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nm:common/cmd/cmd-nm/usr.sbin/snmp/in.snmpd/snmpd.h	1.4"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nm/usr.sbin/snmp/in.snmpd/snmpd.h,v 1.4 1994/08/16 19:57:32 cyang Exp $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
 * All rights reserved.
 */

/*      @(#)snmpd.h	3.2 INTERACTIVE SNMP  source        */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/* search type (i.e., get or get-next) */
#define NEXT       1
#define EXACT      2

/* read flags (access and variable) */
#define NONE       0
#define READ_ONLY  1
#define READ_WRITE 2

#define MAXINTERFACES	20	/* size of array to hold ifLastChange */

typedef struct _var_entry {
  OID class_ptr;
  unsigned int type;
  unsigned rw_flag;
  unsigned int arg;
  char *smux;
  struct _var_entry *child;
  struct _var_entry *sibling;
  VarBindList *(*funct_get)();
  int (*funct_test_set)();
  int (*funct_set)();
  struct _var_entry *next;
} VarEntry;

#if defined(SVR3) || defined(SVR4)
#define N_IPSTAT          0
#define N_IFNET           1
#define N_RTHOST          2
#define N_RTNET           3
#define N_ICMPSTAT        4
#define N_RTSTAT          5
#define N_RTHASHSIZE      6
#define N_ARPTAB          7
#define N_ARPTAB_SIZE     8
#define N_NETTOREGTABLE   9
#define N_REGROUTETABLE  10
#define N_REGHASHSIZE    11
#define N_UDPSTAT        12
#define N_TCPSTAT        13
#define N_TCB            14
#define N_PROVIDER       15
#define N_LASTPROV       16
#define N_NTCP           17
#define N_IPFORWARDING   18
#define N_UDB            19
#define N_TCPMINREXMT    20
#define N_TCPMAXREXMT    21
#define N_IPQ_TTL        22
#define N_IP_TTL         23
#endif

/* add this to files that need it */
#ifndef SNMPD
extern struct nlist nl[];
extern char *system_name;
extern char *kmemf;
extern int kmem;
#endif

/* This is for printing TLI error messages */

extern char *t_errmsg();

