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
#ident	"@(#)debugger:libexp/common/CCconvert.C	1.11"

#include "Machine.h"
#include "CCtype.h"
#include "Rvalue.h"
#include "Interface.h"
#include "Symbol.h"
#include "Language.h"
#include "Fund_type.h"
#include "cvt_util.h"
#include "Tag.h"
#include <string.h>


CVT_ACTION ft_cvt_table[ftgMAX][ftgMAX] = {
	
//                                                                old |
//    ftgSINT   ftgUINT  ftgPTR    ftgFP   ftgSTR   ftgOTHER  <-new   v
//--------------------------------------------------------
	cSINT,   cUINT,   cUINT,   cTOFP,   cSTR,   cNEQV,   // ftgSINT
	cSINT,   cUINT,   cUINT,   cTOFP,   cSTR,   cNEQV,   // ftgUINT
	cSINT,   cUINT,   cNULL,   cNEQV,   cSTR,   cNEQV,   // ftgPTR
	cSINT,   cUINT,   cNEQV,   cTOFP,   cNEQV,  cNEQV,   // ftgFP
	cSINT,   cUINT,   cUINT,   cNEQV,   cNULL,  cNEQV,   // ftgSTR
	cNEQV,   cNEQV,   cNEQV,   cNEQV,   cNEQV,  cNEQV,   // ftgOTHER
};

FT_GRP
ft_group(Fund_type ft)
{
	switch (ft) {
	case ft_char:
		return (GENERIC_CHAR_SIGNED ? ftgSINT : ftgUINT);
	case ft_schar:
	case ft_short:
	case ft_sshort:
	case ft_int:
	case ft_sint:
	case ft_long:
	case ft_slong:
		return ftgSINT;
	case ft_uchar:
	case ft_ushort:
	case ft_uint:
	case ft_ulong:
		return ftgUINT;
	case ft_pointer:
		return ftgPTR;
	case ft_sfloat:
	case ft_lfloat:
	case ft_xfloat:
		return ftgFP;
	case ft_string:
		return ftgSTR;
	case ft_none:
	default:
		return ftgOTHER;
	}
}

static int
CCcvt_fund_type(Fund_type oldtype, Fund_type newtype, Rvalue &rval)
{
	if (oldtype == newtype) 
		return 1;

	register FT_GRP oldgrp = ft_group(oldtype);
	register FT_GRP newgrp = ft_group(newtype);

	switch (ft_cvt_table[oldgrp][newgrp])
	{
	case cNULL: return 1;	// done.
	case cNEQV: return 0;	// not convertable.
	case cSINT:
		return( cvt_to_SINT(oldtype, newtype, rval));
	case cUINT:
		return( cvt_to_UINT(oldtype, newtype, rval));
	case cTOFP:
		return( cvt_to_FP(oldtype, newtype, rval) );
	case cSTR:
		return( cvt_to_STR(oldtype, newtype, rval));
	default:
		printe(ERR_internal, E_ERROR, "CCcvt_fund_type", __LINE__);
		return 0;
	}
	// NOTREACHED
}

static int
CCcvt_user_type(C_base_type *oldtype, C_base_type *newtype, Rvalue &rval, Language lang,
	int flags)
{
	// get the target type
	Type_form new_form = newtype->form();
	Fund_type new_ft;	// only one of these declarations ...
	Symbol new_ut;		//  ... is used (i.e., new is fund or user type)

	// oldtype is a user type, get the Tag
	Symbol old_ut;
	oldtype->user_type(old_ut);
	Tag old_tag = old_ut.tag();

	if (newtype->isIntegral() && newtype->user_type(new_ut))
	{
		// integral user type; can be treated like a fundamental type
		Tag new_tag = new_ut.tag();
		if( new_tag == t_enumtype )
		{
			new_ft = ft_int;
			new_form = TF_fund;
		}
		else if( new_tag==t_bitfield )
		{
                	Attribute *attrPtr;
			if( (attrPtr=new_ut.attribute(an_type)) &&
					attrPtr->form==af_fundamental_type )
                	{
				new_ft = attrPtr->value.fund_type;
				new_form = TF_fund;
			}
			else
			{
				printe(ERR_internal, E_ERROR, "CCcvt_user_type",
					__LINE__);
			}
		}
		else
		{
			printe(ERR_internal, E_ERROR, "CCcvt_user_type", __LINE__);
			return 0;
		}
	}
	else if (newtype->isPointer() && !newtype->fund_type(new_ft)
		&& (old_tag == t_arraytype || old_tag == t_pointertype))
	{
		// pointer to something other than void *
		C_base_type	*old_btype = new_CTYPE(lang);
		C_base_type	*new_btype = new_CTYPE(lang);

		if (!newtype->user_type(new_ut)
			|| !new_ut.type(new_btype, an_basetype))
		{
			printe(ERR_internal, E_ERROR, "CCcvt_user_type",
				__LINE__);
			delete old_btype;
			delete new_btype;
			return 0;
		}
		if (old_tag == t_pointertype)
		{
			if (!old_ut.type(old_btype, an_basetype))
			{
				printe(ERR_internal, E_ERROR, "CCcvt_user_type",
					__LINE__);
				delete old_btype;
				delete new_btype;
				return 0;
			}

			// warnings are put out here instead of after calls to CCtype_match
			// in CCresolve, because there it would give extraneous messages
			// for functions that aren't the best match while trying to  resolve
			// function overloading
			if (!CCtype_match(lang, new_btype, old_btype, 0))
			{
				if (old_btype->isDerivedFrom(new_btype)
					|| (new_btype->isDerivedFrom(old_btype)
						&& !(flags&IS_ASSIGN)))
				{
					;
				}
				else
				{
					if (!(flags&IS_CAST))
						printe(ERR_C_conversion, E_WARNING);
				}
			}
		}
		else
		{
			if (!old_ut.type(old_btype, an_elemtype))
			{
				printe(ERR_internal, E_ERROR, "CCcvt_user_type",
					__LINE__);
				delete old_btype;
				delete new_btype;
				return 0;
			}
			if (!CCtype_match(lang, new_btype, old_btype, flags))
			{
				if (!(flags&IS_CAST))
					printe(ERR_C_conversion, E_WARNING);
			}
		}
		delete old_btype;
		delete new_btype;
	}		
	else
		newtype->fund_type(new_ft);

	switch(old_tag)
	{
	case t_arraytype:
	case t_pointertype:
		if( new_form == TF_fund )
		{
			if(  !CCcvt_fund_type(ft_pointer, new_ft, rval) )
			{
				return 0;
			}
		}
		// other cases here and in the other switch cases are
		// already handled by CCtype_match
		break;
	case t_bitfield:
		if(new_form==TF_fund )
		{
                	Attribute *oldAttrPtr;
			if( !(oldAttrPtr=old_ut.attribute(an_type)) ||
			    oldAttrPtr->form!=af_fundamental_type ||
			    !CCcvt_fund_type(oldAttrPtr->value.fund_type,
								new_ft,rval) )
			{
				return 0;
			}
		}
		break;
	case t_enumtype:
		if( new_form==TF_fund && !CCcvt_fund_type(ft_int,new_ft,rval) )
		{
			return 0;
		}
		break;
	case t_structuretype:
	case t_uniontype:
		// earlier calls (from CCresolve) to CCtype_match insure that 
		// that these  structs or unions are compatible; no conversion 
		// is necessary
		return 1;
	default:
		return 0;
	}

	return 1;
}

int
CCconvert(Rvalue* rval, C_base_type *newtype, Language lang, int flags)
{
	Fund_type ft1, ft2;

	C_base_type	*oldtype = (C_base_type *)rval->type();

	if( oldtype->form() == TF_fund )
	{
		oldtype->fund_type(ft1);

		Symbol ut;
		if (newtype->user_type(ut))
		{	// convert fundamental type to user type
			Tag tag = ut.tag();
			switch( tag )
			{
			case t_arraytype:
			case t_pointertype:
				if(!CCcvt_fund_type(ft1, ft_pointer, *rval))
				{
					printe(ERR_type_convert, E_ERROR);
					return 0;
				}
				break;
			case t_bitfield:
                		Attribute *attrPtr;
				if( !(attrPtr=ut.attribute(an_type)) ||
			     	    attrPtr->form!=af_fundamental_type ||
				    !CCcvt_fund_type(ft1, 
					      attrPtr->value.fund_type, *rval) )
				{
					printe(ERR_type_convert, E_ERROR);
					return 0;
				}
				break;
			case t_enumtype:
				
				if(!CCcvt_fund_type(ft1, ft_int, *rval))
				{
					printe(ERR_type_convert, E_ERROR);
					return 0;
				}
				break;
			default:
				printe(ERR_type_convert, E_ERROR);
				return 0;
			}
		}
		else if (newtype->fund_type(ft2))
		{	// convert fundamental type to fundamental type
			if( !CCcvt_fund_type(ft1, ft2, *rval) )
			{
				printe(ERR_type_convert, E_ERROR);
				return 0;
			}
		}
		else
		{
			printe(ERR_internal, E_ERROR,  "CCconvert", __LINE__);
			return 0;
		}
	}
	else if (!CCcvt_user_type(oldtype, newtype, *rval, lang, flags))
	{
		printe(ERR_type_convert, E_ERROR);
		return 0;
	}

	rval->set_type(newtype->clone());
	return 1;
}

// NOTE:  must be able to convert the type conversions defined
// in compatibleUserTypes and CCtype_match
// MORE - this code doesn't handle qualified types (const, volatile) at all...
static int
compatibleUserTypes(Language lang, C_base_type *lhs_type, C_base_type *rhs_type, int flags)
{
	
	Symbol rhs_ut,
	       lhs_ut;
	rhs_type->user_type(rhs_ut);
	lhs_type->user_type(lhs_ut);

	Tag tag_rhs = rhs_ut.tag();
	Tag tag_lhs = lhs_ut.tag();
	int ret = 1;

	switch (tag_lhs)
	{
	case t_pointertype:
	{
		C_base_type	*rhs_btype = new_CTYPE(lang);
		C_base_type	*lhs_btype = new_CTYPE(lang);

		if (!lhs_ut.type(lhs_btype, an_basetype))
		{
			printe(ERR_internal, E_ERROR, 
					"compatibleUSerTypes", __LINE__);
			ret = 0;
		}
		else if (tag_rhs == t_pointertype)
		{
			if (!rhs_ut.type(rhs_btype, an_basetype))
			{
				printe(ERR_internal, E_ERROR, 
					"compatibleUSerTypes", __LINE__);
				ret = 0;
			}

			Fund_type	lft;
			Fund_type	rft;

			if (flags&IS_CAST)
			{
				ret = 1;
			}
			else if (lhs_btype->fund_type(lft))
			{
				if (lft != ft_void
					&& (lang != C || !rhs_btype->fund_type(rft)
						|| rft != ft_void))
				{
					// pointers to incompatible fundamental types
					// the case of right-fund-type == left-fund-type
					// already covered in CCtype_match by
					// *lhs_type == *rhs_type
					// int * = void * is ok in C, not ok in C++
					ret = 0;
				}
			}
			else if (rhs_btype->fund_type(rft))
			{
				// left is user type, right is fundamental type
				if (rft == ft_void)
				{
					if (lang != C)
						ret = 0;
				}
				else
					ret = 0;
			}
			else if (!CCtype_match(lang, lhs_btype, rhs_btype, flags))
			{
				if (rhs_btype->isDerivedFrom(lhs_btype)
					|| (lhs_btype->isDerivedFrom(rhs_btype)
						&& !(flags&IS_ASSIGN)))
				{
					;
				}
				else
				{
					ret = 0;
				}
			}
		}
		else if (tag_rhs == t_arraytype)
		{
			if (!rhs_ut.type(rhs_btype, an_elemtype))
			{
				printe(ERR_internal, E_ERROR, 
					"compatibleUSerTypes", __LINE__);
				ret = 0;
			}
			else if (!(flags&IS_CAST)
				&& !CCtype_match(lang, lhs_btype, rhs_btype, flags))
			{	
				ret = 0;
			}
		}
		else
		{
			ret = 0;
		}
		delete rhs_btype;
		delete lhs_btype;
		return ret;
	}

	case t_arraytype:
	{
		C_base_type	*rhs_etype = new_CTYPE(lang);
		C_base_type	*lhs_etype = new_CTYPE(lang);

		// in C and C++, array types match if the element types
		// match and the sizes of the arrays match (meaning same
		// number of elements)

		if (!lhs_ut.type(lhs_etype, an_elemtype))
		{
			printe(ERR_internal, E_ERROR, 
					"compatible_types", __LINE__);
			ret = 0;
		}
		else if (rhs_ut.type(rhs_etype, an_elemtype)	// not an array
			|| rhs_type->size() != lhs_type->size()
			|| !CCtype_match(lang, lhs_etype, rhs_etype, flags))
		{
			ret = 0;
		}
		delete rhs_etype;
		delete lhs_etype;
		return ret;
	}

	case t_enumtype:
	{
		const char *rhs_name = rhs_ut.name();
		const char *lhs_name = lhs_ut.name();

		return rhs_name && lhs_name && !strcmp(lhs_name, rhs_name);
	}
	case t_structuretype:
	case t_uniontype:
		if (tag_lhs == tag_rhs)
		{
			// note: if they are the same size then they are considered
			// compatible, this is considerably looser that ANSI C.
			return lhs_type->size() == rhs_type->size();
		}
		return 0;

	case t_functiontype:
		if (tag_rhs == t_functiontype || tag_rhs == t_global_sub
			|| tag_rhs == t_subroutine)
		{
			C_base_type	*lhs_rtype = new_CTYPE(lang);
			C_base_type	*rhs_rtype = new_CTYPE(lang);

			if (!lhs_ut.type(lhs_rtype, an_resulttype)
				|| !rhs_ut.type(rhs_rtype, an_resulttype))
			{
				printe(ERR_internal, E_ERROR,
					"compatibleUserTypes", __LINE__);
				ret = 0;
			}
			else
			{
				// MORE - this checks that the return types match,
				// but does not check compatibility of argument types
				Fund_type	ft1;
				Fund_type	ft2;

				if (lhs_rtype->fund_type(ft1) && ft1 == ft_void
					&& rhs_rtype->fund_type(ft2) && ft2 == ft_void)
					;
				else if (!CCtype_match(lang, lhs_rtype, rhs_rtype, flags))
					ret = 0;
			}
			delete rhs_rtype;
			delete lhs_rtype;
			return ret;
		}
		return 0;

	default:
		return 0;
	}
}


int
CCtype_match(Language lang, C_base_type *lhs_type, C_base_type *rhs_type, int flags, CVT_ACTION *action)
{
	Type_form lhs_form = lhs_type->form();
	Type_form rhs_form = rhs_type->form();
	Fund_type lhs_ft,
		  rhs_ft;
	Symbol rhs_ut,
	       lhs_ut;

	if (action)
		*action = cNULL;

	if (*lhs_type == *rhs_type)
		return 1;

	if (lhs_form == TF_user && rhs_form == TF_user)
	{
		return compatibleUserTypes(lang, lhs_type, rhs_type, flags);
	}
	else if (lhs_form == TF_fund && rhs_form == TF_fund)
	{
		if (!lhs_type->fund_type(lhs_ft) || !rhs_type->fund_type(rhs_ft))
		{
			printe(ERR_internal, E_ERROR, "CCtype_match", __LINE__);
			return 0;
		}

do_fund_type:
		FT_GRP	left = ft_group(lhs_ft);
		FT_GRP	right = ft_group(rhs_ft);
		CVT_ACTION act = ft_cvt_table[left][right];
		if (act == cNEQV)
			return 0;

		if (left != right && action)
			*action = act;
		return 1;	
	}

	// else lhs_form != rhs_form
	Symbol utype;

	// if one of the operands is an enum, treat it as an int
	// also, check for combinations of void * and other pointers
	if (rhs_form == TF_user)
	{
		// since the form of lhs_type is TF_fund, if it is a pointer,
		// it can only be void *, but that is compatible with any
		// rhs pointer type or array
		if (lhs_type->isPointer() && rhs_type->isPtrType())
			return 1;
		if (!rhs_type->user_type(utype))
			return 0;
		if (utype.tag() == t_enumtype)	
		{
			lhs_type->fund_type(lhs_ft);
			rhs_ft = ft_int;
			goto do_fund_type;
		}
	}
	else
	{
		// in this case, we have rhs == void * and lhs == pointer to something else
		// this is ok for assignments in C but not in C++
		if (lhs_type->isPointer() && rhs_type->isPointer())
		{
			if (lang != C && (flags&IS_ASSIGN))
				return 0;
			else if (!(flags&IS_CAST))
				return 1;
		}
		if (!lhs_type->user_type(utype))
			return 0;
		if (utype.tag() == t_arraytype && rhs_type->isPointer()
			&& !(flags&IS_ASSIGN))
			return 1;

		// in C++, int cannot be assigned to enum without cast
		if (utype.tag() == t_enumtype && !((flags&IS_ASSIGN) && lang != C))	
		{
			rhs_type->fund_type(rhs_ft);
			lhs_ft = ft_int;
			goto do_fund_type;
		}
	}

	return 0;
}
