/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:acc/audit/audit.cf/Space.c	1.5"

#include <config.h>
#include <sys/types.h>
#include <sys/audit.h>

#define ADT_VER		"4.0"
#define ADT_BSIZE	20480
#define ADT_NLVLS	4
#define ADT_NBUF	2

char adt_ver[8] = ADT_VER;
int adt_bsize = ADT_BSIZE;
int adt_lwp_bsize = ADT_LWP_BSIZE;
uint adt_nbuf = ADT_NBUF;

int adt_nlvls = ADT_NLVLS;
