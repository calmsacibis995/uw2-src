/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/sms.h	1.1"

#if !defined(_SMS_H_INCLUDED)
#define _SMS_H_INCLUDED

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if defined(__WATCOMC__) && !defined(NETWARE_V311) && !defined(NETWARE_V312)
#include <nwlocale.h>
#endif
#define _CLIB_HDRS_INCLUDED

#include <smstypes.h>
#include <smsdefns.h>
#include <smsdrapi.h>

#include <smspcode.h>
#if !defined(NETWARE_V311) && !defined(NETWARE_V312)
#include <smssdapi.h>
#endif

#include <smstsapi.h>

#include <smsutapi.h>

#include <smsfids.h>

#endif
