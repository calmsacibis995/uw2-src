/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/netselect/netcspace.h	1.8.8.1"
#ident  "$Header: $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*       PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*       Copyright Notice
*
* Notice of copyright on this source code product does not indicate
*  publication.
*
*       (c) 1986,1987,1988.1989  Sun Microsystems, Inc
*       (c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*       (c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/

struct nc_data {
	char         *string;
	unsigned long value;
};

const static struct nc_data nc_semantics[] = {
	"tpi_clts",	NC_TPI_CLTS,
	"tpi_cots",	NC_TPI_COTS,
	"tpi_cots_ord",	NC_TPI_COTS_ORD,
	"tpi_raw",	NC_TPI_RAW,
	NULL,		(unsigned)-1L
};

const static struct nc_data nc_flag[] = {
	"-",		NC_NOFLAG,
	"v",		NC_VISIBLE,
	"b",		NC_BROADCAST,
	NULL,		(unsigned)-1L
};

#define	NC_NOERROR	0
#define	NC_NOMEM	1
#define	NC_NOSET	2
#define	NC_OPENFAIL	3
#define	NC_BADLINE	4
#define	NC_BAD_HANDLE	5
