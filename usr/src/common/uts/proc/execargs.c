/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/execargs.c	1.5"
#ident	"$Header: $"

#include <mem/faultcatch.h>
#include <mem/uas.h>
#include <mem/vmparam.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/sysmacros.h>
#include <util/types.h>


/*
 * int arglistsz(vaddr_t argv, int *argc_p, int *argsize_p, int maxsize)
 *	Compute the number of arguments and total size of strings
 *	in an argument list.
 *
 * Calling/Exit State:
 *	No spinlocks should be held on entry, none are held on return.
 *
 * Remarks:
 *	The parameter 'argv' is the argument list pointer (a user address)
 *	and 'maxsize' is the string size limit.
 *
 *	Returns -2 if the string size is too long, -1 if a bad user address
 *	is supplied, otherwise returns 0 with '*argc_p' = argument count,
 *	and '*argsize_p' = total string size.
 */
int
arglistsz(vaddr_t argv,
	  int *argc_p,
	  size_t *argsize_p,
	  int maxsize)
{
	int	argc = 0;	/* # of arguments */
	size_t	argsize = 0;	/* # of characters in argument list */
	vaddr_t	argp;		/* pointer to argument (user space) */
	int	len;

	ASSERT(KS_HOLD0LOCKS());

	if (!VALID_USR_RANGE(argv, sizeof (char *)))
		return -1;

	CATCH_FAULTS(CATCH_UFAULT) {
		while ((argp = (vaddr_t)uas_charp_in(argv)) != NULL) {
			if (!VALID_USR_RANGE(argp, 1)) {
				(void)END_CATCH();
				return -1;
			}
			if ((len = uas_strlen((char *)argp)) >= maxsize - argsize) {
				(void)END_CATCH();
				return -2;
			}
			argsize += len + 1;
			++argc;
			argv += sizeof (char *);
		}
	}
	if (END_CATCH() != 0)
		return -1;

	*argc_p = argc;
	*argsize_p = argsize;

	return 0;
}


/*
 * int copyarglist(int argc, vaddr_t from_argv, int pdelta,
 *		   vaddr_t to_argv, vaddr_t to_argp, boolean_t from_kernel)
 *
 *	Copy an argument list and its string into a compact form at an
 *	interim address.  Although we can assume the interim addresses are
 *	OK, the pointers in the array must be rechecked because	their image
 *	could be in shared writable pages.
 *
 * Calling/Exit State:
 *	No spin locks can be held on entry to this function, none are held
 *	on return.
 *
 * Remarks:
 *	'argc' is the argument count.
 *	'from_argv' is the "from" argument pointer (a user address
 *	if 'from_kernel' is B_FALSE, or a kernel/uarea address if
 *	'from_kernel' is B_TRUE),
 *	'pdelta' is the pointer delta to apply to the values
 *	in the new pointer list, 'to_argv' is the "to" argument
 *	pointer (a user address), and 'to_argp' is the "to"
 *	string pointer.
 *
 *	The strings pointed to by the from pointer list
 *	are copied to the "to" string space and the new pointer
 *	list is constructed with pointers to the string starts
 *	offset by pdelta.  The reason for pdelta is that
 *	exec first builds the stack frame at the wrong place
 *	(since it cannot clobber existing data), then the image
 *	is moved to the right place virtually in the new address space.
 *
 *	The size of the copied strings is returned on success
 *	and -1 is returned on failure.
 */
int
copyarglist(int argc,
	    vaddr_t from_argv,		/* user or kernel address */
	    int pdelta,
	    vaddr_t to_argv,		/* user address */
	    vaddr_t to_argp,		/* user address */
	    boolean_t from_kernel)
{
	vaddr_t	to_base = to_argp;
	int	len;

	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Note that the from_argv has already been checked
	 * but the string pointers can get clobbered because
	 * they may be in shared pages that change.
	 * Thus the pointers must get checked.
	 * But the strings need not get checked for length now.
	 * This relies on there being no virtual neighbor at the end
	 * of the temporary image virtual space.
	 * Thus, there would be a fault that fails.
	 * Of course, implementations on other machines must make sure
	 * that an overrun will not be processed as a stack growth.
	 * That affects the code that choses the virtual hole to use.
	 */

	CATCH_FAULTS(CATCH_UFAULT) {
		while (argc-- > 0) {
			if (from_kernel) {
				char *from_argp = *(char **)from_argv;
				len = strlen(from_argp) + 1;
				uas_copyout(from_argp, (caddr_t)to_argp, len);
			} else {
				vaddr_t from_argp =
					(vaddr_t)uas_charp_in(from_argv);
				if (!VALID_USR_RANGE(from_argp, 1)) {
					(void)END_CATCH();
					return -1;
				}
				len = uas_strcpy_len((caddr_t)to_argp,
						     (caddr_t)from_argp) + 1;
			}
			uas_charp_out((char **)to_argv,
				      (caddr_t)to_argp + pdelta);
			to_argp += len;
			from_argv += sizeof (char *);
			to_argv += sizeof (char *);
		}
	}
	if (END_CATCH() != 0)
		return -1;
	return to_argp - to_base;
}
