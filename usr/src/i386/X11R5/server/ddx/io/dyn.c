/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/io/dyn.c	1.8"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

#include "xwin_io.h"
#include "siconfig.h"

/*
 *	The link between the core Xwin server and the Display Library is the
 *	"ScreenInterface" structure. 
 *
 */
#ifdef SHARED_DISPLIB 
#include <dlfcn.h>

int
LoadDisplayLibrary(name, init_func)
  char *name;
  int *init_func;
{
    void *handle;
    SI_1_0_FunctionsP *pp_SI_1_0_DisplayFuncs;

    handle = dlopen(name, RTLD_NOW);
    if(handle == NULL) {
	ErrorF("dlopen <%s> failed\nReason: %s\n", name, dlerror());
	return(SI_FAIL);
    }

    *init_func = (int)dlsym(handle, "DM_InitFunction");

    if( *init_func == NULL) {
	/*
	 * old display module
	 */
	pp_SI_1_0_DisplayFuncs =
	  (SI_1_0_FunctionsP *)dlsym(handle, "DisplayFuncs");
	if((pp_SI_1_0_DisplayFuncs == NULL) ||
	   (*pp_SI_1_0_DisplayFuncs == NULL)) {
	    ErrorF("dlsym DisplayFuncs/DM_InitFunction in <%s> failed\nReason: %s\n",
		   name, dlerror());
	    dlclose(handle);
	    return(SI_FAIL);
	}
	siScreens[0].funcsPtr = (SI_1_1_FunctionsP)*pp_SI_1_0_DisplayFuncs;
    }
    return (SI_SUCCEED);
}
#else
int
LoadDisplayLibrary(name, init_func)
  char *name;
  int *init_func;
{
    extern int DM_InitFunction;
    SI_1_0_FunctionsP p_SI_1_0_DisplayFuncs;
	
    /*
     * NOTE: this function is used only to build an archive version
     * of the server (mainly for debugging)
     */
    *init_func = (int)&DM_InitFunction;

    if ( *init_func == NULL) {
	if (p_SI_1_0_DisplayFuncs == NULL) {
	    ErrorF("Cannot find a valid DisplayFuncs\n");
	    return(SI_FAIL);
	}
	siScreens[0].funcsPtr = (SI_1_1_FunctionsP)p_SI_1_0_DisplayFuncs;
	return(SI_SUCCEED);
    }

    return (DM_InitFunction ? &DM_InitFunction : 0);
}
#endif
