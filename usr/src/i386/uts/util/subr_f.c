/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:util/subr_f.c	1.11"
#ident	"$Header: $"

/*
 * Miscellaneous family-specific kernel subroutines.
 */

#include <proc/iobitmap.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/seg.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/dl.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#define	CTOI(c)		((c) & 0xf)			/* numeric char->int */
#define TEN_TIMES(n)	(((n) << 3) + ((n) << 1))	/* int to an int */

/*
 * C-library string functions.
 * Assembler versions of others are in util/string.s.
 */

/*
 * char *
 * strncpy(char *, const char *, size_t)
 *
 *	Copy argument 2 to argument 1, truncating or null-padding
 *	to always copy argument 3 number of bytes.
 *
 * Calling/Exit State:
 *
 *	Return the first argument.
 */
char *
strncpy(char *s1, const char *s2, size_t n)
{
	char *os1 = s1;

	n++;
	while (--n != 0 && (*s1++ = *s2++) != '\0')
		continue;
	if (n != 0) {
		while (--n != 0)
			*s1++ = '\0';
	}
	return os1;
}

/*
 * int
 * strncmp(const char *, const char *, size_t)
 *
 *	Compare strings.
 *
 * Calling/Exit State:
 *
 *	Compare at most the third argument number of bytes.
 *	Return *s1-*s2 for the last characters in s1 and s2 which
 *	were compared.
 */
int
strncmp(const char *s1, const char *s2, size_t n)
{
	if (s1 == s2)
		return 0;
	n++;
	while (--n != 0 && *s1 == *s2++) {
		if (*s1++ == '\0')
			return 0;
	}
	return (n == 0) ? 0 : *s1 - *--s2;
}

/*
 * char *
 * strpbrk(const char *string, const char *brkset)
 *
 * Calling/Exit State:
 *	Return ptr to first occurance of any character from `brkset'
 *	in the character string `string'; NULL if none exists.
 */
char *
strpbrk(const char *string, const char *brkset)
{
	char *p;

	do {
		for (p = (char *)brkset; *p != '\0' && *p != *string; ++p)
			;
		if (*p != '\0')
			return (char *)string;
	} while (*string++);

	return (char *)NULL;
}

/*
 * int
 * stoi(char **)
 *
 *	Returns the integer value of the string of decimal numeric
 *	chars beginning at **str.
 *
 * Calling/Exit State:
 *
 *	Does no overflow checking.
 *	Note: updates *str to point at the last character examined.
 *	Assumes the caller has responsibility to mutex the return value.
 */
int
stoi(char **str)
{
	char	*p = *str;
	int	n;
	int	c;

	for (n = 0; (c = *p) >= '0' && c <= '9'; p++) {
		n = TEN_TIMES(n) + CTOI(c);
	}
	*str = p;
	return n;
}

/*
 * void
 * numtos(ulong_t, char *)
 *
 *	Simple-minded conversion of a long into a
 *	null-terminated character string.
 *
 * Calling/Exit State:
 *
 *	Caller must ensure there's enough space
 *	to hold the result.
 */
void
numtos(ulong_t num, char *s)
{
	int i = 0;
	ulong_t mm = 1000000000;
	int t;

	if (num < 10) {
		*s++ = num + '0';
		*s = '\0';
	} else while (mm) {
		t = num / mm;
		if (i || t) {
			i++;
			*s++ = t + '0';
			num -= t * mm;
		}
		mm = mm / 10;
	}
	*s = '\0';
}



/*
 * Function for efficient conversion of time units: from the number of
 * ticks (stored in a double long word) to seconds and nanoseconds, for
 * representing in a timestruc_t.
 */

/*
 * void
 * ticks_to_timestruc(timestruc_t *tp, dl_t *nticksp)
 *
 *	Convert a dl_t number of "ticks" into a timestruc_t.
 *	These dl_t things are sure a pain to manipulate.
 *	It takes forever to operate on them too.
 *
 * Calling/Exit State:
 *
 *	This is purely a conversion utility, so no lock protection is needed.
 */
void
ticks_to_timestruc(timestruc_t *tp, dl_t *nticksp)
{

#define	BILLION		1000000000
#define TWO_EXP_31	0x80000000


	static	boolean_t	first_time = B_TRUE;
	static	dl_t		billion, tres;
	static	uint_t		a, b;
	static	uint_t		c, d;
	
	
	if (first_time) {
		a = (BILLION / timer_resolution);
		b = (BILLION % timer_resolution);
		c = (TWO_EXP_31 / a);
		d = (TWO_EXP_31 % a);
		tres.dl_hop = 0;
		tres.dl_lop = timer_resolution;
		billion.dl_hop = 0;
		billion.dl_lop = BILLION;
		first_time = B_FALSE;
	}

	/*
	 * PERF: 
	 *	The nticks computed first need to be converted into 
	 * 	units of nanoseconds. From this nanosecond count, a
	 *	count of the number of seconds and a count of the leftover 
	 *	nanoseconds need to derived, for the fields
	 *		tp->tv_sec and 
	 *		tp->tv_nsec 
	 *	respectively.
 	 *
	 *	Ordinarily, this could be done by the following sequence
	 *	of equivalent double-long computations: 
	 *		nanosecs = (nticks * timer_resolution);
	 *		q = (nanosecs / billion);
	 *		r = (nanosecs % billion);
	 *	and then the tv_sec and tv_nsec fields of tp
	 *	can be set to q.dl_lop and r.dl_lop respectively.
	 *	
	 *	However, since the above double long computations can
	 *	be very expensive, we perform the following single long
	 *	computations to arrive at the desired results.
	 *
	 *	I. if timer_resolution < billion
	 *	--------------------------------
	 *		let a = (billion / timer_resolution);
	 *		let b = (billion % timer_resolution);
	 *	then 
	 *		SECONDS = ((nticks * timer_resolution) / billion);
	 *	can be computed first as
	 *		SECONDS = (nticks / a);
	 *	and if (b > 0) can then be corrected as:
	 *		SECONDS -= ((SECONDS * b) / billion)
	 *
	 *	and the nanosecond remainder, 
	 *		NANOSECONDS = ((nticks * timer_resolution) % billion);
	 *	can be computed first as
	 *		NANOSECONDS = (timer_resolution * (nticks % a)); 
	 *	and if (b > 0) can then be corrected as:
	 *		NANOSECONDS -= (SECONDS * b);
	 *
	 *	II. if timer_resolution >= billion
	 *	----------------------------------
	 *		This is probably atypical, and we can compute the
	 *	counts directly with the kernel double long functions that
	 *	are available.
	 */
	if (timer_resolution < BILLION) {

		uint_t  seconds_count;
		uint_t	nanosec_count;
	
		seconds_count = nticksp->dl_lop / a;

		if (nticksp->dl_hop != 0) {
			if (nticksp->dl_hop < a) {
				seconds_count += ((nticksp->dl_hop << 1) * c);
				/*
				 * Compute the approximate nanosecond part,
				 * 	timer_resolution * (nticks % a)
				 */
				nanosec_count =  ((nticksp->dl_hop << 1) * d);
				nanosec_count += (nticksp->dl_lop % a);
				nanosec_count %= a;

			} else {
				seconds_count = INT_MAX;
				nanosec_count = 0;
			}
		} else {
			nanosec_count = (nticksp->dl_lop % a);
			nanosec_count *= timer_resolution;
		}

		/*
		 * and then correct both the second and the nanosecond
		 * counts for any errors due to b.
		 */
		if (b > 0) {
			seconds_count -= ((seconds_count * b) / BILLION);
			nanosec_count -= (seconds_count * b);
		}

		tp->tv_sec = seconds_count;
		tp->tv_nsec = nanosec_count;

	} else {
		dl_t nanosecs, q, r;

		nanosecs = lmul((*nticksp), tres);
	        q = ldivide(nanosecs, billion);
		r = lmod(nanosecs, billion);
       		tp->tv_sec = q.dl_lop;
		tp->tv_nsec = r.dl_lop;
	}

}


#ifdef MERGE386

/*
 * void
 * mrg_getparm(enum mrg_parm parm, void *valuep)
 *	Get values needed by the add-on MERGE386 module.
 *
 * Calling/Exit State:
 *	On return, (*valuep) is set to the value of the parameter indicated
 *	by parm.
 *
 *	All of these parameters are either constant (from the base kernel's
 *	point of view) or are per-LWP, per-proc, or per-engine fields accessed
 *	in context, so no locking is needed.
 */
void
mrg_getparm(enum mrg_parm parm, void *valuep)
{
	switch (parm) {
	case PARM_OFFSET_VM86P:
		*(size_t *)valuep = offsetof(user_t, u_vm86p);
		break;
	case PARM_OFFSET_AR0:
		*(size_t *)valuep = offsetof(user_t, u_ar0);
		break;
	case PARM_OFFSET_FAULTCATCH:
		*(size_t *)valuep = offsetof(user_t, u_fault_catch);
		break;
	case PARM_ULWPP:
		*(lwp_t **)valuep = u.u_lwpp;
		break;
	case PARM_OFFSET_L_SPECIAL:
		*(size_t *)valuep = offsetof(lwp_t, l_special);
		break;
	case PARM_IDTP:
		*(struct gate_desc **)valuep = cur_idtp;
		break;
	case PARM_LDTP:
		*(struct segment_desc **)valuep =
				u.u_dt_infop[DT_LDT]->di_table;
		break;
	case PARM_GDTP:
		*(struct segment_desc **)valuep =
				u.u_dt_infop[DT_GDT]->di_table;
		break;
	case PARM_TSSP:
		(void) iobitmap_sync();
		if (u.u_lwpp->l_tssp == NULL)
			*(struct tss386 **)valuep = NULL;
		else
			*(struct tss386 **)valuep = &u.u_lwpp->l_tssp->st_tss;
		break;
	case PARM_RUID:
		*(uid_t *)valuep = CRED()->cr_ruid;
		break;
	case PARM_AS:
		*(struct as **)valuep = u.u_procp->p_as;
		break;
	case PARM_SVC_INTR:
		*(boolean_t *)valuep = servicing_interrupt() ? B_TRUE : 
		  					       B_FALSE;
		break;
	}
}

/*
 *
 * void
 * mrg_clr_vm86_flg(void);
 *
 *	Clear the SPECF_VM86 flag indicating that the process is no longer
 *	a merge process.
 *
 * Calling/Exit State:
 *	This function is called by the merge driver. 
 *	It is always called in context of the lwp.
 *
 * Description:
 *	The kernel clears the flag in the lwp structure and
 * 	the plocal structure.
 */
void
mrg_clr_vm86_flg(void)
{
	u.u_lwpp->l_special &= ~SPECF_VM86;
	l.special_lwp &= ~SPECF_VM86;
}

/*
 *
 * void
 * mrg_set_vm86_flg(void)
 *
 *	Set the SPEC_VM86 flag indicating that the process is a merge 
 *	process.
 *
 * Calling/Exit State:
 *	This function is called by the merge driver. 
 *	It is always called in context of the lwp.
 *
 * Description:
 *	The kernel sets the flag in the lwp structure and
 * 	the plocal structure.
 */
void
mrg_set_vm86_flg(void)
{
	u.u_lwpp->l_special |= SPECF_VM86;
	l.special_lwp |= SPECF_VM86;
}

/*
 *
 * boolean_t
 * mrg_chk_vm86_flg(void);
 *
 *	Check if the SPECF_VM86 flag is set for the lwp.
 *
 * Calling/Exit State:
 *	This function is called by the merge driver. 
 *	It is always called in context of the lwp.
 */
boolean_t
mrg_chk_vm86_flg(void)
{
	if (l.special_lwp & SPECF_VM86) {
		ASSERT(u.u_lwpp->l_special & SPECF_VM86);
		return B_TRUE;
	}
	ASSERT(!(u.u_lwpp->l_special & SPECF_VM86));
	return B_FALSE;
}

#endif /* MERGE386 */
