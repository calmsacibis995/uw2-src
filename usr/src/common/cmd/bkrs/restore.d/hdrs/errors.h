/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/restore.d/hdrs/errors.h	1.2.5.3"
#ident  "$Header: errors.h 1.2 91/06/21 $"

/* 
	This file contains error definitions for all error messages in backup 
	command.
*/

/* option \"%c\" is invalid.\n" */
#define	ERROR0	0

/* argument \"%s\" is invalid.\n" */
#define	ERROR1	1

/* "\"%s\% is not a valid user name.\n" */
#define	ERROR2	2

/* "All option arguments are invalid.\n" */
#define ERROR3	3

/* "\"%s\" is an invalid argument for the %c option.\n" */
#define ERROR4	4

/* "Only one of the \"%s\" options may be used at a time.\n" */
#define	ERROR5	5

/* "Only able to restore %d in one %s command.\n" */
#define	ERROR6	6

/* "Unable to read pending request entry for restore id %s.\n" */
#define	ERROR7	7

/* "Restore id %s does not exist.\n" */
#define	ERROR8	8

/* "Pending restore request table has bad format.\n" */
#define	ERROR9	9

/* "Must have the same effective uid to cancel restore id %s.\n" */
#define	ERROR10 10

/* "Unable to cancel restore id %s.\n" */
#define	ERROR11	11

/* "Unable to open pending request table.\n" */
#define	ERROR12	12

/* "Unable to get new restore id for %s, please try again.\n" */
#define	ERROR13	13

/* "Unable to read pending restore request table.\n" */
#define	ERROR14	14

/* "Restore request id for %s is %s.\n" */
#define	ERROR15	15

/* "There is no information about %s.\n" */
#define ERROR16 16

/* "\"%s\" is an invalid jobid.\n" */    
#define ERROR17 17                       
