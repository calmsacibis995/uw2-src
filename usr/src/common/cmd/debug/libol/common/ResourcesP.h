/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _RESOURCESP_H
#define _RESOURCESP_H

#ident	"@(#)debugger:libol/common/ResourcesP.h	1.1"

// toolkit specific members of the Resources class
// included by ../../gui.d/common/Resources.h

struct XrmOptionDescRec;

#define RESOURCE_TOOLKIT_SPECIFICS		\
public:						\
	XrmOptionDescRec	*get_options(int &noptions);

#endif
