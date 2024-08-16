/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>

/* conflict with X11/Xos.h. Must undefine SHARELIB to get the correct time.h */
#undef SHARELIB
#undef timezone
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)olps:error.c	1.2"
#endif

/*
 *************************************************************************
 *
 * Description:
 *		This file merely includes the message strings
 *		for olprintscreen.
 *
 *******************************file*header*******************************
 */

						/* #includes go here	*/

#include <string.h>
#include <X11/IntrinsicP.h>

#include <psmsgstrs>
