#ident	"@(#)systemmon:main.C	1.2"

/////////////////////////////////////////////////
// main.c: main driver for sar gui 
///////////////////////////////////////////////////
#include <Xm/Xm.h>
#include "Application.h"
#include "WorkArea.h"

void main ( unsigned int argc, char **argv )
{
    	// Instantiate the main application object. This initializes
	//  a few data types,structures that we need during the program 
    	Application *sysmon= new Application ( &argc, argv, "System_Monitor");	

    	// Instantiate the work area and manage its base widget
	WorkArea *w = new WorkArea (theApplication->baseWidget(), "WorkArea");
	w->manage();

	// Realize the main loop here
	sysmon->RealizeLoop ();	
}
