/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_SLIP_SLIP_H	/* wrapper symbol for kernel use */
#define _NET_INET_SLIP_SLIP_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/slip/slip.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991 UNIX System Laboratories, Inc.
 *  	          All rights reserved.
 */

/*
 * System V STREAMS TCP - Release 3.0 
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI) 
 * All Rights Reserved. 
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code. 
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.  This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates. 
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies. 
 */


#ifdef _KERNEL_HEADERS

#include <io/stream.h>			/* REQUIRED */
#include <net/inet/if.h>		/* REQUIRED */
#include <net/inet/in_comp.h>		/* REQUIRED */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/stream.h>			/* REQUIRED */
#include <net/if.h>			/* REQUIRED */
#include <netinet/in_comp.h>		/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */

#else

#include <sys/stream.h>			/* REQUIRED */
#include <net/if.h>			/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

struct slip_state {
	mblk_t		*ss_mp;		/* where we gather incoming octets */
	u_long		ss_flags;	/* state information and such	*/
	u_long		ss_mtu;		/* mtu for this interface	*/
	mblk_t		*ss_ioctl_mp;	/* pointer to ioctl message */
	struct ifstats	*ss_ifstats;	/* if stat structure		*/
	struct incompress	*ss_comp;	/* tcp compression struct */
	char		ss_ifname[IFNAMSIZ];	/* the name of this if.	*/
	lock_t		*ss_lock;	/* MP lock */
};

typedef struct slip_state slip_state_t;

/*
 * macros for state flags
 */
#define SS_ERROR		0x00000001 /* when this is set an error in  */
					   /* framing has occured, which    */
					   /* implies that we eat octets    */
					   /* until we receive a frame_end  */
#define SS_ESCAPE		0x00000002 /* when set it implies the next  */
					   /* octet should be escaped       */
#define SS_COMPRESS		0x00000004 /* set when we are compressing   */
#define SS_AUTOCOMPRESS		0x00000008 /* set when we will accept       */
					   /* compressed packets	    */
#define SS_NOICMP		0x00000010 /* if set we will not send out   */
					   /* any icmp packets		    */
#define SS_BOUND		0x00000020 /* interface is bound	    */
#define SS_HANGUP		0x00000040 /* have received hangup message  */

/* SS_INIT_FLAGS is used to initialize the ss_flags field in slip_state_t */
#define SS_INIT_FLAGS	0

#define MAXSLIP	296
#define MINSLIP	3	/* minimum packet size for slip. if we get something */
			/* smaller, we just drop it silently		*/

#define	SLIP_MODID 13

#endif /* _KERNEL */

/* ioctl's */
#define	SLIPIOCTL	('P'<<8)
#define	S_MTU		(SLIPIOCTL | 0xff)
#define	S_COMPRESSON	(SLIPIOCTL | 0xfe)
#define	S_COMPRESSOFF	(SLIPIOCTL | 0xfd)
#define	S_COMPRESSAON	(SLIPIOCTL | 0xfc)
#define	S_COMPRESSAOFF	(SLIPIOCTL | 0xfb)
#define	S_NOICMP	(SLIPIOCTL | 0xfa)
#define	S_ICMP		(SLIPIOCTL | 0xf9)
#define S_SETSPEED	(SLIPIOCTL | 0xf8)

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_SLIP_SLIP_H */
