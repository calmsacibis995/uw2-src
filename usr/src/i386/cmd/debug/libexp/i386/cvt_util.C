/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ident	"@(#)debugger:libexp/i386/cvt_util.C	1.14"

#include <string.h>
#include <errno.h>
#include "Itype.h"
#include "CCtype.h"
#include "Language.h"
#include "Fund_type.h"
#include "Tag.h"
#include "Rvalue.h"
#include "Symbol.h"
#include "Interface.h"
#include "cvt_util.h"
#include "setjmp.h"
#include "fpemu.h"
#include "Machine.h"

#ifdef __cplusplus
extern "C" int fpsetsticky(int);
#else
extern int fpsetsticky(int);
#endif

// extended precision floating-point constants used in evaluation
// use fpemu format

fp_x_t fp_zero;
fp_x_t fp_one = { 0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0x3f };
fp_x_t fp_neg_one = { 0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0xbf };

int
cvtTo_and_return_ULONG(Rvalue *rval, unsigned long &ulRslt )
{
	Itype rslt;
	C_base_type ulTYPE(ft_ulong); // cfront 1.2 has trouble with implicit ...
				//   ... parameter conversion; do it explicitly
	
	if ((!CCconvert(rval, &ulTYPE, C, 0))
		|| ( rval->get_Itype(rslt) == SINVALID ))
	{
		printe(ERR_internal, E_ERROR, "cvtToULONG", __LINE__);
		return 0;
	}

	ulRslt = rslt.iuint4;
	return 1;
}

int
cvtTo_and_return_LONG(Rvalue *rval,  long &lRslt )
{
        Itype rslt;
        C_base_type lTYPE(ft_long); // cfront 1.2 has trouble with implicit ...
                              //   ... parameter conversion; do it explicitly
        if ((!CCconvert(rval, &lTYPE, C, 0))
        	|| ( rval->get_Itype(rslt) == SINVALID ))
        {
                printe(ERR_internal, E_ERROR, "cvtToLONG", __LINE__);
                return 0;
        }

        lRslt = rslt.iint4;
        return 1;
}

int
cvtTo_and_return_INT(Rvalue *rval, int &iRslt)
{
	return cvtTo_and_return_LONG(rval, (long&)iRslt );
}

int
cvtTo_and_return_UINT(Rvalue *rval, unsigned int &uiRslt)
{
	return cvtTo_and_return_ULONG(rval, (unsigned long&)uiRslt );
}

int
cvtTo_and_return_FLOAT(Rvalue *rval, float &fltRslt)
{
        Itype rslt;
        C_base_type fltTYPE(ft_sfloat); // cfront 1.2 has trouble with implicit ...
                              //   ... parameter conversion; do it explicitly
        if ((!CCconvert(rval, &fltTYPE, C, 0))
        	|| ( rval->get_Itype(rslt) == SINVALID ))
        {
                printe(ERR_internal, E_ERROR, "cvtToDOUBLE", __LINE__);
                return 0;
        }

        fltRslt = rslt.isfloat;
        return 1;
}

int
cvtTo_and_return_DOUBLE(Rvalue *rval, double &dblRslt )
{
        Itype rslt;
        C_base_type dblTYPE(ft_lfloat); // cfront 1.2 has trouble with implicit ...
                              //   ... parameter conversion; do it explicitly
        if ((!CCconvert(rval, &dblTYPE, C, 0))
        	|| ( rval->get_Itype(rslt) == SINVALID ))
        {
                printe(ERR_internal, E_ERROR, "cvtToDOUBLE", __LINE__);
                return 0;
        }

        dblRslt = rslt.idfloat;
        return 1;
}

// cfront 1.2 doesn't support long double
// use floating point emulation
int
cvtTo_and_return_LDOUBLE(Rvalue *rval, fp_x_t &ldRslt )
{
        Itype rslt;
        C_base_type ldblTYPE(ft_xfloat); // cfront 1.2 has trouble with implicit ...
                              //   ... parameter conversion; do it explicitly
        if ((!CCconvert(rval, &ldblTYPE, C, 0))
        	|| ( rval->get_Itype(rslt) == SINVALID ))
        {
                printe(ERR_internal, E_ERROR, "cvtToLDOUBLE", __LINE__);
                return 0;
        }

	cvt_to_emu(ldRslt, rslt.rawbytes);
        return 1;
}


// -- conversion routines:
//    - return 0 with NO message if types clearly incompatable.
//    - change value to match size and representation requirements
//      of the new type.

int
cvt_to_SINT(Fund_type oldtype, Fund_type newtype, Rvalue &rval)
{
	if( oldtype == newtype )
		return 1;

	Stype	oldStype;

	Itype Ival;
	oldStype = rval.get_Itype(Ival);

	long l;
	switch(oldStype)
	{
	case Schar: l = Ival.ichar; break;
	case Sint1: l = Ival.iint1; break;
	case Sint2: l = Ival.iint2; break;
	case Sint4: l = Ival.iint4; break;
	case Suchar: l = Ival.iuchar; break;
	case Suint1: l = Ival.iuint1; break;
	case Suint2: l = Ival.iuint2; break;
	case Suint4: l = Ival.iuint4; break;
	case Ssfloat: l = (long)Ival.isfloat; break;
	case Sdfloat: l = (long)Ival.idfloat; break;
	case Sxfloat: 
		{
			errno = 0;
			l = fp_xtol(Ival.ixfloat);
			if (errno)
			{
				printe(ERR_float_eval, E_ERROR);
				return 0;
			}
			break;
		}
	case Saddr: l = (long)Ival.iaddr; break;
	case Sdebugaddr: 
		{
		char * end_addr;
		l = strtol(Ival.idebugaddr, &end_addr, 0);
		if (Ival.idebugaddr == end_addr) return 0;
		break;
		}
	default:
		printe(ERR_internal, E_ERROR, "cvt_to_UINT", __LINE__);
		return 0;
	}

	TYPE tmpType;
	tmpType = newtype;
	Stype newStype;
	tmpType.get_Stype(newStype);

	void	*valPtr;
	switch(newStype)
	{
	case Schar: 
		Ival.ichar = (char)l;
		valPtr = &Ival.ichar;
		break;
	case Sint1:
		Ival.iint1 = (char)l;
		valPtr = &Ival.iint1;
		break;
	case Sint2: 
		Ival.iint2 = (short)l;
		valPtr = &Ival.iint2;
		break;
	case Sint4: 
		Ival.iint4 = l;
		valPtr = &Ival.iint4;
		break;
	default:
		printe(ERR_internal, E_ERROR, "cvt_to_SINT", __LINE__);
		return 0;
	}

	rval.reinit(valPtr, tmpType.size(), tmpType.clone());
	return 1;
}

int
cvt_to_UINT(Fund_type oldtype, Fund_type newtype, Rvalue &rval)
{
	if( oldtype == newtype )
		return 1;

	Stype	oldStype;

	Itype Ival;
	oldStype = rval.get_Itype(Ival);

	unsigned long ul;
	switch(oldStype)
	{
	case Schar: ul = Ival.ichar; break;
	case Sint1: ul = Ival.iint1; break;
	case Sint2: ul = Ival.iint2; break;
	case Sint4: ul = Ival.iint4; break;
	case Suchar: ul = Ival.iuchar; break;
	case Suint1: ul = Ival.iuint1; break;
	case Suint2: ul = Ival.iuint2; break;
	case Suint4: ul = Ival.iuint4; break;
	case Ssfloat: ul = (unsigned long)Ival.isfloat; break;
	case Sdfloat: ul = (unsigned long)Ival.idfloat; break;
	case Sxfloat:
		{
			errno = 0;
			ul = fp_xtoul(Ival.ixfloat);
			if (errno)
			{
				printe(ERR_float_eval, E_ERROR);
				return 0;
			}
			break;
		}
	case Saddr: ul = Ival.iaddr; break;
	case Sdebugaddr: 
		{
		char * end_addr;
		ul = (unsigned long)strtol(Ival.idebugaddr, &end_addr, 0);
		if (Ival.idebugaddr == end_addr) return 0;
		break;
		}
	default:
		printe(ERR_internal, E_ERROR, "cvt_to_UINT", __LINE__);
		return 0;
	}

	TYPE tmpType;
	tmpType = newtype;
	Stype newStype;
	tmpType.get_Stype(newStype);

	void	*valPtr;
	switch(newStype)
	{
	case Suchar: 
		Ival.iuchar = (unsigned char)ul;
		valPtr = (void *)&Ival.iuchar;
		break;
	case Suint1: 
		Ival.iuint1 = (unsigned char)ul;
		valPtr = (void *)&Ival.iuint1;
		break;
	case Suint2: 
		Ival.iuint2 = (unsigned short)ul;
		valPtr = (void *)&Ival.iuint2;
		break;
	case Suint4: 
		Ival.iuint4 = ul;
		valPtr = (void *)&Ival.iuint4;
		break;
	case Saddr:
		Ival.iuint4 = ul;
		valPtr = (void *)&Ival.iaddr;
		break;
	case Sdebugaddr:
		Ival.iuint4 = ul;
		valPtr = (void *)&Ival.idebugaddr;
		break;
	default:
		printe(ERR_internal, E_ERROR, "cvt_to_UINT", __LINE__);
		return 0;
	}

	rval.reinit(valPtr, tmpType.size(), tmpType.clone());
	return 1;
}

int
cvt_to_FP(Fund_type oldtype, Fund_type newtype, Rvalue &rval)
{
	if( oldtype == newtype )
		return 1;

	Stype	oldStype;
	Itype Ival;
	oldStype = rval.get_Itype(Ival);

	// long double ld;-- Cfront 1.3 can't handle this
	// use floating point emulation
	double d;
	fp_x_t	ld;
	switch(oldStype)
	{
	case Schar: d = Ival.ichar; break;
	case Sint1: d = Ival.iint1; break;
	case Sint2: d = Ival.iint2; break;
	case Sint4: d = Ival.iint4; break;
	case Suchar: d = Ival.iuchar; break;
	case Suint1: d = Ival.iuint1; break;
	case Suint2: d = Ival.iuint2; break;
	case Suint4: d = Ival.iuint4; break;
	case Ssfloat: d = Ival.isfloat; break;
	case Sdfloat: d = Ival.idfloat; break;
	case Sxfloat:   cvt_to_emu(ld, Ival.rawbytes);

			break;
	default:
		printe(ERR_internal, E_ERROR, "cvt_INT_to_FP", __LINE__);
		return 0;
	}

	TYPE tmpType;
	tmpType = newtype;
	Stype newStype;
	tmpType.get_Stype(newStype);

	void	*valPtr;
	switch(newStype)
	{
	case Ssfloat:
		
		if (oldStype == Sxfloat)
		{
			fp_f_t	f;
			errno = 0;
			f = fp_xtof(ld);
			if (errno)
			{
				printe(ERR_float_eval, E_ERROR);
				return 0;
			}
			Ival.isfloat = *(float *)&f.ary[0];
		}
		else
			Ival.isfloat = d;
		valPtr = (void *)&Ival.isfloat;
		break;
	case Sdfloat:
		if (oldStype == Sxfloat)
		{
			fp_d_t	dtmp;
			errno = 0;
			dtmp = fp_xtod(ld);
			if (errno)
			{
				printe(ERR_float_eval, E_ERROR);
				return 0;
			}
			Ival.idfloat = *(double *)&dtmp.ary[0];
		}
		else
			Ival.idfloat = d;
		valPtr = (void *)&Ival.idfloat;
		break;
	case Sxfloat:
		if (oldStype == Sxfloat)
		{
			cvt_to_internal(Ival.rawbytes, ld);
		}
		else
		{
			double2extended(&d, (void *)Ival.rawbytes);
		}
		valPtr = (void *)Ival.rawbytes;
		break;
	default:
		printe(ERR_internal, E_ERROR, "cvt_INT_to_FP", __LINE__);
		return 0;
	}


	rval.reinit(valPtr, tmpType.size(), tmpType.clone());
	return 1;
}


int
cvt_to_STR(Fund_type oldtype, Fund_type newtype, Rvalue &rval)
{
	if( oldtype == newtype )
		return 1;

	Stype	oldStype;

	Itype Ival;
	oldStype = rval.get_Itype(Ival);

	long l;
	switch(oldStype)
	{
	case Schar: l = Ival.ichar; break;
	case Sint1: l = Ival.iint1; break;
	case Sint2: l = Ival.iint2; break;
	case Sint4: l = Ival.iint4; break;
	case Suchar: l = Ival.iuchar; break;
	case Suint1: l = Ival.iuint1; break;
	case Suint2: l = Ival.iuint2; break;
	case Suint4: l = Ival.iuint4; break;
	case Ssfloat: l = (long)Ival.isfloat; break;
	case Sdfloat: l = (long)Ival.idfloat; break;
	case Sxfloat:
		{
			errno = 0;
			l = fp_xtol(Ival.ixfloat);
			if (errno)
			{
				printe(ERR_float_eval, E_ERROR);
				return 0;
			}
			break;
		}
	case Saddr: l = (long)Ival.iaddr; break;
	default:
		printe(ERR_internal, E_ERROR, "cvt_to_STR", __LINE__);
		return 0;
	}

	TYPE tmpType;
	tmpType = newtype;
	Stype newStype;
	tmpType.get_Stype(newStype);
	int length;

	void	*valPtr;
	switch(newStype)
	{
	case Sdebugaddr: 
	{
		char * s = new char[32];  // When can this be freed?
		if(oldStype == Saddr)
			sprintf(s, "0x%x", l);
		else
			sprintf(s, "%i", l);
		valPtr = (void *)s;
		length = strlen(s);
		break;
	}
	default:
		printe(ERR_internal, E_ERROR, "cvt_to_STR", __LINE__);
		return 0;
	}

	rval.reinit(valPtr, length+1, tmpType.clone());
	return 1;
}

#ifdef __cplusplus
extern void add_error_handler(void(*)(int));
#else
extern void add_error_handler(SIG_HANDLER);
#endif

void
#ifdef __cplusplus
init_fp_error_recovery(void(*recovery_function)(int))
#else
init_fp_error_recovery(SIG_HANDLER recovery_function)
#endif
{
	add_error_handler(recovery_function);
}

void
clear_fp_error(void)
{
        asm("fclex");
        asm("finit");
	fpsetsticky(0);
}

void
clear_fp_error_recovery(void)
{
	add_error_handler(0);
}

void
extended2double(void *, double *)
{
	asm("movl	8(%ebp),%eax");
	asm("fldt	(%eax)");
	asm("movl	12(%ebp),%eax");
	asm("fstpl	(%eax)");
}

void
double2extended(double *, void *)
{
	asm("movl	8(%ebp),%eax");
	asm("fldl	(%eax)");
	asm("movl	12(%ebp),%eax");
	asm("fstpt	(%eax)");
}

// format an extended float for printf
// if cfront 1.2 supported long double, this would not be necessary
// for %f style formats we allocate a buffer that will be big enough
// for all eventualities
char *
print_extended(void *value, char format, char *format_str, char *buf)
{
	char	*val = (char *)value;
	char	*dest;

	if (format == 'f')
	{
		// dest is deleted by print_rvalue()
		// big enough to handle LDBL_MAX
		dest = new(char[5000]); 
		sprintf(dest, format_str, *(unsigned long *)(&val[0]),
			*(unsigned long *)(&val[4]),
			(unsigned long)*(unsigned short *)(&val[8]));
		return dest;
	}
	sprintf(buf, format_str, *(unsigned long *)(&val[0]),
		*(unsigned long *)(&val[4]),
		(unsigned long)*(unsigned short *)(&val[8]));
	return 0;
}

// convert fp emulation number to internal long double form
void
cvt_to_internal(void *internal, fp_x_t &emu)
{
	// memory form has 12 bytes - register form has 10
	char	*p = (char *)internal;
	memcpy(internal, (void *)&emu.ary[0], EXTENDED_SIZE);
	p[10] = p[11] = 0;
}

// convert internal long double form to fp emulation form
void
cvt_to_emu(fp_x_t &emu, void *internal)
{
	memcpy((void *)&emu.ary[0], internal, EXTENDED_SIZE);
}
