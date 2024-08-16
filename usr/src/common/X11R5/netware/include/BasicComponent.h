/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwmisc:netware/include/BasicComponent.h	1.2"
///////////////////////////////////////////////////////////////
// BasicComponent.h: First version of a class to define 
//                    a protocol for all components
///////////////////////////////////////////////////////////////
#ifndef BASICCOMPONENT_H
#define BASICCOMPONENT_H
#include <Xm/Xm.h>

class BasicComponent {
    
  protected:
    
    char    *_name;
    Widget   _w;    
    
    // Protected constructor to prevent instantiation
    BasicComponent ( const char * );   
    
  public:
    
    virtual ~BasicComponent();
    virtual void manage();   // Manage and unmanage widget tree
    virtual void unmanage();
    Widget baseWidget() { return _w; }
	virtual void ResponseCallback( short type, Boolean response );
};
#endif

