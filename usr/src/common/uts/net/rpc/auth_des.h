/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_AUTH_DES_H	/* wrapper symbol for kernel use */
#define _NET_RPC_AUTH_DES_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/auth_des.h	1.14"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	auth_des.h, Protocol for DES style authentication for
 *	rpc. It uses des encryption for credentials.
 *
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <svc/time.h>		/* REQUIRED */
#include <net/rpc/auth.h>	/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */
#include <sys/time.h>		/* REQUIRED */
#include <rpc/auth.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * there are two kinds of "names": fullnames and nicknames
 */
enum authdes_namekind {
	ADN_FULLNAME, 
	ADN_NICKNAME
};

/*
 * base credentials sizes for fullnames and nicknames
 */
#define	BASE_DESFCREDSZ		5
#define	BASE_DESNCREDSZ		2

/*
 * A fullname contains the network name of the client, 
 * a conversation key and the window
 */
struct authdes_fullname {
	char *name;	/* network name of client, up to MAXNETNAMELEN */
	des_block key;	/* conversation key */
	u_long window;	/* associated window */
};

/*
 * des style credentials.
 */
struct authdes_cred {
	enum authdes_namekind adc_namekind;
	struct authdes_fullname adc_fullname;
	u_long adc_nickname;
};

/*
 * des authentication verifier 
 */
struct authdes_verf {
	union {
		struct timeval adv_ctime;	/* clear time */
		des_block adv_xtime;		/* encrypted time */
	} adv_time_u;
	u_long adv_int_u;
};

/*
 * des authentication verifier: client variety
 *
 * adv_timestamp is the current time.
 * adv_winverf is the credential window + 1.
 * Both are encrypted using the conversation key.
 */
#define adv_timestamp	adv_time_u.adv_ctime
#define adv_xtimestamp	adv_time_u.adv_xtime
#define adv_winverf	adv_int_u

/*
 * des authentication verifier: server variety
 *
 * adv_timeverf is the client's timestamp + client's window
 * adv_nickname is the server's nickname for the client.
 * adv_timeverf is encrypted using the conversation key.
 */
#define adv_timeverf	adv_time_u.adv_ctime
#define adv_xtimeverf	adv_time_u.adv_xtime
#define adv_nickname	adv_int_u

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_RPC_AUTH_DES_H */
