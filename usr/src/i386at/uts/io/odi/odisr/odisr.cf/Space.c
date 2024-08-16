/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/odi/odisr/odisr.cf/Space.c	1.2"
#ident	"$Header: $"

#include	<sys/types.h>
#include	<sys/route.h>
#include	<config.h>

/*
 * global variables
 */
struct _Board_SRTrack_ boardRoute[ODISR_NBOARD];

int	odisr_nboard	= ODISR_NBOARD;

/* 
 * the followings define route broadcast for each frame
 * SR_LIM_BRDCAST_MASK or SR_GEN_BRDCAST_MASK (All routes broadcast)
 */
uchar_t	unknowndestaddr = SR_GEN_BRDCAST_MASK;
uchar_t	broadcastframes = SR_GEN_BRDCAST_MASK;
uchar_t	multicastframes = SR_GEN_BRDCAST_MASK;
uchar_t	brdcast_response = SR_GEN_BRDCAST_MASK;

int	odisr_boardage = ODISR_BOARDAGE; /* in minutes */

