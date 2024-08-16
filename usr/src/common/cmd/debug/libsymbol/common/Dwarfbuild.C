#ident	"@(#)debugger:libsymbol/common/Dwarfbuild.C	1.10"
#include	"Dwarfbuild.h"
#include	"Interface.h"
#include	"Object.h"
#include	"ELF.h"
#include	"Locdesc.h"
#include	"Syminfo.h"
#include	"Tag.h"
#include	"Iaddr.h"
#include	"dwarf.h"
#include	<string.h>


Dwarfbuild::Dwarfbuild( ELF *obj )
{
	ptr = 0;
	object = obj;
	length = 0;
	nextoff = 0;
	Sectinfo	sinfo;
	if (!object->getsect(s_debug, &sinfo))
	{
		entry_offset = 0;
		entry_end = 0;
	}
	else
	{
		entry_data = (char *)sinfo.data;
		entry_offset = sinfo.offset;
		entry_end = entry_offset + sinfo.size;
		entry_base = sinfo.vaddr;
	}
	if (!object->getsect(s_line, &sinfo))
		stmt_offset = 0;
	else
	{
		stmt_data = (char *)sinfo.data;
		stmt_offset = sinfo.offset;
		stmt_end = stmt_offset + sinfo.size;
		stmt_base = sinfo.vaddr;
	}
}

// this is not inlined because of the destructors this function must call
Dwarfbuild::~Dwarfbuild()
{
}

char
Dwarfbuild::get_byte()
{
	char *	p;

	p = ptr; ++ptr;
	length -= 1;
	return *p;
}

// the following code will not work if the target byte order
// is different from the host byte order
short
Dwarfbuild::get_2byte()
{
	short	x;
	char *	p = (char*)&x;

	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr;
	length -= 2;
	return x;
}

long
Dwarfbuild::get_4byte()
{
	long	x;
	char 	*p = (char*)&x;

	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr;
	length -= 4;
	return x;
}

char *
Dwarfbuild::get_string()
{
	char *		s;
	register int	len;

	len = strlen(ptr)+1;
	s = new(char[len]);
	memcpy(s,ptr,len);
	ptr += len;
	length -= len;
	return s;
}

Addrexp
Dwarfbuild::make_chunk(void * p, int len)
{
	char *		s;

	s = new(char[len]);
	memcpy(s,(char*)p,len);
	return Addrexp(s);
}

// internal debuger tags indexed by dwarf tag values
static Tag	tagname[] = 
{
	t_none,			// TAG_padding
	t_arraytype,		// TAG_array_type
	t_structuretype,	// TAG_class_type
	t_entry,		// TAG_entry_point
	t_enumtype,		// TAG_enumeration_type
	t_argument,		// TAG_formal_parameter
	t_global_sub,		// TAG_global_subroutine
	t_global_variable,	// TAG_global_variable
	t_none,			// 
	t_none,			// 
	t_label,		// TAG_label
	t_block,		// TAG_lexical_block
	t_local_variable,	// TAG_local_variable
	t_structuremem,		// TAG_member 
	t_none,			// 
	t_pointertype,		// TAG_pointer_type
	t_reftype,		// TAG_reference_type
	t_sourcefile,		// TAG_compile_unit
	t_stringtype,		// TAG_string_type
	t_structuretype,	// TAG_structure_type
	t_subroutine,		// TAG_subroutine
	t_functiontype,		// TAG_subroutine_type
	t_typedef,		// TAG_typedef
	t_uniontype,		// TAG_union_type
	t_unspecargs,		// TAG_unspecified_parameters
	t_none,			// TAG_variant
	t_none,			// TAG_common_block
	t_none,			// TAG_common_inclusion
	t_none,			// TAG_inheritance
	t_none,			// TAG_inlined_subroutine
	t_none,			// TAG_module
	t_none,			// TAG_ptr_to_member_type
	t_none,			// TAG_set_type
	t_none,			// TAG_subrange_type
	t_none,			// TAG_with_stmt
};

// types indexed by dwarf tag value
static Attr_name	typename[] = 
{
	an_nomore,		// TAG_padding
	an_elemtype,		// TAG_array_type
	an_nomore,		// TAG_class_type
	an_resulttype,		// TAG_entry_point
	an_nomore,		// TAG_enumeration_type
	an_type,		// TAG_formal_parameter
	an_resulttype,		// TAG_global_subroutine
	an_type,		// TAG_global_variable
	an_nomore,		//
	an_nomore,		//
	an_nomore,		// TAG_label
	an_nomore,		// TAG_lexical_block
	an_type,		// TAG_local_variable
	an_type,		// TAG_member
	an_nomore,		// 
	an_basetype,		// TAG_pointer_type
	an_basetype,		// TAG_reference_type
	an_nomore,		// TAG_compile_unit
	an_nomore,		// TAG_string_type
	an_nomore,		// TAG_structure_type
	an_resulttype,		// TAG_subroutine
	an_resulttype,		// TAG_subroutine_type
	an_type,		// TAG_typedef
	an_nomore,		// TAG_union_type
	an_nomore,		// TAG_unspecified_parameters
	an_nomore,		// TAG_variant
	an_nomore,		// TAG_common_block
	an_nomore,		// TAG_common_inclusion
	an_nomore,		// TAG_inheritance
	an_nomore,		// TAG_inlined_subroutine
	an_nomore,		// TAG_module
	an_nomore,		// TAG_ptr_to_member_type
	an_nomore,		// TAG_set_type
	an_nomore,		// TAG_subrange_type
	an_nomore,		// TAG_with_stmt
};


void
Dwarfbuild::skip_attribute( short attrname )
{
	short	len2;
	long	word;

	switch( attrname & FORM_MASK )
	{
	case FORM_NONE:				
				break;
	case FORM_ADDR:
	case FORM_REF:		(void)get_4byte();		
				break;
	case FORM_BLOCK2:	len2 = get_2byte();
				length -= len2;
				ptr += len2;		
				break;
	case FORM_BLOCK4:	word = get_4byte();
				length -= word;
				ptr += word;	
				break;
	case FORM_DATA2:	(void)get_2byte();		
				break;
	case FORM_DATA8:	(void)get_4byte();
	case FORM_DATA4:	(void)get_4byte();		
				break;
	case FORM_STRING:	word = strlen(ptr) + 1;
				length -= word;
				ptr += word;		
				break;
	default:
		printe(ERR_bad_debug_entry, E_ERROR, entry_offset +
			(ptr-2)-entry_data);
		length = 0;
	}
}

void
Dwarfbuild::get_ft( Attr_form & form, Attr_value & value )
{
	short	ft;
	form = af_fundamental_type;
	if ((ft = get_2byte()) > FT_label)
		value.fund_type  = ft_none;
	else
		value.fund_type = (Fund_type)ft;
}

void
Dwarfbuild::fund_type()
{
	Attr_value	value;
	Attr_form	form;

	get_ft( form, value );
	protorec.add_attr( typename[tag], form, value );
}

void
Dwarfbuild::get_udt( Attr_form & form, Attr_value & value )
{
	form = af_dwarfoffs;
	value.word = entry_offset + get_4byte() - entry_base;
}

void
Dwarfbuild::user_def_type()
{
	Attr_value	value;
	Attr_form	form;

	get_udt( form, value );
	protorec.add_attr( typename[tag], form, value );
}

void
Dwarfbuild::get_mft( Attr_form & form, Attr_value & value )
{
	short		len2;
	int		modcnt;
	char *		p;
	int		i;
	
	// form is 2 byte length, followed by modcnt bytes of modifiers
	// followed by the 2 byte fundamental type
	len2 = get_2byte();
	modcnt = len2 - 2;
	ptr += modcnt;
	length -= modcnt;
	value.fund_type = (Fund_type)get_2byte();
	form = af_fundamental_type;
	p = ptr - 3;
	for ( i = 0 ; i < modcnt ; i++ )
	{
		switch (*p)
		{
		case MOD_pointer_to:
			prototype.add_attr(an_tag,af_tag,t_pointertype);
			break;
		case MOD_reference_to:
			prototype.add_attr(an_tag,af_tag,t_reftype);
			break;
		case MOD_const:
		case MOD_volatile:
		default:
			break;
		}
		prototype.add_attr(an_basetype,form,value);
		prototype.add_attr(an_bytesize, af_int, 4L );
		value.symbol = prototype.put_record();
		form = af_symbol;
		--p;
	}
}

void
Dwarfbuild::mod_fund_type()
{
	Attr_value	value;
	Attr_form	form;

	get_mft( form, value );
	protorec.add_attr( typename[tag], form, value );
}

void
Dwarfbuild::get_mudt( Attr_form & form, Attr_value & value )
{
	short		len2;
	int		modcnt;
	char *		p;
	int		i;

	// form is 2 byte length, followed by modcnt bytes of modifiers
	// followed by the 4 byte reference to the user-defined type
	len2 = get_2byte();
	modcnt = len2 - 4;
	ptr += modcnt;
	length -= modcnt;
	value.word = entry_offset + get_4byte() - entry_base;
	form = af_dwarfoffs;
	p = ptr - 5;
	for ( i = 0 ; i < modcnt ; i++ )
	{
		switch (*p)
		{
		case MOD_pointer_to:
			prototype.add_attr(an_tag,af_tag,t_pointertype);
			break;
		case MOD_reference_to:
			prototype.add_attr(an_tag,af_tag,t_reftype);
			break;
		case MOD_const:
		case MOD_volatile:
		default:
			break;
		}
		prototype.add_attr(an_basetype,form,value);
		prototype.add_attr(an_bytesize, af_int, 4L );
		value.symbol = prototype.put_record();
		form = af_symbol;
		--p;
	}
}

void
Dwarfbuild::mod_u_d_type()
{
	Attr_value	value;
	Attr_form	form;

	get_mudt( form, value );
	protorec.add_attr( typename[tag], form, value );
}

void
Dwarfbuild::sibling()
{
	long	word;
	long	siboff;

	word = get_4byte();
	siboff = entry_offset + word - entry_base;
	protorec.add_attr(an_sibling,af_dwarfoffs,siboff);
	if ( (nextoff != siboff) && (tag != TAG_enumeration_type) )
	{
		// if the sibling is not the next record, we have
		// children and must save the scansize;
		// enum children are handled elsewhere
		protorec.add_attr(an_scansize, af_int, siboff - nextoff);
		protorec.add_attr(an_child, af_dwarfoffs, nextoff);
	}
}

void
Dwarfbuild::name()
{
	buildNameAttrs(protorec, get_string());
}

void
Dwarfbuild::language()
{
	LANG		flang;
	Language	dlang;

	flang = (LANG) get_4byte();
	switch(flang)
	{
		case LANG_C89:	
		case LANG_C:
				dlang = C;	
				break;
		case LANG_C_PLUS_PLUS:	
				dlang = CPLUS;	
				break;
		case LANG_UNK:
		default:	
				dlang = C;	
				break;
	}
	protorec.add_attr(an_language, af_language, dlang);
}

void
Dwarfbuild::get_location( Attr_form & form, Attr_value & value )
{
	Locdesc		locdesc;
	short		len2;
	char 		op;

	locdesc.clear();
	len2 = get_2byte();

	if (len2 == 0)
		return;

	while ( len2 > 0 )
	{
		op = get_byte();
		// the constants subtracted in the following code
		// represent a 2 or 4 byte length plus 1 byte operator
		switch( op )
		{
		case OP_REG:
			locdesc.reg( (int)get_4byte() );
			len2 -= 5;
			break;
		case OP_BASEREG:
			locdesc.basereg( (int)get_4byte() );
			len2 -= 5;
			break;
		case OP_ADDR:
			locdesc.addr( get_4byte() );
			len2 -= 5;
			break;
		case OP_CONST:
			locdesc.offset( get_4byte() );
			len2 -= 5;
			break;
		case OP_DEREF4:
			locdesc.deref4();
			len2 -= 1;
			break;
		case OP_ADD:
			locdesc.add();
			len2 -= 1;
			break;
		default:
			len2 -= 1;
			break;
		}
	}
	value.loc = make_chunk(locdesc.addrexp(),locdesc.size());
	form = af_locdesc;
}

void
Dwarfbuild::location()
{
	Attr_form	form;
	Attr_value	value;
	
	get_location( form, value );
	protorec.add_attr( an_location, form, value );
}

void
Dwarfbuild::byte_size()
{
	protorec.add_attr( an_bytesize, af_int, get_4byte() );
}

void
Dwarfbuild::bit_offset()
{
	protorec.add_attr( an_bitoffs, af_int, get_2byte() );
}

void
Dwarfbuild::bit_size()
{
	protorec.add_attr( an_bitsize, af_int, get_4byte() );
}

void
Dwarfbuild::stmt_list()
{
	long	word;

	word = stmt_offset + get_4byte() - stmt_base;
	protorec.add_attr(an_lineinfo,af_dwarfline, word);
}

Iaddr
Dwarfbuild::low_pc()
{
	Iaddr result = get_4byte();
	protorec.add_attr( an_lopc, af_addr, result );
	return result;
}

void
Dwarfbuild::high_pc()
{
	protorec.add_attr( an_hipc, af_addr, get_4byte() );
}

void
Dwarfbuild::element_list( short attr, long offset )
{
	Attribute	*prev;
	long		len;
	Attr_value	value;
	Attr_form	form;

	prev = 0;
	if ( (attr & FORM_MASK) == FORM_BLOCK2 )
		// old style enum list
		len = get_2byte();
	else if ( (attr & FORM_MASK) == FORM_BLOCK4 )
		len = get_4byte();
	else
	{
		printe(ERR_bad_debug_entry, E_ERROR, entry_offset+
			(ptr-2)-entry_data);
		len = 0;
	}
	while ( len > 0 )
	{
		prototype.add_attr( an_tag, af_tag, t_enumlittype );
		prototype.add_attr( an_litvalue, af_int, get_4byte() );
		len -= 4;
		len -= (strlen(ptr)+1);
		buildNameAttrs(prototype, get_string());
		prototype.add_attr( an_sibling, af_symbol, prev );
		prototype.add_attr( an_parent, af_dwarfoffs, offset );
		prev = prototype.put_record();
	}
	form = af_symbol;
	value.symbol = prev;
	protorec.add_attr( an_child, form, value );
}


enum array_attrs 
{
	ar_elemtype = 0,
	ar_subtype,
	ar_lobound,
	ar_hibound,
	ar_tagtype
};

void
Dwarfbuild::next_item( Attribute * a )
{
	Attribute	attr[5];

	if ( subscr_list( attr ) )
	{
		for(int i = 0; i < 5; i++)
			prototype.add_attr( (Attr_name)attr[i].name, 
				(Attr_form)attr[i].form, attr[i].value );
		a[ar_elemtype].value.symbol = prototype.put_record();
		a[ar_elemtype].form = af_symbol;
		a[ar_elemtype].name = an_elemtype;
	}
	else
	{
		a[ar_elemtype] = attr[ar_elemtype];
	}
}

int
Dwarfbuild::subscr_list( Attribute * a )
{
	char		fmt;
	short		et_name;
	Attr_form	form;
	Attr_value	value;

	a[ar_elemtype].name = an_elemtype;
	a[ar_subtype].name = an_subscrtype;
	a[ar_lobound].name = an_lobound;
	a[ar_hibound].name = an_hibound;
	a[ar_tagtype].name = an_tag;
	a[ar_tagtype].form = af_tag;
	a[ar_tagtype].value.tag = t_arraytype;
	fmt = get_byte();
	if (fmt == FMT_ET)
	{
		et_name = get_2byte();
		switch( et_name )
		{
		case AT_fund_type:
			get_ft( form, value );
			break;
		case AT_user_def_type:
			get_udt( form, value );
			break;
		case AT_mod_fund_type:
			get_mft( form, value );
			break;
		case AT_mod_u_d_type:
			get_mudt( form, value );
			break;
		default:
			skip_attribute( et_name );
			return 0;
		}
		a[ar_elemtype].form = form;
		a[ar_elemtype].value = value;
		return 0;
	}
	else if (fmt > FMT_ET)
		return 0;
	else
	{
		if ((fmt >> 2) & FMT_UDT)
			get_udt( form, value );
		else
			get_ft( form, value );
		a[ar_subtype].form = form;
		a[ar_subtype].value = value;
		if ((fmt >> 1) & FMT_EXPR)
		{
			get_location( form, value );
			a[ar_lobound].form = form;
			a[ar_lobound].value = value;
		}
		else
		{
			a[ar_lobound].form = af_int;
			a[ar_lobound].value.word = get_4byte();
		}
		if (fmt & FMT_EXPR)
		{
			get_location( form, value );
			a[ar_hibound].form = form;
			a[ar_hibound].value = value;
		}
		else
		{
			a[ar_hibound].form = af_int;
			a[ar_hibound].value.word = get_4byte();
		}
		next_item( a );
		return 1;
	}
}

void
Dwarfbuild::subscr_data()
{
	Attribute	attr[5];

	(void)get_2byte();
	if ( subscr_list( attr ) )
	{
		for(int i = 0; i < 4; i++)
			protorec.add_attr( (Attr_name)attr[i].name, 
				(Attr_form)attr[i].form, attr[i].value );
		// make_record adds the tag for attr[4]
	}
}

// mask off form part of attribute name
#define NAME_MASK	0xfff0

Attribute *
Dwarfbuild::make_record( long offset, int )
{
	short		attrname;
	long		word;
	Attribute	*attribute;
	Iaddr		lowpc;
	Tag		tag_internal = t_none;
	int		has_type = 0;

	if ( offset == 0 )
	{
		return 0;
	}
	if ( reflist.lookup(offset,attribute) )
	{
		return attribute;
	}

	if ((offset < entry_offset) || (offset >= entry_end))
		return 0;
	ptr = entry_data + offset - entry_offset;

	word = get_4byte();
	// length is not set here, but get_4byte() doesn't care
	if ( word <= 8 )
		return 0;
	length = word - 4;
	nextoff = offset + word;
	tag = get_2byte();
	if (tag > TAG_with_stmt)
		return 0;
	tag_internal = tagname[tag];
	if ( tag != TAG_compile_unit )
		protorec.add_attr( an_parent, af_symbol, 0L );
		// the value of this attribute is reset by
		// Evaluator::add_parent
	int loc = 0;
	while ( length > 0 )
	{
		attrname = get_2byte();
		switch( attrname )
		{
		case AT_padding:	break;
		case AT_sibling:	sibling(); break;
		case AT_location:	location(); loc = 1; break;
		case AT_name:		name(); break;
		case AT_fund_type:	fund_type(); has_type = 1; break;
		case AT_mod_fund_type:	mod_fund_type(); has_type = 1; break;
		case AT_user_def_type:	user_def_type(); has_type = 1; break;
		case AT_mod_u_d_type:	mod_u_d_type(); has_type = 1; break;
		case AT_byte_size:	byte_size(); break;
		case AT_bit_offset:	bit_offset();
					if (tag_internal == t_structuremem)
						tag_internal = t_bitfield;
					break;
		case AT_bit_size:	bit_size(); break;
		case AT_stmt_list:	stmt_list(); break;
		case AT_high_pc:	high_pc(); loc = 1; break;
		case AT_subscr_data:	subscr_data(); break;
		case AT_low_pc:		lowpc =  low_pc();
					loc = 1;
					// special case for labels:
					// make hipc == lopc
					if (tag == TAG_label)
					{
						protorec.add_attr( an_hipc, 
						af_addr, lowpc);
						
					}
					break;
		case AT_language: 	language(); break;
		// not handled
		// case AT_ordering:
		// case AT_discr:
		// case AT_discr_value:
		// case AT_string_length:
		// case AT_common_reference:
		// case AT_comp_dir:
		// case AT_const_value:	multiple forms!
		// case AT_containing_type:
		// case AT_default_value:	multiple forms!
		// case AT_friends:
		// case AT_inline:
		// case AT_is_optional:
		// case AT_lower_bound:	multiple forms!
		// case AT_program:
		// case AT_private:
		// case AT_producer:
		// case AT_protected:
		// case AT_prototyped:
		// case AT_public:
		// case AT_pure_virtual:
		// case AT_return_addr:
		// case AT_specification:
		// case AT_start_scope:
		// case AT_stride_size:
		// case AT_upper_bound:	multiple forms!
		// case AT_virtual:
		default:		
					// element list attributes
					// may have form block1 or
					// block2
					if ((attrname & NAME_MASK) == 
					(AT_element_list & NAME_MASK))
						element_list(attrname, offset);
					else
					{
						skip_attribute(attrname);
					}
					break;
		}
	}

	// right now can't handle entries for global declarations
	// that are not definitions
	if (!loc && (tag == TAG_global_variable || 
		tag == TAG_global_subroutine))
			tag_internal = t_none;

	// for C and C++, functions with no return type are of type void -
	// needed to distinguish them from functions returning int compiled
	// without debugging information
	if ((tag == TAG_entry_point || tag == TAG_global_subroutine
		|| tag == TAG_subroutine || tag == TAG_subroutine_type)
		&& !has_type)
		protorec.add_attr(an_resulttype, af_fundamental_type, ft_void);

	protorec.add_attr( an_tag, af_tag, tag_internal );
	attribute =  protorec.put_record();
	reflist.add( offset, attribute );
	return attribute;
}

int
Dwarfbuild::get_syminfo( long offset, Syminfo & syminfo )
{
	long	word;
	int	len;
	short	attrname;
	int	loc = 0;

	if ((offset < entry_offset) || (offset >= entry_end))
		return 0;
	ptr = entry_data + offset - entry_offset;
	word = get_4byte();
	// length is not set here, but get_4byte() doesn't care
	if ( word <= 8 )
	{
		syminfo.type = st_none;
		syminfo.bind = sb_weak;
		syminfo.name = 0;
		syminfo.sibling = offset + word;
		syminfo.child = 0;
		syminfo.lo = 0;
		syminfo.hi = 0;
		syminfo.resolved = 0;
		return 1;
	}
	nextoff = offset + word;
	length = word - 4;
	tag = get_2byte();
	switch ( tag )
	{
	case TAG_padding:
		return 0;
	case TAG_global_variable:
		syminfo.type = st_object;
		syminfo.bind = sb_global;
		break;
	case TAG_global_subroutine:
		syminfo.type = st_func;
		syminfo.bind = sb_global;
		break;
	case TAG_subroutine:
		syminfo.type = st_func;
		syminfo.bind = sb_local;
		break;
	case TAG_local_variable:
	case TAG_formal_parameter:
		syminfo.type = st_object;
		syminfo.bind = sb_local;
		break;
	case TAG_compile_unit:
		syminfo.type = st_file;
		syminfo.bind = sb_none;
		break;
	default:
		syminfo.type = st_none;
		syminfo.bind = sb_none;
		break;
	}
	syminfo.name = 0;
	syminfo.sibling = nextoff;
	syminfo.resolved = 1;
	syminfo.child = 0;
	syminfo.lo = 0;
	syminfo.hi = 0;
	while ( length > 0 )
	{
		attrname = get_2byte();
		switch ( attrname )
		{
		case AT_name:
			syminfo.name = (long)ptr;
			len = ::strlen(ptr) + 1;
			ptr += len;
			length -= len;
			break;
		case AT_sibling:
			word = get_4byte();
			syminfo.sibling = entry_offset + word - entry_base;
			syminfo.child = nextoff;
			break;
		case AT_low_pc:
			syminfo.lo = get_4byte();
			loc = 1;
			break;
		case AT_high_pc:
			syminfo.hi = get_4byte();
			loc = 1;
			break;
		case AT_location:
			loc = 1;
			// FALLTHROUGH
		default:
			skip_attribute( attrname );
		}
	}
	// right now can't handle entries for global declarations
	// that are not definitions
	if (syminfo.bind == sb_global && !loc)
		syminfo.resolved = 0;
	return 1;
}

int
Dwarfbuild::cache( long offset, long size )
{
	if ((offset < entry_offset) || ((offset + size) > entry_end))
		return 0;
	else
		return 1;
}

Lineinfo *
Dwarfbuild::line_info( long offset )
{
	Lineinfo *	lineinfo;
	long		line;
	Iaddr		pcval;
	Iaddr		base_address;

	lineinfo = 0;
	if ((offset < stmt_offset) || (offset >= stmt_end))
		return 0;
	ptr = stmt_data + offset - stmt_offset;
	{
		length = get_4byte();
		base_address = get_4byte();
		while ( length > 0 )
		{
			line = get_4byte();
			(void)get_2byte(); // line position not currently used
			pcval =  base_address + get_4byte();
			if ( line != 0 )
			{
				protoline.add_line( pcval, line );
			}
			else
			{
				lineinfo = protoline.put_line( pcval );
				break;
			}
		}
	}
	return lineinfo;
}

long		
Dwarfbuild::globals_offset() 
{ 
	return first_file();
}
