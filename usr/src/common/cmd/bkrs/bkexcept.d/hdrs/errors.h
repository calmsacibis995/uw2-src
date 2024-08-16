/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bkexcept.d/hdrs/errors.h	1.2.5.3"
#ident  "$Header: errors.h 1.2 91/06/21 $"

/* 
	This file contains error definitions for all error messages in bkexcept 
	command.
*/

/* "Option \"%c\" is invalid.\n" */
#define ERROR0	0

/* "Only one argument may be specified.\n" */
#define ERROR1	1

/* "Unable to allocate memory for reading table entry.\n" */
#define ERROR2	2

/* "Unable to %s table %s (return code = %d).\n" */
#define ERROR3	3

/* "Search of table %s failed (return code = %d).\n" */
#define ERROR4	4

/* "Deletion of entry %d from table %s failed (return code = %d).\n" */
#define ERROR5	5

/* "Cannot open %s.\n" */
#define ERROR6	6

/* "'system' call failed, errno = %d.\n" */
#define ERROR7	7

/* "Unable to unlink temporary file %s, errno = %d.\n" */
#define ERROR8	8

/* "Unable to read entry %d of table %s (return code = %d).\n" */
#define ERROR9	9

/* "Table %s does not exist or is not accessible.\n" */
#define ERROR10	10

/* "Open of table %s failed (return code = %d).\n" */
#define ERROR11	11

/* "Warning: table %s has different format than expected.\n" */
#define ERROR12	12

/* "Unable to translate file %s, errno = %d.\n" */
#define ERROR13	13

/* "Pattern specified was null, unable to %s table.\n" */
#define ERROR14	14

/* "Warning: null pattern specified, displaying entire table.\n" */
#define ERROR15	15

/* "Argument \"%s\" is invalid.\n" */
#define ERROR16	16

/* "Warning: table %s has no ENTRY FORMAT record.\n" */
#define ERROR17 17

/* "Warning: table %s had no ENTRY FORMAT record.\nFormat record is being added to table.\n" */
#define ERROR18 18

