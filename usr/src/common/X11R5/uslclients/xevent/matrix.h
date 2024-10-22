/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xevent:matrix.h	1.1"
#endif
/*
 matrix.h (C header file)
	Acc: 575326449 Fri Mar 25 15:54:09 1988
	Mod: 575321188 Fri Mar 25 14:26:28 1988
	Sta: 575570329 Mon Mar 28 11:38:49 1988
	Owner: 2011
	Group: 1985
	Permissions: 644
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/************************************************************************

	Copyright 1987 by AT&T
	All Rights Reserved

	author:
		Ross Hilbert
		AT&T 12/07/87
************************************************************************/

#ifndef _MATRIX_H
#define _MATRIX_H

#define COL_MAJOR	0
#define ROW_MAJOR	1

typedef struct
{
	int rows;
	int cols;
	int order;
	int width;
	int height;
	int pad;
}
	MatrixAttributes;

extern void GetMatrixDimensions ();
extern void GetCellOrigin ();
extern void GetCellIndex ();
extern void GetCellAddress ();
extern void GetCellAtPoint ();

#endif
