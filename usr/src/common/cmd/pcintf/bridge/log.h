/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/log.h	1.1.1.2"
#ifndef	LOG_H
#define	LOG_H

/* @(#)log.h	6.4	modified: 22:49:55 7/12/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include <stdio.h>

#include "pci_proto.h"
#include "pci_types.h"		/* input struct for logPacket() prototype */

/* Logging/debug support */

extern FILE
	*logFile;               /* Log file stream */

struct dbg_struct {
	long	change;		/* Types of changes requested 	*/
	long	set;		/* Use this channel set 	*/
	long	on;		/* Turn these channels on 	*/
	long	off;		/* Turn these channels off 	*/
	long	flip;		/* Invert these channels 	*/
};

extern unsigned long
	dbgEnable;		/* Enabled debug channels */

#define	DBG_LOG		0x0001		/* Local logging channel */
#define	DBG_RLOG	0x0002		/* Remote logging channel */
#define	DBG_VLOG	0x0004		/* Verbose logs */
#define	DBG_PLOG	0x0008		/* Packet logs */
#define DBG_RS232       0x0010          /* RS232 packets */
#define DBG_XPLOG       0x0020          /* log read/write/seek packets */
#define DBG_YPLOG       0x0040          /* log text of read/write packets */

#define PLOG_RCV        0               /* Received packet log */
#define PLOG_XMT        1               /* Transmited packet log */

#ifdef NOLOG
#define	debug(chan, logArgs)
#define logPacket(packet, kind, how)    /* real logPacket()  in logpacket.c */
#define logControl(rmtLogFlag)          /* real logControl() in remotelog.c */
#define logMessage(msgBuf, nBytes)      /* real logMessage() in remotelog.c */
#define logOpen(logBase, logPid)        /* real logOpen()    in log.c */
#define logDOpen(logDesc)               /* real logDOpen()   in log.c */
#define logClose()                      /* real logClose()   in log.c */
#define log(fmt, argVec)                /* real log()        in log.c */
#define ulog(fmt, argVec)               /* real ulog()       in log.c */
#define tlog(fmt, argVec)               /* real tlog()       in log.c */
#define logv(fmt, argVec)               /* real logv()       in log.c */
#define newLogs(logName, namePid, childEnable) /* real newLogs() in log.c */
#define vlog(vLogArgs)
#define Vlog(VLogArgs)
#define dbgOn(onChans)
#define dbgOff(offChans)
#define dbgToggle(togChans)
#define dbgSet(setChans)
#define dbgCheck(checkChans)    0
#else
#ifdef	DEBUG
#define	debug(chan, logArgs)	(((chan) & dbgEnable) ? ulog logArgs : 0)
#else	/* DEBUG! */
#define	debug(chan, logArgs)
#endif	/* !DEBUG */
#define	logchan(chan, logArgs)	(((chan) & dbgEnable) ? ulog logArgs : 0)
#define	vlog(vLogArgs)	debug(DBG_VLOG, vLogArgs)
#define Vlog(VLogArgs)  (((DBG_LOG | DBG_VLOG) & dbgEnable) ? tlog VLogArgs:0)
#define	dbgOn(onChans)		(dbgEnable |= (onChans))
#define	dbgOff(offChans)	(dbgEnable &= ~(offChans))
#define	dbgToggle(togChans)	(dbgEnable ^= togChans)
#define	dbgSet(setChans)	(dbgEnable = (setChans))
#define	dbgCheck(checkChans)	(dbgEnable & (checkChans))
#endif /* ~NOLOG */

/*
   Predicate telling whether logs are currently enabled
*/
#define	logsOn()	(dbgEnable && (logFile != (FILE *)NULL))

/*
   Communications from pcidebug program
*/

#define	chanPat	"/tmp/pcichan.%d"	/* Channel file name pattern */

#define	CHG_SET		0x00000001L	/* Types of changes requested */
#define	CHG_ON		0x00000002L	/* Turn channels on */
#define	CHG_OFF		0x00000004L	/* Turn channels off */
#define	CHG_INV		0x00000008L	/* Invert channels */
#define	CHG_CLOSE	0x00000010L	/* Close log file */
#define	CHG_CHILD	0x00000020L	/* Change child's debugs */

#if !defined(NOLOG)

extern int	log		PROTO((const char *, ...));
extern int	ulog		PROTO((const char *, ...)); /* unconditional */
extern int	tlog		PROTO((const char *, const char *, ...));
extern void	logPacket	PROTO((struct input *, int, long));
extern void	logOpen		PROTO((char *, int));
extern void	logChown	PROTO((uid_t, gid_t));
extern void	logDOpen	PROTO((int));
					/* use a unix descriptor for logging */
extern void	logClose	PROTO((void));
extern long	newLogs		PROTO((char *, int, long *, struct dbg_struct *));

#endif /* !NOLOG */

#if !defined(MERGE386)

extern void	serious		PROTO((char *));
extern void	fatal		PROTO((char *));

#endif	/* !MERGE386 */

#endif	/* !LOG_H */
