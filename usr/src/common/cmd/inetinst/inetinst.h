/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)inetinst:inetinst.h	1.5"

/*
 *  Include file for inetinst.
 *
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include "inetinst_msgs.h"
#include <locale.h>
#include <nl_types.h>
#include <pfmt.h>

/*
 *  Where do we keep everything?
 */
#define	ISPOOL_DIR	"/var/spool/dist"		/* pkg spool dir */
#define	IADM_DIR	"/var/sadm/dist"		/* adm files dir */
#define	ILOG_FILE	"/var/sadm/dist/log"		/* log file */
#define	IOPT_FILE	"/var/sadm/dist/.pkgdefaults"	/* defaults file */
#define	CAT		"/bin/cat"
#define	MAILX		"/bin/mailx"
#define	PKGADD		"/usr/sbin/pkgadd"
#define	PKGINFO		"/bin/pkginfo"
#define	PKGTRANS	"/bin/pkgtrans"
#define	PKGINSTALL	"/usr/sbin/pkginstall"
#define	PKGCAT		"/usr/sbin/pkgcat"
#define	PKGCOPY		"/usr/sbin/pkgcopy"
#define	PKGLIST		"/usr/bin/pkglist"


/*
 *  States for state transition
 */
#define ISTATE_SUCCESS	0	/* successful completion */
#define ISTATE_NONE	1	/* uninitialized */
#define ISTATE_INIT	2	/* initialized (service requested) */
#define ISTATE_SNDOPT	3	/* sending options */
#define ISTATE_RCVOPT	4	/* receiving options */
#define ISTATE_SNDDATA	5	/* sending data */
#define ISTATE_RCVDATA	6	/* receiving data */
#define ISTATE_SNDLOG	7	/* sending log */
#define ISTATE_RCVLOG	8	/* receiving log */
#define ISTATE_ERROR	9	/* error condition */

/*
 *  Error codes
 */
#define	IERR_MIN	100	/* BLOP */
#define	IERR_USAGE	101	/* Usage error */
#define	IERR_SYSTEM	102	/* Bad system call */
#define	IERR_BADFILE	103	/* No such file or directory */
#define	IERR_BADFILE_SERVER	104	/* No such file on server */
#define	IERR_PERM	105	/* No file permissions */
#define	IERR_BADNET	106	/* Bad network connection */
#define	IERR_BADPROTO	107	/* The other side messed up the protocol */
#define	IERR_BADOPT	108	/* Invalid option specified */
#define	IERR_BADHOST	109	/* Invalid host in location specification */
#define	IERR_INTR	110	/* Execution was interrupted by user */
#define IERR_SOURCE_INVAL  111  /* Invalid source specification */
#define IERR_TARGET_INVAL  112  /* Invalid target specification */
#define IERR_CLOSED	113	/* Datastream closed by user */
#define IERR_SUCCESS       0  /* No errror */

/*
 *  Values
 */
#define	IBUF_SIZE	256	/* Buffer for network chatting */

/*
 *  Services
 */
#define ISVC_CAT	"pkgcat"	/* Cat a datastream across the net */
#define ISVC_COPY	"pkgcopy"	/* Copy a package from host to host */
#define ISVC_LIST	"pkglist"	/* List available pkgs on server */
#define ISVC_INSTALL	"pkginstall"	/* Install pkg from server */
#define ISVC_PROXY	"proxy"		/* Perform proxy command */

/*
 *  Options
 */

struct iopts {
	char	*tag;		/*  The TAG (or name) of this option */
	char	*value;		/*  The VALUE of this option */
};

#define IOPT_TARGET		0	/* Target list for software ops */
#define IOPT_TARGET_NAME	"target"
#define IOPT_SOURCE		1	/* Source list for software ops */
#define IOPT_SOURCE_NAME	"source"
#define IOPT_PACKAGE		2	/* Package list for software ops */
#define IOPT_PACKAGE_NAME	"package"
#define IOPT_INTERACTIVE	3	/* Whether we're interactive */
#define IOPT_INTERACTIVE_NAME	"interactive"
#define IOPT_VERBOSE		4	/* Verbose Level */
#define IOPT_VERBOSE_NAME	"verbose"
#define IOPT_REQUESTOR		5	/* Who requested service */
#define IOPT_REQUESTOR_NAME	"requestor"
#define IOPT_MAIL		6	/* Who gets the mail when we're done */
#define IOPT_MAIL_NAME		"mail"
#define IOPT_MAXOPTS		7	/* One more than the last option */

/*
 *  Types of files/directories/devices
 */
#define	ISTAT_PKGADD		1	/* PKGADD directory format */
#define	ISTAT_CAT		2	/* PKGADD datastream format */
#define	ISTAT_NONE		3	/* No pkg associated */


#define	INETD_SERVICE		"inetinst"
#define	ANY_NET			"any"
#define NOBODY			"nobody"

/*
 *  These get sent to the client to let them know whether or not to
 *  expect data.
 */
#define IMSG_DATA_YES	"Data follows.  After completion, connection will be terminated."
#define IMSG_DATA_NO	"No data follows.  After error messages, connection will be terminated."

/*
 *  Prototypes to keep everybody happy
 */
#ifndef _MY_PROTO
#define _MY_PROTO
nl_catd mCat;
int vflag;
extern void clean_exit(int);
extern char * get_nodename();
extern int eval_path(char *, char *);
extern int ishost(char *);
extern int parse_location(char *, char **, char **);
extern void signal_setup();
extern int do_svc_list();
extern int do_svc_copy();
extern int do_svc_install();
extern int do_proxy_svr();
extern int do_proxy_cli(char *, char *);
extern void get_options();
extern void log_init(char *);
extern void log(char *);
extern void log_close();
void log_send();
extern  int inetinst_connect(char *, char *);
extern void netputs(char *, FILE *);
extern void netsendrcv(char *, int);
extern void parse_options_file(char *);
extern void set_default_options(char *);
extern void print_all_options();
extern int set_option(char *, char *);
extern char * get_option(int);
extern void usage(char *);
#endif
