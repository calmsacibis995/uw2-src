/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/siscrinit.c	1.11"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

/*******************************************************
 	Copyrighted as an unpublished work.
 	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 	All rights reserved.
********************************************************/
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/
/* $XConsortium: mfbscrinit.c,v 5.10 90/01/23 15:39:27 rws Exp $ */

#include "X.h"
#include "Xmd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "colormapst.h"		/* SI */
#include "si.h"			/* SI */ 
#include "mistruct.h"
#include "dix.h"
#include "mi.h"
/* #include "mibstore.h" */
#include "servermd.h"
#include "simskbits.h"		/* SI */
#include "sidep.h"		/* SI */

#ifdef XWIN_SAVE_UNDERS	/* SI: save-unders */
#include "sisave.h"
int SUScrnPrivateIndex = 0;
int siDefaultSaveUnder = Always;
#else
int siDefaultSaveUnder = NotUseful;
#endif

extern RegionPtr mfbPixmapToRegion();
extern void miPaintWindow ();

int siGCPrivateIndex = 0; 			/* SI (mfb to si) */
static unsigned long siGCGeneration = 0;	/* SI (mfb to si) */

int mfbGCPrivateIndex = 0;

extern int defaultColorVisualClass;

/* Screen Interface Global Defines */

char	*siSTATEerr = "Can't Set Graphics State";


/* int siDefaultBackingStore = NotUseful; */
int siDefaultBackingStore = Always;

#define BACKING_STORE

#ifdef BACKING_STORE

#include "mibstore.h"
extern void cfbSaveAreas();
extern void cfbRestoreAreas();

miBSFuncRec siBSFuncRec = {		/* SI (mfb to si) */
    cfbSaveAreas,			/* SI (mfb to si) */
    cfbRestoreAreas,			/* SI (mfb to si) */
    (void (*)()) 0,
    (PixmapPtr (*)()) 0,
    (PixmapPtr (*)()) 0,
};
#endif

/*ARGSUSED*/
static Bool
siCloseScreen (index, pScreen)
    int         index;
    ScreenPtr   pScreen;
{
    xfree ((pointer) pScreen->devPrivate);
    return TRUE;
}

Bool
siAllocatePrivates(pScreen,numvisuals,visuals,depths)
  ScreenPtr pScreen;
  int numvisuals;
  VisualRec *visuals;
  DepthRec *depths;
{
    int i;
    si_currentScreen();

    if (siGCGeneration != serverGeneration)	/* SI (mfb to si) */
    {
	siGCPrivateIndex = AllocateGCPrivateIndex();
	siGCGeneration = serverGeneration;	/* SI (mfb to si) */

	mfbGCPrivateIndex =  siGCPrivateIndex;
    }

    for (i = 0; i < numvisuals; i++) {
	visuals[i].vid = FakeClientID(0);
	depths[i].numVids = 1;
	depths[i].vids = &visuals[i].vid;
    }

#ifdef XWIN_SAVE_UNDERS
    /*
     * Allocate private screen structure (save unders)
     * and initialize the private Save Under pointer to Null.
     */
    SUScrnPrivateIndex = AllocateScreenPrivateIndex();
    pScreen->devPrivates[SUScrnPrivateIndex].ptr =
      (pointer) xalloc (sizeof (SUPrivScrn));
    SUPrivate(pScreen) = (SUPrivPtr) NULL;
#endif

    return (AllocateGCPrivate(pScreen, siGCPrivateIndex, sizeof(siPrivGC)));
}

Bool
siScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width)
    register ScreenPtr pScreen;
    pointer pbits;		/* pointer to screen bitmap */
    int xsize, ysize;		/* in pixels */
    int dpix, dpiy;		/* dots per inch */
    int width;			/* pixel width of frame buffer */
{
    register PixmapPtr pPixmap;
    Bool siInitializeColormap();	/* SI */ 
    SIVisualP pVisuals;			/* SI */
    int i;				/* SI */
    VisualRec *visuals;
    DepthRec *depths;
    int numvisuals = 0;
    int rootdepth, rootBitsPerPixel;

    si_prepareScreen(pScreen);

    numvisuals = si_GetInfoVal(SIvisualCNT);

    pVisuals = si_GetInfoVal(SIvisuals);
    rootdepth = pVisuals->SVdepth;

    if (rootdepth & (rootdepth - 1)) {
	/* rootdepth is not even power-of-2... */
	if (rootdepth <= 8) {
	    rootBitsPerPixel = 8;
	} else if (rootdepth <= 16) {
	    rootBitsPerPixel = 16;
	} else {
	    rootBitsPerPixel = 32;
	}
    } else {
	rootBitsPerPixel = rootdepth;
    }


    if (siScrInitGeneration != serverGeneration) {
	siScrInitGeneration = serverGeneration;

	visuals = (VisualPtr)xalloc((unsigned long)
				    (sizeof(VisualRec) * numvisuals));
	if (!visuals)
	    return FALSE;

	depths = (DepthPtr)xalloc((unsigned long)
				  (sizeof(DepthRec) * numvisuals));
	if (!depths) {
	    xfree((pointer)visuals);
	    return FALSE;
	}
    } else {
	visuals = pScreen->visuals;
	depths = pScreen->allowedDepths;
    }

    if (!siAllocatePrivates(pScreen,numvisuals,visuals,depths))
	return FALSE;

    pScreen->width = xsize;
    pScreen->height = ysize;
    pScreen->mmWidth = (xsize * 254) / (dpix * 10);
    pScreen->mmHeight = (ysize * 254) / (dpiy * 10);

    pScreen->numDepths = numvisuals;
    pScreen->allowedDepths = depths;

    pScreen->rootDepth = rootdepth;
    pScreen->minInstalledCmaps = 1;
    pScreen->maxInstalledCmaps = 1;
    pScreen->backingStoreSupport = siDefaultBackingStore;	/* SI */

    /*
     * Should we support BackingStore if there is no support for
     * scr->mem and mem->scr bitblt. For now, don't support save-unders
     * if there is no support from the SDD
     */
    if (si_hassmbitblt && si_hasmsbitblt) {
    	pScreen->saveUnderSupport = siDefaultSaveUnder;
        pScreen->backingStoreSupport = siDefaultBackingStore;
    }
    else {
    	pScreen->saveUnderSupport = NotUseful;
        pScreen->backingStoreSupport = NotUseful;
    }

    /* let CreateDefColormap do whatever it wants */
    pScreen->blackPixel =  pScreen->whitePixel = (Pixel) 0;

    /* cursmin and cursmax are device specific */

    pScreen->numVisuals = numvisuals;
    pScreen->visuals = visuals;

    pPixmap = (PixmapPtr)xalloc((unsigned long)sizeof(PixmapRec));
    if (!pPixmap)
	return FALSE;
    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.depth = rootdepth;
    pPixmap->drawable.bitsPerPixel = rootBitsPerPixel;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.serialNumber = 0;
    pPixmap->drawable.x = 0;
    pPixmap->drawable.y = 0;
    pPixmap->drawable.width = xsize;
    pPixmap->drawable.height = ysize;
    pPixmap->refcnt = 1;
    pPixmap->devPrivate.ptr = pbits;
    pPixmap->devKind = PixmapBytePad(width, rootBitsPerPixel);
    pScreen->devPrivate = (pointer)pPixmap;

#ifdef MITSHM
    ShmRegisterFbFuncs(pScreen);
#endif

    /* anything that cfb doesn't know about is assumed to be done
       elsewhere.  (we put in no-op only for things that we KNOW
       are really no-op).
    */

    pScreen->CreateWindow = siCreateWindow;		/* SI (mfb to si) */
    pScreen->DestroyWindow = siDestroyWindow;		/* SI (mfb to si) */
    pScreen->PositionWindow = siPositionWindow;		/* SI (mfb to si) */
    pScreen->ChangeWindowAttributes = siChangeWindowAttributes;	    /* SI */ 
    pScreen->RealizeWindow = siMapWindow;		/* SI (mfb to si) */
    pScreen->UnrealizeWindow = siUnmapWindow;		/* SI (mfb to si) */

    pScreen->RealizeFont = siRealizeFont; 		/* SI (mfb to si) */
    pScreen->UnrealizeFont = siUnrealizeFont;		/* SI (mfb to si) */
    pScreen->CloseScreen = siCloseScreen;		/* SI (from mfb) */
    pScreen->QueryBestSize = siQueryBestSize;		/* SI (mfb to si) */
    pScreen->GetImage = miGetImage;			/* SI (use mi) */
    pScreen->GetSpans = siGetSpans;			/* SI (from mfb) */
    pScreen->SourceValidate = (void (*)()) 0;
    pScreen->CreateGC = siCreateGC;			/* SI (mfb to si) */
    pScreen->CreatePixmap = siCreatePixmap;		/* SI (mfb to si) */
    pScreen->DestroyPixmap = siDestroyPixmap;		/* SI (mfb to si) */
    pScreen->ValidateTree = miValidateTree;
    pScreen->PostValidateTree = (void (*)()) 0;		/* new in R5 */

    pScreen->InstallColormap = siInstallColormap;	/* SI (mfb to si) */
    pScreen->UninstallColormap = siUninstallColormap;   /* SI (mfb to si)*/
    pScreen->ListInstalledColormaps = siListInstalledColormaps;    /* SI */
    pScreen->StoreColors = siStoreColors;	/* SI (NoopDDA -> si func) */
    pScreen->ResolveColor = siResolveColor;		/* SI (mfb to si) */

    pScreen->RegionCreate = miRegionCreate;
    pScreen->RegionInit = miRegionInit;
    pScreen->RegionCopy = miRegionCopy;
    pScreen->RegionDestroy = miRegionDestroy;
    pScreen->RegionUninit = miRegionUninit;
    pScreen->Intersect = miIntersect;
    pScreen->Inverse = miInverse;
    pScreen->Union = miUnion;
    pScreen->Subtract = miSubtract;
    pScreen->RegionReset = miRegionReset;
    pScreen->TranslateRegion = miTranslateRegion;
    pScreen->RectIn = miRectIn;
    pScreen->PointInRegion = miPointInRegion;
    pScreen->WindowExposures = miWindowExposures;
    pScreen->PaintWindowBackground = miPaintWindow;   /* SI */ 
    pScreen->PaintWindowBorder = miPaintWindow;         /* SI */ 
    pScreen->CopyWindow = siCopyWindow;	        	/* SI (mfb to si) */
    pScreen->ClearToBackground = miClearToBackground;

    pScreen->RegionNotEmpty = miRegionNotEmpty;
    pScreen->RegionEmpty = miRegionEmpty;
    pScreen->RegionExtents = miRegionExtents;
    pScreen->RegionAppend = miRegionAppend;
    pScreen->RegionValidate = miRegionValidate;
    pScreen->BitmapToRegion = mfbPixmapToRegion;
    pScreen->RectsToRegion = miRectsToRegion;
    pScreen->SendGraphicsExpose = miSendGraphicsExpose;

    pScreen->BlockHandler = NoopDDA;
    pScreen->WakeupHandler = NoopDDA;
    pScreen->blockData = (pointer)0;
    pScreen->wakeupData = (pointer)0;

    pScreen->CreateColormap = siInitializeColormap;
    pScreen->DestroyColormap = NoopDDA;
    /*
     * set default white and black values. These also can be reset by
     * command line options.
     */
    pScreen->whitePixel = 1;
    pScreen->blackPixel = 0;

    /* SI: start */
    pVisuals = si_GetInfoVal(SIvisuals);
    for (i = 0; i < numvisuals; i++, pVisuals++) {
	depths[i].depth = pVisuals->SVdepth;
	visuals[i].class = pVisuals->SVtype;

	/*
	 * See if we can fake a user specified visual
	 */
	switch (siColormap->visual) {
	case StaticColor:
		if (pVisuals->SVtype == StaticColor)
			visuals[i].class = siColormap->visual;
		break;
	case GrayScale:
		if (pVisuals->SVtype == PseudoColor)
			visuals[i].class = siColormap->visual;
		break;
	case StaticGray:
		if ((pVisuals->SVtype == PseudoColor) ||
		    (pVisuals->SVtype == GrayScale))
			visuals[i].class = siColormap->visual;
		break;
	}

	visuals[i].bitsPerRGBValue = pVisuals->SVbitsrgb;
	visuals[i].ColormapEntries = pVisuals->SVcmapsz;
	visuals[i].nplanes = pVisuals->SVdepth;

	visuals[i].redMask   = pVisuals->SVredmask;
	visuals[i].greenMask = pVisuals->SVgreenmask;
	visuals[i].blueMask  = pVisuals->SVbluemask;
	visuals[i].offsetRed   = pVisuals->SVredoffset;
	visuals[i].offsetGreen = pVisuals->SVgreenoffset;
	visuals[i].offsetBlue  = pVisuals->SVblueoffset;
    }
    /* SI: end */

    pScreen->defColormap = FakeClientID(0);
    if (defaultColorVisualClass < 0)
    {
	i = 0;
    }
    else
    {
	for (i = 0;
	    (i < numvisuals) && (visuals[i].class != defaultColorVisualClass);
	    i++)
	    ;
	if (i >= numvisuals)
	    i = 0;
    }

    siinitstates(); /* SI */
    siinitfonts();  /* SI */

    pScreen->rootVisual = visuals[i].vid;
#ifdef BACKING_STORE
    miInitializeBackingStore (pScreen, &siBSFuncRec);	/* SI (mfb to si) */
#endif
#ifdef MITSHM
    ShmRegisterFbFuncs(pScreen);
#endif
    /*
     * Now, load any runtime extensions specified in /usr/X/defaults/XwinExtns
     * file. The "mi" specific ones are loaded here and the "scr" specific one
     * are loaded in siScreenInit()
     */
    xwin_init_runtime_extns (INIT_RUNTIME_EXT_SCR);

    return TRUE;
}


void
siExtensionInit()
{
    int ii;
    extern int siNumScreens;
    extern SIScreenRec siScreens[];
    si_currentScreen();

    for (ii=0; ii < siNumScreens; ++ii) {
	/*
	 * Give each SDD the chance to initialize it's extensions
	 */
	pSIScreen = &siScreens[ii];
	if (si_haveexten)
	  si_exten_init();
    }
    pSIScreen = &siScreens[0];

    /*
     * Now, load any runtime extensions specified in /usr/X/defaults/XwinExtns
     * file. The "mi" specific ones are loaded here and the "scr" specific one
     * are loaded in siScreenInit()
     */
    xwin_init_runtime_extns(INIT_RUNTIME_EXT_MI);
}
