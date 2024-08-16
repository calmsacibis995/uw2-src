/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/smsdrapi.h	1.1"

#if !defined(_SMSDRAPI_H_INCLUDED)
#define _SMSDRAPI_H_INCLUDED

#include <smsdrerr.h>

	CCODE NWSMListTSAs(
		char *pattern, 
		NWSM_NAME_LIST **tsaNameList);

	CCODE NWSMListSMDRs(
		char *pattern, 
		NWSM_NAME_LIST **tsaNameList);

	CCODE NWSMTSListTargetServices(
		UINT32 connectionID, 
		char *pattern, 
		NWSM_NAME_LIST **tsNameList);

	CCODE NWSMConvertError(UINT32 connectionID, CCODE error, STRING message);
#endif
