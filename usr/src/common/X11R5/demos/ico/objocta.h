/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4ico:objocta.h	1.1"
/* objocta.h - structure values for octahedron */

{	"octahedron", "octa",	/* long and short names */
	"cube",		/* long name of dual */
	6, 12, 8,	/* number of vertices, edges, and faces */
	{		/* vertices (x,y,z) */
			/* all points must be within radius 1 of the origin */
#define T 1.0
		{  T,  0,  0 },
		{ -T,  0,  0 },
		{  0,  T,  0 },
		{  0, -T,  0 },
		{  0,  0,  T },
		{  0,  0, -T },
#undef T
	},
	{	/* faces (numfaces + indexes into vertices) */
		/*  faces must be specified clockwise from the outside */
		3,	0, 4, 2,
		3,	0, 2, 5,
		3,	0, 5, 3,
		3,	0, 3, 4,
		3,	1, 2, 4,
		3,	1, 5, 2,
		3,	1, 3, 5,
		3,	1, 4, 3,
	}
},		/* leave a comma to separate from the next include file */
/* end */
