/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unix_conv:common/old.ar.h	2.5.2.3"

/* archive file header format */


#define	OARMAG	0177545
struct	oar_hdr {
	char	ar_name[14];
	long	ar_date;
	char	ar_uid;
	char	ar_gid;
	int	ar_mode;
	long	ar_size;
};

#ifdef __STDC__
 #pragma pack(2)
#endif

struct	xar_hdr {
	char	ar_name[14];
	long	ar_date;
	char	ar_uid;
	char	ar_gid;
	unsigned short	ar_mode;
	long	ar_size;
};

#ifdef __STDC__
 #pragma pack()
#endif
