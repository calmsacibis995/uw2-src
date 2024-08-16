/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mp.cmds:common/cmd/priocntl/priocntl.h	1.1"
#ident  "$Header: priocntl.h 1.2 91/06/27 $"
#define	NPIDS	1024	/* max number of pids we pipe to class specific cmd */
#define	NIDS	1024	/* max number of id arguments we handle */

#define	BASENMSZ	16
#define	CSOPTSLN	128	/* max length of class specific opts string */

/*
 * The command string for the sub-command must be big enough for the
 * path, the class specific options, and plenty of space for arguments.
 */
#define	SUBCMDSZ	512

extern void	itoa();
extern int	str2idtyp(), idtyp2str(), idtyp2maxprocs(), idcompar();
extern int	getmyid(), getmyidstr();
extern id_t	clname2cid();
extern char	*basename();

