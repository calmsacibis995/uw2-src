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
#ident	"@(#)debugger:libexp/common/Resolver.C	1.26"

#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include "Resolver.h"
#include "global.h"
#include "Proctypes.h"
#include "CC.h"
#include "CCtype.h"
#include "CCtree.h"
#include "Interface.h"
#include "Symtab.h"
#include "ProcObj.h"
#include "Tag.h"
#include "Fund_type.h"
#include "Link.h"
#include "str.h"
#include "Source.h"


Resolver::Resolver(ProcObj *proc, Iaddr pc)
{
	Symtab *symtab;

	mode    = mFULL_SEARCH;
	pobj = proc;
	symtab = 0;

	if (pobj != 0 && (symtab = pobj->find_symtab(pc)) != 0)
	{
		curScope = symtab->find_scope(pc);
		if (curScope.isnull())
			top_scope = symtab->find_symbol(pc);
	}
}

Resolver::Resolver(Symbol& scope, ProcObj *proc)
{
	// resolver for members of aggregates 
	// e.g.	(C/C++: struct/union/class, ...).

	mode = mLOCAL_ONLY;  // don't climb scope chain.
	pobj   = proc;
	curScope = top_scope = scope;
	symtab = 0;
}

Resolver::Resolver(ProcObj *proc, Symtab *s, Symbol &scope)
{
	// resolver for globals and file statics defined in
	// possibly non-current shared objects

	mode = mSEARCH_OBJ;  // don't climb scope chain
	symtab = s;
	curScope = top_scope = scope;
	pobj = proc;
}

int
Resolver::getEnclosingSrc(Symbol &src)
{
	src.null();

	if( !curScope.isnull() )
	{
		src = curScope;
	}
	else if( !top_scope.isnull() )
	{
		src = top_scope;
	}
	else
	{
		return 0;
	}

	for(; !src.isnull() && !src.isSourceFile();
			   src = src.parent())
		;

	return !src.isnull();
}

int
Resolver::getEnclosingFunc(Symbol& func)
{
	Symbol s = curScope;
	while( !s.isnull() && !s.isSubrtn())
	{
		s = s.parent();
	}

	if( !s.isnull() )
	{
		func = s;
		return 1;
	}
	return 0;
}

int
Resolver::find_enumlit(Symbol& enumtype, const char * id, Symbol& litsym)
{
	Symbol s;

	litsym.null();

	for (s = enumtype.child(); ! s.isnull(); s = s.sibling())
	{
		if (!s.isEnumLitType())
		{
			break;
		}
		Attribute *a = s.attribute(an_litvalue);
	
		if (a != 0 && a->form == af_int)
		{
			char *nname = s.name();
	
			if (nname != 0 && strcmp(nname, id) == 0)
			{
				litsym = s;
				return 1;
			}
		}
	}
	return 0;
}

int
Resolver::cmpName(const char *search_id, const char *sym_name)
{
	if (!search_id || !sym_name)
		return 0;

	return (strcmp(search_id, sym_name) == 0 );
}

// base class version does not try to find another symbol with
// same name
int	
Resolver::getNext(const char *id, Symbol& sym)
{
	if (sym.isnull())
		return lookup(id, sym);

	sym.null();
	return 0;
}

int
Resolver::searchSiblingChain(const char* id, const Symbol& startSym, 
	Symtype user_def_type)
{
	if( startSym.isnull() )
	{
		curSymbol.null();
		return 0;
	}

	Symbol s;
	for( s = startSym; ! s.isnull(); s = s.sibling() )
	{
		if (prismember(&interrupt, SIGINT))
			break;

		if (s.tag() == t_none)
			continue;

		if (cmpName(id, s.name()))
		{ 
			// distinguish tag names and typedef names.
			//  .. typedef enum e { a, b, c } E; int e;  is legal C.
			//     but NOT legal C++ - but we shouldn't see conflicts
			// in C++
			if (checkTag(user_def_type, s))
			{
				curSymbol = s;
				return 1;
			}
		}
		
		if( s.isEnumType() )
		{
			if (find_enumlit(s, id, curSymbol))
				return 1;
		}
	}
	curSymbol.null();
	return 0;
}

//
// Search from curScope outward to and including the top scope
//
int
Resolver::searchOutwardToTopScope(const char *id, Symbol& bottom, 
	Symbol &top, Symtype user_def_type)
{
	Tag tag;
	Symbol s = bottom;

	for( ;!s.isnull(); s=s.parent())
	{
		if (prismember(&interrupt, SIGINT))
			break;
		tag = s.tag();
		switch (tag)
		{
		case t_subroutine:
		case t_global_sub:
		case t_block:
		case t_entry:
		case t_sourcefile:
		case t_extlabel:
		case t_structuretype:
		case t_uniontype:
		case t_label:
			if (searchSiblingChain(id, s.child(), user_def_type))
			{
				return 1;
			}
			break;
		default:
			printe(ERR_unexpected_tag, E_ERROR, tag);
			continue;
		}
		if( s == top )
		{
			break;
		}
	} 
	return 0;
}

int
Resolver::find_static(const char *id, Symtype user_def_type)
{
	Symbol curSource;

	getEnclosingSrc(curSource);
	searchSiblingChain(id, curSource.child(), user_def_type);

	while( !curSymbol.isnull() )
	{
		// static and global symbols are intermixed in a file symbol's
		// children.  Ignore global symbols for now
		if( !curSymbol.isGlobalVar() && !curSymbol.isGlobalSub() )
		{
			// found one that is not a global
			if (checkTag(user_def_type, curSymbol))
				return 1;
		}
		searchSiblingChain(id, curSymbol.sibling(), user_def_type);
	}

	curSymbol.null();
	return 0;
}

int
Resolver::find_global(const char *id)
{
	if (mode == mSEARCH_OBJ)
		curSymbol = symtab->find_global(id);
	else
		curSymbol = pobj->find_global(id);
	return (!curSymbol.isnull());
}

int
Resolver::lookup(const char *id, Symbol& result, Symtype user_def_type)
{
	if ( !pobj )
	{
		return 0;
	}

	sigrelse(SIGINT);
	curSymbol.null();
	result.null();

	switch( mode )
	{
	case mFULL_SEARCH:
		// search for automatics
		getEnclosingFunc(top_scope);
		if( searchOutwardToTopScope(id, curScope, top_scope, 
			user_def_type) )
		{
			result = curSymbol;
			break;
		}
		// FALL-THROUGH

	case mNON_LOCAL:
	case mSEARCH_OBJ:
		// search static, search global
		if( find_static(id, user_def_type) || find_global(id) )
		{
			result = curSymbol;
		}
		break;

	case mLOCAL_ONLY :
		// look for a match only within the limited scope
		// for example, given struct s obj; print obj.mem;
		// look for mem only within the structure declaration of s 
		if (searchSiblingChain(id, top_scope.child(), user_def_type))
		{
			result = curSymbol;
		}
		break;

	default:
		break;
	}

	sighold(SIGINT);
	return !result.isnull();
}

int
Resolver::checkTag(Symtype user_def_type, Symbol &s)
{
	switch(user_def_type)
	{
	case st_notags:	// match anything but a tag name (struct s, union u, etc.)
		if (!s.isUserTagName())
		{
			return 1;
		}
		break;
	case st_tagnames:	// match tag names only
		if (s.isUserTagName())
		{
			return 1;
		}
		break;
	case st_usertypes:	// match typedef or tag names
		if (s.isUserTypeSym())
		{
			return 1;
		}
		break;
	case st_func:	// match functions only, no variables or type names
		if (s.isEntry() || s.isLabel() || s.isBlock())
		{
			return 1;
		}
		break;
	case st_object:	// match variables only, no functions or type names
		if (s.isVariable())
		{
			return 1;
		}
		break;
	case st_any:	// anything is ok
		return 1;
	}
	return 0;
}

Evaluator *
Resolver::evaluator()
{
	if (!curScope.isnull())
	{
		return curScope.get_evaluator();
	}
	else if (!top_scope.isnull())
	{
		return top_scope.get_evaluator();
	}
	else if (!curSymbol.isnull())
	{
		return curSymbol.get_evaluator();
	}
	return 0;
}

// C++ specific functions

class ClassTree : public Link
{
	char		*class_name;
	Symbol		first_member;
	Symbol		current;
public:
			ClassTree(char *name) { class_name = name; }
			ClassTree(char *name, Symbol &s)
				{ class_name = name; first_member = s; }

			// destructor recursively deletes the whole list,
			// should be called on the top-level object only
			~ClassTree();

			// access functions
	const char	*get_name() { return class_name; }
	ClassTree	*get_base() { return (ClassTree *)Link::next(); }
	ClassTree	*get_derived() { return (ClassTree *)Link::prev(); }
	void		set_first_member(Symbol &s) { first_member = s; }

	int		get_first(Symbol &s);
	int		get_next(Symbol &s, Language);
	ClassTree	*clone();
};

ClassTree::~ClassTree()
{
	delete get_base();
	delete class_name;
}

int
ClassTree::get_first(Symbol &s)
{
	s = current = first_member;
	return 1;
}

int
ClassTree::get_next(Symbol &s, Language lang)
{
	if (current.isnull())
		return 0;

	current = current.sibling();
	while (!current.isnull() && !current.isMember())
		current = current.sibling();

	if (current.isnull())
		return 0;

	// else must be a member
	if (lang == CPLUS_ASSUMED)
	{
		// This makes assumptions about the layout of classes generated by cfront.
		// Base and derived class members are all laid out in the same structure
		// with names that look like "Base::mem" or "Derived::mem"
		char	*name = current.name();
		if (!name)
			return 0;

		size_t	len = strlen(class_name);
		if (name[len] != ':' || name[len+1] != ':'
			|| strncmp(class_name, name, len) != 0)
			return 0;
	}
	s = current;
	return 1;
}

ClassTree *
ClassTree::clone()
{
	char		*newname = makestr(class_name);
	ClassTree	*newtree = new ClassTree(newname);

	newtree->set_first_member(first_member);
	if (get_base())
		newtree->append(get_base()->clone());
	return newtree;
}

CCresolver::~CCresolver()
{
	delete class_tree;
	delete member_id;
}

int
CCresolver::searchClass(const char *id, Symtype user_def_type)
{
	// Note that this assumes that top_scope is a class
	if (!class_tree)
		class_tree = buildClassTree(top_scope);
	return searchClass(class_tree, id, user_def_type);
}

int
CCresolver::searchClass(Symbol &classSym, const char *id, Symtype user_def_type)
{
	ClassTree	*tree = buildClassTree(classSym);
	int		ret = searchClass(tree, id, user_def_type);

	delete tree;
	return ret;
}

int
CCresolver::searchClass(ClassTree *tree, const char *id, Symtype user_def_type)
{
	int	is_qualified = 0;
	char	*end = 0;
	size_t	len = 0;

	if ((end = skip_class_name((char *)id)) != 0 && end != id)
	{
		is_qualified = 1;
		len = end - (id + 2);
	}

	// search each base class
	for (ClassTree *cur = tree; cur; cur = cur->get_base())
	{
		if (is_qualified && strncmp(id, cur->get_name(), len) != 0)
			continue;

		// search first for data members
		for (int ret = cur->get_first(curSymbol); ret;
			ret = cur->get_next(curSymbol, lang))
		{
			if (prismember(&interrupt, SIGINT))
				return 0;

			if (cmpName(id, curSymbol.name()))
				return 1;
		}

		// if name not qualified, build qualified
		// name and continue search in static/global symbols
		// for functions and static data members
		if (user_def_type != st_tagnames && user_def_type != st_usertypes
			&& !is_qualified)
		{
			char	*localId = makesf("%s::%s", cur->get_name(), id);
			int	found = 0;

			if (find_static(localId, user_def_type))
				found = 1;
			else if (pobj->find_next_global(localId, curSymbol))
			{
				found = 1;
				inGlobalSearch = 1;
			}
			if (found)
			{
				member_id = localId;
				return 1;
			}
			else
				delete localId;
		}
	}
	return 0;
}

int
CCresolver::cmpName(const char *id, const char *s_name)
{
	if (!id || !s_name)
		return 0;

	if (strcmp(id, s_name) == 0)
	{
		return 1;
	}

	char* parenPos;
	if ((parenPos=strchr(s_name, '(')) && !strchr(id, '('))
	{
		// symbol is prototyped function name and the id is not,
		// compare up to the first left paren (i.e., ignore prototype)
		int len = parenPos-s_name;
		if (len==strlen(id)&&strncmp(s_name, id, len)==0)
		{
			return 1;
		}
	}
	return 0;
}

int
CCresolver::inMemberFunction(Symbol& thisSym, Symbol& classSym)
{
	// is there a "this" argument
	Symbol curFunc;
	if( !getEnclosingFunc(curFunc) )
	{
		return 0;
	}

	if (!searchSiblingChain(THIS_NM, curFunc.child(), st_notags))
	{
		return 0;
	}

	// result of searchSiblingChain returned in curSymbol member
	thisSym = curSymbol;

	if( thisSym.tag() != t_argument )
	{
		return 0;
	}

	// does "this" argument have the correct type
	C_base_type *thisType = new_CTYPE(lang);
	Symbol thisUT;
	if (!thisSym.type(thisType) || !thisType->isPointer()
		|| !thisType->user_type(thisUT))
	{
		delete thisType;
		return 0;
	}
	delete thisType;

	Symbol classStruct = thisUT.arc(an_basetype);
	if( classStruct.tag() != t_structuretype )
	{
		return 0;
	}

	classSym = classStruct;
	return 1;
}

// find symbol with given name.  The type given by user_def_type
// may limit the search to functions, variables, tag names, type names,
// or tag and type names
// MORE - CCresolver needs more work to handle anonymous unions, nested classes,
// multiple inheritance, etc.
int
CCresolver::lookup(const char *id, Symbol& result, Symtype user_def_type)
{
	if ( !pobj )
	{
		return 0;
	}

	sigrelse(SIGINT);
	inGlobalSearch = 0;
	delete member_id;
	member_id = 0;
	curSymbol.null();
	result.null();

	switch( mode )
	{
	case mFULL_SEARCH:
	{
		// search for automatics
		getEnclosingFunc(top_scope);
		if (searchOutwardToTopScope(id, curScope, top_scope, 
			user_def_type))
		{
			result = curSymbol;
			break;
		}
		
		Symbol classSym;
		Symbol thisSym;
		if (inMemberFunction(thisSym, classSym))
		{
			if (searchClass(classSym, id, user_def_type))
			{
				result = curSymbol;
				break;
			}
		}
	}
	// FALL-THROUGH

	case mNON_LOCAL:
	case mSEARCH_OBJ:
		// search static, search global
		if (find_static(id, user_def_type) || find_global(id))
		{
			result = curSymbol;
			break;
		}
		break;

	case mLOCAL_ONLY :
	{
		if (top_scope.isClassType())
		{
			if (searchClass(id, user_def_type))
			{
				result = curSymbol;
				break;
			}
		}
		else if (searchSiblingChain(id, top_scope.child(), user_def_type))
		{
			result = curSymbol;
			break;
		}
		

		break;
	}
	default:
		break;
	}

	sighold(SIGINT);
	return !result.isnull();
}

// used to find multiple instances of overloaded function names
int
CCresolver::getNext(const char *id, Symbol& result)
{
	if (result.isnull())
	{
		// first time through
		if (!pobj)
			return 0;
		sigrelse(SIGINT);
		inGlobalSearch = 0;
		delete member_id;
		member_id = 0;
		curSymbol.null();

		Symbol	classSym;
		Symbol	thisSym;
		int	found = 0;

		// If the name is not qualified (doesn't contain ::) use normal
		// lookup rules (members first, then statics and globals)
		// If the name is qualified, go directly to the statics and globals
		if (!strstr(id, "::") && inMemberFunction(thisSym, classSym))
		{
			if (searchClass(classSym, id, st_func))
			{
				found = 1;
			}
		}

		// search for unqualified name
		if (!found)
		{
			if (find_static(id, st_notags))
			{
				found = 1;
			}
			else if (pobj->find_next_global(id, curSymbol))
			{
				inGlobalSearch = 1;
				found = 1;
			}
		}

		sighold(SIGINT);
		if (found)
			result = curSymbol;
		return found;
	}


	if (curSymbol.isnull())
		return 0;

	// previous search found  function that was a class member
	// use the qualified name instead of the unqualified name
	if (member_id)
		id = member_id;

	if (!inGlobalSearch)
	{
		// global function definitions appear as the children of source 
		// files and as global.  Don't pick up the same symbol twice!
		while (searchSiblingChain(id,curSymbol.sibling(), st_notags))
		{
			if (!curSymbol.isGlobalSub())
			{
				//found a static instance of name
				result = curSymbol;
				return 1;
			}
		}

		// start global search
		inGlobalSearch = 1;
		curSymbol.null();
	}


	if (pobj->find_next_global(id, curSymbol))
	{
		result = curSymbol;
		return 1;
	}
	result.null();
	return 0;
}

CCresolver *
CCresolver::find_base(const char *base)
{
	if (!class_tree)
		class_tree = buildClassTree(top_scope);
	for (ClassTree *cur = class_tree; cur; cur = cur->get_base())
	{
		if (strcmp(base, cur->get_name()) == 0)
		{
			return (CCresolver *)new_resolver(lang, top_scope,
						pobj, cur->clone());
		}
	}
	return 0;
}

int
CCresolver::is_base(const char *base)
{
	if (!class_tree)
		class_tree = buildClassTree(top_scope);
	for (ClassTree *cur = class_tree; cur; cur = cur->get_base())
	{
		if (strcmp(base, cur->get_name()) == 0)
			return 1;
	}
	return 0;
}

// NOTE - this function assumes knowledge about cfront's
// implementation of class layouts.
// Also, this WILL NOT know about base classes that have no data members,
// since it relies on the data member names to derive the class hierarchy
ClassTree *
CCresolver_cfront::buildClassTree(Symbol &sym)
{
	ClassTree	*top = 0;
	char		*class_name = sym.name();
	char		*base_name = 0;
	size_t		base_len = 0;

	for (Symbol child = sym.child(); !child.isnull(); child = child.sibling())
	{
		if (!child.isMember())
			continue;

		char	*simple_name;
		size_t	len;
		char	*member_name = child.name();
		if (!member_name || (simple_name = skip_class_name(member_name)) == 0
			|| simple_name == member_name)
			continue;

		len = simple_name - (member_name + 2);
		if (!base_name || base_len != len
			|| strncmp(base_name, member_name, len) != 0)
		{
			// new base class
			base_name = new char[len + 1];
			strncpy(base_name, member_name, len);
			base_name[len] = '\0';
			base_len = len;

			ClassTree *base_class = new ClassTree(base_name, child);
			if (top)
				base_class->prepend(top);
			top = base_class;

			if (strcmp(base_name, class_name) == 0)
				break;
		}
	}
	if (!top || strcmp(top->get_name(), sym.name()) != 0)
	{
		// top-most class has no data members, so hasn't been added to the list

		char	*name = sym.name();

		base_name = new char[strlen(name) + 1];
		strcpy(base_name, name);
		ClassTree *new_top = new ClassTree(base_name, sym.child());
		if (top)
			new_top->prepend(top);
		top = new_top;
	}
	return top;
}

int
CCresolver_cfront::cmpName(const char *search_id, const char *sym_name)
{
	if (!search_id || !sym_name)
		return 0;

	if (CCresolver::cmpName(search_id, sym_name))
		return 1;

	// if a class contains inherited members, these member names are
	// prefixed by the inherited class name and the "::" operator.
	// Move past this prefix and do compare.
	const char *ptr;
	if ((ptr = skip_class_name((char *)sym_name)) != 0 && ptr != sym_name)
	{
		return CCresolver::cmpName(search_id, ptr);
	}
	return 0;
}

// The special-case code in CCresolver_cgen deals with hoisted type names.
// Because of the problems in translating C++ code with local class
// declarations into C, all local types are hoisted into file scope.  If there are
// multiple types with the same name, they are distinguished by the compiler
// by the mangled name, which includes the name of the enclosing function
// and the line number of the declaration.
// If the user types "whatis type_name", or uses type_name in a cast,
// the debugger has to stand on its head to find the type that is visible
// from the current scope.  (Note that this is not a problem if the user
// prints the type or value of a variable, since the links in the debugging
// information point to the correct type, without having to deal with the name.)
// When it finds a type name, the debugger finds the scope where
// the name was declared by searching from the outside in to find
// the innermost scope whose high and low line numbers encompass
// the line number in the type's mangled name.  (This has to be done
// from outside in - working from the current scope out could show
// a type as visible in the current block, when in fact it was declared
// in an inner block preceding the current location.)  The debugger
// also has to find all the types with the same name and compare their
// scope, since a type in an inner scope will hide one in an outer scope.


// parent_of returns true if scope1 contains scope2
static int
parent_of(Symbol &scope1, Symbol &scope2)
{
	Symbol s = scope2;
	for (; !s.isnull(); s = s.parent())
	{
		if (scope1 == s)
			return 1;
	}
	return 0;
}

int
CCresolver_cgen::searchSiblingChain(const char* id, const Symbol& startSym, 
	Symtype user_def_type)
{
	if (startSym.isnull())
	{
		curSymbol.null();
		return 0;
	}

	Symbol	s;
	int	found = 0;
	for (s = startSym; ! s.isnull(); s = s.sibling())
	{
		if (prismember(&interrupt, SIGINT))
			break;

		if (s.tag() == t_none)
			continue;

		if (cmpName(id, s.name()))
		{ 
			// distinguish tag names and typedef names.
			//  .. typedef enum e { a, b, c } E; int e;  is legal C.
			//     but NOT legal C++ - but we shouldn't see conflicts
			// in C++
			if (checkTag(user_def_type, s))
			{
				if (s.isUserTypeSym())
				{
					Symbol newScope;
					if (in_scope(s, newScope))
					{
						if (found)
						{
							// if we've already found a
							// type with this name in scope,
							// see if the new type declaration
							// hides the previous one.
							if (parent_of(typeScope,
									newScope))
							{
								curSymbol = s;
								typeScope = newScope;
							}
						}
						else
						{
							curSymbol = s;
							typeScope = newScope;
							found++;
						}
					}
				}
				else
				{
					curSymbol = s;
					return 1;
				}
			}
		}
		
		if (s.isEnumType())
		{
			Symbol literal;
			if (find_enumlit(s, id, literal))
			{
				curSymbol = literal;
				return 1;
			}
		}
	}

	if (!found)
		curSymbol.null();
	return found;
}

int
CCresolver_cgen::in_scope(Symbol &s, Symbol &scope)
{
	int	decl_line;
	char	*name;
	char	*ptr;
	char	*func_name;

	scope = curSource;

	// names of global classes are not mangled
	if ((name = s.mangledName()) == 0)
		return 1;

	Symbol	func;
	if (!getEnclosingFunc(func)
		|| (func_name = func.mangledName()) == 0)
		return 0;

	// hoisted names look like name__function__Lnn_nn
	// work backwards to find __Lnn_nn at the end of the name
	ptr = name + strlen(name) - 1;
	while (*ptr && isdigit(*ptr))
		--ptr;
	if (*ptr-- != '_')
		return 0;
	while (*ptr && isdigit(*ptr))
		--ptr;
	if (*ptr != 'L' || *(ptr-1) != '_')
		return 0;
	decl_line = atoi(++ptr);

	// check that the symbol is in the current function
	size_t	len = strlen(func_name);
	ptr -= len + 3;	// allow for __L
	if (ptr < name || strncmp(ptr, func_name, len) != 0)
		return 0;

	Symbol	child = func.child();
	scope = func;
	while (!child.isnull())
	{
		switch (child.tag())
		{
		case t_block:
		case t_entry:
		case t_extlabel:
		case t_label:
			if (in_scope(child, decl_line))
			{
				scope = child;
				child = scope.child();
				break;
			}
			// FALL-THROUGH

		default:
			child = child.sibling();
			break;
		}

	}
	return parent_of(scope, curScope);
}

int
CCresolver_cgen::in_scope(Symbol &scope, long line)
{
	Iaddr	high_pc;
	Iaddr	low_pc;
	long	low = 0;
	long	high = 0;

	high_pc = scope.pc(an_hipc);
	low_pc = scope.pc(an_lopc);
	source.pc_to_stmt(low_pc, low);
	source.pc_to_stmt(high_pc-1, high);

	if (line >= low && line <= high)
		return 1;
	return 0;
}

int
CCresolver_cgen::lookup(const char *id, Symbol &result, Symtype user_def_type)
{
	if (getEnclosingSrc(curSource))
		curSource.source(source);
	return CCresolver::lookup(id, result, user_def_type);
}

// This function assumes knowledge about the C-generating ANSI C++ compiler's
// implementation of class layouts.  The base class is always the
// first member of the derived class, with a name that starts with "__b_"
// I.e., given code that looks like:
// class A { int a; };  class B : A { int b; };
// the generated C code for class B looks like this:
// struct B {
//	struct A __b_A;
//	int b;
// };
// MORE - we do not handle multiple inheritance

ClassTree *
CCresolver_cgen::buildClassTree(Symbol &sym)
{
	Symbol		class_sym;
	char		*name = sym.name();
	char		*base_name = makestr(name);
	ClassTree	*node;

	node = new ClassTree(base_name);
	for (Symbol child = sym.child(); !child.isnull(); child = child.sibling())
	{
		char	*class_name;

		if (!child.isMember() && !child.isnull())
			continue;

		if (is_base_class(child, class_sym, class_name))
		{
			// new base class
			ClassTree *base_class = buildClassTree(class_sym);
			node->prepend(base_class);
		}
		else
		{
			node->set_first_member(child);
			break;
		}
	}
	return node;
}

Resolver *
new_resolver(Language lang, ProcObj *pobj, Iaddr addr)
{
	// CCresolver class is not functional until real C++ DWARF is available
	//if (lang == CPLUS)
	//	return new CCresolver(pobj, addr);
	if (lang == CPLUS_ASSUMED_V2)
		return new CCresolver_cgen(pobj, addr);
	if (lang == CPLUS_ASSUMED)
		return new CCresolver_cfront(pobj, addr);
	return new Resolver(pobj, addr);
}

Resolver *
new_resolver(Language lang, Symbol& sym, ProcObj *pobj, ClassTree *t)
{
	// CCresolver class is not functional until real C++ DWARF is available
	//if (lang == CPLUS)
	//	return new CCresolver(sym, pobj, t);
	if (lang == CPLUS_ASSUMED_V2)
		return new CCresolver_cgen(sym, pobj, t);
	if (lang == CPLUS_ASSUMED)
		return new CCresolver_cfront(sym, pobj, t);
	return new Resolver(sym, pobj);
}

Resolver *
new_resolver(Language lang, ProcObj *pobj, Symtab *stp, Symbol &sym)
{
	// CCresolver class is not functional until real C++ DWARF is available
	//if (lang == CPLUS)
	//	return new CCresolver(stp, sym);
	if (lang == CPLUS_ASSUMED_V2)
		return new CCresolver_cgen(pobj, stp, sym);
	if (lang == CPLUS_ASSUMED)
		return new CCresolver_cfront(pobj, stp, sym);
	return new Resolver(pobj, stp, sym);
}
