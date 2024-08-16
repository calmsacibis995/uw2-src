/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libsymbol/common/Syminfo.h	1.1"
#ifndef Syminfo_h
#define Syminfo_h

enum sym_bind	{ sb_none, sb_local, sb_global, sb_weak };

enum sym_type	{ st_none, st_object, st_func, st_section, st_file };

struct Syminfo {
	long		name;
	long		lo,hi;
	unsigned char	bind,type;
	long		sibling,child;
	int		resolved;
};

#endif /* Syminfo_h */
