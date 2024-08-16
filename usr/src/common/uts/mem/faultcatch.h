/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_FAULTCATCH_H	/* wrapper symbol for kernel use */
#define _MEM_FAULTCATCH_H	/* subject to change without notice */

#ident	"@(#)kern:mem/faultcatch.h	1.13"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This file defines a mechanism for catching kernel page fault errors.
 * Any access to a pageable address should be protected by this mechanism,
 * since the I/O may fail, or (in the case of a user-supplied address)
 * the address may be invalid.
 *
 * Usage:
 *		CATCH_FAULTS(flags)
 *			protected_statement
 *		errno = END_CATCH();
 *
 * The flags define the type of address to protect.  This includes user
 * addresses, seg_u addresses, and seg_map addresses.
 *
 * The value returned by END_CATCH() will be 0 if no fault error occurred,
 * or the errno returned from the fault handler (unless the error occurred
 * on a user address, in which case the fault handler's return value is
 * ignored and EFAULT is returned).
 *
 * Caveats:
 *
 * CATCH_FAULTS should not be used from interrupt routines, or
 * nested within another CATCH_FAULTS.
 *
 * The protected code must not do anything stateful, such as using spl's
 * or setting locks, since it may be aborted in midstream.
 *
 * NOTE: The CATCH_UFAULT flag may be exported to binary add-on modules,
 * so its value should be maintained.  The other flags may only be used
 * by base kernel modules.
 */

#define CATCH_UFAULT		0x0001
#define CATCH_SEGMAP_FAULT	0x0002
#define CATCH_SEGKVN_FAULT	0x0004
#define CATCH_BUS_TIMEOUT	0x4000
#define CATCH_ALL_FAULTS	0x8000

#define CATCH_KERNEL_FAULTS	(CATCH_SEGMAP_FAULT|CATCH_SEGKVN_FAULT)

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */

#else

#include <sys/types.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */


typedef struct fault_catch {
	/*
	 * Note: the fc_flags, fc_errno, and fc_func fields must remain
	 * at constant offsets for binary compatibility with modules
	 * which may use these fields.
	 */
	uint_t	fc_flags;		/* catch flags */
	volatile int	fc_errno;	/* error number from fault, if any */
	void	(*fc_func)();		/* fault handler function */
	/*
	 * End of binary-stable fields.
	 */
	label_t	fc_jmp;			/* setjmp buffer for standard handler */
} fault_catch_t;


#if defined(_KERNEL)

/*
 * NOTE: Although the implementation of CATCH_FAULTS() uses setjmp/longjmp,
 * the enclosed code MUST NOT do anything stateful, since it could be aborted
 * at any point.  This applies to multiprocessing locks as well, so this
 * particular use of setjmp/longjmp is safe in a multiprocessor context.
 */

#define CATCH_FAULTS(flags) \
	if (ASSERT(!servicing_interrupt()), \
	    ASSERT(u.u_fault_catch.fc_flags == 0), \
	    (u.u_fault_catch.fc_errno = 0), \
	    (u.u_fault_catch.fc_flags = (flags)), \
	    setjmp(&u.u_fault_catch.fc_jmp) == 0)

#define END_CATCH() \
	((u.u_fault_catch.fc_flags = 0), \
	 u.u_fault_catch.fc_errno)

extern void fc_jmpjmp(void);

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _MEM_FAULTCATCH_H */
