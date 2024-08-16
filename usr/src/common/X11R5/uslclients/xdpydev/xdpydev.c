/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xdpydev:xdpydev.c	1.1"

/*
 * xdpydev - determine the X display device
 *
 * When run under X the newvt command may need to know what VT device
 * is in use for the X display, so that it can perform the necessary
 * operations to open a new VT on the same device.  It uses xdpydev
 * to obtain this information.
 *
 * Query the server to retrieve the vendor string, which we
 * expect to be of the form "...(using <dev>)..." where <dev>
 * is the display device, e.g. "vt01".  Since the vendor string
 * can be changed by the user, this is not a reliable determinant
 * of the actual display device.  However, this is the same method
 * that xterm uses to respond to the TIOCVTNAME ioctl.
 * In the future we may devise a server extension to provide this
 * information.
 *
 * Note that we could be talking to a remote server, so the device
 * pathname may be meaningless when interpreted on the local machine.
 * Since we don't know what the user intends to do with the pathname,
 * that is not our problem.
 *
 * Code adapted from r5xdpyinfo:xdpyinfo.c and xterm:charproc.c .
 */

#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USING	"(using "	/* magic cookie from server vendor string */
#define USINGSZ	(sizeof USING - 1)

static char *ProgramName;

static void
usage()
{
	fprintf(stderr, "usage:  %s [-display displayname]\n",
		ProgramName);
	exit(1);
}

main(argc, argv)
	int argc;
	char *argv[];
{
	Display *dpy;			/* X connection */
	char *displayname = NULL;	/* server to contact */
	char *p;
	int i;

	ProgramName = argv[0];

	for (i = 1; i < argc; i++) {
		p = argv[i];

		if (p[0] == '-') {
			switch (p[1]) {
			case 'd':
				if (++i >= argc)
					usage();
				displayname = argv[i];
				continue;
			default:
				usage();
			}
		} else
			usage();
	}

	if ((dpy = XOpenDisplay(displayname)) == NULL ||
		(p = XServerVendor(dpy)) == NULL ||
		(p = strstr(p, USING)) == NULL)
		exit(2);

	for (p += USINGSZ, i = 0; p[i] != '\0' && p[i] != ')'; i++)
		;
	printf("/dev/%.*s\n", i, p);
	exit(0);

	/* NOTREACHED */
}
