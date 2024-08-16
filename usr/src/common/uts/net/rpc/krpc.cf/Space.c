/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/krpc.cf/Space.c	1.2"
#ident	"$Header: $"

#include <config.h>

/*
 * retries for client side receive
 */
int recvtries =		RECVRETRIES;

/*
 * maxdupreqs is the number of cached items in the server side duplicate
 * request cache. It should be adjusted to the service load so that there
 * is likely to be a response entry when the first retransmission comes in.
 */
int maxdupreqs =	MAXDUPREQS;

/*
 * timeout for call to keyserver in seconds
 */
int keytimeout = 	KEYTIMEOUT;

/*
 * number of retries for call to keyserver
 */
int keynretry =		KEYNRETRY;

/*
 * timeout for sync in rtime()
 */
int rtimetimeout = 	RTIMETIMEOUT;
