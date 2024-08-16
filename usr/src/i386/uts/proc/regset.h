/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_REGSET_H	/* wrapper symbol for kernel use */
#define _PROC_REGSET_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/regset.h	1.10"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <svc/fp.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/fp.h>		/* REQUIRED */

#else

#include <sys/fp.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/* General register access (386) */

typedef	int	greg_t;
#define	NGREG	19
typedef	greg_t	gregset_t[NGREG];

#define R_SS	18
#define R_ESP	17
#define R_EFL	16
#define R_CS	15
#define R_EIP	14
#define R_EAX	11
#define R_ECX	10
#define R_EDX	9
#define R_EBX	8
#define R_EBP	6
#define R_ESI	5
#define R_EDI	4
#define R_DS	3
#define R_ES	2
#define R_FS	1
#define R_GS	0

/*
 * The following defines are only for compatibility. New code should use
 * the R_XX namespace defined above.
 */

#ifndef _KERNEL

#define SS	R_SS
#define UESP	R_ESP
#define EFL	R_EFL
#define CS	R_CS
#define EIP	R_EIP
#define ERR	13
#define TRAPNO	12
#define EAX	R_EAX
#define ECX	R_ECX
#define EDX	R_EDX
#define EBX	R_EBX
#define ESP	7
#define EBP	R_EBP
#define ESI	R_ESI
#define EDI	R_EDI
#define DS	R_DS
#define ES	R_ES
#define FS	R_FS
#define GS	R_GS

#endif /* !_KERNEL */


/*
 * Floating-point register access
 *  fpregset_t.fp_reg_set is 387 state.
 *  fpregset_t.f_wregs is Weitek state.
 */
typedef struct fpregset {
    union {
	struct fp_chip_ste		/* fp extension state */
	{
            int state[27];		/* 287/387 saved state */
            int status;			/* status word saved at exception */
	} fpchip_state;
	struct fpemul_state fp_emul_space;  /* for emulator(s) */
	int f_fpregs[62];		/* union of the above */
    } fp_reg_set;
    long    f_wregs[33];		/* saved weitek state */
} fpregset_t;

/* Hardware debug register access (386) */

#define NDEBUGREG	8

typedef struct dbregset {
	unsigned	debugreg[NDEBUGREG];
} dbregset_t;

typedef struct regs {
	union {
		unsigned int eax;
		struct {
			unsigned short ax;
		} word;
		struct {
			unsigned char al;
			unsigned char ah;
		} byte;
	} eax;

	union {
		unsigned int ebx;
		struct {
			unsigned short bx;
		} word;
		struct {
			unsigned char bl;
			unsigned char bh;
		} byte;
	} ebx;

	union {
		unsigned int ecx;
		struct {
			unsigned short cx;
		} word;
		struct {
			unsigned char cl;
			unsigned char ch;
		} byte;
	} ecx;

	union {
		unsigned int edx;
		struct {
			unsigned short dx;
		} word;
		struct {
			unsigned char dl;
			unsigned char dh;
		} byte;
	} edx;

	union {
		unsigned int edi;
		struct {
			unsigned short di;
		} word;
	} edi;

	union {
		unsigned int esi;
		struct {
			unsigned short si;
		} word;
	} esi;

	unsigned int eflags;

} regs;

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_REGSET_H */
