/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwcalls.h	1.4"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
/****************************************************************************
  This header will include ALL the nwcalls include files. To
  exclude one or more specific headers, use the following defines:

        #define NOAFP_INC
        #define NOBINDRY_INC
        #define NOCLIENT_INC
        #define NOCONNECT_INC
        #define NODEL_INC
        #define NODENTRY_INC
        #define NODIRECTORY_INC
        #define NODPATH_INC
        #define NOEA_INC
        #define NOERROR_INC
        #define NOFILES_INC
        #define NOMESSAGES_INC
        #define NOMISC_INC
        #define NONAMSPC_INC
        #define NONTTS_INC
        #define NOPRINT_INC
        #define NOQUEUE_INC
        #define NOSERVER_INC
        #define NOSYNC_INC
        #define NOVOL_INC
        #define NOFSE_INC
        #define NOMIGRATE_INC
****************************************************************************/
#ifndef NWCALLS_INC
#define NWCALLS_INC

#ifdef N_PLAT_UNIX
#include <nw/nwcaldef.h>
#include <nw/nwalias.h>

#ifndef NOAFP_INC
#include <nw/nwafp.h>
#endif

#ifndef NOBINDRY_INC
#include <nw/nwbindry.h>
#endif

#ifndef NOCLIENT_INC
#include <nw/nwclient.h>
#endif

#ifndef NOCONNECT_INC
#include <nw/nwconnec.h>
#endif

#ifndef NODEL_INC
#include <nw/nwdel.h>
#endif

#ifndef NODENTRY_INC
#include <nw/nwdentry.h>
#endif

#ifndef NODIRECTORY_INC
#include <nw/nwdirect.h>
#endif

#ifndef NODPATH_INC
#include <nw/nwdpath.h>
#endif

#ifndef NOEA_INC
#include <nw/nwea.h>
#endif

#ifndef NOERROR_INC
#include <nw/nwerror.h>
#endif

#ifndef NOFILES_INC
#include <nw/nwfile.h>
#endif

#ifndef NOMISC_INC
#include <nw/nwmisc.h>
#endif

#ifndef NOMESSAGES_INC
#include <nw/nwmsg.h>
#endif

#ifndef NONAMSPC_INC
#include <nw/nwnamspc.h>
#endif

#ifndef NOQUEUE_INC
#include <nw/nwqms.h>
#endif

#ifndef NOSERVER_INC
#include <nw/nwserver.h>
#endif

#ifndef NOSYNC_INC
#include <nw/nwsync.h>
#endif

#ifndef NONTTS_INC
#include <nw/nwtts.h>
#endif

#ifndef NOVOL_INC
#include <nw/nwvol.h>
#endif

#ifndef NOACCT_INC
#include <nw/nwacct.h>
#endif

#ifndef NOFSE_INC
#include <nw/nwfse.h>
#endif

#ifndef NOMIGRATE_INC
#include <nw/nwmigrat.h>
#endif

#else /* !N_PLAT_UNIX */
#include <nwcaldef.h>
#include <nwalias.h>

#ifndef NOAFP_INC
#include <nwafp.h>
#endif

#ifndef NOBINDRY_INC
#include <nwbindry.h>
#endif

#ifndef NOCLIENT_INC
#include <nwclient.h>
#endif

#ifndef NOCONNECT_INC
#include <nwconnec.h>
#endif

#ifndef NODEL_INC
#include <nwdel.h>
#endif

#ifndef NODENTRY_INC
#include <nwdentry.h>
#endif

#ifndef NODIRECTORY_INC
#include <nwdirect.h>
#endif

#ifndef NODPATH_INC
#include <nwdpath.h>
#endif

#ifndef NOEA_INC
#include <nwea.h>
#endif

#ifndef NOERROR_INC
#include <nwerror.h>
#endif

#ifndef NOFILES_INC
#include <nwfile.h>
#endif

#ifndef NOMISC_INC
#include <nwmisc.h>
#endif

#ifndef NOMESSAGES_INC
#include <nwmsg.h>
#endif

#ifndef NONAMSPC_INC
#include <nwnamspc.h>
#endif

#ifndef NOPRINT_INC
#include <nwprint.h>
#endif

#ifndef NOQUEUE_INC
#include <nwqms.h>
#endif

#ifndef NOSERVER_INC
#include <nwserver.h>
#endif

#ifndef NOSYNC_INC
#include <nwsync.h>
#endif

#ifndef NONTTS_INC
#include <nwtts.h>
#endif

#ifndef NOVOL_INC
#include <nwvol.h>
#endif

#ifndef NOACCT_INC
#include <nwacct.h>
#endif

#ifndef NOFSE_INC
#include <nwfse.h>
#endif

#ifndef NOMIGRATE_INC
#include <nwmigrat.h>
#endif

#endif /* N_PLAT_UNIX */

#endif /* NWCALLS_INC */

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwcalls.h,v 1.7 1994/06/08 23:32:28 rebekah Exp $
*/
