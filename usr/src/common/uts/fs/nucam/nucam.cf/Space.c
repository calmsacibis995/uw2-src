/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucam/nucam.cf/Space.c	1.6"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucam/nucam.cf/Space.c,v 1.3.4.1 1994/12/12 01:10:24 stevbam Exp $"

/*
**  Netware Unix Client
**
**	MODULE:
**		Space.c -	The NetWare UNIX Client Auto Mounter File 
**				System Space.c file.
**
**	ABSTRACT:
**		The Space.c contains buildtime initialized values, for
**		tunables for the NUCAM file system.
**
*/ 
#include <config.h>
#include <sys/tuneable.h>
#include <sys/param.h>
#include <sys/types.h>


/*
 * Estimated number of AMfs node objects.
 */
int	est_amfs_nodes = EST_AMFS_NODES;

