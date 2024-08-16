/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)oampkg:common/cmd/oampkg/libinst/qreason.c	1.2.6.4"
#ident  "$Header: qreason.c 1.2 91/06/27 $"

#include <pfmt.h>

char *
qreason(retcode)
{
	char	*status;

	switch(retcode) {
	  case  0:
	  case 10:
	  case 20:
		status = gettxt("uxpkgtools:114", "was successful");
		break;

	  case  1:
	  case 11:
	  case 21:
		status = gettxt("uxpkgtools:115", "failed");
		break;

	  case  2:
	  case 12:
	  case 22:
		status = gettxt("uxpkgtools:116", "partially failed");
		break;

	  case  3:
	  case 13:
	  case 23:
		status = gettxt("uxpkgtools:117", "was terminated due to user request");
		break;

	  case  4:
	  case 14:
	  case 24:
		status = gettxt("uxpkgtools:118", "was suspended (administration)");
		break;

	  case  5:
	  case 15:
	  case 25:
		status = gettxt("uxpkgtools:119", "was suspended (interaction required)");
		break;

	  case 99:
		status = gettxt("uxpkgtools:120", "failed (internal error)");
		break;

	  default:
		status = gettxt("uxpkgtools:121", "failed with an unrecognized error code.");
		break;
	}

	return(status);
}

