/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:textedit/basic.h	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: basic.h,v $ $Revision: 1.3 $ $Date: 92/03/13 15:43:14 $ */

/************************************************************
 *     basic.h -- basic functions and macros
 ************************************************************/

#define false 0
#define true 1

#include <X11/Intrinsic.h>

#define BasicMalloc(siz) XtMalloc(siz)
#define BasicRealloc(buf,siz) XtRealloc(buf,siz)
#define BasicFree(ptr) XtFree(ptr)

#define strdup(strto,strfrom) ( (strto) = XtMalloc(strlen(strfrom)+1), strcpy(strto,strfrom) )
