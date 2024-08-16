/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)exclusives:Exclusives.h	1.9"
#endif

#ifndef _OlExclusives_h
#define _OlExclusives_h

/*
 * Author:	Karen S. Kendler
 * Date:	16 August 1988
 * File:	Exclusives.h - Public definitions for Exclusives widget
 *
 *	Copyright (c) 1989 AT&T
 */

#include <Xol/Manager.h>	/* include superclasses' header */

extern WidgetClass     exclusivesWidgetClass;

typedef struct _ExclusivesClassRec   *ExclusivesWidgetClass;
typedef struct _ExclusivesRec        *ExclusivesWidget;

#endif /*  _OlExclusives_h  */

/* DON'T ADD STUFF AFTER THIS */
