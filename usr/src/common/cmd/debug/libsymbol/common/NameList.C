#ident	"@(#)debugger:libsymbol/common/NameList.C	1.7"
#include	"Attribute.h"
#include	"Build.h"
#include	"NameList.h"
#include	<string.h>

NameEntry::NameEntry()
{
	namep = 0;
	form = af_none;
	value.word = 0;
}

NameEntry::NameEntry( const NameEntry & name_entry )
{
	namep = name_entry.namep;
	form = name_entry.form;
	value = name_entry.value;
}

// Make a NameEntry instance
Rbnode *
NameEntry::makenode()
{
	char *	s;

	s = new(char[sizeof(NameEntry)]);
	memcpy(s,(char*)this,sizeof(NameEntry));
	return (Rbnode*)s;
}

//used to do lookup
int
NameEntry::cmpName(const char* s)
{
	int rslt;
	const char *p;

	if( !(rslt=strcmp(namep, s)))
	{
		return rslt;
	}

	// if the name is "X::", looking for any member of class "X"
	if ((p = strstr(namep, "::")) != 0 && *(p+2) == '\0')
		return strncmp(namep, s, p-namep+2);
	else if ((p = strstr(s, "::")) != 0 && *(p+2) == '\0')
		return strncmp(namep, s, p-s+2);

	// special case for "operator()", since parens are part of
	// function name
	if ((p = strstr(namep, "::operator()")) != 0)
	{
		return strncmp(namep, s, p-namep + sizeof("::operator()")-1);
	}

	// demangled function names are prototyped.  If one of the
	// names is prototyped, compare name up to the first '(' (i.e.,
	// up to the parameter list).  Either this->namep or s could
	// be prototyped, but if they are both prototyped  don't do
	// abbreviated comparision.

	char* namepParenPosition;
	int namepLen;
	namepLen = ( (namepParenPosition=strchr( namep, '(')) ? 
			namepParenPosition - (char*)namep: strlen(namep) );

	char* sParenPosition;
	int sLen;
	sLen = ( (sParenPosition=strchr( s, '(')) ?
			sParenPosition - (char*)s: strlen(s) );

	if( (!namepParenPosition || !sParenPosition) && namepLen==sLen )
		rslt = strncmp(namep,s,sLen);

	return rslt;
}

// used to do insert
int
NameEntry::cmp( Rbnode & t )
{
	NameEntry *	name_entry = (NameEntry*)(&t);

	return ( strcmp(namep,name_entry->namep));
}

void
NameEntry::setNodeName(char* name)
{
	if ((namep = demangle_name(name)) == 0)
		namep = name;
}

NameEntry *
NameList::add( const char * s, const Attribute * a )
{
	NameEntry	node;

	node.setNodeName((char *)s);
	node.form = af_symbol;
	node.value.symbol = (Attribute *) a;
	return (NameEntry*)tinsert(node);
}

NameEntry *
NameList::add( const char * s, long w, Attr_form form )
{
	NameEntry	node;

	node.setNodeName((char *)s);
	node.form = form;
	node.value.word = w;
	return (NameEntry*)tinsert(node);
}
