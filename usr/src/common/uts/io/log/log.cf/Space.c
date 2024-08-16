/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/log/log.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>	/* to collect tunable parameters */
#include <sys/log.h>
#include <sys/stream.h>

int log_cnt = NLOG + 6;
struct log log_log[NLOG + 6];
unsigned long loghiwat = LOGHIWAT;
unsigned long loglowat = LOGLOWAT;
