/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/rsnotify.d/hdrs/errors.h	1.2.5.2"
#ident  "$Header: errors.h 1.2 91/06/21 $"

/* 
	This file contains error definitions for all error messages in bkstatus 
	command.
*/

/* "Option \"%c\" is invalid.\n" */
#define ERROR0	0

/* "Argument \"%s\" is invalid.\n" */
#define ERROR1	1

/* "Unable to allocate memory for reading table entry.\n" */
#define ERROR2	2

/* "Unable to read table entry number %d (return code = %d).\n" */
#define ERROR3	3

/* "Unable to assign %s value to table entry number %d (return code = %d).\n" */
#define ERROR4	4

/* "Warning: table %s has different format than expected.\n" */
#define ERROR5	5

/* "Open of table %s failed (return code = %d).\n" */
#define ERROR6	6
