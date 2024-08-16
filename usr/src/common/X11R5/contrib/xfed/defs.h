/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xfed:defs.h	1.1"
extern int errno;
/*
extern char *malloc();
extern char *realloc();
*/
struct box {
	int w, h;
	int x, y;
};

struct character {
	char    *charId;
	int     encoding;
	int	notadobe;
	int     swidth[2];
	int     dwidth[2];
	struct  box bbx;
	int     nrows;
	char    **rows;
};

struct font {
	int     rev, subrev;
	char    **comments;
	unsigned int	ncomments, szcomments;
	char    *fontname;
	int     sizes[3];
	struct box     boundingbox;
	char	**props;
	unsigned int    nprops;
	int     nchars;
	struct box maxbbx;
	struct  character *characters;
};

struct font font;
