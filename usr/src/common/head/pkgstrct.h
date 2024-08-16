/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamhdrs:common/head/pkgstrct.h	1.9.5.7"
#ident  "$Header: $"

#define CLSSIZ	12
#define PKGSIZ	14
#define ATRSIZ	14
#define PRIVSIZ 255

#define BADFTYPE	'?'
#define BADMODE		0xFFFFFFFF
#define TMPMODE		0xFFFFFFFE
#define BADOWNER	"?"
#define BADGROUP	"?"
#define	BADMAC		0
#define	BADPRIV		"?"
#define BADID		0
#define BADMAJOR	0xFFFFFFFF
#define BADMINOR	0xFFFFFFFF
#define BADCLASS	"none"
#define BADINPUT	1 /* not EOF */
#define BADCONT		(-1L)
	
extern char	*errstr;
	
struct ainfo {
	char	*local;
	mode_t	mode;
	char	owner[ATRSIZ+1];
	char	group[ATRSIZ+1];
	int	macid;		   /* security related - MAC level */
	char	priv_fix[PRIVSIZ+1]; /* security related - fixed privileges */
	char	priv_inh[PRIVSIZ+1]; /* security related - inheritable privileges */
	major_t	major;
	minor_t	minor;
};

struct cinfo {
	long	cksum;
	long	size;
	long	modtime;
};

struct pinfo {
	char	status;
	char	pkg[PKGSIZ+1];
	char	editflag;
	char	aclass[ATRSIZ+1];
	struct pinfo	
		*next;
};

struct cfent {
	short	volno;
	char	ftype;
	char	class[CLSSIZ+1];
	char	*path;
	struct ainfo ainfo;
	struct cinfo cinfo;
	short	npkgs;
	short	quoted;
	struct pinfo	
		*pinfo;
};

/* averify() & cverify() error codes */
#define	VE_EXIST	0x0001
#define	VE_FTYPE	0x0002
#define	VE_ATTR		0x0004
#define	VE_CONT		0x0008
#define	VE_FAIL		0x0010
#define VE_TIME		0x0020

/* ckvolseq() 4th arg options */
#define LOG		1
#define NOLOG		0
