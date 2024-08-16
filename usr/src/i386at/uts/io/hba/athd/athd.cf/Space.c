/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-pdi:io/hba/athd/athd.cf/Space.c	1.1"
#ident	"$Header: $"

/* athd_waitsecs is the maximum number of seconds athd_wait will wait for  */
/* the desired status prior to returning an error. A value of 25 should be */
/* sufficient for any controller/drive, but some slower drives might need  */
/* longer in a worst case scenario during a reset of the controller/drive. */

int	athd_waitsecs = 25;

