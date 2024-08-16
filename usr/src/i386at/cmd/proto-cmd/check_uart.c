/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)proto-cmd:i386at/cmd/proto-cmd/check_uart.c	1.3"

#include <fcntl.h>

main( argc, argv )
	int argc;
	char *argv[];
{
	exit( open( argv[1], O_RDONLY|O_NDELAY, 0 ) );
}
