/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.bootpd/dumptab.c	1.2"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * dumptab.c - handles dumping the database
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

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>	/* inet_ntoa */

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>

#ifdef SVR4
#include <memory.h>
/* Yes, memcpy is OK here (no overlapped copies). */
#define bcopy(a,b,c)    memcpy(b,a,c)
#define bzero(p,l)      memset(p,0,l)
#define bcmp(a,b,c)     memcmp(a,b,c)
#endif

#include <arpa/bootp.h>
#include "hash.h"
#include "bootpd.h"
#include "hwaddr.h"
#include "report.h"
#include "patchlevel.h"

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

static void dump_generic P((FILE *, struct shared_bindata *));
static void dump_host P((FILE *, struct host *));
static void list_ipaddresses P((FILE *, struct in_addr_list *));

#undef P

#ifndef	DEBUG
void dumptab(filename)
    char *filename;
{
    report(LOG_INFO, "No dumptab support!");
}
#else	/* DEBUG */

/*
 * Dump the internal memory database to bootpd_dump.
 */

void
dumptab(filename)
    char *filename;
{
    int n;
    struct host *hp;
    FILE *fp;
    long t;
    /* Print symbols in alphabetical order for reader's convenience. */
    static char legend[] = "#\n# Legend:\t(see bootptab.5)\n\
#\tfirst field -- hostname (not indented)\n\
#\tbf -- bootfile\n\
#\tbs -- bootfile size in 512-octet blocks\n\
#\tcs -- cookie servers\n\
#\tdf -- dump file name\n\
#\tdn -- domain name\n\
#\tds -- domain name servers\n\
#\tef -- extension file\n\
#\tgw -- gateways\n\
#\tha -- hardware address\n\
#\thd -- home directory for bootfiles\n\
#\thn -- host name set for client\n\
#\tht -- hardware type\n\
#\tim -- impress servers\n\
#\tip -- host IP address\n\
#\tlg -- log servers\n\
#\tlp -- LPR servers\n\
#\tns -- IEN-116 name servers\n\
#\tra -- reply address override\n\
#\trl -- resource location protocol servers\n\
#\trp -- root path\n\
#\tsa -- boot server address\n\
#\tsm -- subnet mask\n\
#\tsw -- swap server\n\
#\ttc -- template host (points to similar host entry)\n\
#\ttd -- TFTP directory\n\
#\tto -- time offset (seconds)\n\
#\tts -- time servers\n\
#\tvm -- vendor magic number\n\
#\tTn -- generic option tag n\n\
\n";
	
    /*
     * Open bootpd.dump file.
     */
    if ((fp = fopen(filename, "w")) == NULL) {
		report(LOG_ERR, "error opening \"%s\": %s",
		       filename, get_errmsg());
		exit(1);
    }
	
    t = time(NULL);
    fprintf(fp, "\n# %s %s.%d\n", progname, VERSION, PATCHLEVEL);
    fprintf(fp, "# %s: dump of bootp server database.\n", filename);
    fprintf(fp, "# Dump taken %s", ctime(&t));
    fwrite(legend, 1, sizeof(legend)-1, fp);
	
    n = 0;
    for (hp = (struct host *) hash_FirstEntry(nmhashtable); hp != NULL;
		 hp = (struct host *) hash_NextEntry(nmhashtable)) {
	    dump_host(fp, hp);
	    fprintf(fp, "\n");
	    n++;
    }
    fclose(fp);
	
    report(LOG_INFO, "dumped %d entries to \"%s\".", n, filename);
}



/*
 * Dump all the available information on the host pointed to by "hp".
 * The output is sent to the file pointed to by "fp".
 */

static void
dump_host(fp, hp)
	FILE *fp;
	struct host *hp;
{
    /* Print symbols in alphabetical order for reader's convenience. */
    if (hp) {
		fprintf(fp, "%s:", (hp->hostname ?
							hp->hostname->string : "?"));
		if (hp->flags.bootfile) {
			fprintf(fp, "\\\n\t:bf=%s:", hp->bootfile->string);
		}
		if (hp->flags.bootsize) {
			fprintf(fp, "\\\n\t:bs=");
			if (hp->flags.bootsize_auto) {
				fprintf(fp, "auto:");
			} else {
				fprintf(fp, "%d:", hp->bootsize);
			}
		}
		if (hp->flags.cookie_server) {
			fprintf(fp, "\\\n\t:cs=");
			list_ipaddresses(fp, hp->cookie_server);
			fprintf(fp, ":");
		}
		if (hp->flags.dump_file) {
			fprintf(fp, "\\\n\t:df=%s:", hp->dump_file->string);
		}
		if (hp->flags.domain_name) {
			fprintf(fp, "\\\n\t:dn=%s:", hp->domain_name->string);
		}
		if (hp->flags.domain_server) {
			fprintf(fp, "\\\n\t:ds=");
			list_ipaddresses(fp, hp->domain_server);
			fprintf(fp, ":");
		}
		if (hp->flags.exten_file) {
			fprintf(fp, "\\\n\t:ef=%s:", hp->exten_file->string);
		}
		if (hp->flags.gateway) {
			fprintf(fp, "\\\n\t:gw=");
			list_ipaddresses(fp, hp->gateway);
			fprintf(fp, ":");
		}
		/* FdC: swap_server (see below) */
		if (hp->flags.homedir) {
			fprintf(fp, "\\\n\t:hd=%s:", hp->homedir->string);
		}
		/* FdC: dump_file (see above) */
		/* FdC: domain_name (see above) */
		/* FdC: root_path (see below) */
		if (hp->flags.name_switch && hp->flags.send_name) {
			fprintf(fp, "\\\n\t:hn:");
		}
		if (hp->flags.htype) {
			int hlen = haddrlength(hp->htype);
			fprintf(fp, "\\\n\t:ht=%u:", (unsigned) hp->htype);
			if (hp->flags.haddr) {
				fprintf(fp, "ha=%s:",
					haddrtoa(hp->haddr, hlen));
			}
		}
		if (hp->flags.impress_server) {
			fprintf(fp, "\\\n\t:im=");
			list_ipaddresses(fp, hp->impress_server);
			fprintf(fp, ":");
		}
		/* NetBSD: swap_server (see below) */
		if (hp->flags.iaddr) {
			fprintf(fp, "\\\n\t:ip=%s:", inet_ntoa(hp->iaddr));
		}
		if (hp->flags.log_server) {
			fprintf(fp, "\\\n\t:lg=");
			list_ipaddresses(fp, hp->log_server);
			fprintf(fp, ":");
		}
		if (hp->flags.lpr_server) {
			fprintf(fp, "\\\n\t:lp=");
			list_ipaddresses(fp, hp->lpr_server);
			fprintf(fp, ":");
		}
		if (hp->flags.name_server) {
			fprintf(fp, "\\\n\t:ns=");
			list_ipaddresses(fp, hp->name_server);
			fprintf(fp, ":");
		}
		if (hp->flags.reply_addr) {
			fprintf(fp, "\\\n\t:ra=%s:", inet_ntoa(hp->reply_addr));
		}
		if (hp->flags.rlp_server) {
			fprintf(fp, "\\\n\t:rl=");
			list_ipaddresses(fp, hp->rlp_server);
			fprintf(fp, ":");
		}
		if (hp->flags.root_path) {
			fprintf(fp, "\\\n\t:rp=%s:", hp->root_path->string);
		}
		if (hp->flags.bootserver) {
			fprintf(fp, "\\\n\t:sa=%s:", inet_ntoa(hp->bootserver));
		}
		if (hp->flags.subnet_mask) {
			fprintf(fp, "\\\n\t:sm=%s:", inet_ntoa(hp->subnet_mask));
		}
		if (hp->flags.swap_server) {
			fprintf(fp, "\\\n\t:sw=%s:", inet_ntoa(hp->subnet_mask));
		}
		if (hp->flags.tftpdir) {
			fprintf(fp, "\\\n\t:td=%s:", hp->tftpdir->string);
		}
		/* NetBSD: rootpath (see above) */
		/* NetBSD: domainname (see above) */
		/* NetBSD: dumpfile (see above) */
		if (hp->flags.time_offset) {
		    fprintf(fp, "\\\n\t:to=%ld:", hp->time_offset);
		}
		if (hp->flags.time_server) {
			fprintf(fp, "\\\n\t:ts=");
			list_ipaddresses(fp, hp->time_server);
			fprintf(fp, ":");
		}
		if (hp->flags.vm_cookie) {
			fprintf(fp, "\\\n\t:vm=");
			if (!bcmp(hp->vm_cookie, vm_rfc1048, 4)) {
				fprintf(fp, "rfc1048:");
			} else if (!bcmp(hp->vm_cookie, vm_cmu, 4)) {
				fprintf(fp, "cmu:");
			} else {
				fprintf(fp, "%d.%d.%d.%d:",
						(int) ((hp->vm_cookie)[0]),
						(int) ((hp->vm_cookie)[1]),
						(int) ((hp->vm_cookie)[2]),
						(int) ((hp->vm_cookie)[3]));
			}
		}
		if (hp->flags.generic) {
			dump_generic(fp, hp->generic);
		}
    }
}


static void
dump_generic(fp, generic)
    FILE *fp;
    struct shared_bindata   *generic;
{
    u_char *bp = generic->data;
    u_char *ep = bp + generic->length;
    u_char tag;
    int len;
	
    while (bp < ep) {
		tag = *bp++;
		if (tag == TAG_PAD)
			continue;
		if (tag == TAG_END)
			return;
		len = *bp++;
		if (bp + len > ep) {
			fprintf(fp, " #junk in generic! :");
			return;
		}
		fprintf(fp, "\\\n\t:T%d=", tag);
		while (len) {
			fprintf(fp, "%02X", *bp);
			bp++; len--;
			if (len) fprintf(fp, ".");
		}
		fprintf(fp,":");
    }
}



/*
 * Dump an entire struct in_addr_list of IP addresses to the indicated file.
 *
 * The addresses are printed in standard ASCII "dot" notation and separated
 * from one another by a single space.  A single leading space is also
 * printed before the first adddress.
 *
 * Null lists produce no output (and no error).
 */

static void
list_ipaddresses(fp, ipptr)
    FILE *fp;
    struct in_addr_list *ipptr;
{
    unsigned count;
    struct in_addr *addrptr;
	
    if (ipptr) {
		count = ipptr->addrcount;
		addrptr = ipptr->addr;
		while (count > 0) {
			fprintf(fp, "%s", inet_ntoa(*addrptr++));
			count--;
			if (count) fprintf(fp, ", ");
		}
    }
}

#endif	/* DEBUG */
