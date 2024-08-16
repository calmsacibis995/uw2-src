/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/yp/yperr_string.c	1.2.8.2"
#ident  "$Header: $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

#include <rpcsvc/ypclnt.h>
extern char *gettxt();

/*
 * This returns a pointer to an error message string appropriate to an input
 * yp error code.  An input value of zero will return a success message.
 * In all cases, the message string will start with a lower case chararacter,
 * and will be terminated neither by a period (".") nor a newline.
 */
/* Need to comment out 'const' to get cia to work */
const char *
yperr_string(code)
	int code;
{
	char *pmesg;
	
	switch (code) {

	case 0:  {
		pmesg = gettxt("uxnsl:200", "yp operation succeeded");
		break;
	}
		
	case YPERR_BADARGS:  {
		pmesg = gettxt("uxnsl:201", "args to yp function are bad");
		break;
	}
	
	case YPERR_RPC:  {
		pmesg = gettxt("uxnsl:202", "RPC failure on yp operation");
		break;
	}
	
	case YPERR_DOMAIN:  {
		pmesg = gettxt("uxnsl:203", "can't bind to a server which serves domain");
		break;
	}
	
	case YPERR_MAP:  {
		pmesg = gettxt("uxnsl:204", "no such map in server's domain");
		break;
	}
		
	case YPERR_KEY:  {
		pmesg = gettxt("uxnsl:205", "no such key in map");
		break;
	}
	
	case YPERR_YPERR:  {
		pmesg = gettxt("uxnsl:206", "internal yp server or client error");
		break;
	}
	
	case YPERR_RESRC:  {
		pmesg = gettxt("uxnsl:207", "local resource allocation failure");
		break;
	}
	
	case YPERR_NOMORE:  {
		pmesg = gettxt("uxnsl:208", "no more records in map database");
		break;
	}
	
	case YPERR_PMAP:  {
		pmesg = gettxt("uxnsl:209", "can't communicate with rpcbind");
		break;
		}
		
	case YPERR_YPBIND:  {
		pmesg = gettxt("uxnsl:210", "can't communicate with ypbind");
		break;
		}
		
	case YPERR_YPSERV:  {
		pmesg = gettxt("uxnsl:211", "can't communicate with ypserv");
		break;
		}
		
	case YPERR_NODOM:  {
		pmesg = gettxt("uxnsl:212", "local domain name not set");
		break;
	}

	case YPERR_BADDB:  {
		pmesg = gettxt("uxnsl:213", "yp map data base is bad");
		break;
	}

	case YPERR_VERS:  {
		pmesg = gettxt("uxnsl:214", "yp client/server version mismatch");
		break;
	}

	case YPERR_ACCESS: {
		pmesg = gettxt("uxnsl:215", "permission denied");
		break;
	}

	case YPERR_BUSY: {
		pmesg = gettxt("uxnsl:216", "database is busy");
		break;
	}
		
	default:  {
		pmesg = gettxt("uxnsl:217", "unknown yp client error code");
		break;
	}
	
	}

	return(pmesg);
}
