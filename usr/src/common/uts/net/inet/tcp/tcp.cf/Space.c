/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/tcp/tcp.cf/Space.c	1.7"
#ident	"$Header: $"

#include <config.h>
#include <sys/types.h>
#include <sys/bitmasks.h>
#include <netinet/tcp_debug.h>

/*
 * TCP "minor devices" bitmask array.
 * TCP_UNITS (from System file).
 */
int	tcpdev_cnt = TCP_UNITS;
uint_t	tcpdev_words = BITMASK_NWORDS(TCP_UNITS);
uint_t	tcpdev[BITMASK_NWORDS(TCP_UNITS)];
/*
 * ntcp is used by user-level snmp code.
 */
int	ntcp = TCP_UNITS;
/*
 * XXX
 * TCPWINDOW is a tunable.
 */
int	tcp_recvspace = TCPWINDOW;
int	net_tcpwindow = TCPWINDOW;
/*
 * The default time-to-live for TCP segments.
 * TCPTTL is a tunable.
 */
unsigned char	tcpttl = TCPTTL;

#define SMALL_WIN_THRESHOLD 1024
#define TCP_SMALL_WINDOW 4*1024 
/* 
 * XXX
 * if mss < mss_sw_threshold, then use small window size 
 */
int	mss_sw_threshold = SMALL_WIN_THRESHOLD;
int	tcp_small_recvspace = TCP_SMALL_WINDOW;	/* small window size */
/*
 * XXX
 */
int	tcp_round_mss = 1;
/*
 * TCP_NDEBUG has been set to one,which disables debugging via trpt
 * (and saves memory).  To enable debugging via trpt make TCP_NDEBUG
 * the same size as TCP_UNITS.  With TCP_NDEBUG set to 1, the
 * SIOTCPGDATA ioctl will fail with errno set to ENOMEM.
 */
#define TCP_NDEBUG	1
int	tcp_ndebug = TCP_NDEBUG;
struct tcp_debug	tcp_debug[TCP_NDEBUG];

int	tcpconsdebug = 0;	/* tracing to console */
int	tcpalldebug = 0;	/* trace all connections */
int	tcpprintfs = 0;		/* tell me about checksum errors */
