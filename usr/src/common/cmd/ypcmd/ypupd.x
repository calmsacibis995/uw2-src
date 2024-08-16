/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ypcmd:ypupd.x	1.2.6.2"
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
*	(c) 1986,1987,1988.1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

/*
 * YP update service protocol
 */
const MAXMAPNAMELEN = 255;
const MAXYPDATALEN  = 1023;
const MAXERRMSGLEN  = 255;

program YPU_PROG {
	version YPU_VERS {
		u_int YPU_CHANGE(ypupdateargs) = 1;
		u_int YPU_INSERT(ypupdateargs) = 2;
		u_int YPU_DELETE(ypdeleteargs) = 3;
		u_int YPU_STORE(ypupdateargs)  = 4;
	} = 1;
} = 100028;

typedef opaque yp_buf<MAXYPDATALEN>;

struct ypupdate_args {
	string mapname<MAXMAPNAMELEN>;
	yp_buf key;
	yp_buf datum;
};

struct ypdelete_args {
	string mapname<MAXMAPNAMELEN>;
	yp_buf key;
};

