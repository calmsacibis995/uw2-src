/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)inetinst:pkgcopy.c	1.6"

#include "inetinst.h"
#include <libgen.h>

main(int argc, char *argv[])
{
	char	*source;		/* Source specification for list */
	char	*target;		/* Target specification for SW */
        char    *network;               /* Network type (tcp/spx) */
	char	*src_host;		/* Host part of source */
	char	*src_device;		/* Device part of source */
	char	*trg_host;		/* Host part of target */
	char	*trg_device;		/* Device part of target */
	char	*service;		/* Service requested from server */
	char	*ptr;			/* tmp ptr for string manipulation */
	char	package[IBUF_SIZE]="";	/* Package(s) argument for list */
	char	logbuf[IBUF_SIZE];	/* Buffer for logging messages */
	char	netbuf[IBUF_SIZE];	/* Buffer for network I/O */
	char	cmdbuf[IBUF_SIZE];	/* Buffer for commands */
	pid_t	pid;
	int	retval;			/* For storing return values */
	int	c;			/* For parsing command line */
        int     nflag=0;                /* Network option tracking flag */
	int	sflag=0;		/* Source option tracking flag */
	int	tflag=0;		/* Target option tracking flag */

	vflag=0;		/* Verbose option tracking flag */

	/*
	 *  Set up for i18n
	 */
	setlocale(LC_ALL, "");
	mCat = catopen(MCAT, 0);
	setcat("inetinst.cat.m");
	setlabel("UX:inetinst");

	opterr = 0;
        while((c = getopt(argc, argv, "vn:s:t:")) != -1) {
		switch (c) {
			case '?':
				usage(argv[0]);
				break;
			case 'v':
				vflag = 1;
				break;
			case 'n':
				/*
				*  network - specified in form
				*      tcp or spx
				*/
				if (nflag>0) {
					usage(argv[0]);
					break;
				}
				network = strdup(optarg);
				if (strcmp(network, "spx") && strcmp(network, "tcp")) {
					usage(argv[0]);
					break;
				}

				nflag++;
				break;
			case 't':
				/*
				*  Target - specified in form
				*      host[:device] or
				*      [host:]device
				*/
				if (tflag>0) {
					usage(argv[0]);
					break;
				}
				target = strdup(optarg);
				tflag++;
                                break;
			case 's':
				/*
				*  Source - specified in form
				*      host[:device] or
				*      [host:]device
				*/
				if (sflag>0) {
					usage(argv[0]);
					break;
				}
				source = strdup(optarg);
				sflag++;
                                break;
                        default:
                                usage(argv[0]);
                                break;
                }
        }

        /*
	 *  Determine what service we're requesting, and start the log.
	 */
	service = strdup(basename(argv[0]));
	log_init(service);

        /*
         *  Now go get the names of the packages requested.
         */
	if (optind == argc) {
		strcat(package, " ");
	}

	for (argv = &argv[optind]; optind < argc; optind++, argv++) {
		strcat(package, *argv);
		strcat(package, ":");
	}

	/*
	 *  Set up signal handling so that log will be dealt with
	 *  effectively on interruption.
	signal_setup();

	/*
	 *  We need to split up the source and target specifications
	 *  so that we can figure out what machine to connect with,
	 *  and where to put the software when we get it.
	 */
	if (sflag > 0) {
		if ((retval=parse_location(source, &(src_host), &(src_device))!=0)){
			log(catgets(mCat, MSG_SET, C_ERR_SOURCE, M_ERR_SOURCE));
			clean_exit(IERR_SOURCE_INVAL);
		}
	} else {
		src_host = strdup(get_nodename());
		src_device = ISPOOL_DIR;
	}

	if (tflag > 0) {
		if ((retval=parse_location(target, &(trg_host), &(trg_device))!=0)){
			log(catgets(mCat, MSG_SET, C_ERR_TARGET, M_ERR_TARGET));
			clean_exit(IERR_TARGET_INVAL);
		}
	} else {
		trg_host = strdup(get_nodename());
		trg_device = ISPOOL_DIR;
	}

        if (!nflag) {
                network = strdup(ANY_NET);
        }

	/*
	 *  Open up the TLI connection
	 */
	inetinst_connect(src_host, network);

	/*
	 *  Initialize session with HELO; get response
	 */
	sprintf(netbuf, "HELO %s\n", get_nodename());
	netsendrcv(netbuf, vflag);

	/*
	 *  Request SERVICE; get response
	 */
	sprintf(netbuf, "SERVICE %s\n", service);
	netsendrcv(netbuf, vflag);

	/*
	 *  Announce OPTIONS; get response
	 */
	sprintf(netbuf, "OPTIONS\n");
	netsendrcv(netbuf, vflag);

	/*
	 *  Send OPTION 'package'; get response
	 */
	sprintf(netbuf, "package %s\n", package);
	netsendrcv(netbuf, vflag);

	/*
	 *  Send OPTION 'source'; get response
	 */
	sprintf(netbuf, "source %s:%s\n", src_host, src_device);
	netsendrcv(netbuf, vflag);

	/*
	 *  Send OPTION 'target'; get response
	 */
	sprintf(netbuf, "target %s:%s\n", trg_host, trg_device);
	netsendrcv(netbuf, vflag);

	/*
	 *  Request DATA; get response
	 */
	sprintf(netbuf, "DATA\n");
	netsendrcv(netbuf, vflag);
	if(strncmp(netbuf, "Data", 4)) {
		sprintf(logbuf, "%s:%d:%s\n", MCAT, C_TERMINATED, M_TERMINATED);
		pfmt(stderr, MM_ACTION, logbuf, ILOG_FILE, get_nodename(), src_host);
		clean_exit(IERR_BADFILE_SERVER);
	}
	memset(netbuf, '\0', IBUF_SIZE);

	/*
	 *  Now separate out the :-spearated package list into
	 *  a format the local tools understand.
	 */
	while ((ptr = (char *)strchr(package, ':')) != NULL) {
		*(ptr) = ' ';
	}

	/*
	 *  If this machine is the target, we do the pkgtrans
	 *  right from stdin.
	 *  Otherwise, the server we asked for the software had to
	 *  run a proxy command, so all we want to do is show the output.
	 */
	if (!strcmp(get_nodename(), trg_host)) {
		sprintf(logbuf,"%s -n - %s %s\n", PKGTRANS, trg_device, package);
		log(logbuf);
		sprintf(cmdbuf,"%s -n - %s %s", PKGTRANS, trg_device, package);
		retval=system(cmdbuf);
		retval=system_exit(retval);
	} else {
		while(fgets(netbuf, IBUF_SIZE, stdin) != NULL) {
			write(2, netbuf, IBUF_SIZE);
		}
		retval=0;
	}
	

	/*
	 *  Close log
	 */
	clean_exit(retval);
}

/*
 *  Usage message
 *	INPUT	name of this executable
 *	OUTPUT	usage message, exit with IERR_USAGE
 */
void
usage(char *progname)
{
	char	logbuf[IBUF_SIZE];	/* Buffer for logging messages */

	sprintf(logbuf, ":%d:%s\n", C_USAGE_PKGCOPY, M_USAGE_PKGCOPY);
	pfmt(stderr, MM_ACTION, logbuf, progname);
	exit(IERR_USAGE);
}
