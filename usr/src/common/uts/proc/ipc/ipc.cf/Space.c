/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/ipc/ipc.cf/Space.c	1.1"
#ident	"$Header: $"

#include <config.h>	/* to collect tunable parameters */

#include <sys/types.h>
#include <sys/map.h>
#include <sys/ipc.h>
#include <sys/shm.h>    /* for struct shminfo */
#include <sys/sem.h>    /* for struct seminfo */
#include <sys/msg.h>	/* for struct msginfo */


struct	map	shmmap[SHMMNI/2+2] ;
struct	shminfo	shminfo
		      ={SHMMAX,
			SHMMIN,
			SHMMNI,
			SHMSEG} ;

struct seminfo seminfo = {
	SEMMAP,
	SEMMNI,
	SEMMNS,
	SEMMNU,
	SEMMSL,
	SEMOPM,
	SEMUME,
	16+8*SEMUME,
	SEMVMX,
	SEMAEM
};

struct	msginfo	msginfo = {
	MSGMAP,
	MSGMAX,
	MSGMNB,
	MSGMNI,
	MSGSSZ,
	MSGTQL,
	MSGSEG
};
