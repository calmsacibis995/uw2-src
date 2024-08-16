#ident	"@(#)prtsetup2:ps_application.C	1.12"
/*----------------------------------------------------------------------------
 *	ps_application.c
 */

#include <Xm/Xm.h>

#include "ps_hdr.h"
#include "dispInfo.h"
#include "ps_i18n.h"
#include "ps_application.h"

extern "C" { 
# include <libMDtI/DesktopP.h>
}
#include <FIconBoxI.h>

/*----------------------------------------------------------------------------
 *	This is the constructor for the Application class.
 */
Application::Application (int argc, char** argv, char* name)
		   : BasicComponent (name)
{
	// Initialize the drag and drop software 
	//OlDnDVCXInitialize ();

	// Initialize the Intrinsics
	XtSetLanguageProc (0, 0, 0);
	_w = XtAppInitialize (&d_appContext,
						  name,
						  NULL,
						  0,
						  &argc,
						  argv,
						  NULL,
						  NULL,
						  0);
	//InitializeGizmos("prtsetup", "prtsetup");
	DtiInitialize (_w);

	XtGetApplicationResources (_w,
							   &d_resData,
							   moreResources,
							   XtNumber (moreResources),
							   0,
							   0);
}

/*----------------------------------------------------------------------------
 *	Realize the Application and execute the main processing loop.
 */
void
Application::RealizeLoop (DispInfo* di)
{
	Window						owner;
	Pixmap						icon;
	Pixmap						mask;
	unsigned int				w;
	unsigned int				h;
	long						back;

	XtVaSetValues (_w,
				   XmNmappedWhenManaged,
				   False,
				   XmNwidth,
				   di->Width (),
				   XmNheight,
				   di->Height (),
				   0);
	XtRealizeWidget (_w);
	owner = DtSetAppId (XtDisplay (_w), XtWindow (_w), "prtsetup");
	if (owner != None) {
		XMapWindow (XtDisplay (_w), owner);
		XRaiseWindow (XtDisplay (_w), owner);
		XFlush (XtDisplay (_w));
		exit (0);
	}
	DmGetPixmap (XtScreen (_w), "/usr/X/lib/pixmaps/prtsetup48.icon");

	// Create the process icon
	XtVaGetValues (_w, XmNbackground, &back, NULL);
	XReadPixmapFile (XtDisplay (_w),
					 RootWindowOfScreen (XtScreen (_w)),
					 DefaultColormapOfScreen (XtScreen (_w)),
					 "/usr/X/lib/pixmaps/prtsetup48.icon",
					 &w,
					 &h,
					 DefaultDepthOfScreen (XtScreen (_w)),
					 &icon,
					 back);	
	mask = 0;
	XtVaSetValues (_w,
				   XtNiconPixmap, icon,
				   XtNiconMask,	mask,
				   XtNiconName,	GetLocalStr (TXT_appName),
				   0);

	XtMapWidget (_w);
	di->SetWindow ();
	XtAppMainLoop (d_appContext);
}

