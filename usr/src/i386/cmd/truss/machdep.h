/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)truss:i386/cmd/truss/machdep.h	1.1.1.1"
#ident	"$Header: machdep.h 1.1 91/07/09 $"

#define R_1	9	/* EDX */
#define R_0	11	/* EAX */
#define	R_PC	14	/* EIP */
#define	R_PS	16	/* EFL */
#define	R_SP	17	/* UESP */

#define SYSCALL_OFF	7

typedef	ulong_t	syscall_t;
#define	SYSCALL	(ulong_t)0x9a
#define	ERRBIT	0x1

#define PRT_SYS	prt_si86

#if	defined(__STDC__)
extern	CONST char *	si86name( int );
extern	void	prt_si86( int , int );
#else	/* defined(__STDC__) */
extern	CONST char *	si86name();
extern	void	prt_si86();
#endif
