/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nm:common/cmd/cmd-nm/usr.sbin/snmp/snmpstat/system.c	1.2"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nm/usr.sbin/snmp/snmpstat/system.c,v 1.3 1994/05/27 17:41:30 rbell Exp $"
#ifndef lint
static char TCPID[] = "@(#)system.c	1.1 STREAMWare TCP/IP SVR4.2 source";
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
static char SNMPID[] = "@(#)system.c	2.1 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright (c) 1987, 1988, 1989 Kenneth W. Key and Jeffrey D. Case 
 */

/*
 * Revision History: 
 *
 * 2/6/89 JDC Amended copyright notice 
 *
 * Professionalized error messages a bit 
 *
 * Expanded comments and documentation 
 *
 * Added code for genErr 
 *
 */

/*
 * system.c 
 *
 * print out the system entries 
 */

#include <unistd.h>
#include "snmpio.h"

int print_sys_info();
int print_sysII_info();

static char *dots[] = {
	"sysDescr",
	"sysObjectID",
	"sysUpTime",
	""
};
static char *dotsII[] = {
	"sysContact",
	"sysName",
	"sysLocation",
	"sysServices",
	""
};

syspr(community)
	char *community;
{
	int ret;

	ret = doreq(dots, community, print_sys_info);
	(void) doreq(dotsII, community, print_sysII_info);
	return(ret);
}

/* ARGSUSED */
print_sys_info(vb_list_ptr, lineno, community)
	VarBindList *vb_list_ptr;
	int lineno;
	char *community;
{
	int index;
	VarBindList *vb_ptr;
	OctetString *descr;
	char oid[128];
	unsigned long timeticks;

	index = 0;
	vb_ptr = vb_list_ptr;
	if (!targetdot(dots[index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	descr = vb_ptr->vb_ptr->value->os_value;

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	if (make_dot_from_obj_id(vb_ptr->vb_ptr->value->oid_value, oid) == -1) {
		fprintf(stderr, gettxt(":48", "snmpstat: can't translate %s\n"), dots[index]);
		return(-1);
	}

	vb_ptr = vb_ptr->next;
	if (!targetdot(dots[++index], vb_ptr->vb_ptr->name, lineno)) {
		return(-1);
	}
	timeticks = vb_ptr->vb_ptr->value->sl_value;

	if (lineno == 0) {
		printf(gettxt(":49", "System Group\n"));
	}

	print_octetstring(gettxt(":50", "Description"), descr);
	printf(gettxt(":51", "ObjectID: %s\n"), oid);
	print_timeticks(gettxt(":52", "UpTime"), timeticks);

	return(0);
}

/* ARGSUSED */
print_sysII_info(vb_list_ptr, lineno, community)
	VarBindList *vb_list_ptr;
	int lineno;
	char *community;
{
	int index;
	VarBindList *vb_ptr;
	OctetString *contact;
	OctetString *name;
	OctetString *location;
	int services;

	index = 0;
	vb_ptr = vb_list_ptr;
	if (!targetdot(dotsII[index], vb_ptr->vb_ptr->name, lineno + 1)) {
		contact = NULL;
	} else {
		contact = vb_ptr->vb_ptr->value->os_value;
	}

	vb_ptr = vb_ptr->next;
	if (!targetdot(dotsII[++index], vb_ptr->vb_ptr->name, lineno + 1)) {
		name = NULL;
	} else {
		name = vb_ptr->vb_ptr->value->os_value;
	}

	vb_ptr = vb_ptr->next;
	if (!targetdot(dotsII[++index], vb_ptr->vb_ptr->name, lineno + 1)) {
		location = NULL;
	} else {
		location = vb_ptr->vb_ptr->value->os_value;
	}

	vb_ptr = vb_ptr->next;
	if (!targetdot(dotsII[++index], vb_ptr->vb_ptr->name, lineno + 1)) {
		services = 0;
	} else {
		services = vb_ptr->vb_ptr->value->sl_value;
	}

	if (contact != NULL)
		print_octetstring(gettxt(":53", "Contact"), contact);
	if (name != NULL)
		print_octetstring(gettxt(":54", "Name"), name);
	if (location != NULL)
		print_octetstring(gettxt(":55", "Location"), location);
	if (services != 0)
		print_services(services);

	return(0);
}

print_octetstring(label, os_ptr)
	char *label;
	OctetString *os_ptr;
{
	int i;

	printf("%s: ", label);
	for (i = 0; i < os_ptr->length; i++)
		putchar((int) os_ptr->octet_ptr[i]);
	putchar('\n');
}

#define plural(x)	(x != 1 ? "s": "")

print_timeticks(label, timeticks)
	char *label;
	unsigned long timeticks;
{
	unsigned long cf;
	unsigned long tim;
	int docomma;

	printf("%s:", label);

	docomma = 0;

	cf = 24L * 60L * 60L * 100L;
	tim = timeticks / cf;
	if (tim) {
		printf(gettxt(":56", " %ld day%s"), tim, plural(tim));
		docomma = 1;
	}
	timeticks %= cf;

	cf /= 24L;
	tim = timeticks / cf;
	if (tim) {
		if (docomma)
			printf(",");
		printf(gettxt(":57", " %ld hour%s"), tim, plural(tim));
		docomma = 1;
	}
	timeticks %= cf;

	cf /= 60L;
	tim = timeticks / cf;
	if (tim) {
		if (docomma)
			printf(",");
		printf(gettxt(":58", " %ld minute%s"), tim, plural(tim));
		docomma = 1;
	}
	timeticks %= cf;

	cf /= 60L;
	tim = timeticks / cf;
	if (tim) {
		if (docomma)
			printf(",");
		printf(gettxt(":59", " %ld second%s"), tim, plural(tim));
		docomma = 1;
	}
	timeticks %= cf;

	cf /= 100L;
	tim = timeticks / cf;
	if (tim || docomma == 0) {
		if (docomma)
			printf(",");
		printf(gettxt(":60", " %ld hundredth%s"), tim, plural(tim));
	}
	printf("\n");
}

#define twoto(n)	(1 << (n))

char *services_list[] = {
	"none",
	"physical",
	"datalink/subnetwork",
	"internet",
	"end-to-end",
	"5",
	"6",
	"applications"
};

print_services(services)
	int services;
{
	int i;
	int need_comma;

	printf(gettxt(":61", "Services:"));

	i = sizeof(services_list)/sizeof(services_list[0]) - 1;
	need_comma = 0;
	while (services != 0 && i > 0) {
		if (services & twoto(i - 1)) {
			if (need_comma)
				printf(",");
			printf(" %s", services_list[i]);
			need_comma = 1;
			services -= twoto(i);
		}
		i--;
	}
	printf("\n");
}
