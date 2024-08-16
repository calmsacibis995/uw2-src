/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/tputcfent.c	1.7.6.6"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/lib/libpkg/tputcfent.c,v 1.1 91/02/28 20:55:41 ccs Exp $"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <pkgstrct.h>

#include <pfmt.h>
	
extern int	cftime();

#define user_pub 4

void
tputcfent(ept, fp)
struct cfent *ept;
FILE *fp;
{
	int	count, status;
	char	*pt;
	struct pinfo *pinfo;

	if(ept->path == NULL)
		return;

	(void) pfmt(fp, MM_NOSTD, "uxpkgtools:702:Pathname: %s\n", ept->path);
	(void) pfmt(fp, MM_NOSTD, "uxpkgtools:703:Type: ");

	switch(ept->ftype) {
	  case 'f':
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:704:regular file\n");
		break;

	  case 'd':
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:705:directory\n");
		break;

	  case 'x':
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:706:exclusive directory\n");
		break;

	  case 'v':
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:707:volatile file\n");
		break;

	  case 'e':
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:708:editted file\n");
		break;

	  case 'p':
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:709:named pipe\n");
		break;

	  case 'i':
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:710:installation file\n");
		break;

	  case 'c':
	  case 'b':
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:711:%s special device\n", 
			(ept->ftype == 'b') ? gettxt("uxpkgtools:712", "block") : gettxt("uxpkgtools:713", "character"));
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:714:Major device number: %d\n", 
			ept->ainfo.major);
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:715:Minor device number: %d\n", 
			ept->ainfo.minor);
		break;

	  case 'l':
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:716:linked file\n");
		pt = (ept->ainfo.local ? ept->ainfo.local : gettxt("uxpkgtools:113", "(unknown)"));
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:717:Source of link: %s\n", pt);
		break;

	  case 's':
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:718:symbolic link\n");
		pt = (ept->ainfo.local ? ept->ainfo.local : gettxt("uxpkgtools:113", "(unknown)"));
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:717:Source of link: %s\n", pt);
		break;

	  case 'L':
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:794:advisory link\n");
		pt = (ept->ainfo.local ? ept->ainfo.local : gettxt("uxpkgtools:113", "(unknown)"));
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:795:Source of link or copy: %s\n", pt);
		break;

	  default:
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:719:unknown\n");
		break;
	}

	if(!strchr("lsinL", ept->ftype)) {
		if(ept->ainfo.mode < 0)
			(void) pfmt(fp, MM_NOSTD, "uxpkgtools:720:Expected mode: ?\n");
		else
			(void) pfmt(fp, MM_NOSTD, "uxpkgtools:721:Expected mode: %o\n", 
				ept->ainfo.mode);
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:722:Expected owner: %s\n", ept->ainfo.owner);
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:723:Expected group: %s\n", ept->ainfo.group);
		if(ept->ainfo.macid < 0)
			(void) pfmt(fp, MM_NOSTD, "uxpkgtools:724:Expected MAC level (security field): %d\n", user_pub);
		else
			(void) pfmt(fp, MM_NOSTD, "uxpkgtools:724:Expected MAC level (security field): %d\n", ept->ainfo.macid);
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:725:Expected fixed privileges (security field): %s\n", ept->ainfo.priv_fix);
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:726:Expected inheritable privileges (security field): %s\n", ept->ainfo.priv_inh);
	}
	if(strchr("?infv", ept->ftype)) {
		char	timebuf[64];

		(void) cftime(timebuf, "%c", &(ept->cinfo.modtime));
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:727:Expected file size (bytes): %ld\n", 
			ept->cinfo.size);
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:728:Expected sum(1) of contents: %ld\n", 
			ept->cinfo.cksum);
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:729:Expected last modification: %s\n", 
		  (ept->cinfo.modtime > 0) ? timebuf : "?");
	}
	if(ept->ftype == 'i') {
		(void) fputc('\n', fp);
		return;
	}

	status = count = 0;
	if(pinfo = ept->pinfo) {
		(void) pfmt(fp, MM_NOSTD, "uxpkgtools:730:Referenced by the following packages:\n\t");
		while(pinfo) {
			if(pinfo->status)
				status++;
			(void) fprintf(fp, "%-15s", pinfo->pkg);
			if((++count % 5) == 0) {
				(void) fputc('\n', fp);
				(void) fputc('\t', fp);
				count = 0;
			}
			pinfo = pinfo->next;
		}
		(void) fputc('\n', fp);
	}
	(void) pfmt(fp, MM_NOSTD, "uxpkgtools:731:Current status: %s\n", status ? 
		gettxt("uxpkgtools:246", "partially installed") : gettxt("uxpkgtools:157", "installed"));
	(void) fputc('\n', fp);
}
