/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/fd/fd.cf/Space.c	1.2"

#include <config.h>


int	fd_door_sense = FD_DOOR_SENSE;
/*
 * fd_do_config_cmd controls whether the drive issues
 * a CONFIGURE command to enable the FIFO. Not all
 * controllers support the CONFIGURE command, and
 * should cleanly fail the command in these cases.
 * Just in case they do not, by setting this symbol
 * to 0, we can inhibit the driver from issueing the
 * CONFIGURE command.
 */
int	fd_do_config_cmd = 1;
