/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ifndef	LOG_MGMT_H
#define	LOG_MGMT_H
/*==================================================================*/
/*
**
*/
#ident	"@(#)lp:include/logMgmt.h	1.4.5.4"
#ident	"$Header: $"

#include	"boolean.h"
#include	"pwd.h"


/*------------------------------------------------------------------*/
/*
*/
#ifdef	__STDC__

extern	void	WriteLogMsg (char *);
extern	void	SetLogMsgTagFn (char *(*) ());
extern	boolean	OpenLogFile (char *);

#else

extern	void	WriteLogMsg ();
extern	void	SetLogMsgTagFn ();
extern	boolean	OpenLogFile ();

#endif
/*==================================================================*/
#endif
