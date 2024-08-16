/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_TIMOD_H	/* wrapper symbol for kernel use */
#define _NET_TIMOD_H	/* subject to change without notice */

#ident	"@(#)kern:net/timod.h	1.14"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/* Internal flags used by the library  and providers */

#define USED		0x00001	/* date structure in use. */
#define	FATAL		0x00002	/* fatal error M_ERROR occurred   */
#define	WAITIOCACK	0x00004	/* waiting for info for ioctl act */
#define	MORE		0x00008	/* more data */
#define	EXPEDITED	0x00010	/* processing expedited TSDU */
#define	CLTS		0x00020	/* connectionless transport */
#define	COTS		0x00040	/* connection-oriented transport */
#define	CONNWAIT	0x00100	/* waiting for connect confirmation */
#define	LOCORDREL	0x00200	/* local end has orderly released */
#define	REMORDREL	0x00400	/* remote end had orderly released */
#define	NAMEPROC	0x00800	/* processing a NAME ioctl */
#define FLOWCNTL	0x01000	/* endpoint normal data is flow controlled */
#define FLOWCNTLEX	0x02000	/* endpoint expedited data is flow controlled */

/* Internal buffer size (in bytes) pre-allocated for address fields */
#define PRADDRSZ	128

/* Sleep timeout in open */
#define TIMWAIT	(1*HZ)

/* Timod ioctls */
#define		TIMOD 		('T'<<8)
#define		TI_GETINFO	(TIMOD|140)
#define		TI_OPTMGMT	(TIMOD|141)
#define		TI_BIND		(TIMOD|142)
#define		TI_UNBIND	(TIMOD|143)
#define		TI_GETMYNAME	(TIMOD|144)
#define		TI_GETPEERNAME	(TIMOD|145)
#define		TI_SETMYNAME	(TIMOD|146)
#define		TI_SETPEERNAME	(TIMOD|147)


/* TI interface user level structure - one per open file */

struct _ti_user {
	ulong	ti_flags;	/* flags              */
	int	ti_rcvsize;	/* rcv buffer size    */
	char   *ti_rcvbuf;	/* rcv buffer         */
	int	ti_ctlsize;	/* ctl buffer size    */
	char   *ti_ctlbuf;	/* ctl buffer         */
	char   *ti_lookdbuf;	/* look data buffer   */
	char   *ti_lookcbuf;	/* look ctl buffer    */
	int	ti_lookdsize;   /* look data buf size */
	int	ti_lookcsize;   /* look ctl buf size  */
	int	ti_maxpsz;	/* TIDU size          */
	long	ti_servtype;	/* service type       */
	int     ti_lookflg;	/* buffered look flag */
	int	ti_state;	/* user level state   */
	int	ti_ocnt;	/* # outstanding connect indications */
        int     ti_qlen;        /* connect indication queue size */
        int     ti_tsdu;        /* the amount of TSDU supported by provider */
        ulong   ti_provider_flgs; /* user visible provider flags */
#ifdef _REENTRANT
	mutex_t	lock;	/* lock for this structure */
#endif /* _REENTRANT */
};

/* Old TI interface user level structure - needed for compatibility */

struct _oldti_user {
	ushort	ti_flags;	/* flags              */
	int	ti_rcvsize;	/* rcv buffer size    */
	char   *ti_rcvbuf;	/* rcv buffer         */
	int	ti_ctlsize;	/* ctl buffer size    */
	char   *ti_ctlbuf;	/* ctl buffer         */
	char   *ti_lookdbuf;	/* look data buffer   */
	char   *ti_lookcbuf;	/* look ctl buffer    */
	int	ti_lookdsize;   /* look data buf size */
	int	ti_lookcsize;   /* look ctl buf size  */
	int	ti_maxpsz;	/* TIDU size          */
	long	ti_servtype;	/* service type       */
	int     ti_lookflg;	/* buffered look flag */
};

#define OPENFILES     ulimit(4, 0)

#ifdef _KERNEL

/*
 * Routine to be used by transport providers to process
 * TI_GETMYNAME and TI_GETPEERNAME ioctls.
 */
extern int ti_doname(queue_t *, mblk_t *, caddr_t, uint, caddr_t, uint);

/*
 * Return values for ti_doname.
 */
#define DONAME_FAIL	0	/* failing ioctl (done) */
#define DONAME_DONE	1	/* done processing */
#define DONAME_CONT	2	/* continue proceesing (not done yet)*/

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_TIMOD_H */
