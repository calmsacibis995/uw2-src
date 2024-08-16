#ident	"@(#)debugger:libexp/common/CCtype.C	1.19"

#include "Attribute.h"
#include "Buffer.h"
#include "Fund_type.h"
#include "Interface.h"
#include "Language.h"
#include "Machine.h"
#include "ProcObj.h"
#include "Symbol.h"
#include "CC.h"
#include "CCtype.h"
#include "CCtree.h"
#include "Tag.h"
#include "Resolver.h"
#include "Protorec.h"
#include "Parser.h"
#include "libexp.h"
#include "Locdesc.h"
#include "Place.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>

static const int indentation = 4;

const char *
CC_fund_type(Fund_type ftype)
{
	switch(ftype)
	{
	case ft_char:		return "char";
	case ft_schar:		return "signed char";
	case ft_uchar:		return "unsigned char";
	case ft_short:		return "short";
	case ft_ushort:		return "unsigned short";
	case ft_sshort:		return "signed short";
	case ft_int:		return "int";
	case ft_sint:		return "signed int";
	case ft_uint:		return "unsigned int";
	case ft_long:		return "long";
	case ft_slong:		return "signed long";
	case ft_ulong:		return "unsigned long";
	case ft_pointer:	return "void *";
	case ft_sfloat:		return "float";
	case ft_lfloat:		return "double";
	case ft_xfloat:		return "long double";
	case ft_void:		return "void";
	default:		return "";	
	}
}

// CC_print_type prints the C-style declaration of a type in the
// result buffer.  Since type declarations in C work
// from the inside out, it collects the type information
// in two buffers, result and suffix.  The suffix buffer is appended
// to as it works down the type tree, result is appended to as it
// comes back up, and when it is back at the top, suffix is appended
// to the result.  So for
//	int (*f[3])()
// (an array of pointers to functions returning int), it will
// produce the suffix of "[3])()" on the way down, and the result buffer
// of "int (*f" on the way back up, and the two are concatenated together
// before returning from print_type.

// print_type is the top-level function, and is called with a symbol
// (from symbols -t or whatis name) or with a type (whatis expression
// or from a cast in CCresolve).  The entity passed to print_type
// may be anything (variable, function, type name, enumeration literal,
// etc.), and it makes the call to the appropriate function, which
// may include do_type.  do_type operates on a type structure only,
// but if the type is an aggregate, it may call print_children, which
// in turn calls print_type again on each member.

class CC_print_type
{
	Buffer	*result;
	Buffer	*suffix;
	int	level;
	ProcObj	*pobj;
	Language lang;

	int	is_real_type_name(const char *name);
	int	print_parameters(Symbol &func);
	int	print_children(Symbol &parent);
	int	print_enum(Symbol &parent);
	int	do_type(C_base_type *t, const char *name = 0, int print_all = 0);
	int	do_type(Symbol &, const char *class_name = 0);
	void	print_base_classes(Symbol &sym);

	void	init(Language l, Buffer *, ProcObj *);
	void	cleanup();

public:
	int	print_type(C_base_type *, Language l, Buffer *, ProcObj *);
	int	print_type(Symbol &, Language l, Buffer *, ProcObj *);
};

static CC_print_type	cc_print_type;

void
CC_print_type::init(Language l, Buffer *r, ProcObj *p)
{
	lang = l;
	result = r;
	result->clear();
	suffix = buf_pool.get();
	suffix->clear();
	level = 0;
	pobj = p;
}

void
CC_print_type::cleanup()
{
	result->add((char *)*suffix);
	buf_pool.put(suffix);
}

// add_text appends a string to the result buffer, after checking to see
// if a blank is needed to separate two words
static void
add_text(Buffer *buf, const char *text)
{
	if (!text)
		return;

	if (buf->size() > 1 && isalnum(((char *)*buf)[buf->size()-2]))
		buf->add(' ');
	buf->add(text);
}

int
CC_print_type::is_real_type_name(const char *name)
{
	if (!name)
		return 0;

	switch (lang)
	{
	case C:
	case CPLUS_ASSUMED:	// returns 0 for ".anything"
		return (name[0] != '.');

	case CPLUS_ASSUMED_V2:	// returns 0 for "__Tdigits"
		if (*name++ == '_' && *name++ == '_' && *name++ == 'T')
		{
			while (*name && isdigit(*name))
				name++;
			return (*name);
		}
		return 1;

	default:
		return 1;
	}
}

// do_type recursively calls itself to create a string representation
// of the type of the symbol.  name is non-zero only for aggregate
// (struct/union/class) members.  Printing the type of a top-level
// entity doesn't need the name, since that is what the user asked for.
// The only affect of print_all is on whether or not to print the children
// of an aggregate type.  print_all is 1 if the top-level print_type
// was called from whatis type-name, and 0 if it was called from
// symbols -t or whatis symbol-name

int
CC_print_type::do_type(C_base_type *t, const char *name, int print_all)
{
	C_base_type		subtype;
	Fund_type	ftype;
	Symbol		tsym, tsym2;
	Attribute	*attr;
	const char	*type_name;
	
	if (t->form() == TF_user && t->user_type(tsym))
	{
		if (tsym.isnull())
		{
			printe(ERR_cant_get_type, E_ERROR);
			return 0;
		}

		Tag tag = tsym.tag();
		switch (tag)
		{
		default:
			add_text(result, name);
			break;

		case t_typedef:
			add_text(result, tsym.name());
			add_text(result, name);
			break;

		case t_pointertype:
			// find out if it is a pointer to a function or array, if so,
			// add parens.  If it is a pointer to anything else
			// the parens are not needed
			if (!tsym.type(&subtype, an_basetype, 0))
			{
				printe(ERR_no_type, E_ERROR, tsym.name());
				return 0;
			}
			if (subtype.form() == TF_user && subtype.user_type(tsym2)
				&& ((tsym2.tag() == t_functiontype)
				   || (tsym2.tag() == t_arraytype)))
			{
				suffix->add(")");
				if (!do_type(&subtype))
					return 0;
				result->add("(*");
			}
			else
			{
				if (!do_type(&subtype))
					return 0;
				add_text(result, "*");
			}
			add_text(result, name);
			break;

		case t_arraytype:
			if ((attr = tsym.attribute(an_hibound))
				&& attr->form == af_int)
			{
				char dim[MAX_INT_DIGITS+3];
				sprintf(dim, "[%d]", attr->value.word+1);
				suffix->add(dim);
			}
			else
				suffix->add("[]");
			if (!tsym.type(&subtype, an_elemtype, 0))
			{
				printe(ERR_no_type, E_ERROR, tsym.name());
				return 0;
			}
			if (!do_type(&subtype, 0, print_all))
				return 0;
			add_text(result, name);
			break;

		case t_enumtype:
		case t_structuretype:
		case t_uniontype:
			if (tag == t_structuretype)
				result->add("struct");
			else if (tag == t_enumtype)
				result->add("enum");
			else
				result->add("union");

			type_name = tsym.name();
			if (is_real_type_name(type_name))
				add_text(result, type_name);

			if (print_all && tag != t_enumtype)
			{
				if (!print_children(tsym))
					return 0;
			}
			add_text(result, name);
			break;

		case t_functiontype:
			if (!tsym.type(&subtype, an_resulttype, 0))
			{
				printe(ERR_no_type, E_ERROR, tsym.name());
				return 0;
			}
			if (!do_type(&subtype) || !print_parameters(tsym))
				return 0;
			break;
		}
	}
	else if (t->fund_type(ftype))
	{
		result->add(CC_fund_type(ftype));
		add_text(result, name);
	}
	else
	{
		printe(ERR_internal, E_ERROR, "CC_print_type::do_type", __LINE__);
		return 0;
	}
	return 1;
}

// print_parameters, print_children, and print_type can't use the Buffer pool
// since they are called recursively, and the buffer pool has an arbitrary
// limit on the number of buffers available.

int
CC_print_type::print_parameters(Symbol &func)
{
	Symbol	param = func.child();
	int	seen_one = 0;
	C_base_type	t;
	Buffer	*save_result;
	Buffer	*save_suffix;
	Buffer	rbuf;
	Buffer	sbuf;

	save_suffix = suffix;
	save_result = result;
	result = &rbuf;
	suffix = &sbuf;

	rbuf.add('(');
	while (!param.isnull())
	{
		if (param.isArgument() && param.type(&t, an_type, 0))
		{
			if (seen_one)
				rbuf.add(", ");
			if (!do_type(&t))
			{
				suffix = save_suffix;
				result = save_result;
				return 0;
			}
			rbuf.add((char *)sbuf);
			sbuf.clear();
			seen_one = 1;
		}
		else if (param.isUnspecArgs())
		{
			if (seen_one)
				rbuf.add(", ");
			rbuf.add("...");
			break;
		}
		param = param.sibling();
	}
	rbuf.add(')');

	suffix = save_suffix;
	result = save_result;
	suffix->add((char *)rbuf);
	return 1;
}

// print_base_classes makes assumptions about the layout of structures
// generated by the C-generating ANSI C++ compiler.  This does not work
// with cfront-generated code
void
CC_print_type::print_base_classes(Symbol &sym)
{
	Symbol	class_sym;
	char	*class_name;
	int	first = 1;

	for (; !sym.isnull(); sym = sym.sibling())
	{
		while (!sym.isMember() && !sym.isnull())
			sym = sym.sibling();

		if (!is_base_class(sym, class_sym, class_name))
			return;

		if (first)
		{
			result->add(": ");
			first = 0;
		}
		else
			result->add(", ");
		result->add(class_name);
	}
}

// is_static is called from print_children, which is looking
// through the global name space for class members.  Any variables defined
// there are static members.  Functions are static if the first argument is not
// a this pointer
static int
is_static(Symbol &sym)
{
	if (sym.isVariable())
		return 1;

	Symbol	child;
	for (child = sym.child(); !child.isnull(); child = child.sibling())
	{
		if (!child.isArgument())
			continue;

		return (strcmp(child.name(), THIS_NM) != 0);
	}
	return 0;
}

#define	MAX_LINE	80

int
CC_print_type::print_children(Symbol &parent)
{
	Symbol	child;
	Buffer	*save_suffix;
	Buffer	childbuf;
	char	class_name[BUFSIZ];
	char	*space_ptr;

	level++;
	space_ptr = spaces.get_spaces(level*indentation);

	sprintf(class_name, "%.*s::", BUFSIZ-3, parent.name());
	save_suffix = suffix;
	suffix = &childbuf;

	child = parent.child();
	if (lang == CPLUS || lang == CPLUS_ASSUMED_V2)
		print_base_classes(child);

	result->add(" {\n");
	// MORE - this loop will have to deal with declarations of member
	// functions and static members from a real DWARF-generating C++ compiler
	for (; !child.isnull(); child = child.sibling())
	{
		// don't print compiler-introduced names
		char *name = child.name();
		if (lang == CPLUS_ASSUMED)
		{
			char *ptr = skip_class_name(name);
			if (!ptr || strcmp(ptr, VTBL_POINTER_NAME) == 0)
				continue;
		}
		else
		{
			if (strcmp(name, VTBL_POINTER_NAME) == 0)
				continue;
		}

		result->add(space_ptr);
		if (!do_type(child, class_name))
		{
			suffix = save_suffix;
			return 0;
		}
		result->add((char *)childbuf);
		childbuf.clear();

		if (child.tag() == t_bitfield)
		{
			Attribute	*attr;
			char		buf[MAX_INT_DIGITS+3];
			
			if ((attr = child.attribute(an_bitsize)) == 0
				|| attr->form != af_int)
			{
				printe(ERR_internal, E_ERROR, "CC_print_type::print_children",
					__LINE__);
				suffix = save_suffix;
				return 0;
			}
			sprintf(buf, ":%d;\n", (int)attr->value.word);
			result->add(buf);
		}
		else
			result->add(";\n");
	}

	// The debugging information provided by cfront or c++fe does not include
	// function members or static data members in the class declaration,
	// so the global name space is searched for anything that starts
	// with "class_name::".  find_next_global takes a string and
	// a null symbol, and returns the first symbol whose name starts
	// with the string.  On subsequent calls, pass in the previous
	// symbol, and it returns the next symbol that matches
	// (they are in alphabetical order)
	if (level == 1 && (lang == CPLUS_ASSUMED || lang == CPLUS_ASSUMED_V2))
	{
		child.null();
		while (pobj->find_next_global(class_name, child))
		{
			// don't print actual vtbl as part of class
			char *name = skip_class_name(child.name());
			if (!name || strcmp(name, VTBL_NAME) == 0)
				continue;

			result->add(space_ptr);
			if (is_static(child))
				result->add("static ");
			if (!do_type(child, class_name))
			{
				suffix = save_suffix;
				return 0;
			}

			result->add((char *)childbuf);
			result->add(";\n");
			childbuf.clear();
		}
	}

	level--;
	result->add(spaces.get_spaces(level*indentation));
	result->add("} ");
	suffix = save_suffix;
	return 1;
}

int
CC_print_type::print_enum(Symbol &parent)
{
	Symbol		child;
	int		first = 1;
	char		buf[MAX_INT_DIGITS+2];
	char		*ptr = strrchr((char *)*result, '\n');
	int		linelen = ptr ? strlen(ptr) : result->size()-1;

	result->add(" { ");
	for (child = parent.child(); !child.isnull(); child = child.sibling())
	{
		if (first)
		{
			first = 0;
		}
		else
		{
			result->add(", ");
			linelen += 2;
		}
		if (linelen >= MAX_LINE)
		{
			result->add('\n');
			linelen = 0;
		}
		result->add(child.name());

		Attribute	*literal = child.attribute(an_litvalue);
		if (!literal || literal->form != af_int)
		{
			printe(ERR_internal, E_ERROR, "CC_print_type::print_enum", __LINE__);
			return 0;
		}
		sprintf(buf, "=%d", literal->value.word);
		result->add(buf);
		linelen += strlen(child.name()) + strlen(buf);
	}
	result->add(" }");
	return 1;
}

// this path is reached from symbols -t, via the global print_type function,
// or from whatis struct/union/class name, which will get into
// CC_print_type::print_children, which will call do_type on each member
int
CC_print_type::do_type(Symbol &s, const char *class_name)
{
	Tag		tag = s.tag();
	C_base_type		t;
	const char	*name;

	switch(tag)
	{
	case t_label:
		result->add("label");
		return 1;

	// prints the name of the enumeration
	case t_enumlittype:
		{
			Symbol &p = s.parent();
			t = p;
		}
		return do_type(&t);

	// prints the name of the enumeration, followed by all enum constants
	case t_enumtype:
		t = s;
		if (!do_type(&t))
			return 0;
		return print_enum(s);

	case t_structuretype:
	case t_uniontype:
	{
		Buffer		*save_suffix;
		Buffer		buf;

		t = s;
		save_suffix = suffix;
		suffix = &buf;
		if (!do_type(&t, 0, 1))
		{
			suffix = save_suffix;
			return 0;
		}
		suffix = save_suffix;
		result->add((char *)buf);
		return 1;
	}

	case t_global_sub:
	case t_subroutine:
	case t_entry:
		name = s.name();
		if (level)
		{
			if (name && class_name)
			{
				// class member, class name already shown
				int len = strlen(class_name);
				if (strncmp(name, class_name, len) == 0)
					name += len;
			}
		}
		else if (name)
		{
			// printing the name is not necessary here, since it would
			// have gotten here only if user did syms -t or whatis fname
			if (!strchr(name, '(') && !strstr(name, "::"))
				name = 0;
		}
		if (s.type(&t, an_resulttype, 0))
		{
			if (!do_type(&t, name))
				return 0;
		}
		else if (name)
		{
			result->add("void");
			add_text(result, name);
		}
		if (!name || !strchr(name, '(')) // C++ name includes prototype info.
			return print_parameters(s);
		return 1;

	case t_typedef:
		if (!s.type(&t, an_type, 0))
			return 0;
		return do_type(&t);

	case t_argument:
	case t_global_variable:
	case t_local_variable:
	case t_unionmem:
	case t_structuremem:
	default:
		if (!s.type(&t, an_type, 0))
			return 0;
		if (level)
		{
			name = s.name();
			if (name && class_name)
			{
				int len = strlen(class_name);
				if (strncmp(name, class_name, len) == 0)
					name += len;
			}
			return do_type(&t, name, 1);
		}
		else
			return do_type(&t);
	}
}

// this path is reached from whatis expression, via C_base_type::print
int
CC_print_type::print_type(C_base_type *t, Language l, Buffer *b, ProcObj *p)
{
	init(l, b, p);
	int ret = do_type(t, 0);
	cleanup();
	return ret;
}

// this path is reached from whatis expression, via C_base_type::print
int
CC_print_type::print_type(Symbol &sym, Language l, Buffer *b, ProcObj *p)
{
	init(l, b, p);
	int ret = do_type(sym, 0);
	cleanup();
	return ret;
}

// this path is reached from symbols -t
int
print_type(Symbol& s, Buffer *result, ProcObj *pobj)
{
	Language lang = current_language(pobj);

	switch(lang)
	{
		case C:
		case CPLUS:
		case CPLUS_ASSUMED:
		case CPLUS_ASSUMED_V2:
			return cc_print_type.print_type(s, lang, result, pobj);

		default:
			printe(ERR_internal, E_ERROR, "print_type", __LINE__);
			result->clear();
			return 0;
	}
}

C_base_type *
new_CTYPE(Language lang)
{
	switch (lang)
	{
		case CPLUS_ASSUMED:	return new CPP_cfront_type();
		case CPLUS_ASSUMED_V2:	return new CPP_cgen_type();

		default:
		case C:			return new C_type();
	}
}

C_base_type *
new_CTYPE(Language lang, Fund_type ft)
{
	switch (lang)
	{
		case CPLUS_ASSUMED:	return new CPP_cfront_type(ft);
		case CPLUS_ASSUMED_V2:	return new CPP_cgen_type(ft);

		default:
		case C:			return new C_type(ft);
	}
}

C_base_type *
new_CTYPE(Language lang, const Symbol &sym)
{
	switch (lang)
	{
		case CPLUS_ASSUMED:	return new CPP_cfront_type(sym);
		case CPLUS_ASSUMED_V2:	return new CPP_cgen_type(sym);

		default:
		case C:			return new C_type(sym);
	}
}

TYPE *
C_base_type::clone()
{
	if (!this)
		return 0;
	return (TYPE *)(new C_base_type(*this));
}

TYPE *
C_base_type::clone_type()
{
	return (TYPE *)(new C_base_type());
}

// this path is reached from whatis expression
int
C_base_type::print(Buffer *result, ProcObj *pobj)
{
	return cc_print_type.print_type(this, C, result, pobj);
}

int
C_base_type::size()
{
	if (isnull()) 
		return 0; // don't know.

	if (form() == TF_user) 
	{
		// if a user-type doesn't have a bytesize attribute or an
		// an element type attributte, can't determine the size

		Attribute *a = symbol.attribute(an_bytesize);

		if (a != 0 && a->form == af_int) 
		{
			return a->value.word;
		}

		C_base_type elemtype;
		symbol.type(&elemtype, an_elemtype);
		if (!elemtype.isnull()) 
		{
			// for C, need only the high bound (low bound is 0)
			int span = elemtype.size();
			a = symbol.attribute(an_hibound);
			if (a != 0 && a->form == af_int) 
			{
				return span * (a->value.word + 1);
			}
		}

		return 0;
	}

	return TYPE::size();
}

// return true if pointer
int
C_base_type::isPointer()
{
	return ((_form == TF_user && symbol.tag()==t_pointertype)
		|| (_form == TF_fund && ft == ft_pointer));
}

int
C_base_type::isIntegral()
{
	Tag	t;

	if ((_form == TF_fund) &&
		((ft >= ft_char && ft < ft_pointer) ||
		(ft == ft_string)))
	{
		return 1; // assume it is convertible
	}
	else if (_form == TF_user
		&& (((t = symbol.tag()) == t_enumtype)
			|| t==t_enumlittype || t==t_bitfield))
	{
		return 1;
	}
	return 0;
}

int
C_base_type::isFloatType()
{
	return (_form == TF_fund && (ft == ft_sfloat
					|| ft == ft_lfloat || ft == ft_xfloat));
}

// is_vtbl_member assumes that the calling function 
// checked for a class member named __vtbl
int
C_base_type::is_vtbl_member(ProcObj *pobj, Symbol &sym, VTBL_descriptor &desc)
{
	Symbol		type_symbol;
	C_base_type	*mptr_type;
	C_base_type	*member_type;

	member_type = (C_base_type *)clone_type();
	mptr_type = (C_base_type *)clone_type();
	if (!sym.type(member_type)
		|| !member_type->isPtrType()
		|| !member_type->deref_type(mptr_type)
		|| !mptr_type->user_type(type_symbol)
		|| !type_symbol.isClassType())
	{
		delete member_type;
		delete mptr_type;
		return 0;
	}

	desc.size = mptr_type->size();
	desc.type = member_type;
	delete mptr_type;

	Locdesc locdesc;
	if (sym.locdesc(locdesc)) 
	{
		Place loc;
		loc = locdesc.place(pobj, pobj->curframe());
		if (loc.isnull() || loc.kind != pAddress)
		{
			desc.null();
			return 0;
		}
		desc.offset = loc.addr;
	}
	return 1;
}

//===================================================================
// Type Building member functions
//===================================================================

// NOTE the next four routines will allocate an attribute list that 
// isn't freed (since attribute lists are never freed). 
// This will be fixed at a
// later date.   For now it shouldn't be too big problem because they 
// are small, they are only generated for cast and sizeof operators,
// and they  will be inaccessible after the containing etree node 
// is deleted.

C_base_type *
C_base_type::build_ptrToBase(Resolver *context)
{
	Protorec	protoUT;	// proto new user type
	C_base_type	ptrTYPE(ft_pointer);

	protoUT.add_attr(an_tag, af_tag, t_pointertype);

	if (_form == TF_fund)
	{
		protoUT.add_attr(an_basetype, af_fundamental_type, ft);
	}
	else if (_form == TF_user)
	{
		protoUT.add_attr(an_basetype, af_symbol, 
				(void *) symbol.attribute(an_count));
	}
	else
	{
		printe(ERR_internal, E_ERROR, "C_base_type::build_ptrProto", __LINE__);
		return 0;
	}
	protoUT.add_attr(an_bytesize, af_int, ptrTYPE.size());

	C_base_type	*newtype = (C_base_type *)clone_type();
	Symbol		sym(protoUT.put_record(), context->evaluator());
	*newtype = sym;
	return newtype;
}

C_base_type *
C_base_type::build_ptrTYPE(Resolver *context)
{
	C_base_type	*new_type = (C_base_type *)clone();

	if (isPointer())
	{
		return new_type;
	}
	else if (!isArray() || !deref_type(new_type))
	{
		delete new_type;
		return 0;
	}

	C_base_type	*ptr_type = new_type->build_ptrToBase(context);
	delete new_type;
	return ptr_type;
}

#define NUM_ATTRS	5
enum array_attrs 
{
	ar_elemtype = 0,
	ar_subtype,
	ar_lobound,
	ar_hibound,
	ar_tagtype
};

static void subscr_list(Attribute *, Attribute *, IntList *);

// build symbol for possibly multi-dimensional arrays
static void
next_item(Attribute * a, Attribute *elem, IntList *list)
{
	Attribute	attr[NUM_ATTRS];
	Protorec	prototype;
	
	if (list->next())
	{
		subscr_list( attr, elem, list );
		for(int i = 0; i < NUM_ATTRS; i++)
			prototype.add_attr( (Attr_name)attr[i].name, 
				(Attr_form)attr[i].form, attr[i].value );
		a[ar_elemtype].value.symbol = prototype.put_record();
		a[ar_elemtype].form = af_symbol;
		a[ar_elemtype].name = an_elemtype;
	}
	else
	{
		a[ar_elemtype] = *elem;
	}
}

static void
subscr_list(Attribute * a, Attribute *elem, IntList *list)
{
	int	subscript;

	subscript = list->val();

	a[ar_subtype].name = an_subscrtype;
	a[ar_subtype].form = af_fundamental_type;
	a[ar_subtype].value.fund_type = ft_int;
	a[ar_lobound].name = an_lobound;
	a[ar_lobound].form = af_int;
	a[ar_lobound].value.word = 0;
	a[ar_hibound].name = an_hibound;
	a[ar_hibound].form = af_int;
	a[ar_hibound].value.word = subscript - 1;
	a[ar_tagtype].value.tag = t_arraytype;
	a[ar_tagtype].name = an_tag;
	a[ar_tagtype].form = af_tag;

	next_item(a, elem, list);
}

C_base_type *
C_base_type::build_arrayTYPE(Resolver *context, IntList *list)
{
	Protorec	protoUT;	// proto new user type
	Attribute	elem_type;
	Attribute	attr[NUM_ATTRS];

	list->first();

	elem_type.name = an_elemtype;
	if (_form == TF_fund)
	{
		elem_type.form = af_fundamental_type;
		elem_type.value.fund_type = ft;
	}
	else if (_form == TF_user)
	{
		elem_type.form = af_symbol;
		elem_type.value.word = 
			(long)(void *)symbol.attribute(an_count);
	}
	else
	{
		printe(ERR_internal, E_ERROR, "TYPE::build_arrayTYPE",
			__LINE__);
		return 0;
	}

	subscr_list(attr, &elem_type, list);
	for(int i = 0; i < NUM_ATTRS; i++)
	{
		protoUT.add_attr( (Attr_name)attr[i].name, 
			(Attr_form)attr[i].form, attr[i].value);
	}

	C_base_type	*new_type = (C_base_type *)clone_type();
	Symbol		sym(protoUT.put_record(), context->evaluator());

	*new_type = sym;
	return new_type;
}

C_base_type *
C_base_type::build_subrtnTYPE(Resolver *context)
{
	Protorec	protoUT;	// proto new user type

	protoUT.add_attr(an_tag, af_tag, t_functiontype);
	if (_form == TF_fund)
	{
		protoUT.add_attr(an_resulttype, af_fundamental_type, ft);
	}
	else if (_form == TF_user)
	{
		protoUT.add_attr(an_resulttype, af_symbol, 
				(void *) symbol.attribute(an_count));
	}
	else
	{
		printe(ERR_internal, E_ERROR, "TYPE::build_subrtnTYPE",
			__LINE__);
		return 0;
	}
	
	C_base_type	*newtype = (C_base_type *)clone_type();
	Symbol		sym(protoUT.put_record(), context->evaluator());

	*newtype = sym;
	return newtype;
}

int
C_base_type::has_vtbl(ProcObj *, VTBL_descriptor &)
{
	return 0;
}

int
C_base_type::isDerivedFrom(C_base_type *)
{
	return 0;
}

//===================================================================
// C-specific functions
//===================================================================

// this path is reached from whatis expression
int
C_type::print(Buffer *result, ProcObj *pobj)
{
	return cc_print_type.print_type(this, C, result, pobj);
}

TYPE *
C_type::clone()
{
	if (!this)
		return 0;
	return (TYPE *)(new C_type(*this));
}

TYPE *
C_type::clone_type()
{
	return (TYPE *)(new C_type());
}

//===================================================================
// C++-specific functions
//===================================================================

TYPE *
CPP_cgen_type::clone()
{
	if (!this)
		return 0;
	return (TYPE *)(new CPP_cgen_type(*this));
}

TYPE *
CPP_cgen_type::clone_type()
{
	return (TYPE *)(new CPP_cgen_type());
}

// this path is reached from whatis expression
int
CPP_cgen_type::print(Buffer *result, ProcObj *pobj)
{
	return cc_print_type.print_type(this, CPLUS_ASSUMED_V2, result, pobj);
}

int
CPP_cgen_type::isDerivedFrom(C_base_type *cptr)
{
	CPP_cgen_type	*base = (CPP_cgen_type *)cptr;

	if (!isClass() || !base->isClass())
	{
		return 0;
	}

	// loop through children, looking for a base class

	Symbol	member;
	Symbol	class_sym;
	char	*class_name;

	member = symbol.child();
	while (!member.isnull())
	{
		if (!member.isMember())
		{
			member = member.sibling();
			continue;
		}

		// MORE - currently this handles only single inheritance
		if (is_base_class(member, class_sym, class_name))
		{
			if (strcmp(base->symbol.name(), class_name) == 0)
				return 1;
			else
				member = class_sym.child();
		}
		else
			break;
	}

	return 0;
}

// has_vtbl returns 1 if the class contains a vtbl.
// Information about the vtbl is returned in desc
int
CPP_cgen_type::has_vtbl(ProcObj *pobj, VTBL_descriptor &desc)
{
	if (_form != TF_user || symbol.isnull() || !isClass())
		return 0;

	return find_vtbl_in_base_class(pobj, symbol.child(), desc);
}

// The vtbl pointer may not be found in a scan of top-level members,
// but the class may still have a vtbl if the class has base classes,
// and the derived type does not define any new virtual functions.
// In that case, the class is using the same vtbl pointer member as
// as its base class.  This function searches first for a vtbl pointer
// in the top-level members, and then recursively through the base classes.
int
CPP_cgen_type::find_vtbl_in_base_class(ProcObj *pobj, Symbol &sym,
	VTBL_descriptor &desc)
{
	Symbol	base_sym;
	for (; !sym.isnull(); sym = sym.sibling())
	{
		if (!sym.isMember())
			continue;

		Symbol	class_sym;
		char	*class_name;
		if (base_sym.isnull() && is_base_class(sym, class_sym,  class_name))
		{
			base_sym = class_sym;
			continue;
		}
		
		char	*name = sym.name();
		if (!name || strcmp(name, VTBL_POINTER_NAME) != 0)
			continue;

		if (is_vtbl_member(pobj, sym, desc))
			return 1;
	}

	if (base_sym.isnull())
		return 0;

	if (find_vtbl_in_base_class(pobj, base_sym.child(), desc))
	{
		Locdesc locdesc;
		if (base_sym.locdesc(locdesc)) 
		{
			Place loc;
			loc = locdesc.place(pobj, pobj->curframe());
			if (loc.isnull() || loc.kind != pAddress)
			{
				desc.null();
				return 0;
			}
			desc.offset += loc.addr;
		}
		return 1;
	}
	return 0;
}

//===================================================================
// cfront-specific functions
//===================================================================

TYPE *
CPP_cfront_type::clone()
{
	if (!this)
		return 0;
	return (TYPE *)(new CPP_cfront_type(*this));
}

TYPE *
CPP_cfront_type::clone_type()
{
	return (TYPE *)(new CPP_cfront_type());
}

// this path is reached from whatis expression
int
CPP_cfront_type::print(Buffer *result, ProcObj *pobj)
{
	return cc_print_type.print_type(this, CPLUS_ASSUMED, result, pobj);
}

int
CPP_cfront_type::isDerivedFrom(C_base_type *cptr)
{
	CPP_cfront_type	*base = (CPP_cfront_type *)cptr;

	if (!isClass() || !base->isClass())
	{
		return 0;
	}

	// loop through children, looking for any member name that starts with
	// class_name::

	Buffer	*buf = buf_pool.get();
	Symbol	child;
	size_t	len;

	buf->clear();
	buf->add(base->symbol.name());
	buf->add("::");
	len = buf->size() - 1; // don't include null byte

	for (child = symbol.child(); !child.isnull(); child = child.sibling())
	{
		if (child.isMember() && strncmp(child.name(), (char *)*buf, len) == 0)
		{
			buf_pool.put(buf);
			return 1;
		}
	}

	buf_pool.put(buf);
	return 0;
}

// has_vtbl returns 1 if the class contains a vtbl.
// Information about the vtbl is returned in desc
int
CPP_cfront_type::has_vtbl(ProcObj *pobj, VTBL_descriptor &desc)
{
	if (_form != TF_user || symbol.isnull() || !isClass())
		return 0;

	Symbol	child;
	for (child = symbol.child(); !child.isnull(); child = child.sibling())
	{
		if (!child.isMember())
			continue;

		char	*name = skip_class_name(child.name());
		if (!name || strcmp(name, VTBL_POINTER_NAME) != 0)
			continue;

		if (is_vtbl_member(pobj, child, desc))
			return 1;
	}

	return 0;
}

// This makes assumptions about the ANSI C++ C-generating compiler's strategy
// for laying out class members.  It returns 1 if a class member is a struct
// with a name that looks like "__b_classname"
int
is_base_class(Symbol &sym, Symbol &class_sym, char *&class_name)
{
	CPP_cgen_type	type;
	char		*name;

	if (sym.isnull()
		|| (name = sym.name()) == 0
		|| strncmp(name, BASE_CLASS_PREFIX, sizeof(BASE_CLASS_PREFIX)-1) != 0
		|| !sym.type(&type)
		|| !type.isClass()
		|| !type.user_type(class_sym)
		|| (class_name = class_sym.name()) == 0
		|| strcmp(class_name, name + sizeof(BASE_CLASS_PREFIX)-1) != 0)
		return 0;

	return 1;
}
