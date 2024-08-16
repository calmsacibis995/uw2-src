#ident	"@(#)debugger:libutil/common/overload.C	1.6"

#include "Buffer.h"
#include "Interface.h"
#include "Language.h"
#include "ProcObj.h"
#include "Machine.h"
#include "Msgtab.h"
#include "Symbol.h"
#include "Vector.h"
#include "global.h"
#include "utility.h"
#include "str.h"
#include <string.h>
#include <signal.h>
#include <stdio.h>

static void
add_choice(int choice, const char *name, Buffer *buffer, const Symbol &sym, Vector *vector)
{
	Overload_data	data;
	char		buf[MAX_INT_DIGITS+1];

	sprintf(buf, "%d\t", choice);
	buffer->add(buf);
	buffer->add(name);
	buffer->add('\n');
	data.function = sym;
	vector->add(&data, sizeof(Overload_data));
}

// if the user specified an overloaded function without giving the prototype,
// show the user the list of choices and ask which one to use.
// if pv is non-zero, the final choice in the list will be "All of the above"
// and a pointer to a vector of selected symbols will be returned in pv.

int
resolve_overloading(ProcObj *pobj, const char *fname, Symbol &symbol, Vector **pv)
{
	if (pv)
	{
		*pv = 0;
	}

	Language lang = current_language(pobj);
	if (lang != CPLUS && lang != CPLUS_ASSUMED && lang != CPLUS_ASSUMED_V2)
	{
		return 1;
	}

	// If the name the user typed in (fname) had prototype information,
	// then we already have a symbol match and don't have to do the overload resolution.
	// Also, if the symbol name does not include the prototype, then it is a simple,
	// non-overloaded C function, so just return
	char	*name = pobj->symbol_name(symbol);
	if ((strchr(name, '(') == 0) || (strchr(fname, '(') != 0))
	{
		return 1;
	}

	// buffer is used to build up the list of choices to display
	Buffer		*buffer = buf_pool.get();
	size_t		len;
	int		nfuncs = 0;
	char		*func;
	Vector		*vector = vec_pool.get();
	int		index;

	buffer->clear();
	vector->clear();

	// if inside a member function, function may have resolved to class::function
	// in that case, should only search for other functions within the same class
	if (strstr(name, "::") == 0)
	{
		len = strlen(fname);
		func = new char[len+1];
		strcpy(func, fname);
	}
	else
	{
		char *ptr = strchr(name, '(');
		len = ptr - name;
		func = new char[len + 1];
		strncpy(func, name, len);
		func[len] = '\0';
	}
	sigrelse(SIGINT);

	// search first for file statics
	if (symbol.tag() == t_subroutine)
	{
		add_choice(++nfuncs, name, buffer, symbol, vector);

		Symbol	localsym = symbol.sibling();
		for ( ; !localsym.isnull(); localsym = localsym.sibling() )
		{
			if (prismember(&interrupt, SIGINT))
			{
				buf_pool.put(buffer);
				vec_pool.put(vector);
				sighold(SIGINT);
				return 0;
			}

			if (localsym.tag() != t_subroutine)
				continue;

			name = pobj->symbol_name(localsym);
			if (strncmp(func, name, len) != 0 || name[len] != '(')
				continue;

			add_choice(++nfuncs, name, buffer, localsym, vector);
		}
	}

	// add all the globals with the same name
	Symbol	globalsym;
	while (pobj->find_next_global(func, globalsym) && !globalsym.isnull())
	{
		if (prismember(&interrupt, SIGINT))
		{
			buf_pool.put(buffer);
			vec_pool.put(vector);
			sighold(SIGINT);
			return 0;
		}

		name = pobj->symbol_name(globalsym);
		if (strncmp(func, name, len) != 0 || name[len] != '(')
			continue;

		add_choice(++nfuncs, name, buffer, globalsym, vector);
	}

	sighold(SIGINT);
	if (nfuncs == 1)
	{
		buf_pool.put(buffer);
		vec_pool.put(vector);
		return 1;
	}

	// if pv is non-null, calling function allows for possibility of
	// multiple selection, so use message that includes "All of the above"
	// at the end
	if (pv)
	{
		index = query(QUERY_which_func_or_all, func, (char *)*buffer,
			nfuncs+1);
	}
	else
		index = query(QUERY_which_function, func, (char *)*buffer);
	buf_pool.put(buffer);

	if (index > 0 && index <= nfuncs)
	{
		// user selected one symbol
		Overload_data	data;

		symbol = data.function = ((Overload_data *)vector->ptr())[index-1].function;
		vector->clear();
		vector->add(&data, sizeof(Overload_data));
		if (pv)
			*pv = vector;
		else
			vec_pool.put(vector);
		return 1;
	}
	else if (index <= 0)
	{
		// bad number, query already complained
		vec_pool.put(vector);
		return 0;
	}
	else if (index == nfuncs+1 && pv)
	{
		// user selected "All of the above"
		// calling function releases vector
		*pv = vector;
		symbol = ((Overload_data *)vector->ptr())[0].function;
		return 1;
	}
	else
	{
		// user gave a number out of range - query can't catch this because it
		// doesn't know the upper bound
		char	buf[MAX_INT_DIGITS+1];
		sprintf(buf, "%d", index);
		printe(ERR_bad_query_answer, E_ERROR, buf);
		vec_pool.put(vector);
		return 0;
	}
}
