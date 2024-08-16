#ident	"@(#)libclass:BasicComponent.C	1.2"
///////////////////////////////////////////////////////////
// BasicComponent.C: Initial version of a class to define 
//                    a protocol for all components
///////////////////////////////////////////////////////////
#include <assert.h>
#include "BasicComponent.h"
#include <stdio.h>

BasicComponent::BasicComponent ( const char *name )
{
    _w = NULL;
    assert ( name != NULL );  // Make sure programmers provide name
    _name = strdup ( name );
}

BasicComponent::~BasicComponent()
{
    if( _w )
	XtDestroyWidget ( _w );
    delete _name;
}

void BasicComponent::manage()
{
    assert ( _w != NULL );
    XtManageChild ( _w );
}

void BasicComponent::unmanage()
{
    assert ( _w != NULL );
    XtUnmanageChild ( _w );
}

void BasicComponent::ResponseCallback( short type, Boolean response )
{
}
