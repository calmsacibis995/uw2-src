/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bkoper.d/hdrs/errors.h	1.3.5.2"
#ident  "$Header: errors.h 1.2 91/06/21 $"

/* "Unable to understand this command." */
#define	ERROR0	0

/* There are new backup operations requiring service. */
#define	ERROR1	1

/* No more backup operations are WAITING for operator action
	at this time.\nType q to quit: "*/
#define	ERROR2	2

/* Backup operation %d, jobid %s tag %s  no longer needs service. */
#define	ERROR3	3

/* "Unable to read entry number %d in status table." */
#define	ERROR4	4

/* "Unexpected error: %s" */
#define	ERROR5	5

/* "Unable to open backup status log (%s): %s" */
#define	ERROR6	6

/* "Unable to open backup status log (%s): %d" */
#define	ERROR7	7

/* "%s is not a valid user." */
#define	ERROR8	8

/* "No backup operations are WAITING for operator action at this time." */
#define	ERROR9	9

/* "Which prompt do you want to respond to?... */
#define	ERROR10	10

/* "? " */
#define	ERROR11	11

/* "Current header number: %d" */
#define	ERROR12	12

/* "Unable to understand this '%c' command." */
#define	ERROR13	13

/* "Backup operation %d does not exist." */
#define	ERROR14	14

/* "Unable to send volume information to the method: %s" */
#define	ERROR15	15

/* "Unknown or inaccessible device: %s\n" */
#define	ERROR16	16

/* The volume label, %s, is incorrect\n. */
#define ERROR17 17
