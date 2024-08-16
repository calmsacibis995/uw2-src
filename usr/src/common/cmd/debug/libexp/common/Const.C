#ident	"@(#)debugger:libexp/common/Const.C	1.4"


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <math.h>
#include "Interface.h"
#include "Const.h"
#include "fpemu.h"

Const&
Const::init(ConstKind kind, const char *s)
{
	int		base;
	unsigned long	ultmp;
	double		dtmp;

	if (*s == '0')
	{
		if (*(s+1) == 'x' || *(s+1) == 'X')
			base = 16;
		else
			base = 8;
	}
	else
		base = 10;
	const_kind = kind;
	errno = 0;

	switch (const_kind)
	{
	case CK_CHAR:
		{
			// multiple character constants ('ab') have type int
			int val = 0;
			for (int index = 0; *s;  s++, index++)
				val = (val << 8)|*s;
			if (index > 1)
			{
				i = val;
				const_kind = CK_INT;
			}
			else
				c = val;
		}
		break;

	case CK_INT:
		ultmp = strtoul(s, 0, base);
		if (ultmp == ULONG_MAX && errno == ERANGE)
		{
			printe(ERR_int_overflow, E_ERROR, s);
			i = 0;
		}
		else if (ultmp <= INT_MAX)
		{
			i = (int)ultmp;
		}
		else if (ultmp <= UINT_MAX && base != 10)
		{
			ui = (unsigned int)ultmp;
			const_kind = CK_UINT;
		}
		else if (ultmp <= LONG_MAX)
		{
			l = (long)ultmp;
			const_kind = CK_LONG;
		}
		else
		{
			ul = ultmp;
			const_kind = CK_ULONG;
		}
		break;

	case CK_UINT:
		ultmp = strtoul(s, 0, base);
		if (ultmp == ULONG_MAX && errno == ERANGE)
		{
			printe(ERR_int_overflow, E_ERROR, s);
			ui = 0;
		}
		else if (ultmp <= UINT_MAX)
		{
			ui = (unsigned int)ultmp;
		}
		else
		{
			ul = ultmp;
			const_kind = CK_ULONG;
		}
		break;

	case CK_LONG:
		ultmp = strtoul(s, 0, base);
		if (ultmp == ULONG_MAX && errno == ERANGE)
		{
			printe(ERR_int_overflow, E_ERROR, s);
			l = 0;
		}
		else if (ultmp <= LONG_MAX)
		{
			l = (long)ultmp;
		}
		else
		{
			ul = ultmp;
			const_kind = CK_ULONG;
		}
		break;

	case CK_ULONG:
		ul = strtoul(s, 0, base);
		if (ul == ULONG_MAX && errno == ERANGE)
		{
			printe(ERR_int_overflow, E_ERROR, s);
		}
		break;

	case CK_DOUBLE:
		d = strtod(s, 0);
		if (d == HUGE_VAL && errno == ERANGE)
		{
			printe(ERR_float_overflow, E_ERROR, s);
		}
		else if (d == -HUGE_VAL && errno == ERANGE)
		{
			printe(ERR_float_underflow, E_ERROR, s);
		}
		break;

	case CK_FLOAT:
		dtmp = strtod(s, 0);
		f = (float)dtmp;
		if ((dtmp == HUGE_VAL && errno == ERANGE)
			|| dtmp > FLT_MAX)
		{
			printe(ERR_float_overflow, E_ERROR, s);
		}
		else if ((dtmp == -HUGE_VAL && errno == ERANGE)
			|| dtmp < FLT_MIN)
		{
			printe(ERR_float_underflow, E_ERROR, s);
		}
		break;

	case CK_XFLOAT:
		errno = 0;
		x = fp_atox(s);
		if (errno)
			printe(ERR_float_overflow, E_ERROR, s);
		break;
	default:
		printe(ERR_internal, E_ERROR, "Const::init", __LINE__);
	}
	return *this;
}
