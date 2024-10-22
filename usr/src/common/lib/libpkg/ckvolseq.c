/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/ckvolseq.c	1.6.5.5"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/lib/libpkg/ckvolseq.c,v 1.1 91/02/28 20:55:03 ccs Exp $"

#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <pkgstrct.h>

extern char	errbuf[];
extern int	access(),
		cverify();
extern void	logerr();

#define PKGMAP	"pkgmap"
#define PKGINFO	"pkginfo"

#define MSG_SEQ		"uxpkgtools:601:Volume is out of sequence."
#define MSG_CORRUPT \
	"uxpkgtools:602:Volume is corrupt or is not part of the appropriate package."
#define ERR_NOPKGMAP	"uxpkgtools:603:ERROR:unable to process <%s>"
#define ERR_BADPKGINFO	"uxpkgtools:603:ERROR:unable to process <%s>"

int
ckvolseq(dir, part, nparts, dologerr)
char	*dir;
int	part, nparts, dologerr;
{
	static struct cinfo cinfo;
	char	ftype, path[PATH_MAX];

	if(part > 0) {
		ftype = 'f';
		if(part == 1) {
			/* 
			 * save stats about content information of pkginfo
			 * file in order to verify multi-volume packages
			 */
			cinfo.cksum = cinfo.size = cinfo.modtime = (-1L);
			(void) sprintf(path, "%s/pkginfo", dir);
			if(cverify(0, &ftype, path, &cinfo)) {
				if ( dologerr == LOG ) {
					logerr(ERR_BADPKGINFO, path);
					logerr(errbuf);
				}
				return(1);
			}
			(void) sprintf(path, "%s/pkgmap", dir);
			if(access(path, 0)) {
				if ( dologerr == LOG )
					logerr(ERR_NOPKGMAP, path);
				return(2);
			}
		} else {
			/* temp fix due to summit problem */
			cinfo.modtime = (-1);

			/* pkginfo file doesn't match first floppy */
			(void) sprintf(path, "%s/pkginfo", dir);
			if(cverify(0, &ftype, path, &cinfo)) {
				if ( dologerr == LOG ) {
					logerr(MSG_CORRUPT);
					logerr(errbuf);
				}
				return(1);
			}
		}
	} else
		part = (-part);

	/*
	 * each volume in a multi-volume package must
	 * contain either the root.n or reloc.n directories
	 */
	if(nparts != 1) {
		/* look for multi-volume specification */
		(void) sprintf(path, "%s/root.%d", dir, part);
		if(access(path, 0) == 0)
			return(0);
		(void) sprintf(path, "%s/reloc.%d", dir, part);
		if(access(path, 0) == 0)
			return(0);
		if(part == 1) {
			(void) sprintf(path, "%s/install", dir, part);
			if(access(path, 0) == 0)
				return(0);
		} 
		if(nparts) {
			if ( dologerr == LOG )
				logerr(MSG_SEQ);
			return(2);
		}
	}
	return(0);
}
