/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/pt/ptm.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>	/* to collect tunable parameters */
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/termios.h>
#include <sys/ptms.h>

/*
** NUMREGPT and NUMSCOPT are tunables, set in 'Mtune' for the number
** of regular/SCO pseudo terms respecitvly.
*/

int pt_cnt = NUMREGPT;

int pt_sco_cnt = NUMSCOPT;

/*
** total number of pt's is NUMREGPT+NUMSCOPT, it should
** be equal to PTM_UNITS, but for consistency we'll use the
** values from Mtune.
*/
struct pt_ttys ptms_tty[NUMREGPT+NUMSCOPT];

