/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _BLURB_
#define _BLURB_
/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

/************************************************************************
          Copyright 1988, 1991 by Carnegie Mellon University

                          All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted, provided
that the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of Carnegie Mellon University not be used
in advertising or publicity pertaining to distribution of the software
without specific, written prior permission.

CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
************************************************************************/
#endif /* _BLURB_ */

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.bootpd/bootpd.h	1.3"

/*
 * bootpd.h -- common header file for all the modules of the bootpd program.
 */

#include "hash.h"

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

#ifndef PRIVATE
#define PRIVATE static
#endif

#ifndef SIGUSR1
#define SIGUSR1			 30	/* From 4.3 <signal.h> */
#endif

#define MAXHADDRLEN		  6	/* Max hw address length in bytes */
#define MAXSTRINGLEN		 80	/* Max string length */
#define SUBNETKEY		 "..net"/* first part of name for net entry */


/*
 * Return pointer to static string which gives full network error message.
 */
#define get_network_errmsg get_errmsg


/*
 * Nice typedefs. . .
 */

typedef int boolean;
typedef unsigned char byte;


/*
 * Data structure used to hold an arbitrary-lengthed list of IP addresses.
 * The list may be shared among multiple hosts by setting the linkcount
 * appropriately.
 */

struct in_addr_list {
    unsigned		linkcount, addrcount;
    struct in_addr	addr[1];		/* Dynamically extended */
};


/*
 * Data structures used to hold shared strings and shared binary data.
 * The linkcount must be set appropriately.
 */

struct shared_string {
    unsigned		linkcount;
    char		string[1];		/* Dynamically extended */
};

struct shared_bindata {
    unsigned		linkcount, length;
    byte		data[1];		/* Dynamically extended */
};


/*
 * Flag structure which indicates which symbols have been defined for a
 * given host.  This information is used to determine which data should or
 * should not be reported in the bootp packet vendor info field.
 */

struct flag {
    unsigned	bootfile	:1,
		bootserver	:1,
		bootsize	:1,
		bootsize_auto	:1,
		cookie_server	:1,
		domain_server	:1,
		gateway		:1,
		generic		:1,
		haddr		:1,
		homedir		:1,
		htype		:1,
		impress_server	:1,
		iaddr		:1,
		log_server	:1,
		lpr_server	:1,
		name_server	:1,
		name_switch	:1,
		rlp_server	:1,
		send_name	:1,
		subnet_mask	:1,
		tftpdir		:1,
		time_offset	:1,
		time_server	:1,
		dump_file	:1,
		domain_name	:1,
		swap_server	:1,
		root_path	:1,
		exten_file	:1,
		reply_addr	:1,
		vm_cookie	:1;
};



/*
 * The flags structure contains TRUE flags for all the fields which
 * are considered valid, regardless of whether they were explicitly
 * specified or indirectly inferred from another entry.
 *
 * The gateway and the various server fields all point to a shared list of
 * IP addresses.
 *
 * The hostname, home directory, and bootfile are all shared strings.
 *
 * The generic data field is a shared binary data structure.  It is used to
 * hold future RFC1048 vendor data until bootpd is updated to understand it.
 *
 * The vm_cookie field specifies the four-octet vendor magic cookie to use
 * if it is desired to always send the same response to a given host.
 *
 * Hopefully, the rest is self-explanatory.
 */

struct host {
    unsigned		    linkcount;		/* hash list inserts */
    struct flag		    flags;		/* ALL valid fields */
    struct in_addr_list	    *cookie_server,
			    *domain_server,
			    *gateway,
			    *impress_server,
			    *log_server,
			    *lpr_server,
			    *name_server,
			    *rlp_server,
			    *time_server;
    struct shared_string    *bootfile,
			    *hostname,
			    *domain_name,
			    *homedir,
			    *tftpdir,
    			    *dump_file,
    			    *exten_file,
    			    *root_path;
    struct shared_bindata   *generic;
    byte		    vm_cookie[4],
			    htype,  /* RFC826 says this should be 16-bits but
				       RFC951 only allocates 1 byte. . . */
			    haddr[MAXHADDRLEN];
    long		    time_offset;
    unsigned int	    bootsize;
    struct in_addr	    bootserver,
			    iaddr,
			    swap_server,
			    reply_addr,
			    subnet_mask;
};


struct host_list {
    struct host 	*host_ptr;
    struct host_list 	*next;
};


/*
 * Variables shared among modules.
 */

extern int debug;
extern char *bootptab;
extern char *progname;

extern u_char vm_cmu[4];
extern u_char vm_rfc1048[4];

extern hash_tbl *hwhashtable;
extern hash_tbl *iphashtable;
extern hash_tbl *nmhashtable;

