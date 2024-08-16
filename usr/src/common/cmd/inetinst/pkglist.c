/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)inetinst:pkglist.c	1.3"

/*
 *  pkglist.c
 *
 *  Client side of network install.  Request list of software from
 *  a server and outputs the datastream received.
 */
#include "inetinst.h"
#include <libgen.h>

main(int argc, char *argv[])
{
	char	*source;		/* Source specification for list */
	char	*src_host;		/* Host part of source */
        char    *network;               /* Network type (tcp/spx) */
	char	*src_device;		/* Device part of source */
	char	*service;		/* Service requested from server */
	char	package[IBUF_SIZE]="";	/* Package(s) argument for list */
	char	logbuf[IBUF_SIZE];	/* Buffer for logging messages */
	char	netbuf[IBUF_SIZE];	/* Buffer for network I/O */
	int	retval;			/* For storing return values */
	int	c;			/* For parsing command line */
        int     nflag=0;                /* Network option tracking flag */
	int	sflag=0;		/* Source option tracking flag */
	int	stdout_fd;		/* FD to preserve stdout */

	vflag=0;		/* Verbose option tracking flag */

	/*
	 *  Set up for i18n
	 */
	setlocale(LC_ALL, "");
	mCat = catopen(MCAT, 0);
	setcat("inetinst.cat.m");
	setlabel("UX:inetinst");

	set_default_options((char *)NULL);
	opterr = 0;
	while((c = getopt(argc, argv, "vn:s:")) != -1) {
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
        /* package = strdup(argv[optind]); */
	if (optind == argc) {
		usage(argv[0]);
	}

	for (argv = &argv[optind]; optind < argc; optind++, argv++) {
		strcat(package, *argv);
		strcat(package, ":");
	}

	/*
	 *  If no source was specified, assume local host.
	 */
	if (sflag > 0) {
		if ((retval=parse_location(source, &(src_host), &(src_device))!=0)){
			log(catgets(mCat, MSG_SET, C_ERR_SOURCE, M_ERR_SOURCE));
			/* clean_exit(IERR_SOURCE_INVAL); */
			usage(argv[0]);
		}
	} else {
		src_host = strdup(get_nodename());
		src_device = ISPOOL_DIR;
	}

        if (!nflag) {
                network = strdup(ANY_NET);
        }

	/*
	 *  Set up signal handling so that log will be dealt with
	 *  effectively on interruption.
	signal_setup();

	/*
	 *  Open up the TLI connection
	 */
	stdout_fd = dup(1);
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
	 *  Request DATA; get response
	 */
	sprintf(netbuf, "DATA\n");
	netsendrcv(netbuf, vflag);

	if (vflag) {
		sprintf(logbuf, catgets(mCat, MSG_SET, C_TRANS_START, M_TRANS_START), src_host);
		log(logbuf);
	}
	/*
	 *  Read data until there's no more.
	 */
	dup2(stdout_fd, 1);
	while(fgets(netbuf, IBUF_SIZE, stdin) != NULL) {
		sprintf(logbuf, "%s", netbuf);
		write(stdout_fd, logbuf, strlen(logbuf));
		log(netbuf);
	}

	if (vflag) {
		sprintf(logbuf, catgets(mCat, MSG_SET, C_TRANS_END, M_TRANS_END), src_host);
		log(logbuf);
	}

	/*
	 *  Close log
	 */
	log_close();
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

	sprintf(logbuf, ":%d:%s", C_USAGE_PKGLIST, M_USAGE_PKGLIST); 
	pfmt(stderr, MM_ACTION, logbuf, progname);
	exit(IERR_USAGE);
}
