/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/attrb.h	1.4"
#ifndef	_ATTRB_H
#define _ATTRB_H

#define	ACTIVE		1
#define	GRAYED		2
#define	GREYED		2
#define	INVISIBLE	4
#define	NONACTIVE	8
#define	DISPLAY		16
#define	REQUIRED	32
#define	NOVISIBLE	64
#define	DATAVALID	128
#define	APPENDDATA	256
#define NONCURRENT	512
#define FORMTITLE	1024


typedef	struct vchar_attr{
	ulong	c_flgs;
	ulong	c_attr;
} VCHAR_ATTR;

#define	ATTR_CHG	0x80000000
#define	ATTR_CHTYPE	0x40000000

#endif	_ATTRB_H
