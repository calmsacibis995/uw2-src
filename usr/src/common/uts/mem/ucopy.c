/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/ucopy.c	1.19"
#ident	"$Header: $"

/*
 * This is a collection of routines which access user addresses.
 * They provide protection against user address page fault errors using
 * CATCH_FAULTS, and validate user permissions.
 */

#include <mem/faultcatch.h>
#include <mem/uas.h>
#include <mem/vmparam.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/types.h>
#include <util/ksynch.h>
#include <util/ipl.h>

extern int upc_scale(int, ulong_t);

/*
 * int
 * ucopyin(const void *src, void *dst, size_t cnt, uint_t catch_flags)
 *	Copy bytes from user to kernel address space.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  src		pointer to user source buffer
 *	  dst		pointer to kernel destination buffer
 *	  cnt		number of bytes to copy
 *	  catch_flags	CATCH_FAULTS flags needed for kernel buffer
 *	Returns:
 *	  0 if successful, else errno
 */
int
ucopyin(const void *src, void *dst, size_t cnt, uint_t catch_flags)
{
	if (cnt == 0)		/* Explicit check for 0 count,		   */
		return 0;	/*   since it will fail VALID_USR_RANGE(). */
	if (!VALID_USR_RANGE(src, cnt))
		return EFAULT;
	CATCH_FAULTS(catch_flags | CATCH_UFAULT) {
		uas_copyin((void *)src, dst, cnt);
	}
	return END_CATCH();
}

/*
 * int
 * copyin(const void *src, void *dst, size_t cnt)
 *	Copy bytes from user to kernel address space.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  src		pointer to user source buffer
 *	  dst		pointer to kernel destination buffer
 *	  cnt		number of bytes to copy
 *	Returns:
 *	  0 if successful, else -1
 *	The kernel buffer must be non-pageable.
 */
int
copyin(const void *src, void *dst, size_t cnt)
{
	if (cnt == 0)		/* Explicit check for 0 count,		   */
		return 0;	/*   since it will fail VALID_USR_RANGE(). */
	if (!VALID_USR_RANGE(src, cnt))
		return -1;
	CATCH_FAULTS(CATCH_UFAULT)
		uas_copyin((void *)src, dst, cnt);
	return END_CATCH() ? -1 : 0;
}

/*
 * int
 * ucopyout(const void *src, void *dst, size_t cnt, uint_t catch_flags)
 *	Copy bytes from kernel to user address space.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  src		pointer to kernel source buffer
 *	  dst		pointer to user destination buffer
 *	  cnt		number of bytes to copy
 *	  catch_flags	CATCH_FAULTS flags needed for kernel buffer
 *	Returns:
 *	  0 if successful, else errno
 */
int
ucopyout(const void *src, void *dst, size_t cnt, uint_t catch_flags)
{
	int	error;

	if (cnt == 0)		/* Explicit check for 0 count,		   */
		return 0;	/*   since it will fail VALID_USR_RANGE(). */
	if (!WRITEABLE_USR_RANGE(dst, cnt))
		return EFAULT;
	CATCH_FAULTS(catch_flags | CATCH_UFAULT) {
		uas_copyout(src, (void *)dst, cnt);
	}
	error = END_CATCH();
	END_USER_WRITE(dst, cnt);
	return error;
}

/*
 * int
 * copyout(const void *src, void *dst, size_t cnt)
 *	Copy bytes from kernel to user address space.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  src		pointer to kernel source buffer
 *	  dst		pointer to user destination buffer
 *	  cnt		number of bytes to copy
 *	Returns:
 *	  0 if successful, else -1
 *	The kernel buffer must be non-pageable.
 */
int
copyout(const void *src, void *dst, size_t cnt)
{
	int	error;

	if (cnt == 0)		/* Explicit check for 0 count,		   */
		return 0;	/*   since it will fail VALID_USR_RANGE(). */
	if (!WRITEABLE_USR_RANGE(dst, cnt))
		return -1;
	CATCH_FAULTS(CATCH_UFAULT)
		uas_copyout(src, (void *)dst, cnt);
	error = END_CATCH();
	END_USER_WRITE(dst, cnt);
	return error ? -1 : 0;
}

/*
 * int
 * fubyte(const char *addr)
 *	Read a byte from user address space.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  addr		user address to read from
 *	Returns:
 *	  the byte read, or -1 on error
 */
int
fubyte(const char *addr)
{
	int	val;

	if (!VALID_USR_RANGE(addr, sizeof *addr))
		return -1;
	CATCH_FAULTS(CATCH_UFAULT)
		val = (unsigned char)uas_char_in(addr);
	if (END_CATCH() != 0)
		return -1;
	return val;
}

/*
 * int
 * fushort(const ushort_t *addr, ushort_t *valp)
 *	Read an unsigned short from user address space.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  addr		user address to read from
 *	  valp		kernel variable in which to store the value
 *	Returns:
 *	  error code; zero for success
 */
int
fushort(const ushort_t *addr, ushort_t *valp)
{
	if (!VALID_USR_RANGE(addr, sizeof *addr))
		return EINVAL;
	CATCH_FAULTS(CATCH_UFAULT)
		*valp = uas_ushort_in(addr);
	return END_CATCH();
}

/*
 * int
 * fuword(const int *addr)
 *	Read a word from user address space.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  addr		user address to read from
 *	Returns:
 *	  the word (int) read, or -1 on error
 *
 * Remarks:
 *	-1 could actually be the value at the user address;
 *	it is up to the caller to ensure this isn't a legal value.
 */
int
fuword(const int *addr)
{
	int	val;

	if (!VALID_USR_RANGE(addr, sizeof *addr))
		return -1;
	CATCH_FAULTS(CATCH_UFAULT)
		uas_copyin((void *)addr, &val, sizeof(int));
	if (END_CATCH() != 0)
		return -1;
	return val;
}

/*
 * int
 * subyte(char *addr, char val)
 *	Write a byte to user address space.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  addr		user address to write to
 *	  val		byte value to write
 *	Returns:
 *	  0 if successful, or -1 on error
 */
int
subyte(char *addr, char val)
{
	int	error;

	if (!WRITEABLE_USR_RANGE(addr, sizeof *addr))
		return -1;
	CATCH_FAULTS(CATCH_UFAULT)
		uas_char_out(addr, val);
	error = END_CATCH();
	END_USER_WRITE(addr, sizeof *addr);
	return error ? -1 : 0;
}

/*
 * int
 * sushort(ushort_t *addr, ushort_t val)
 *	Write an unsigned short to user address space.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  addr		user address to write to
 *	  val		unsigned short value to write
 *	Returns:
 *	  error code; zero for success
 */
int
sushort(ushort_t *addr, ushort_t val)
{
	int	error;

	if (!WRITEABLE_USR_RANGE(addr, sizeof *addr))
		return EINVAL;
	CATCH_FAULTS(CATCH_UFAULT)
		uas_ushort_out(addr, val);
	error = END_CATCH();
	END_USER_WRITE(addr, sizeof *addr);
	return error;
}

/*
 * int
 * suword(int *addr, int val)
 *	Write a word to user address space.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  addr		user address to write to
 *	  val		word (int) value to write
 *	Returns:
 *	  0 if successful, or -1 on error
 */
int
suword(int *addr, int val)
{
	int	error;

	if (!WRITEABLE_USR_RANGE(addr, sizeof *addr))
		return -1;
	CATCH_FAULTS(CATCH_UFAULT)
		uas_copyout(&val, addr, sizeof(int));
	error = END_CATCH();
	END_USER_WRITE(addr, sizeof *addr);
	return error ? -1 : 0;
}

/*
 * int
 * copyinstr(const char *from, char *to, size_t max, size_t *np)
 *	Copy a string from a user address into a kernel buffer.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  from		pointer to user source null-terminated string
 *	  to		pointer to kernel destination buffer
 *	  max		size of destination buffer, including room for null
 *	  np		out arg, if non-NULL, set to string length, inc. null
 *	Returns:
 *	  0 if successful, else errno (ENAMETOOLONG or EFAULT).
 *	  If not successful, *np is not changed.
 */
int
copyinstr(const char *from, char *to, size_t max, size_t *np)
{
	int	len;

	if (!VALID_USR_RANGE(from, 1))
		return EFAULT;
	CATCH_FAULTS(CATCH_UFAULT) {
		len = uas_strcpy_max(to, from, max);
	}
	if (END_CATCH() != 0)
		return EFAULT;
	if (len == -1)
		return ENAMETOOLONG;
	if (np != NULL)
		*np = len + 1;	/* include null byte */
	return 0;
}


/*
 * int
 * uzero(void *dst, size_t cnt)
 *	Zero a buffer in user address space.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  dst		pointer to user destination buffer
 *	  cnt		number of bytes to zero
 *	Returns:
 *	  0 if successful, or errno from pagefault error.
 */
int
uzero(void *dst, size_t cnt)
{
	int	error;

	if (!WRITEABLE_USR_RANGE(dst, cnt))
		return EFAULT;
	CATCH_FAULTS(CATCH_UFAULT)
		uas_bzero(dst, cnt);
	error = END_CATCH();
	END_USER_WRITE(dst, cnt);
	return error;
}


/*
 * void
 * fc_jmpjmp(void)
 *	Standard handler for catching page fault errors.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	At startup time, u.u_fault_catch.fc_func gets set to this
 *	so that page fault errors will work properly with CATCH_FAULTS()
 *	(see faultcatch.h)
 *
 * NOTE: This function should really be in a different file, something like
 * a pagefault.c if and when such exists.
 */

void
fc_jmpjmp(void)
{
	longjmp(&u.u_fault_catch.fc_jmp);
}


/*
 * void
 * addupc(void (*pc)(), int ticks)
 *	Charges "ticks" units against the specified pc.
 *
 * Calling/Exit State:
 *	This routine may sleep. No spin locks can be held on entry and
 *	no spin locks will be held on return. 
 */

void
addupc(void (*pc)(), int ticks)
{
	long		offset;
	u_short		*bucketp;
	struct prof	*profp = u.u_procp->p_profp;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	SLEEP_LOCK(&profp->pr_lock, PRIMED);

	/*
	 * Now that we have the sleep lock on the prof structure, check 
	 * to see if profiling is still on!
	 */
	if (u.u_lwpp->l_trapevf & EVF_PL_PROF) {
		if ((offset = ((vaddr_t)pc - (vaddr_t)profp->pr_off)) < 0) {
			SLEEP_UNLOCK(&profp->pr_lock);
			return;
		}

		/*
	 	 * Based on the offset, get the appropriate bucket.
	 	 */

		offset = upc_scale(offset, (ulong_t)profp->pr_scale);

		/*
	 	 * Validate the bucket number.
	 	 */

		if (offset >= (profp->pr_size / sizeof (u_short))) {
			SLEEP_UNLOCK(&profp->pr_lock);
			return;		/* Invalid bucket number */
		}

		bucketp = (u_short *)profp->pr_base + offset;
		if (!WRITEABLE_USR_RANGE((vaddr_t)bucketp, sizeof (u_short))) {

			/*
		 	 * The specified bucket is not in the user-writable 
		 	 * range. Turn off profiling. Note that 
			 * since in addupc() we hold the sleep lock
			 * on the prof structure,
		 	 * the profiling state cannot have changed.
			 *
			 * Note: We turn off the L_CLOCK flag in the 
			 * current context, but we do not clear the L_CLOCK
			 * flag of other contexts in the process. In the 
			 * worst case this MAY result in an extra "tick"
			 * to be charged if profiling were to be turned on 
			 * again. 
			 * we consider this anomoly harmless. 
		 	 */
			(void)LOCK(&u.u_procp->p_mutex, PLHI);
			trapevunproc(u.u_procp, EVF_PL_PROF, B_TRUE);
			UNLOCK(&u.u_procp->p_mutex, PLBASE);
			if (u.u_lwpp->l_flag & L_CLOCK) {
				(void)LOCK(&u.u_lwpp->l_mutex, PLHI);
				u.u_lwpp->l_flag &= ~L_CLOCK;
				UNLOCK(&u.u_lwpp->l_mutex, PLBASE);
			}
			SLEEP_UNLOCK(&profp->pr_lock);
			return;
		}
		CATCH_FAULTS(CATCH_UFAULT) {
			uas_ushort_add(bucketp, ticks);
        	}
		if (END_CATCH() != 0) {
			/*
		 	 * We faulted and the fault could not be resolved!
		 	 * The user has given us an invalid address. Turn off
		 	 * profiling. Also turn L_CLOCK flag in the 
			 * current context off. 
		 	 */ 
			(void)LOCK(&u.u_procp->p_mutex, PLHI);
			trapevunproc(u.u_procp, EVF_PL_PROF, B_TRUE);
			UNLOCK(&u.u_procp->p_mutex, PLBASE);
			if (u.u_lwpp->l_flag & L_CLOCK) {
				(void)LOCK(&u.u_lwpp->l_mutex, PLHI);
				u.u_lwpp->l_flag &= ~L_CLOCK;
				UNLOCK(&u.u_lwpp->l_mutex, PLBASE);
			}
		}	
		END_USER_WRITE((vaddr_t)bucketp, sizeof (u_short));
	}
	SLEEP_UNLOCK(&profp->pr_lock);
}
