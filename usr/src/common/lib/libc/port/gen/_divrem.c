/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_divrem.c	1.1"

#include "synonyms.h"
#include <limits.h>

	/*
	* _[il]divrem(*num, den, carry):
	*	*num += carry; carry = *num / den; *num %= den; return carry;
	* where:
	*	no overflow occurs,
	*	den must be 2 or more, and
	*	*num is left in the range [0,den-1].
	*/

#if INT_MAX == LONG_MAX
	#pragma weak _idivrem = _ldivrem
#else

int
#ifdef __STDC__
_idivrem(int *num, int den, int carry)
#else
_idivrem(num, den, carry)int *num, den, carry;
#endif
{
	long tot = carry;

	tot += *num;
	carry = 0;
	if (tot >= den)
	{
		carry = tot / den;
		tot -= carry * (long)den;
	}
	else if (tot < 0)
	{
		carry = tot / -den;
		if ((tot += carry * den) != 0)
		{
			carry++;
			tot += den;
		}
		carry = -carry;
	}
	*num = tot;
	return carry;
}

#endif /*INT_MAX == LONG_MAX*/

long
#ifdef __STDC__
_ldivrem(long *num, long den, long carry)
#else
_ldivrem(num, den, carry)long *num, den, carry;
#endif
{
	if (carry != 0)
	{
		if (carry < 0)
		{
			if (LONG_MIN - carry > *num)
			{
				long over, n;
	
				over = -1;
				if (carry <= -den)
				{
					over -= n = carry / -den;
					carry += n * den;
				}
				if (*num <= -den)
				{
					over -= n = *num / -den;
					*num += n * den;
				}
				if ((*num += carry + den) < 0)
				{
					over--;
					*num += den;
				}
				return over;
			}
		}
		else /* carry > 0 */
		{
			if (LONG_MAX - carry < *num)
			{
				long over, n;
	
				if (carry < den)
					over = 0;
				else
				{
					over = n = carry / den;
					carry -= n * den;
				}
				if (*num >= den)
				{
					over += n = *num / den;
					*num -= n * den;
				}
				if ((*num += carry) >= den)
				{
					over++;
					*num -= den;
				}
				return over;
			}
		}
		*num += carry;
		carry = 0;
	}
	if (*num >= den)
	{
		carry = *num / den;
		*num -= carry * (long)den;
	}
	else if (*num < 0)
	{
		carry = *num / -den;
		if ((*num += carry * den) != 0)
		{
			carry++;
			*num += den;
		}
		carry = -carry;
	}
	return carry;
}
