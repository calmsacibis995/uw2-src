/*
 * $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.	All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.	This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 *
 */

// Formatted printing of values.
// If _USL_PRINTF is defined we support the following special
// printf format characters:
// %a, %A  - NCEG hex floating-point format
// %b, %B  - integer in binary format
// %C	- wide character
// %S   - wide character string

#ident	"@(#)debugger:libexp/common/print_rval.C	1.24"

#include "Iaddr.h"
#include "CC.h"
#include "CCtype.h"
#include "Fund_type.h"
#include "Interface.h"
#include "Symbol.h"
#include "Tag.h"
#include "Attribute.h"
#include "Locdesc.h"
#include "Place.h"
#include "Buffer.h"
#include "ProcObj.h"
#include "Rvalue.h"
#include "Value.h" // for bit fields
#include "cvt_util.h" // for extended floats
#include "Machine.h" 
#include "Language.h"
#include "str.h"
#include "Expr.h"
#include "utility.h"
#include "libexp.h"
#include "CCtree.h"

#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <ctype.h>
#include <stdio.h> 

#ifdef _USL_PRINTF
#include <wchar.h>
#include <limits.h>
#endif

enum Chunk_kind {a_long, a_double, a_pointer };

static const int indent_increment = 4;

class Print_chunk
{
private:
	ProcObj		*pobj;
	void		*value;
	int		size;
	char		format;
	char		*format_str;
	Chunk_kind	chunk_kind;
	int		special_type;
	Buffer		*buf;
	
public:

		Print_chunk(ProcObj * pobj, void * value, int size, TYPE *type, 
			char format, char *format_str, Buffer *);

		void Print_next();
		int  Valid() { return(size > 0); }
};


#ifndef __cplusplus
overload is_string;
#endif

static inline int
is_string(TYPE *element_type)
{
	Fund_type element_fund_type;
	return element_type->fund_type(element_fund_type)
		&& (element_fund_type == ft_char
		|| element_fund_type == ft_schar
		|| element_fund_type == ft_uchar);
}

static int
is_string(Symbol &type_symbol)
{
	TYPE	element_type;
	if (!type_symbol.type(&element_type, an_elemtype))
	{
		printe(ERR_internal, E_ERROR, "is_string", __LINE__);
		return 0;
	}

	return is_string(&element_type);
}

static const char * escape_sequence(char c);
static char *get_program_string(ProcObj *pobj, void **address, int limit = 0);
#ifdef _USL_PRINTF
static char *get_program_wcstring(ProcObj *pobj, void **address);
#endif
static void check_for_fp_error(int sig);

class CC_print
{
	ProcObj		*pobj;
	Buffer		*buf;
	int		verbose;
	int		indent_count;
	int		brief;
	Language	lang;
	char		format;
	char		*format_str;
	char		*space_ptr;

	void	print_fund_type(void *raw_value, int size, TYPE *);
	void	print_pointer(void *raw_value, TYPE *type);
	void	print_enum(void *raw_value, int size, Symbol type_symbol);
	void	print_composite(Symbol type_symbol, void *value, int bump_indent,
				const char *member_prefix);
	void	print_array(Symbol type_symbol, void *value, int size);
	void	print_function(void *raw_value, int size);
	void	print_member(TYPE *member_type, void *value, Msg_id msg_type,
				const char *name, int index,
				const char *member_prefix);
	void	print_user_type(void *raw_value, int size, TYPE *type);
	void	print_string(unsigned char * the_string, int max_length);

public:
		CC_print(Language l,ProcObj *p, Buffer *buf, int verbose, int brief,
				char format, char *format_str);
	void	print_it(void *raw_value, int size, TYPE *type);
};

void
print_rvalue(ProcObj * pobj, void *raw_value, int size, TYPE *type, 
	char format, char *format_str, Buffer *buf, int verbose)
{
	Language lang = current_language(pobj);

	switch (lang)
	{
	case C:
	case CPLUS:
	case CPLUS_ASSUMED:
	case CPLUS_ASSUMED_V2:
		{
			CC_print cc_print(lang, pobj, buf, verbose,
				(format == DEBUGGER_BRIEF_FORMAT), format, format_str);
			cc_print.print_it(raw_value, size, type);
		}
		break;

	case UnSpec:
	default:
		printe(ERR_internal, E_ERROR, "print_rvalue", __LINE__);
		break;
	}
}
Print_chunk::Print_chunk( ProcObj * pobj, void * value, int size, TYPE *type,
	char format, char *format_str, Buffer *buf)
{
	this->pobj = pobj;
	this->value = value;
	this->size = size;
	this->format = format;
	this->format_str = format_str;
	this->buf = buf;
	special_type = 0;
	// Set chunk_kind and special_type
	switch (format)
	{
#ifdef _USL_PRINTF
	case 'b':
	case 'B':
	case 'C':
#endif
	case 'c':
	case 'd':
	case 'i':
	case 'u':
	case 'o':
	case 'p':
	case 'x':
	case 'X':
		chunk_kind = a_long;
		if (type->form() == TF_fund)
		{
			Fund_type fund_type;
			type->fund_type(fund_type);
			switch (fund_type)
			{
			case ft_char:
				if (GENERIC_CHAR_SIGNED)
					special_type = 1;
				break;
			case ft_schar:
			case ft_short:
			case ft_sshort:
			case ft_int:
			case ft_sint:
			case ft_long:
			case ft_slong:
				special_type = 1;
				break;
			default:
				break;
			}
	 	}
	 	else if (type->form() == TF_user)
	 	{
			Symbol type_symbol;
			type->user_type(type_symbol);
			if (!type_symbol.isnull() && 
				type_symbol.isEnumType())
				special_type = 1;
			break;
	 	}
	 	break;
#ifdef _USL_PRINTF
	case 'a':
	case 'A':
#endif
	case 'e':
	case 'E':
	case 'f':
	case 'g':
	case 'G':
		chunk_kind = a_double;
		int len;
		len = strlen(format_str);
		if (format_str[len-2] == 'L')
			special_type = 1;
		else if (type->form() == TF_fund)
		{
			Fund_type fund_type;
			type->fund_type(fund_type);
			if (fund_type == ft_xfloat)
				special_type = 1;
		}
		break;
	case 's':
#ifdef _USL_PRINTF
	case 'S':
#endif
		chunk_kind = a_pointer;
		// special_type is:
		// = 0 if value is dereferenced pointer (in debug's memory)
		// = 1 if value is program pointer (must be read from program's
		//		address space)
		// = 2 if value is a program pointer to a wide
		//	char string
		if (type->form() == TF_fund)
	 	{
			Fund_type fund_type;
			type->fund_type(fund_type);
			if (fund_type != ft_string)
			{
#ifdef _USL_PRINTF
				if (format == 'S')
					special_type = 2;
				else
#endif
					special_type = 1;
			}
	 	}
		else if (type->form() == TF_user)
	 	{
			Symbol type_symbol;
			type->user_type(type_symbol);
			if (type_symbol.isnull())
				break;
			Tag type_tag = type_symbol.tag();
			switch (type_tag)
			{
			case t_structuretype:
			case t_uniontype:
			case t_arraytype:
				break;
			default:
#ifdef _USL_PRINTF
				if (format == 'S')
					special_type = 2;
				else
#endif
					special_type = 1;
				break;
			}
		}
	 	break;
	default:
		{
		char	f[4];
		f[0] = '%';
		f[1] = '%';
		f[2] = format;
		f[3] = 0;
		printe(ERR_format_spec, E_ERROR, f);
		chunk_kind = a_long;
		size = 0;
		break;
		}
	}
}

static jmp_buf the_jmp_buf;

void 
Print_chunk::Print_next(void)
{
	char	lbuf[PRINT_SIZE+400]; // local buffer for sprintfs - we assume
				// cannot request a field width greater
				// than PRINT_SIZE
				// we leave room for %f style formats with
				// large mantissa and exponent

	int	bytes_used;

	switch (chunk_kind)
	{
	case a_long:
	{
		long the_long;
		if (size < sizeof(short))
		{
			if (special_type) 
				the_long = *(signed char*)value;
			else	
				the_long = *(unsigned char*)value;
			bytes_used = sizeof(char);
	 	}
		else if (size < sizeof(int))
	 	{
			if (special_type) 
				the_long = *(short*)value;
			else
				the_long = *(unsigned short*)value;
			bytes_used = sizeof(short);
		}
	 	else if (size < sizeof(long))
	 	{
			if (special_type) 
				the_long = *(int*)value;
			else	
				the_long = *(unsigned int*)value;
			bytes_used = sizeof(int);
	 	}
		else
	 	{
			if (special_type)
				the_long = *(long*)value;
			else			
				the_long = *(unsigned long*)value;
			bytes_used = sizeof(long);
		}
		sprintf(lbuf, format_str, the_long);
		buf->add(lbuf);
		break;
	}
	case a_double:
	{
		double the_double;
		if (setjmp(the_jmp_buf) != 0)
		{
			clear_fp_error_recovery();
			size = 0;
			return;
		}
		if (size < sizeof(float))
		{
			printe(ERR_float_eval, E_ERROR);
			size = 0;
			return;
		}
		init_fp_error_recovery(check_for_fp_error);
		if (size < sizeof(double))
		{
			the_double = *(float*)value;
			bytes_used = sizeof(float);
		}
		else if (!special_type)
		{
			the_double = *(double*)value;
			bytes_used = sizeof(double);
		}
	 	if (special_type)  // extended float
	 	{
			// assumes that size is correct!
#ifdef NO_LONG_DOUBLE
			// target machine printf doesn't support long double
			extended2double((void *)value, &the_double);
	 		sprintf(lbuf, format_str, the_double);
	 		buf->add(lbuf);
#else
			char	*tmpbuf;
			tmpbuf = print_extended((void*)value, format, 
				format_str, lbuf);
			if (tmpbuf)
			{
				// tmpbuf is used for %f formats with 
				// long doubles; we allow only PRINT_SIZE
				// chars; the long double may actually
				// be longer
				tmpbuf[PRINT_SIZE] = 0;
				if (strlen(tmpbuf) == PRINT_SIZE)
				{
					printe(ERR_short_read,
						E_WARNING);
				}
				buf->add(tmpbuf);
				delete tmpbuf;
			}
			else
	 			buf->add(lbuf);
#endif
			bytes_used = XFLOAT_SIZE;
		}
		else
		{
			sprintf(lbuf, format_str, the_double);
			buf->add(lbuf);
		}
	 	clear_fp_error_recovery();
		break;
	}
	case a_pointer:
	{
		void * the_pointer;
		if (special_type == 0) // value is chars to be printed
		{
			the_pointer = value;
			bytes_used = size;
		}
		else // value is debugger or program pointer
		{
			if (size < sizeof(void*))
			{
				if (size < sizeof(short))
					the_pointer = 
						(void*)(*(char*)value);
				else if (size < sizeof(int))
					the_pointer =
						(void*)(*(short*)value);
				else if (size < sizeof(long))
					the_pointer = 
						(void*)(*(int*)value);
				else
					the_pointer = *(void **)value;
				bytes_used = size;
			}
			else
			{
				the_pointer = *(void **)value;
				bytes_used = sizeof(void*);
			}
			void *addr = the_pointer;
#ifdef _USL_PRINTF
			if (special_type == 2)
				the_pointer = get_program_wcstring(pobj, &addr);
			else
#endif
				the_pointer = get_program_string(pobj, &addr);
			if (the_pointer == 0)
			{
				size = 0;
				return;
			}
		}
		if (the_pointer)
		{
			sprintf(lbuf, format_str, the_pointer);
			buf->add(lbuf);
		}
		break;
	}
	}
	value = (char*)value + bytes_used;
	size = size - bytes_used;
	if (size > 0)
		buf->add(" "); // coming back for more...
}

#define READ_SIZE	100

static char *
get_program_string( ProcObj * pobj, void **address, int limit)
{
	static char	*tbuf;
	static int	buflen;

	int		actual_limit = limit ? limit : PRINT_SIZE;
	char		*bufptr = tbuf;
	void		*addr = *address;

	if (!pobj)
	{
		printe(ERR_internal, E_ERROR, "get_program_string", __LINE__);
		return 0;
	}

	if (!tbuf)
	{
		// Allocate dynamically first time
		// We never free this space.
		buflen = READ_SIZE + 1;
		tbuf = new char[buflen];
		bufptr = tbuf;
	}

	int current_size = 0;
	while (current_size < actual_limit)
	{
		// this could be PRINT_SIZE, but we want to avoid
		// allocating a large block of memory unnecessarily
		int char_count = pobj->read((long)addr, READ_SIZE, bufptr);
		if (char_count <= 0)
		{
			// don't complain about the bad read, since this is probably
			// just an uninitialized pointer - the user will just see
			// the hex address printed instead of the string
			*address = addr;
			return 0;
		}
		else
		{
			bufptr[char_count] = 0; // adds a null character
		}
	
		int temp_len = strlen(bufptr);
		current_size += temp_len;
		if (temp_len != char_count) 
			return tbuf; // read a null

		addr = (char*)addr + char_count;
		if (current_size + 1 == buflen)
		{
			buflen += READ_SIZE;
			if ((tbuf = (char *)realloc(tbuf, buflen)) == 0)
				new_handler();
			bufptr = tbuf + current_size;
		}
		else
			bufptr += char_count;
	}
	printe(ERR_short_read, E_WARNING);
	return tbuf;
}

#ifdef _USL_PRINTF
// Get a wide character string.
// We want to print out no more than PRINT_SIZE characters

static char *
get_program_wcstring( ProcObj * pobj, void **address)
{
	static wchar_t	*tbuf;
	static int	buflen;
	wchar_t		*bufptr;
	void		*addr = *address;

	if (!pobj)
	{
		printe(ERR_internal, E_ERROR, "get_program_string", __LINE__);
		return 0;
	}
	if (!tbuf)
	{
		// Allocate dynamically first time
		// We never free this space.
		buflen = READ_SIZE + 1;
		tbuf = new wchar_t[buflen];
	}

	bufptr = tbuf;
	int current_size = 0;
	while (current_size < PRINT_SIZE)
	{
		// this could be PRINT_SIZE, but we want to avoid
		// allocating a large block of memory unnecessarily
		int wchar_count;
		int char_count = pobj->read((long)addr, READ_SIZE * sizeof(wchar_t),
			(char *)bufptr);

		if (char_count <= 0)
		{
			// don't complain about the bad read, since this is probably
			// just an uninitialized pointer - the user will just see
			// the hex address printed instead of the string
			*address = addr;
			return 0;
		}
		wchar_count = char_count / sizeof(wchar_t);
		// round back to wchar_t boundary
		char_count = wchar_count * sizeof(wchar_t);
		bufptr[wchar_count] = 0; // adds a null character
	
		int temp_len = wcslen(bufptr);
		current_size += temp_len;
		if (temp_len != wchar_count) 
			return (char *)tbuf; // read a null

		addr = (char*)addr + char_count;
		if (current_size + 1 == buflen)
		{
			buflen += READ_SIZE;
			tbuf = (wchar_t *) realloc(tbuf, buflen * sizeof(wchar_t));
			if (!tbuf)
				new_handler();
			bufptr = tbuf + current_size;
		}
		else
			bufptr += wchar_count;
	}
	printe(ERR_short_read, E_WARNING);
	return (char *)tbuf;
}
#endif


#ifdef __cplusplus
extern void add_error_handler(void(*)(int));
#else
extern void add_error_handler(SIG_HANDLER);
#endif

static void
check_for_fp_error(int sig)
{
	if (sig == SIGFPE)
	{
		printe(ERR_float_eval, E_ERROR);
		clear_fp_error();
		sigrelse(sig);
		longjmp(the_jmp_buf, 1);
	}
}

CC_print::CC_print(Language l, ProcObj *p, Buffer *bptr, int v, int b,
	char fmt, char *fmt_str)
{
	pobj = p;
	verbose = v;
	lang = l;
	brief = b;
	buf = bptr;
	format = fmt;
	format_str = fmt_str;
	indent_count = 0;
	space_ptr = 0;
}

void
CC_print::print_it(void *raw_value, int size, TYPE *type)
{
	if (format && format != DEBUGGER_FORMAT && !brief)
	{
		Print_chunk chunk(pobj, raw_value, size, type, format, 
			format_str, buf);
		while (chunk.Valid()) 
			chunk.Print_next();
		return;
	}

	if (!type || type->isnull())
	{
		Print_chunk chunk(pobj, raw_value, size, 0, 'X', "%X", buf);
		while (chunk.Valid()) 
			chunk.Print_next();
	}
	if (type->form() == TF_fund)
	{
		print_fund_type(raw_value, size, type);
	}
	else if (type->form() == TF_user)
	{
		print_user_type(raw_value, size, type);
	}
	else
	{
		printe(ERR_internal, E_ERROR, "CC_print::print_it", __LINE__);
	}
}


void
CC_print::print_fund_type(void *raw_value, int size, TYPE *type)
{
	Fund_type fund_type;

	type->fund_type(fund_type);
	switch (fund_type)
	{
	case ft_char:
	case ft_schar:
	case ft_uchar:
	{
		char lbuf[MAX_LONG_DIGITS + sizeof(" (or '')") + 6];
		int	c;
		char	*p = lbuf;

#if GENERIC_CHAR_SIGNED
		if (fund_type == ft_uchar)
#else
		if (fund_type == ft_uchar || fund_type == ft_char)
#endif
			c = *(unsigned char*)raw_value;
		else
			c = *(char*)raw_value;
		p += sprintf(p, "%d", c);
		if (!brief)
		{
			if (isprint(c))
				sprintf(p, " (or '%c')", c);
			else if (c > 6)
			// 0-6 have no real escape sequence
				sprintf(p, " (or '%s')",
					escape_sequence(c));
		}
		buf->add(lbuf);
		break;
	}
	case ft_short:
	case ft_sshort:
	case ft_int:
	case ft_sint:
	case ft_long:
	case ft_slong:
		{
			Print_chunk chunk(pobj, raw_value, size, type,
				'd', "%d", buf);
			while (chunk.Valid()) 
				chunk.Print_next();
		}
		break;

	case ft_ushort:
	case ft_uint:
	case ft_ulong:
		{
			Print_chunk chunk(pobj, raw_value, size, type, 
				'u', "%u", buf);
			while (chunk.Valid()) 
				chunk.Print_next();
		}
		break;

	case ft_pointer:
	case ft_void:
		{
			Print_chunk chunk(pobj, raw_value, size, type, 
				'x',"0x%x", buf);
			while (chunk.Valid()) 
				chunk.Print_next();
		}
		break;

	case ft_sfloat:
	case ft_lfloat:
		{
			Print_chunk chunk(pobj, raw_value, size, type, 
				'g', "%g", buf);
			while (chunk.Valid())
				chunk.Print_next();
		}
		break;

	case ft_xfloat:
		{
			Print_chunk chunk(pobj, raw_value, size, type, 
				'g', "%Lg", buf);
			while (chunk.Valid()) 
				chunk.Print_next();
		}
		break;
	case ft_string:
		{
			buf->add('"'); 
			Print_chunk chunk(pobj, raw_value, size, type, 
				's', "%s", buf);
			while (chunk.Valid()) 
				chunk.Print_next();
			buf->add('"');
		}
		break;

	default:
		printe(ERR_internal, E_ERROR, "print_fund_type", 
			__LINE__);
		return;
	}
}


void
CC_print::print_enum(void *raw_value, int size, Symbol type_symbol)
{
	// Output format:
	// 	literal	brief	output
	//	yes	yes	<literal>
	//	yes	no	<literal> (or <value>)
	//	no	yes	<value>
	//	no	no	<value>
	//

	long value;
	if (size == sizeof(char)) 
		value = *(char*)raw_value;
	else if (size == sizeof(short))
		value = *(short*)raw_value;
	else if (size == sizeof(int))
		value = *(int*)raw_value;
	else if (size == sizeof(long))
		value = *(long*)raw_value;
	else 
	{
		printe(ERR_internal, E_ERROR, "print_enum", __LINE__);
		return;
	}

	int found_literal = 0;
	if (!type_symbol.isnull())
	{
		Symbol literal_list = type_symbol.child();
		while (!literal_list.isnull())
		{
			Attribute *literal = literal_list.attribute(an_litvalue);
			if (!literal || literal->form != af_int)
			{
				printe(ERR_internal, E_ERROR,
					"print_enum", __LINE__);
				return;
			}
			if (literal->value.word == value)
			{
				found_literal = 1;
				buf->add(literal_list.name());
				if (!brief)
					buf->add(" (or ");
				break;
			}
			literal_list = literal_list.sibling();
		}
	}
	// Print value
	if (!brief || !found_literal)
	{
		char	lbuf[MAX_INT_DIGITS+1];

		sprintf(lbuf, "%d", value);
		buf->add(lbuf);
		if (found_literal)
			buf->add(')');
	}
}

void
CC_print::print_pointer(void *raw_value, TYPE *type)
{
	char	lbuf[MAX_LONG_DIGITS+3];
	TYPE	referenced_type;

	if (type->deref_type(&referenced_type) && is_string(&referenced_type))
	{
		if (*(void **)raw_value == NULL)
		{
			buf->add("NULL");
			return;
		}
		else
		{
			void *addr = *(void **)raw_value;
			char *str = get_program_string(pobj, &addr, 100);
			if (str)
			{
				print_string((unsigned char *)str, 100);
				return;
			}
		}
	}
	sprintf(lbuf, "0x%x", *(void **)raw_value);
	buf->add(lbuf);
}

void
CC_print::print_function(void *raw_value, int size)
{
	char	lbuf[MAX_LONG_DIGITS+3];

	if (size != sizeof (void*))
	{
		printe(ERR_internal, E_ERROR, "print_function", __LINE__);
		return;
	}
	sprintf(lbuf, "0x%x", *(void **)raw_value);
	buf->add(lbuf);
}

void
CC_print::print_composite(Symbol type_symbol, void *value, int bump_indent,
	const char *member_prefix)
{
	char	class_name[BUFSIZ];
	int	len;

	if (bump_indent)
		indent_count += indent_increment;
	if (!brief)
	{
		space_ptr = spaces.get_spaces(indent_count);
		if (lang == CPLUS_ASSUMED || lang == CPLUS_ASSUMED_V2)
		{
			sprintf(class_name, "%.*s::", BUFSIZ-3, type_symbol.name());
			len = strlen(class_name);
		}
	}

	C_base_type *member_type = new_CTYPE(lang);
	Symbol member_symbol = type_symbol.child();
	Symbol class_sym;
	char   *base_class_name;
	while (!member_symbol.isnull())
	{
		char *name;
		if (!member_symbol.isMember())
		{
			member_symbol = member_symbol.sibling();
			continue;
		}
		if (!member_symbol.type(member_type)) 
		{
			printe(ERR_internal, E_ERROR, "CC_print_composite", __LINE__);
			break;
		}
		Attribute *member_location = member_symbol.attribute(an_location);
		if (!member_location || member_location->form != af_locdesc)
		{
			printe(ERR_internal, E_ERROR, "CC_print_composite", __LINE__);
			break;
		}
		if (!brief)
		{
			name = member_symbol.name();
			if (lang != C)
			{
				char *ptr = skip_class_name(name);
				if (!ptr || (!verbose && strcmp(ptr, VTBL_POINTER_NAME) == 0))
				{
					// by default, don't show vtbl pointers
					member_symbol = member_symbol.sibling();
					continue;
				}
				if (lang == CPLUS_ASSUMED)
				{
					if (strncmp(name, class_name, len) == 0)
						name = ptr;
				}
			}
		}

		Locdesc member_locdesc;
		member_locdesc = member_location->value.loc;
		Place member_place = member_locdesc.place(0, 0, Iaddr(value));
		if (member_symbol.tag() == t_bitfield)
		{
			char	bitbuf[sizeof(long)];
			int	size = member_type->size();

			Obj_info bit_field_obj(pobj, 0, 
				member_type->clone(), member_symbol, member_place);
			memcpy(bitbuf, (void*)member_place.addr, size);
			bit_field_obj.extractBitFldValue(bitbuf, size);
			print_it(bitbuf, size, member_type);
			if (!brief)
			{
				if (lang == CPLUS_ASSUMED)
					printm(MSG_print_member1, space_ptr,
						"", name, (char *)*buf);
				else
					printm(MSG_print_member1, space_ptr,
						member_prefix, name, (char *)*buf);
				buf->clear();
			}
		}
		else
		{
			if (lang == CPLUS_ASSUMED_V2
				&& is_base_class(member_symbol, class_sym, base_class_name))
			{
				char *prefix = 0;
				if (!brief)
				{
					prefix = makesf("%s%s::", member_prefix,
						base_class_name);
				}
				print_composite(class_sym, (void *)member_place.addr, 0, prefix);
				if (!brief)
					delete prefix;
			}
			else
			{
				print_member(member_type, (void *)member_place.addr, 
					MSG_print_member1, name, 0, member_prefix);
			}
		}

		member_symbol = member_symbol.sibling();
		if (!member_symbol.isnull() && brief) 
			buf->add(", ");
	}
	delete member_type;

	// Look through global symbols to find static class members
	// These are printed only in full (non-brief) mode, as they are
	// a special case, and need an indication that they are static
	// Do this search only once - at the top level
	if (!brief && indent_count == indent_increment
		&& (lang == CPLUS_ASSUMED || lang == CPLUS_ASSUMED_V2))
	{
		member_symbol.null();
		while (pobj->find_next_global(class_name, member_symbol))
		{
			if (!member_symbol.isVariable())
				continue;

			char *name = member_symbol.name();
			name += len; // skip "class_name::"

			// don't print actual vtbl as part of class
			if (strcmp(name, VTBL_NAME) == 0)
				continue;

			Expr	expr(member_symbol.name(), pobj);
			Rvalue	rvalue;

			if (!expr.eval(pobj) || !expr.rvalue(rvalue)
				|| rvalue.isnull())
			{
				printe(ERR_internal, E_ERROR, "CC_print_composite", __LINE__);
				break;
			}

			print_member(rvalue.type(), rvalue.dataPtr(),
				MSG_print_member2, name, 0, "");
		}
	}

	if (bump_indent)
		indent_count -= indent_increment;
	if (!brief)
	{
		// number of spaces must stay in sync with indent_count
		space_ptr = spaces.get_spaces(indent_count);
	}
}

void
CC_print::print_array(Symbol type_symbol, void *value, int size)
{
	C_base_type *element_type = new_CTYPE(lang);
	if (!type_symbol.type(element_type, an_elemtype))
	{
		printe(ERR_internal, E_ERROR, "CC_print_array", __LINE__);
		delete element_type;
		return;
	}

	// Determine length of array
	Attribute *high_bound_attr = type_symbol.attribute(an_hibound);
	if (!high_bound_attr || high_bound_attr-> form != af_int) 
	{
		printe(ERR_internal, E_ERROR, "CC_print_array", __LINE__);
		delete element_type;
		return;
	}

	int high_bound = (int)high_bound_attr->value.word + 1;

	if (is_string(element_type))
	{
		// declarations of the form char[1] are sometimes used to
		// indicate the beginning of an array of unknown length
		print_string((unsigned char*)value, high_bound == 1 ? 100 : high_bound);
		delete element_type;
		return;
	}

	int	element_size = size/high_bound;

	indent_count += indent_increment;
	if (!brief)
	{
		space_ptr = spaces.get_spaces(indent_count);
	}

	// Print the elements.
	for (int element_index = 0; element_index < high_bound;
		element_index++)
	{
		print_member(element_type, value, MSG_array_member, 0, element_index, "");
		value = (char*)value + element_size;
		if (element_index < high_bound - 1 && brief) 
			buf->add(", ");
	}

	indent_count -= indent_increment;
	if (!brief)
	{
		space_ptr = spaces.get_spaces(indent_count);
	}
	delete element_type;
}

void
CC_print::print_string(unsigned char * the_string, int max_length)
{
	buf->add('"');
	unsigned char * first_char_to_add = the_string;
	while(*the_string && max_length)
	{
		if (!isprint(*the_string))
		{
			unsigned char non_printable = *the_string;
			if (the_string != first_char_to_add)
			{
				// Handle characters up to first 
				// character to translate.
				*the_string = 0;
				buf->add((char *)first_char_to_add);
				*the_string = non_printable;
			}
			// Translate character.
			buf->add(escape_sequence(non_printable));
			first_char_to_add = the_string + 1;
		}
		// Next character.
		the_string++, max_length--;
	}
	if (the_string != first_char_to_add)
	{
		unsigned char last_char = *the_string;
		*the_string = 0;
		buf->add((char *)first_char_to_add);
		*the_string = last_char;
	}
	if (!max_length && *the_string) 
		buf->add("...");
	buf->add('"');
}

static const char *
escape_sequence(char c)
{
	switch (c)
	{
	case '\a': return "\\a";
	case '\b': return "\\b";
	case '\f': return "\\f";
	case '\n': return "\\n";
	case '\r': return "\\r";
	case '\t': return "\\t";
	case '\v': return "\\v";
	default:
		{
		static char octal_form[6];
		sprintf(octal_form, "\\%03o", (unsigned char)c);
		return octal_form;
		}
	}
}

void
CC_print::print_member(TYPE *member_type, void *value,
	Msg_id msg_type, const char *name, int index, const char *member_prefix)
{
	Symbol	type_symbol;
	Tag	tag;

	if (member_type->user_type(type_symbol)
		&& (((tag = type_symbol.tag()) == t_structuretype)
			|| tag == t_uniontype
			|| (tag == t_arraytype && !is_string(type_symbol))))
	{
		if (brief)
		{
			if (msg_type == MSG_array_member)
			{
				buf->add("{");
				print_it(value, member_type->size(), member_type);
				buf->add("}");
			}
			else
			{
				buf->add("{*}");
			}
			return;
		}

		if (msg_type == MSG_array_member)
			printm(msg_type, space_ptr, index, "{");
		else if (msg_type == MSG_print_member2)
			printm(msg_type, space_ptr, name, "{");
		else if (lang == CPLUS_ASSUMED)
			printm(msg_type, space_ptr, "", name, "{");
		else
			printm(msg_type, space_ptr, member_prefix, name, "{");
		if (get_ui_type() == ui_gui)
		{
			if (tag == t_arraytype)
				printm(MSG_start_array);
			else
				printm(MSG_start_class);
		}
		print_it(value, member_type->size(), member_type);
		printm(MSG_val_close_brack, space_ptr, "\n");
	}
	else
	{
		print_it(value, member_type->size(), member_type);
		if (!brief)
		{
			if (msg_type == MSG_array_member)
				printm(msg_type, space_ptr, index, (char *)*buf);
			else if (msg_type == MSG_print_member2)
				printm(msg_type, space_ptr, name, (char *)*buf);
			else if (lang == CPLUS_ASSUMED)
				printm(msg_type, space_ptr, "", name, (char *)*buf);
			else
				printm(msg_type, space_ptr, member_prefix,
					name, (char *)*buf);
			buf->clear();
		}
	}
}

void
CC_print::print_user_type(void *raw_value, int size, TYPE *type)
{
	Symbol type_symbol;
	Tag    type_tag;

	if (!type->user_type(type_symbol)
		|| type_symbol.isnull()
		|| ((type_tag = type_symbol.tag()) == t_none))
	{
		printe(ERR_internal, E_ERROR, "CC_print_user_type", __LINE__);
		return;
	}

	switch (type_tag)
	{
	case t_enumlittype:
		type_symbol.null();  // and go on to ..
	case t_enumtype:
		print_enum(raw_value, size, type_symbol);
		break;

	case t_pointertype:
		print_pointer(raw_value, type);
		break;

	case t_structuretype:
	case t_uniontype:
		if (!indent_count)
		{
			if (brief)
				buf->add("{");
			else
			{
				buf->add("{\n");
				printm(MSG_print_val, (char *)*buf);
				buf->clear();
				if (get_ui_type() == ui_gui)
					printm(MSG_start_class);
			}
		}
		print_composite(type_symbol, raw_value, 1, "");
		if (!indent_count)
		{
			if (brief)
				buf->add('}');
			else
				printm(MSG_val_close_brack, "", "");
		}
		break;

	case t_arraytype:
		if (indent_count || is_string(type_symbol))
		{
			print_array(type_symbol, raw_value, size);
		}
		else
		{
			if (brief)
				buf->add("{");
			else
			{
				buf->add("{\n");
				printm(MSG_print_val, (char *)*buf);
				buf->clear();
				if (get_ui_type() == ui_gui)
					printm(MSG_start_array);
			}
			print_array(type_symbol, raw_value, size);
			if (brief)
				buf->add('}');
			else
				printm(MSG_val_close_brack, "", "");
		}
		break;

	case t_functiontype:
		print_function(raw_value, size);
		break;

	default:
		printe(ERR_internal, E_ERROR, "CC_print_user_type", __LINE__);
		break;
	}
}
