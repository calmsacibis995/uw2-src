/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)head-nuc:msgmaxlen.h	1.1"
#ident "$Id: msgmaxlen.h,v 1.1 1994/06/09 18:03:30 vtag Exp $"
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
 *
 *
 */
#ifndef _msgmaxlen_h
#define _msgmaxlen_h

/*
 * nl_msgmax.h - This file retrieves the value NL_TEXTMAX from the appropriate
 * system include file.
 */

/* Make sure limits.h has not been included yet. We do this by
 * checking to see if CHAR_BIT has been defined. The note below
 * will generate a compile time error.
 */
#if defined(CHAR_BIT)
 Note to developers: You must include this file before limits.h,
 because there are two definitions of NL_TEXTMAX. We want the
 definition in nl_types.h, not limits.h.
#endif

#include <nl_types.h>
#define MSG_MAX_LEN (NL_TEXTMAX)	/* Length of strings. */

#endif /* _msgmaxlen_h */



