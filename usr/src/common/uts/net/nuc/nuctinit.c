/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nuctinit.c	1.9"
#ident 	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nuctinit.c,v 2.51.2.2 1994/12/21 02:47:49 ram Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: nuctool.c
 *	ABSTRACT: Driver for NUCtools package
 */ 

#include <net/nuc/nuc_prototypes.h>
#include <io/ddi.h>

#define TRUE -1
#define FALSE 0

long	nuctInitialized = 0;

int
nuctinit()
{

	nuctInitialized = FALSE;
	NWtlInitSemaphore();	
	nuctInitialized = TRUE;
}
