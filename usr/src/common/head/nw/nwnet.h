/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwnet.h	1.4"
/****************************************************************************
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/
#ifndef	_NWNET_HEADER_
#define	_NWNET_HEADER_

#ifdef N_PLAT_UNIX
#ifndef __NWDSTYPE_H
#include <nw/nwdstype.h>
#endif

#ifndef _NWALIAS_H
#include <nw/nwalias.h>
#endif

#ifndef	__NWDSDEFS_H
#include <nw/nwdsdefs.h>
#endif

#ifndef __NWDSERR_H
#include <nw/nwdserr.h>
#endif

#ifndef	_NWDSNAME_HEADER_
#include <nw/nwdsname.h>
#endif

#ifndef	_NWDSFILT_HEADER_
#include <nw/nwdsfilt.h>
#endif

#ifndef	_NWDSDC_HEADER_
#include <nw/nwdsdc.h>
#endif

#ifndef 	_NWDSMISC_HEADER_
#include <nw/nwdsmisc.h>
#endif

#ifndef _NWDSACL_HEADER_
#include <nw/nwdsacl.h>
#endif

#ifndef	_NWDSAUD_HEADER_
#include <nw/nwdsaud.h>
#endif

#ifndef	_NWDSDSA_HEADER_
#include <nw/nwdsdsa.h>
#endif

#ifndef	_NWDSSCH_HEADER_
#include <nw/nwdssch.h>
#endif

#ifndef	_NWDSATTR_HEADER_
#include <nw/nwdsattr.h>
#endif

#ifndef	_NWDSASA_HEADER_
#include <nw/nwdsasa.h>
#endif

#ifndef	_NWDSPART_HEADER_
#include <nw/nwdspart.h>
#endif

#ifndef _NWDSBUFT_HEADER_
#include <nw/nwdsbuft.h>
#endif

#ifndef __NAMES_H
#include <nw/nwdsnmtp.h>
#endif

#ifndef _UNICODE_HEADER_
#include <nw/unicode.h>
#endif

#else /* !N_PLAT_UNIX */

#ifndef __NWDSTYPE_H
#include <nwdstype.h>
#endif

#ifndef _NWALIAS_H
#include <nwalias.h>
#endif

#ifndef	__NWDSDEFS_H
#include <nwdsdefs.h>
#endif

#ifndef __NWDSERR_H
#include <nwdserr.h>
#endif

#ifndef	_NWDSNAME_HEADER_
#include <nwdsname.h>
#endif

#ifndef	_NWDSFILT_HEADER_
#include <nwdsfilt.h>
#endif

#ifndef	_NWDSDC_HEADER_
#include <nwdsdc.h>
#endif

#ifndef 	_NWDSMISC_HEADER_
#include <nwdsmisc.h>
#endif

#ifndef _NWDSACL_HEADER_
#include <nwdsacl.h>
#endif

#ifndef	_NWDSAUD_HEADER_
#include <nwdsaud.h>
#endif

#ifndef	_NWDSDSA_HEADER_
#include <nwdsdsa.h>
#endif

#ifndef	_NWDSSCH_HEADER_
#include <nwdssch.h>
#endif

#ifndef	_NWDSATTR_HEADER_
#include <nwdsattr.h>
#endif

#ifndef	_NWDSASA_HEADER_
#include <nwdsasa.h>
#endif

#ifndef	_NWDSPART_HEADER_
#include <nwdspart.h>
#endif

#ifndef _NWDSBUFT_HEADER_
#include <nwdsbuft.h>
#endif

#ifndef __NAMES_H
#include <nwdsnmtp.h>
#endif

#ifndef _UNICODE_HEADER_
#include <unicode.h>
#endif

#endif /* N_PLAT_UNIX */

#ifndef NWNLM
#ifndef _AUDIT_H
#ifdef N_PLAT_UNIX
#include <nw/nwaudit.h>
#else /* !N_PLAT_UNIX */
#include <nwaudit.h>
#endif /* N_PLAT_UNIX */
#endif

#ifndef NWNDSCON_INC
#ifdef N_PLAT_UNIX
#include <nw/nwndscon.h>
#else /* !N_PLAT_UNIX */
#include <nwndscon.h>
#endif /* N_PLAT_UNIX */
#endif

#else

#ifndef _NWNDS_H_INCLUDED
#include <nwnds.h>
#endif

#endif /* NWNLM */

#endif
