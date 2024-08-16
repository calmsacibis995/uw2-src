/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwmsg.h	1.4"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWMSG_INC
#define NWMSG_INC

#ifndef NWCALDEF_INC
#ifdef N_PLAT_UNIX
# include <nw/nwcaldef.h>
#else /* !N_PLAT_UNIX */
# include <nwcaldef.h>
#endif /* N_PLAT_UNIX */
#endif

#ifdef __cplusplus
extern "C" {
#endif

NWCCODE N_API NWDisableBroadcasts
(
   NWCONN_HANDLE  conn
);

NWCCODE N_API NWEnableBroadcasts
(
   NWCONN_HANDLE  conn
);

NWCCODE N_API NWSendBroadcastMessage
(
   NWCONN_HANDLE  conn,
   pnstr8         message,
   nuint16        connCount,
   pnuint16       connList,
   pnuint8        resultList
);

NWCCODE N_API NWGetBroadcastMessage
(
   NWCONN_HANDLE  conn,
   pnstr8         message
);

NWCCODE N_API NWGetBroadcastMode
(
   NWCONN_HANDLE  conn,
   pnuint16       mode
);

NWCCODE N_API NWSetBroadcastMode
(
   NWCONN_HANDLE  conn,
   nuint16        mode
);

NWCCODE N_API NWBroadcastToConsole
(
   NWCONN_HANDLE  conn,
   pnstr8         message
);

NWCCODE N_API NWSendConsoleBroadcast
(
   NWCONN_HANDLE  conn,
   pnstr8         message,
   nuint16        connCount,
   pnuint16       connList
);


#ifdef __cplusplus
}
#endif

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwmsg.h,v 1.6 1994/06/08 23:33:13 rebekah Exp $
*/
