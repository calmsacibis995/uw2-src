/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/polltest.h	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW4U_SPX_TEST_TLI_POLLTEST_H  /* wrapper symbol for kernel use */
#define _NET_NW4U_SPX_TEST_TLI_POLLTEST_H  /* subject to change without notice */

#ident	"$Id: polltest.h,v 1.2 1994/02/18 15:06:36 vtag Exp $"
/*--------------------------------------------------------------------
*	Constants
*-------------------------------------------------------------------*/
#define MAXFDS			100
#define NUMPACKETS	10
#define TIMEOUT		500


/*--------------------------------------------------------------------
*	Function Prototypes
*-------------------------------------------------------------------*/
void CheckEvents (int ccode, short expected, struct pollfd fds[], int nfds);
void DisplayCount (int nfds);
void EstablishIpxComms (struct pollfd fds[], int nfds, int major, int minor);
void ListenFd (int control, struct pollfd fds[], int nfds, int major,
					int minor);
void IpxPollClnt (int mode, int major);
void IpxPollSrvr (int mode, int nfds, int major);
void RcvIpxData (struct pollfd fds[], int nfds);
void RcvSpxData (struct pollfd fds[], int nfds);
void RcvSpxDisc (struct pollfd fds[], int nfds);
void SetupFds (struct pollfd fds[], int nfds, char *procol,	int mode);
void SpxPollClnt (int mode, int major);
void SpxPollSrvr (int mode, int nfds, int major);
void TeardownFds (struct pollfd fds[], int nfds);
void tlipoll (struct pollfd fds[], int nfds, int timeout, short expected);
void ZeroCount (void);

#endif /* _NET_NW4U_SPX_TEST_TLI_POLLTEST_H */
