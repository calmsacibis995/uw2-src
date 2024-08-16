/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libexp/common/Const.h	1.3"

#include "Itype.h"

enum ConstKind {
    CK_CHAR, CK_INT, CK_UINT, CK_LONG, CK_ULONG, CK_FLOAT, CK_DOUBLE, CK_XFLOAT
};

struct Const {
    ConstKind const_kind;
    union {
	char		c;
	int		i;
	unsigned int	ui;
	long		l;
	unsigned long	ul;
	float		f;
	double		d;
	Ixfloat		x;
    };

    Const& init(ConstKind, const char *);
};
