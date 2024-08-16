#ident	"@(#)debugger:libexp/common/ParsedRep.C	1.6"

#include "List.h"
#include "ProcObj.h"
#include "Value.h"
#include "Resolver.h"
#include "ParsedRep.h"

// null base clase versions of virtual functions; C++ 1.2
// does not allow pure virtual functions

Value *
ParsedRep::eval(Language, ProcObj *, Frame *, int, Vector **)
{
	return 0; 
}

ParsedRep *
ParsedRep::clone()	// make deep copy
{
	return 0; 
}

int 
ParsedRep::triggerList(Language, ProcObj *, Resolver *, List &, 
	Value *&)
{
	return 0;
}

int 
ParsedRep::exprIsTrue(Language, ProcObj *, Frame *)
{
	return 0;
}

int 
ParsedRep::getTriggerLvalue(Place&)
{
	return 0;
}

int 
ParsedRep::getTriggerRvalue(Rvalue&)
{
	return 0;
}

ParsedRep* 
ParsedRep::copyEventExpr(List&, List&, ProcObj*)
{
	return 0;
}

int
ParsedRep::print_type(ProcObj *, const char *)
{
	return 0;
}
