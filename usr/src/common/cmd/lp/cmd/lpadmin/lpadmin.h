/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:cmd/lpadmin/lpadmin.h	1.10.1.3"
#ident	"$Header: $"

#define BEGIN_CRITICAL	{ ignore_signals(); {
#define END_CRITICAL	} trap_signals(); }

extern void		ignore_signals(),
			trap_signals();

extern int		a,
			banner,
#if	defined(DIRECT_ACCESS)
			C,
#endif
			filebreak,
			h,
			j,
			l,
			M,
			o,
			Q,
			W,
			scheduler_active;

extern char		*A,
			*c,
			*cpi,
			*d,
			*O,
			*D,
			*e,
			*f,
			**f_allow,
			**f_deny,
			*F,
			**H,
			*i,
			**I,
			*length,
			*lpi,
			*m,
			modifications[128],
			*p,
			*r,
			*stty,
			**S,
			**T,
			*u,
			**u_allow,
			**u_deny,
			*U,
			*v,
			*width,
			*x;

extern	char		*s,
			**R;

#ifdef	NETWORKING
extern	level_t		R_hilevel,
			R_lolevel;	/*  Level range for the R */
#endif

#if	defined(LPUSER)
extern SCALED		cpi_sdn,
			length_sdn,
			lpi_sdn,
			width_sdn;
#endif

#if	defined(PR_MAX)
extern PRINTER		*oldp;

extern PWHEEL		*oldS;
#endif

extern unsigned short	daisy;

extern char		*Local_System;

extern char		*getdflt();

extern char		*getcopy();

extern int		ismodel(),
			output(),
			verify_form(),
			do_align();

extern void		do_fault(),
			do_mount(),
			do_printer(),
			do_pwheel(),
			done(),
			fromclass(),
			newdflt(),
			newcopy(),
			options(),
			rmdest(),
			startup(),
			usage();

#if	defined(__STDC__)
void			send_message( int , ... );
#else
extern void		send_message();
#endif
