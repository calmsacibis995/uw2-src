/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/sap_err.c	1.7"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: sap_err.c,v 1.11 1994/09/06 17:54:22 mark Exp $"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */
#include <stdio.h>
#include <stdarg.h>
#include <sys/sap_app.h>
#include "nwmsg.h"
#include "nwnet/utilmsgtable.h"
#include "nwconfig.h"
#include <util_proto.h>

int
SAPPerror(int saperr, char * format, ...)
{
	va_list		ap;
	int			cc;
	static	int	bound = 0;

	if( bound == 0) {
		cc = MsgBindDomain(MSG_DOMAIN_SAPL, MSG_DOMAIN_UTIL_FILE, MSG_UTIL_REV_STR);
		if(cc != NWCM_SUCCESS) {
			return cc;
		}
		bound++;
	}

	saperr = -saperr;

	if(saperr < 1 || saperr > SAP_ERRNO_LAST)
	{
		fprintf(stderr, "%s", MsgDomainGetStr(MSG_DOMAIN_SAPL, SAPM_OUT_RANGE));
		fprintf(stderr, "%d\n", saperr);
		return(0);
	}

	va_start(ap, format);
	cc = vfprintf(stderr, format, ap);
	va_end(ap);

	cc |= fprintf(stderr, "%s", MsgDomainGetStr(MSG_DOMAIN_SAPL, saperr));

	if( saperr < SAPL_ERRNO_START)
		return cc;
	if( saperr < SAPD_START) {
		perror("");
	}
	return cc;
}
