/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)flat:FCheckBoxP.h	1.5"
#endif

#ifndef _OL_FCHECKBOXP_H
#define _OL_FCHECKBOXP_H

/************************************************************************
    Description:
	This is the flat check box container's private header file.
*/

#include <Xol/FButtonsP.h>	/* superclasses' header */
#include <Xol/FCheckBox.h>	/* public header */

/************************************************************************
    Define Widget Class Part and Class Rec
*/

	/* Full class record declaration.  Since this class adds no new
	   fields over its superclass, just use typedefs for this class's
	   structures.
	*/

typedef FlatButtonsClassRec	_FlatCheckBoxClassRec;
typedef FlatButtonsClassRec	FlatCheckBoxClassRec;

				/* External class record declaration */

extern FlatCheckBoxClassRec flatCheckBoxClassRec;

/************************************************************************
    Define Item Instance Structure
*/

typedef FlatButtonsItemRec FlatCheckBoxItemRec;

/************************************************************************
    Define Widget Instance Structure

*/

	/* Full instance record declaration.  Since this class adds no new
	   fields over its superclass, just use typedefs for this class's
	   instance structure.
	*/

typedef FlatButtonsRec	_FlatCheckBoxRec;
typedef FlatButtonsRec	FlatCheckBoxRec;

#endif /* _OL_FCHECKBOXP_H */
