/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/restore.d/hdrs/rs.h	1.3.5.3"
#ident  "$Header: rs.h 1.2 91/06/21 $"

/* Are we a restore command or a urestore command */
#define	IS_RESTORE(c)	(!strcmp(c, "restore"))
#define	IS_URESTORE(c)	(!strcmp(c, "urestore"))

/* Option flag #defines */
#define	AFLAG	0x1
#define	cFLAG	0x2
#define	dFLAG	0x4
#define	DFLAG	0x8
#define	FFLAG	0x10
#define	mFLAG	0x20
#define	nFLAG	0x40
#define	oFLAG	0x80
#define	PFLAG	0x100
#define	vFLAG	0x200
#define	sFLAG	0x400
#define	SFLAG	0x800
/* SVR3.2 Functionality flags */
#define MFLAG	0x1000
#define OFLAG	0x2000
#define TFLAG	0x4000
#define WFLAG	0x8000
#define iFLAG	0x10000
#define wFLAG	0x20000

#define	NFLAGS	12

#define	NRESTORES	50	/* Only restore NRESTORES objects per command */

