/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ndt.cmd:ndtstat.c	1.3"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/ndt.d/ndtstat.c,v 1.2 1994/01/31 21:52:11 duck Exp $"

/*
 *        Copyright Novell Inc. 1991
 *        (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
 *
 *        No part of this file may be duplicated, revised, translated, localized
 *        or modified in any manner or compiled, linked or uploaded or
 *        downloaded to or from any computer system without the prior written
 *        consent of Novell, Inc.
 *
 *
 *  Netware Unix Client 
 *        Author: Duck
 *       Created: Sun May  5 14:06:53 MDT 1991
 *
 *  MODULE:
 *
 *  ABSTRACT:
 *
 */
#include	<sys/types.h>
#include	<syms.h>
#include	<sys/nwctrace.h>
#include	<sys/traceuser.h>
#include	"ndt.h"

#ifdef DBMALLOC
#include "malloc.h"
#endif


		NameOff	*find_symbol();






Stat *
findStopWatch(addr, statHead)
unsigned int	addr;
Stat			*statHead;
{
	Stat	*stat_p;
	NameOff *no_s;

	if( (stat_p = statHead->st_next) == (Stat *)0) {	/* first time thru	*/
		if( (stat_p = (Stat *)calloc(1, sizeof(Stat))) == (Stat *)0) {
			printf("ndt: stat calloc error\n");
			exit( 1);
		}
		statHead->st_next = stat_p;

	} else {

		while( stat_p) {
			if( stat_p->st_addr == addr )
				return( stat_p);
			stat_p = stat_p->st_next;
		}

		/*
		 *	Get a new entry
		 */
		if( (stat_p = (Stat *)calloc(1, sizeof(Stat))) == (Stat *)0) {
			printf("ndt: stat calloc error\n");
			exit( 1);
		}

		stat_p->st_next = statHead->st_next;	/* link before old head	*/
		statHead->st_next = stat_p;
	}

	stat_p->st_addr = addr;

	return( stat_p);
}





Stat *
findStat(addr, statHead)
unsigned int	addr;
Stat			*statHead;
{
	Stat	*stat_p, *lastStat_p;
	NameOff *no_s;

	if( (stat_p = statHead->st_next) == (Stat *)0) {	/* first time thru	*/
		if( (stat_p = (Stat *)calloc(1, sizeof(Stat))) == (Stat *)0) {
			printf("ndt: stat calloc error\n");
			exit( 1);
		}
		statHead->st_next = stat_p;
		no_s = find_symbol( addr);

	} else {									/* search existing entries	*/

		while( stat_p) {
			if( stat_p->st_base <= addr && addr <= stat_p->st_end )
				return( stat_p);
			stat_p = stat_p->st_next;
		}

		no_s = find_symbol( addr);

		stat_p = statHead->st_next;
		lastStat_p = stat_p;
		while( stat_p) {
			if( stat_p->st_base == no_s->base ) {
				stat_p->st_end = addr;
				return( stat_p);
			}
			lastStat_p = stat_p;
			stat_p = stat_p->st_next;
		}

		/*
		 *	Get a new entry
		 */
		if( (stat_p = (Stat *)calloc(1, sizeof(Stat))) == (Stat *)0) {
			printf("ndt: stat calloc error\n");
			exit( 1);
		}

#ifdef LINK_TO_HEAD
		stat_p->st_next = statHead->st_next;	/* link new one before old head	*/
		statHead->st_next = stat_p;
#endif LINK_TO_HEAD

		lastStat_p->st_next = stat_p;			/* link to the end				*/
	}

	stat_p->st_end     =
	stat_p->st_addr    = addr;

	stat_p->st_base    = no_s->base;
	stat_p->st_offset  = no_s->offset;
	stat_p->st_modname = no_s->name;

	return( stat_p);
}
