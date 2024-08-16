/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/misprite.c	1.4"

/*
 *	Copyright (c) 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

/*
 * misprite.c
 *
 * machine independent software sprite routines
 */

/* $XConsortium: misprite.c,v 5.38 91/07/19 23:19:31 keith Exp $ */

/*
Copyright 1989 by the Massachusetts Institute of Technology

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of M.I.T. not be used in
advertising or publicity pertaining to distribution of the software
without specific, written prior permission.  M.I.T. makes no
representations about the suitability of this software for any
purpose.  It is provided "as is" without express or implied warranty.
*/

# include   "X.h"
# include   "Xproto.h"
# include   "misc.h"
# include   "pixmapstr.h"
# include   "input.h"
# include   "mi.h"
# include   "cursorstr.h"
# include   "font.h"
# include   "scrnintstr.h"
# include   "colormapst.h"
# include   "windowstr.h"
# include   "gcstruct.h"
# include   "mipointer.h"
# include   "mispritest.h"
# include   "dixfontstr.h"
# include   "fontstruct.h"

#include "si.h"
#include "sidep.h"

/*
 * screen wrappers
 */

static int  miSpriteScreenIndex;
static unsigned long miSpriteGeneration = 0;

static Bool	    miSpriteCloseScreen();
static void	    miSpriteGetImage();
static void	    miSpriteGetSpans();
static void	    miSpriteSourceValidate();
static Bool	    miSpriteCreateGC();
static void	    miSpriteBlockHandler();
static void	    miSpriteInstallColormap();
static void	    miSpriteStoreColors();

static void	    miSpritePaintWindowBackground();
static void	    miSpritePaintWindowBorder();
static void	    miSpriteCopyWindow();
static void	    miSpriteClearToBackground();

static void	    miSpriteSaveDoomedAreas();
static RegionPtr    miSpriteRestoreAreas();
static void	    miSpriteComputeSaved();


#define SetupWindowScreen(w)						\
    ScreenPtr pScreen = (w)->drawable.pScreen

#define SetupDrawableScreen(d)						\
    ScreenPtr pScreen = (d)->pScreen

#define SetupGCScreen(g)						\
    ScreenPtr pScreen = (g)->pScreen

#define SetupMapScreen(m)						\
    ScreenPtr pScreen = (m)->pScreen

#define SetupSpriteGCPriv(g)						\
    miSpriteGCPtr pSpriteGCPriv = (miSpriteGCPtr)			\
        ((g)->devPrivates[miSpriteGCIndex].ptr)

#define SetupSpriteScreenPriv(s)					\
    miSpriteScreenPtr  pSpriteScreenPriv = (miSpriteScreenPtr)		\
        ((s)->devPrivates[miSpriteScreenIndex].ptr)

#define SCREEN_PROLOGUE(pScreen, field)					\
  ((pScreen)->field = pSpriteScreenPriv->field)

#define SCREEN_EPILOGUE(pScreen, field, wrapper)			\
    ((pScreen)->field = wrapper)

/*
 * GC func wrappers
 */

static int  miSpriteGCIndex;

static void miSpriteValidateGC (),  miSpriteCopyGC ();
static void miSpriteDestroyGC(),    miSpriteChangeGC();
static void miSpriteChangeClip(),   miSpriteDestroyClip();
static void miSpriteCopyClip();

static GCFuncs	miSpriteGCFuncs = {
    miSpriteValidateGC,
    miSpriteChangeGC,
    miSpriteCopyGC,
    miSpriteDestroyGC,
    miSpriteChangeClip,
    miSpriteDestroyClip,
    miSpriteCopyClip,
};

#define GC_FUNC_PROLOGUE(pGC)					\
    miSpriteGCPtr   pGCPriv =					\
	(miSpriteGCPtr) (pGC)->devPrivates[miSpriteGCIndex].ptr;\
    (pGC)->funcs = pGCPriv->wrapFuncs;				\
    if (pGCPriv->wrapOps)					\
	(pGC)->ops = pGCPriv->wrapOps;

#define GC_FUNC_EPILOGUE(pGC)					\
    pGCPriv->wrapFuncs = (pGC)->funcs;				\
    (pGC)->funcs = &miSpriteGCFuncs;				\
    if (pGCPriv->wrapOps)					\
    {								\
	pGCPriv->wrapOps = (pGC)->ops;				\
	(pGC)->ops = &miSpriteGCOps;				\
    }

/*
 * GC op wrappers
 */

static void	    miSpriteFillSpans(),	miSpriteSetSpans();
static void	    miSpritePutImage();
static RegionPtr    miSpriteCopyArea(),		miSpriteCopyPlane();
static void	    miSpritePolyPoint(),	miSpritePolylines();
static void	    miSpritePolySegment(),	miSpritePolyRectangle();
static void	    miSpritePolyArc(),		miSpriteFillPolygon();
static void	    miSpritePolyFillRect(),	miSpritePolyFillArc();
static int	    miSpritePolyText8(),	miSpritePolyText16();
static void	    miSpriteImageText8(),	miSpriteImageText16();
static void	    miSpriteImageGlyphBlt(),	miSpritePolyGlyphBlt();
static void	    miSpritePushPixels(),	miSpriteLineHelper();
static void	    miSpriteChangeClip(),	miSpriteDestroyClip();
static void	    miSpriteCopyClip();

static GCOps miSpriteGCOps = {
    miSpriteFillSpans,	    miSpriteSetSpans,	    miSpritePutImage,	
    miSpriteCopyArea,	    miSpriteCopyPlane,	    miSpritePolyPoint,
    miSpritePolylines,	    miSpritePolySegment,    miSpritePolyRectangle,
    miSpritePolyArc,	    miSpriteFillPolygon,    miSpritePolyFillRect,
    miSpritePolyFillArc,    miSpritePolyText8,	    miSpritePolyText16,
    miSpriteImageText8,	    miSpriteImageText16,    miSpriteImageGlyphBlt,
    miSpritePolyGlyphBlt,   miSpritePushPixels,	    miSpriteLineHelper,
};

/*
 * testing only -- remove cursor for every draw.  Eventually,
 * each draw operation will perform a bounding box check against
 * the saved cursor area
 */

#define GC_SETUP_CHEAP(pDrawable)					\
    SetupSpriteScreenPriv((pDrawable)->pScreen);

#define GC_SETUP(pDrawable, pGC)					\
    GC_SETUP_CHEAP(pDrawable)						\
    miSpriteGCPtr	pGCPrivate = (miSpriteGCPtr)			\
	(pGC)->devPrivates[miSpriteGCIndex].ptr;			\
    GCFuncs *oldFuncs = pGC->funcs;

#define GC_SETUP_AND_CHECK(pDrawable, pGC)				\
    GC_SETUP(pDrawable, pGC);						\
    if (GC_CHECK((WindowPtr)pDrawable))					\
	miSpriteRemoveCursor (pDrawable->pScreen);
    
#define GC_CHECK(pWin)							\
    (pSpriteScreenPriv->isUp &&						\
        (pSpriteScreenPriv->pCacheWin == pWin ?				\
	    pSpriteScreenPriv->isInCacheWin : (				\
	    ((int) (pSpriteScreenPriv->pCacheWin = (pWin))) ,		\
	    (pSpriteScreenPriv->isInCacheWin =				\
		(pWin)->drawable.x < pSpriteScreenPriv->saved.x2 &&	\
		pSpriteScreenPriv->saved.x1 < (pWin)->drawable.x +	\
				    (int) (pWin)->drawable.width &&	\
		(pWin)->drawable.y < pSpriteScreenPriv->saved.y2 &&	\
		pSpriteScreenPriv->saved.y1 < (pWin)->drawable.y +	\
				    (int) (pWin)->drawable.height &&	\
		(pWin)->drawable.pScreen->RectIn (&(pWin)->borderClip,	\
			&pSpriteScreenPriv->saved) != rgnOUT))))

#define GC_OP_PROLOGUE(pGC) { \
    (pGC)->funcs = pGCPrivate->wrapFuncs; \
    (pGC)->ops = pGCPrivate->wrapOps; \
    }

#define GC_OP_EPILOGUE(pGC) { \
    pGCPrivate->wrapOps = (pGC)->ops; \
    (pGC)->funcs = oldFuncs; \
    (pGC)->ops = &miSpriteGCOps; \
    }

/*
 * pointer-sprite method table
 */

static Bool miSpriteRealizeCursor (),	miSpriteUnrealizeCursor ();
static void miSpriteSetCursor (),	miSpriteMoveCursor ();

miPointerSpriteFuncRec miSpritePointerFuncs = {
    miSpriteRealizeCursor,
    miSpriteUnrealizeCursor,
    miSpriteSetCursor,
    miSpriteMoveCursor,
};

static Bool siSpriteRealizeCursor(),	siSpriteUnrealizeCursor();
static void siSpriteSetCursor(),	siSpriteMoveCursor();

miPointerSpriteFuncRec siSpritePointerFuncs = {
    siSpriteRealizeCursor,
    siSpriteUnrealizeCursor,
    siSpriteSetCursor,
    siSpriteMoveCursor,
};

/*
 * other misc functions
 */

static void miSpriteRemoveCursor (),	miSpriteRestoreCursor();

/*
 * miSpriteInitialize -- called from device-dependent screen
 * initialization proc after all of the function pointers have
 * been stored in the screen structure.
 */

Bool
miSpriteInitialize (pScreen, cursorFuncs, screenFuncs)
    ScreenPtr		    pScreen;
    miSpriteCursorFuncPtr   cursorFuncs;
    miPointerScreenFuncPtr  screenFuncs;
{
    miSpriteScreenPtr		pSpriteScreenPriv;
    VisualPtr			pVisual;
    extern int			AllocateScreenPrivateIndex();
    miPointerSpriteFuncPtr	pointerFuncs;
    si_prepareScreen(pScreen);
    
    if (miSpriteGeneration != serverGeneration)
      {
	  miSpriteScreenIndex = AllocateScreenPrivateIndex ();
	  if (miSpriteScreenIndex < 0)
	    return FALSE;
	  miSpriteGCIndex = AllocateGCPrivateIndex ();
	  if (miSpriteGCIndex < 0)
	    return FALSE;
	  miSpriteGeneration = serverGeneration;
      }
    if (!AllocateGCPrivate(pScreen, miSpriteGCIndex, sizeof(miSpriteGCRec)))
      return FALSE;
    pSpriteScreenPriv = (miSpriteScreenPtr) xalloc((unsigned long)
						   sizeof(miSpriteScreenRec));
    if (!pSpriteScreenPriv)
      return FALSE;
    if (si_havetruecursor) {
	pointerFuncs = &siSpritePointerFuncs;
    } else {
	pointerFuncs = &miSpritePointerFuncs;
    }
    if (!miPointerInitialize (pScreen, pointerFuncs, screenFuncs,TRUE))
      {
	  xfree ((pointer) pSpriteScreenPriv);
	  return FALSE;
      }
    for (pVisual = pScreen->visuals;
	 pVisual->vid != pScreen->rootVisual;
	 pVisual++)
      ;

    pSpriteScreenPriv->pVisual               = pVisual;
    pSpriteScreenPriv->CloseScreen           = pScreen->CloseScreen;
    pSpriteScreenPriv->GetImage              = pScreen->GetImage;
    pSpriteScreenPriv->GetSpans              = pScreen->GetSpans;
    pSpriteScreenPriv->SourceValidate        = pScreen->SourceValidate;
    pSpriteScreenPriv->CreateGC              = pScreen->CreateGC;
    pSpriteScreenPriv->BlockHandler          = pScreen->BlockHandler;
    pSpriteScreenPriv->InstallColormap       = pScreen->InstallColormap;
    pSpriteScreenPriv->StoreColors           = pScreen->StoreColors;

    pSpriteScreenPriv->PaintWindowBackground = pScreen->PaintWindowBackground;
    pSpriteScreenPriv->PaintWindowBorder     = pScreen->PaintWindowBorder;
    pSpriteScreenPriv->CopyWindow            = pScreen->CopyWindow;
    pSpriteScreenPriv->ClearToBackground     = pScreen->ClearToBackground;

    pSpriteScreenPriv->SaveDoomedAreas       = pScreen->SaveDoomedAreas;
    pSpriteScreenPriv->RestoreAreas          = pScreen->RestoreAreas;

    pSpriteScreenPriv->pCursor               = NULL;
    pSpriteScreenPriv->x                     = 0;
    pSpriteScreenPriv->y                     = 0;
    pSpriteScreenPriv->isUp                  = FALSE;
    pSpriteScreenPriv->shouldBeUp            = FALSE;
    pSpriteScreenPriv->pCacheWin             = NullWindow;
    pSpriteScreenPriv->isInCacheWin          = FALSE;
    pSpriteScreenPriv->checkPixels           = TRUE;
    pSpriteScreenPriv->pInstalledMap         = NULL;
    pSpriteScreenPriv->pColormap             = NULL;
    pSpriteScreenPriv->funcs                 = cursorFuncs;

    pScreen->devPrivates[miSpriteScreenIndex].ptr =
      (pointer) pSpriteScreenPriv;

    pScreen->CloseScreen                     = miSpriteCloseScreen;
    pScreen->InstallColormap                 = miSpriteInstallColormap;
    pScreen->StoreColors                     = miSpriteStoreColors;

    if (!si_havetruecursor) {
	pScreen->GetImage                    = miSpriteGetImage;
	pScreen->GetSpans                    = miSpriteGetSpans;
	pScreen->SourceValidate              = miSpriteSourceValidate;
	pScreen->CreateGC                    = miSpriteCreateGC;
	pScreen->BlockHandler                = miSpriteBlockHandler;

	pScreen->PaintWindowBackground       = miSpritePaintWindowBackground;
	pScreen->PaintWindowBorder           = miSpritePaintWindowBorder;
	pScreen->CopyWindow                  = miSpriteCopyWindow;
	pScreen->ClearToBackground           = miSpriteClearToBackground;

	pScreen->SaveDoomedAreas             = miSpriteSaveDoomedAreas;
	pScreen->RestoreAreas                = miSpriteRestoreAreas;
    }
    return TRUE;
}

/*
 * Screen wrappers
 */

/*
 * CloseScreen wrapper -- unwrap everything, free the private data
 * and call the wrapped function
 */

static Bool
miSpriteCloseScreen (i, pScreen)
    ScreenPtr	pScreen;
{
    SetupSpriteScreenPriv(pScreen);

    pScreen->CloseScreen           = pSpriteScreenPriv->CloseScreen;
    pScreen->GetImage              = pSpriteScreenPriv->GetImage;
    pScreen->GetSpans              = pSpriteScreenPriv->GetSpans;
    pScreen->SourceValidate        = pSpriteScreenPriv->SourceValidate;
    pScreen->CreateGC              = pSpriteScreenPriv->CreateGC;
    pScreen->BlockHandler          = pSpriteScreenPriv->BlockHandler;
    pScreen->InstallColormap       = pSpriteScreenPriv->InstallColormap;
    pScreen->StoreColors           = pSpriteScreenPriv->StoreColors;

    pScreen->PaintWindowBackground = pSpriteScreenPriv->PaintWindowBackground;
    pScreen->PaintWindowBorder     = pSpriteScreenPriv->PaintWindowBorder;
    pScreen->CopyWindow            = pSpriteScreenPriv->CopyWindow;
    pScreen->ClearToBackground     = pSpriteScreenPriv->ClearToBackground;

    pScreen->SaveDoomedAreas       = pSpriteScreenPriv->SaveDoomedAreas;
    pScreen->RestoreAreas          = pSpriteScreenPriv->RestoreAreas;

    xfree ((pointer) pSpriteScreenPriv);

    return (*pScreen->CloseScreen) (i, pScreen);
}

static void
miSpriteGetImage (pDrawable, sx, sy, w, h, format, planemask, pdstLine)
    DrawablePtr	    pDrawable;
    int		    sx, sy, w, h;
    unsigned int    format;
    unsigned long   planemask;
    pointer	    pdstLine;
{
    SetupDrawableScreen(pDrawable);
    SetupSpriteScreenPriv(pScreen);
    
    SCREEN_PROLOGUE (pScreen, GetImage);

    if (pDrawable->type == DRAWABLE_WINDOW &&
        pSpriteScreenPriv->isUp &&
	ORG_OVERLAP(&pSpriteScreenPriv->saved,pDrawable->x,pDrawable->y, sx, sy, w, h))
    {
	miSpriteRemoveCursor (pScreen);
    }

    (*pScreen->GetImage) (pDrawable, sx, sy, w, h,
			  format, planemask, pdstLine);

    SCREEN_EPILOGUE (pScreen, GetImage, miSpriteGetImage);
}

static void
miSpriteGetSpans (pDrawable, wMax, ppt, pwidth, nspans, pdstStart)
    DrawablePtr	pDrawable;
    int		wMax;
    DDXPointPtr	ppt;
    int		*pwidth;
    int		nspans;
    unsigned int *pdstStart;
{
    SetupDrawableScreen(pDrawable);
    SetupSpriteScreenPriv(pScreen);
    
    SCREEN_PROLOGUE (pScreen, GetSpans);

    if (pDrawable->type == DRAWABLE_WINDOW && pSpriteScreenPriv->isUp)
    {
	register DDXPointPtr    pts;
	register int    	*widths;
	register int    	nPts;
	register int    	xorg,
				yorg;

	xorg = pDrawable->x;
	yorg = pDrawable->y;

	for (pts = ppt, widths = pwidth, nPts = nspans;
	     nPts--;
	     pts++, widths++)
 	{
	    if (SPN_OVERLAP(&pSpriteScreenPriv->saved,pts->y+yorg,
			     pts->x+xorg,*widths))
	    {
		miSpriteRemoveCursor (pScreen);
		break;
	    }
	}
    }

    (*pScreen->GetSpans) (pDrawable, wMax, ppt, pwidth, nspans, pdstStart);

    SCREEN_EPILOGUE (pScreen, GetSpans, miSpriteGetSpans);
}

static void
miSpriteSourceValidate (pDrawable, x, y, width, height)
    DrawablePtr	pDrawable;
    int		x, y, width, height;
{
    SetupDrawableScreen(pDrawable);
    SetupSpriteScreenPriv(pScreen);
    
    SCREEN_PROLOGUE (pScreen, SourceValidate);

    if (pDrawable->type == DRAWABLE_WINDOW && pSpriteScreenPriv->isUp &&
	ORG_OVERLAP(&pSpriteScreenPriv->saved, pDrawable->x, pDrawable->y,
		    x, y, width, height))
    {
	miSpriteRemoveCursor (pScreen);
    }

    if (pScreen->SourceValidate)
	(*pScreen->SourceValidate) (pDrawable, x, y, width, height);

    SCREEN_EPILOGUE (pScreen, SourceValidate, miSpriteSourceValidate);
}

static Bool
miSpriteCreateGC (pGC)
    GCPtr   pGC;
{
    Bool	ret;
    SetupGCScreen(pGC);
    SetupSpriteGCPriv(pGC);
    SetupSpriteScreenPriv(pScreen);

    SCREEN_PROLOGUE (pScreen, CreateGC);

    ret = (*pScreen->CreateGC) (pGC);

    pSpriteGCPriv->wrapOps = NULL;
    pSpriteGCPriv->wrapFuncs = pGC->funcs;
    pGC->funcs = &miSpriteGCFuncs;

    SCREEN_EPILOGUE (pScreen, CreateGC, miSpriteCreateGC);

    return ret;
}

static void
miSpriteBlockHandler (i, blockData, pTimeout, pReadmask)
    int	i;
    pointer	blockData;
    pointer	pTimeout;
    pointer	pReadmask;
{
    ScreenPtr	pScreen = screenInfo.screens[i];
    SetupSpriteScreenPriv(pScreen);

    SCREEN_PROLOGUE(pScreen, BlockHandler);
    
    (*pScreen->BlockHandler) (i, blockData, pTimeout, pReadmask);

    SCREEN_EPILOGUE(pScreen, BlockHandler, miSpriteBlockHandler);

    if (!pSpriteScreenPriv->isUp && pSpriteScreenPriv->shouldBeUp)
      miSpriteRestoreCursor (pScreen);
}

static void
miSpriteInstallColormap (pMap)
    ColormapPtr	pMap;
{
    SetupMapScreen(pMap);
    SetupSpriteScreenPriv(pScreen);

    SCREEN_PROLOGUE(pScreen, InstallColormap);
    
    (*pScreen->InstallColormap) (pMap);

    SCREEN_EPILOGUE(pScreen, InstallColormap, miSpriteInstallColormap);

    pSpriteScreenPriv->pInstalledMap = pMap;
    if (pSpriteScreenPriv->pColormap != pMap)
    {
    	pSpriteScreenPriv->checkPixels = TRUE;
	if (pSpriteScreenPriv->isUp)
	    miSpriteRemoveCursor (pScreen);
    }
}

static void
miSpriteStoreColors (pMap, ndef, pdef)
    ColormapPtr	pMap;
    int		ndef;
    xColorItem	*pdef;
{
    int		i;
    int		updated;
    VisualPtr	pVisual;
    SetupMapScreen(pMap);
    SetupSpriteScreenPriv(pScreen);

    SCREEN_PROLOGUE(pScreen, StoreColors);
    
    (*pScreen->StoreColors) (pMap, ndef, pdef);

    SCREEN_EPILOGUE(pScreen, StoreColors, miSpriteStoreColors);

    if (pSpriteScreenPriv->pColormap == pMap)
    {
	updated = 0;
	pVisual = pMap->pVisual;
	if (pVisual->class == DirectColor)
	{
	    /* Direct color - match on any of the subfields */
#if 0
#define MaskMatch(a,b,mask) ((a) & (pVisual->mask) == (b) & (pVisual->mask))
#else /* reduce precedence confusion */
#define MaskMatch(a,b,mask) (((a) & (pVisual->mask)) == ((b) & (pVisual->mask)))
#endif

#define UpdateDAC(plane,dac,mask) {\
    if (MaskMatch (pSpriteScreenPriv->colors[plane].pixel,\
		   pdef[i].pixel,mask)) {\
	pSpriteScreenPriv->colors[plane].dac = pdef[i].dac; \
	updated = 1; \
    } \
}

#define CheckDirect(plane) \
	    UpdateDAC(plane,red,redMask) \
	    UpdateDAC(plane,green,greenMask) \
	    UpdateDAC(plane,blue,blueMask)

	    for (i = 0; i < ndef; i++)
	    {
		CheckDirect (SOURCE_COLOR)
		CheckDirect (MASK_COLOR)
	    }
	}
	else
	{
	    /* PseudoColor/GrayScale - match on exact pixel */
	    for (i = 0; i < ndef; i++)
	    {
	    	if (pdef[i].pixel ==
		    pSpriteScreenPriv->colors[SOURCE_COLOR].pixel)
	    	{
		    pSpriteScreenPriv->colors[SOURCE_COLOR] = pdef[i];
		    if (++updated == 2)
		    	break;
	    	}
	    	if (pdef[i].pixel ==
		    pSpriteScreenPriv->colors[MASK_COLOR].pixel)
	    	{
		    pSpriteScreenPriv->colors[MASK_COLOR] = pdef[i];
		    if (++updated == 2)
		    	break;
	    	}
	    }
	}
    	if (updated)
    	{
	    pSpriteScreenPriv->checkPixels = TRUE;
	    if (pSpriteScreenPriv->isUp)
	    	miSpriteRemoveCursor (pScreen);
    	}
    }
}

static void
miSpriteFindColors (pScreen)
    ScreenPtr	pScreen;
{
    CursorPtr	pCursor;
    xColorItem	*sourceColor, *maskColor;
    SetupSpriteScreenPriv(pScreen);
    extern void FakeAllocColor(), FakeFreeColor();

    pCursor = pSpriteScreenPriv->pCursor;
    sourceColor = &pSpriteScreenPriv->colors[SOURCE_COLOR];
    maskColor = &pSpriteScreenPriv->colors[MASK_COLOR];
    if (!(pCursor->foreRed == sourceColor->red &&
	  pCursor->foreGreen == sourceColor->green &&
          pCursor->foreBlue == sourceColor->blue &&
	  pCursor->backRed == maskColor->red &&
	  pCursor->backGreen == maskColor->green &&
	  pCursor->backBlue == maskColor->blue) ||
	 pSpriteScreenPriv->pColormap != pSpriteScreenPriv->pInstalledMap)
    {
	pSpriteScreenPriv->pColormap = pSpriteScreenPriv->pInstalledMap;
	sourceColor->red = pCursor->foreRed;
	sourceColor->green = pCursor->foreGreen;
	sourceColor->blue = pCursor->foreBlue;
	FakeAllocColor (pSpriteScreenPriv->pColormap, sourceColor);
	maskColor->red = pCursor->backRed;
	maskColor->green = pCursor->backGreen;
	maskColor->blue = pCursor->backBlue;
	FakeAllocColor (pSpriteScreenPriv->pColormap, maskColor);
	/* "free" the pixels right away, don't let this confuse you */
	FakeFreeColor(pSpriteScreenPriv->pColormap, sourceColor->pixel);
	FakeFreeColor(pSpriteScreenPriv->pColormap, maskColor->pixel);
    }
    pSpriteScreenPriv->checkPixels = FALSE;
}

/*
 * BackingStore wrappers
 */

static void
miSpriteSaveDoomedAreas (pWin, pObscured, dx, dy)
    WindowPtr	pWin;
    RegionPtr	pObscured;
    int		dx, dy;
{
    BoxRec	cursorBox;
    SetupWindowScreen(pWin);
    SetupSpriteScreenPriv(pScreen);

    SCREEN_PROLOGUE (pScreen, SaveDoomedAreas);

    if (pSpriteScreenPriv->isUp)
    {
	cursorBox = pSpriteScreenPriv->saved;

	if (dx || dy)
 	{
	    cursorBox.x1 += dx;
	    cursorBox.y1 += dy;
	    cursorBox.x2 += dx;
	    cursorBox.y2 += dy;
	}
	if ((* pScreen->RectIn) (pObscured, &cursorBox) != rgnOUT)
	    miSpriteRemoveCursor (pScreen);
    }

    (*pScreen->SaveDoomedAreas) (pWin, pObscured, dx, dy);

    SCREEN_EPILOGUE (pScreen, SaveDoomedAreas, miSpriteSaveDoomedAreas);
}

static RegionPtr
miSpriteRestoreAreas (pWin, prgnExposed)
    WindowPtr	pWin;
    RegionPtr	prgnExposed;
{
    RegionPtr	result;
    SetupWindowScreen(pWin);
    SetupSpriteScreenPriv(pScreen);
    
    SCREEN_PROLOGUE (pScreen, RestoreAreas);

    if (pSpriteScreenPriv->isUp)
    {
	if ((* pScreen->RectIn) (prgnExposed,
				 &pSpriteScreenPriv->saved) != rgnOUT)
	    miSpriteRemoveCursor (pScreen);
    }

    result = (*pScreen->RestoreAreas) (pWin, prgnExposed);

    SCREEN_EPILOGUE (pScreen, RestoreAreas, miSpriteRestoreAreas);

    return result;
}

/*
 * Window wrappers
 */

static void
miSpritePaintWindowBackground (pWin, pRegion, what)
    WindowPtr	pWin;
    RegionPtr	pRegion;
    int		what;
{
    SetupWindowScreen(pWin);
    SetupSpriteScreenPriv(pScreen);

    SCREEN_PROLOGUE (pScreen, PaintWindowBackground);

    if (pSpriteScreenPriv->isUp)
    {
	/*
	 * If the cursor is on the same screen as the window, check the
	 * region to paint for the cursor and remove it as necessary
	 */
	if ((* pScreen->RectIn) (pRegion, &pSpriteScreenPriv->saved) != rgnOUT)
	    miSpriteRemoveCursor (pScreen);
    }

    (*pScreen->PaintWindowBackground) (pWin, pRegion, what);

    SCREEN_EPILOGUE (pScreen, PaintWindowBackground,
		     miSpritePaintWindowBackground);
}

static void
miSpritePaintWindowBorder (pWin, pRegion, what)
    WindowPtr	pWin;
    RegionPtr	pRegion;
    int		what;
{
    SetupWindowScreen(pWin);
    SetupSpriteScreenPriv(pScreen);

    SCREEN_PROLOGUE (pScreen, PaintWindowBorder);

    if (pSpriteScreenPriv->isUp)
    {
	/*
	 * If the cursor is on the same screen as the window, check the
	 * region to paint for the cursor and remove it as necessary
	 */
	if ((* pScreen->RectIn) (pRegion, &pSpriteScreenPriv->saved) != rgnOUT)
	    miSpriteRemoveCursor (pScreen);
    }

    (*pScreen->PaintWindowBorder) (pWin, pRegion, what);

    SCREEN_EPILOGUE (pScreen, PaintWindowBorder, miSpritePaintWindowBorder);
}

static void
miSpriteCopyWindow (pWin, ptOldOrg, pRegion)
    WindowPtr	pWin;
    DDXPointRec	ptOldOrg;
    RegionPtr	pRegion;
{
    BoxRec	cursorBox;
    int		dx, dy;
    SetupWindowScreen(pWin);
    SetupSpriteScreenPriv(pScreen);

    SCREEN_PROLOGUE (pScreen, CopyWindow);

    if (pSpriteScreenPriv->isUp)
    {
	/*
	 * check both the source and the destination areas.  The given
	 * region is source relative, so offset the cursor box by
	 * the delta position
	 */
	cursorBox = pSpriteScreenPriv->saved;
	dx = pWin->drawable.x - ptOldOrg.x;
	dy = pWin->drawable.y - ptOldOrg.y;
	cursorBox.x1 -= dx;
	cursorBox.x2 -= dx;
	cursorBox.y1 -= dy;
	cursorBox.y2 -= dy;
	if ((* pScreen->RectIn) (pRegion, &pSpriteScreenPriv->saved) != rgnOUT ||
	    (* pScreen->RectIn) (pRegion, &cursorBox) != rgnOUT)
	    miSpriteRemoveCursor (pScreen);
    }

    (*pScreen->CopyWindow) (pWin, ptOldOrg, pRegion);

    SCREEN_EPILOGUE (pScreen, CopyWindow, miSpriteCopyWindow);
}

static void
miSpriteClearToBackground (pWin, x, y, w, h, generateExposures)
    WindowPtr	pWin;
    short	x,y;
    unsigned short w,h;
    Bool	generateExposures;
{
    int		realw, realh;
    SetupWindowScreen(pWin);
    SetupSpriteScreenPriv(pScreen);

    SCREEN_PROLOGUE (pScreen, ClearToBackground);

    if (GC_CHECK(pWin))
    {
	if (!(realw = w))
	    realw = (int) pWin->drawable.width - x;
	if (!(realh = h))
	    realh = (int) pWin->drawable.height - y;
	if (ORG_OVERLAP(&pSpriteScreenPriv->saved,
			pWin->drawable.x, pWin->drawable.y,
			x, y, realw, realh))
	{
	    miSpriteRemoveCursor (pScreen);
	}
    }

    (*pScreen->ClearToBackground) (pWin, x, y, w, h, generateExposures);

    SCREEN_EPILOGUE (pScreen, ClearToBackground, miSpriteClearToBackground);
}

/*
 * GC Func wrappers
 */

static void
miSpriteValidateGC (pGC, changes, pDrawable)
    GCPtr	pGC;
    Mask	changes;
    DrawablePtr	pDrawable;
{
    GC_FUNC_PROLOGUE (pGC);

    (*pGC->funcs->ValidateGC) (pGC, changes, pDrawable);
    
    pGCPriv->wrapOps = NULL;
    if (pDrawable->type == DRAWABLE_WINDOW &&
	((WindowPtr) pDrawable)->viewable)
    {
	WindowPtr   pWin;
	RegionPtr   pRegion;

	pWin = (WindowPtr) pDrawable;
	pRegion = &pWin->clipList;
	if (pGC->subWindowMode == IncludeInferiors)
	    pRegion = &pWin->borderClip;
	if ((*pDrawable->pScreen->RegionNotEmpty) (pRegion))
	    pGCPriv->wrapOps = pGC->ops;
    }

    GC_FUNC_EPILOGUE (pGC);
}

static void
miSpriteChangeGC (pGC, mask)
    GCPtr	    pGC;
    unsigned long   mask;
{
    GC_FUNC_PROLOGUE (pGC);

    (*pGC->funcs->ChangeGC) (pGC, mask);
    
    GC_FUNC_EPILOGUE (pGC);
}

static void
miSpriteCopyGC (pGCSrc, mask, pGCDst)
    GCPtr	    pGCSrc, pGCDst;
    unsigned long   mask;
{
    GC_FUNC_PROLOGUE (pGCDst);

    (*pGCDst->funcs->CopyGC) (pGCSrc, mask, pGCDst);
    
    GC_FUNC_EPILOGUE (pGCDst);
}

static void
miSpriteDestroyGC (pGC)
    GCPtr   pGC;
{
    GC_FUNC_PROLOGUE (pGC);

    (*pGC->funcs->DestroyGC) (pGC);
    
    GC_FUNC_EPILOGUE (pGC);
}

static void
miSpriteChangeClip (pGC, type, pvalue, nrects)
    GCPtr   pGC;
    int		type;
    pointer	pvalue;
    int		nrects;
{
    GC_FUNC_PROLOGUE (pGC);

    (*pGC->funcs->ChangeClip) (pGC, type, pvalue, nrects);

    GC_FUNC_EPILOGUE (pGC);
}

static void
miSpriteCopyClip(pgcDst, pgcSrc)
    GCPtr	pgcDst, pgcSrc;
{
    GC_FUNC_PROLOGUE (pgcDst);

    (* pgcDst->funcs->CopyClip)(pgcDst, pgcSrc);

    GC_FUNC_EPILOGUE (pgcDst);
}

static void
miSpriteDestroyClip(pGC)
    GCPtr	pGC;
{
    GC_FUNC_PROLOGUE (pGC);

    (* pGC->funcs->DestroyClip)(pGC);

    GC_FUNC_EPILOGUE (pGC);
}

/*
 * GC Op wrappers
 */

static void
miSpriteFillSpans(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nInit;			/* number of spans to fill */
    DDXPointPtr pptInit;		/* pointer to list of start points */
    int		*pwidthInit;		/* pointer to list of n widths */
    int 	fSorted;
{
    GC_SETUP(pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
    {
	register DDXPointPtr    pts;
	register int    	*widths;
	register int    	nPts;

	for (pts = pptInit, widths = pwidthInit, nPts = nInit;
	     nPts--;
	     pts++, widths++)
 	{
	     if (SPN_OVERLAP(&pSpriteScreenPriv->saved,pts->y,pts->x,*widths))
	     {
		 miSpriteRemoveCursor (pDrawable->pScreen);
		 break;
	     }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->FillSpans) (pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);

    GC_OP_EPILOGUE (pGC);
}

static void
miSpriteSetSpans(pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted)
    DrawablePtr		pDrawable;
    GCPtr		pGC;
    int			*psrc;
    register DDXPointPtr ppt;
    int			*pwidth;
    int			nspans;
    int			fSorted;
{
    GC_SETUP(pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
    {
	register DDXPointPtr    pts;
	register int    	*widths;
	register int    	nPts;

	for (pts = ppt, widths = pwidth, nPts = nspans;
	     nPts--;
	     pts++, widths++)
 	{
	     if (SPN_OVERLAP(&pSpriteScreenPriv->saved,pts->y,pts->x,*widths))
	     {
		 miSpriteRemoveCursor(pDrawable->pScreen);
		 break;
	     }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->SetSpans) (pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted);

    GC_OP_EPILOGUE (pGC);
}

static void
miSpritePutImage(pDrawable, pGC, bitsPerPixel, x, y, w, h, leftPad, format, pBits)
    DrawablePtr	  pDrawable;
    GCPtr   	  pGC;
    int		  bitsPerPixel;
    int	    	  x;
    int	    	  y;
    int	    	  w;
    int	    	  h;
    int	    	  format;
    char    	  *pBits;
{
    GC_SETUP(pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
    {
	if (ORG_OVERLAP(&pSpriteScreenPriv->saved,pDrawable->x,pDrawable->y,
			x,y,w,h))
 	{
	    miSpriteRemoveCursor (pDrawable->pScreen);
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PutImage) (pDrawable, pGC, bitsPerPixel, x, y, w, h, leftPad, format, pBits);

    GC_OP_EPILOGUE (pGC);
}

static RegionPtr
miSpriteCopyArea (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty)
    DrawablePtr	  pSrc;
    DrawablePtr	  pDst;
    GCPtr   	  pGC;
    int	    	  srcx;
    int	    	  srcy;
    int	    	  w;
    int	    	  h;
    int	    	  dstx;
    int	    	  dsty;
{
    RegionPtr rgn;

    GC_SETUP(pDst, pGC);

    /* check destination/source overlap. */
    if (GC_CHECK((WindowPtr) pDst) &&
	 (ORG_OVERLAP(&pSpriteScreenPriv->saved, pDst->x, pDst->y,
		      dstx, dsty, w, h) ||
	  ((pDst == pSrc) &&
	   ORG_OVERLAP(&pSpriteScreenPriv->saved, pSrc->x, pSrc->y,
		       srcx, srcy, w, h))))
    {
	miSpriteRemoveCursor (pDst->pScreen);
    }
 
    GC_OP_PROLOGUE (pGC);

    rgn = (*pGC->ops->CopyArea) (pSrc, pDst, pGC, srcx, srcy, w, h,
				 dstx, dsty);

    GC_OP_EPILOGUE (pGC);

    return rgn;
}

static RegionPtr
miSpriteCopyPlane (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty, plane)
    DrawablePtr	  pSrc;
    DrawablePtr	  pDst;
    register GC   *pGC;
    int     	  srcx,
		  srcy;
    int     	  w,
		  h;
    int     	  dstx,
		  dsty;
    unsigned long  plane;
{
    RegionPtr rgn;

    GC_SETUP(pDst, pGC);

    /*
     * check destination/source for overlap.
     */
    if (GC_CHECK((WindowPtr) pDst) &&
	(ORG_OVERLAP(&pSpriteScreenPriv->saved, pDst->x, pDst->y,
		     dstx, dsty, w, h) ||
	 ((pDst == pSrc) &&
	  ORG_OVERLAP(&pSpriteScreenPriv->saved, pSrc->x,
		      pSrc->y, srcx, srcy, w, h))))
    {
	miSpriteRemoveCursor (pDst->pScreen);
    }

    GC_OP_PROLOGUE (pGC);

    rgn = (*pGC->ops->CopyPlane) (pSrc, pDst, pGC, srcx, srcy, w, h,
				  dstx, dsty, plane);

    GC_OP_EPILOGUE (pGC);

    return rgn;
}

static void
miSpritePolyPoint (pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		mode;		/* Origin or Previous */
    int		npt;
    xPoint 	*pptInit;
{
    xPoint	t;
    int		n;
    BoxRec	cursor;
    register xPoint *pts;

    GC_SETUP (pDrawable, pGC);

    if (npt && GC_CHECK((WindowPtr) pDrawable))
    {
	cursor.x1 = pSpriteScreenPriv->saved.x1 - pDrawable->x;
	cursor.y1 = pSpriteScreenPriv->saved.y1 - pDrawable->y;
	cursor.x2 = pSpriteScreenPriv->saved.x2 - pDrawable->x;
	cursor.y2 = pSpriteScreenPriv->saved.y2 - pDrawable->y;

	if (mode == CoordModePrevious)
	{
	    t.x = 0;
	    t.y = 0;
	    for (pts = pptInit, n = npt; n--; pts++)
	    {
		t.x += pts->x;
		t.y += pts->y;
		if (cursor.x1 <= t.x && t.x <= cursor.x2 &&
		    cursor.y1 <= t.y && t.y <= cursor.y2)
		{
		    miSpriteRemoveCursor (pDrawable->pScreen);
		    break;
		}
	    }
	}
	else
	{
	    for (pts = pptInit, n = npt; n--; pts++)
	    {
		if (cursor.x1 <= pts->x && pts->x <= cursor.x2 &&
		    cursor.y1 <= pts->y && pts->y <= cursor.y2)
		{
		    miSpriteRemoveCursor (pDrawable->pScreen);
		    break;
		}
	    }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PolyPoint) (pDrawable, pGC, mode, npt, pptInit);

    GC_OP_EPILOGUE (pGC);
}

static void
miSpritePolylines (pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr	  pDrawable;
    GCPtr   	  pGC;
    int	    	  mode;
    int	    	  npt;
    DDXPointPtr	  pptInit;
{
    BoxPtr	cursor;
    register DDXPointPtr pts;
    int		n;
    int		x, y, x1, y1, x2, y2;
    int		lw;
    int		extra;

    GC_SETUP (pDrawable, pGC);

    if (npt && GC_CHECK((WindowPtr) pDrawable))
    {
	cursor = &pSpriteScreenPriv->saved;
	lw = pGC->lineWidth;
	x = pptInit->x + pDrawable->x;
	y = pptInit->y + pDrawable->y;

	if (npt == 1)
	{
	    extra = lw >> 1;
	    if (LINE_OVERLAP(cursor, x, y, x, y, extra))
		miSpriteRemoveCursor (pDrawable->pScreen);
	}
	else
	{
	    extra = lw >> 1;
	    /*
	     * mitered joins can project quite a way from
	     * the line end; the 11 degree miter limit limits
	     * this extension to 10.43 * lw / 2, rounded up
	     * and converted to int yields 6 * lw
	     */
	    if (pGC->joinStyle == JoinMiter)
		extra = 6 * lw;
	    else if (pGC->capStyle == CapProjecting)
		extra = lw;
	    for (pts = pptInit + 1, n = npt - 1; n--; pts++)
	    {
		x1 = x;
		y1 = y;
		if (mode == CoordModeOrigin)
		{
		    x2 = pDrawable->x + pts->x;
		    y2 = pDrawable->y + pts->y;
		}
		else
		{
		    x2 = x + pts->x;
		    y2 = y + pts->y;
		}
		x = x2;
		y = y2;
		LINE_SORT(x1, y1, x2, y2);
		if (LINE_OVERLAP(cursor, x1, y1, x2, y2, extra))
		{
		    miSpriteRemoveCursor (pDrawable->pScreen);
		    break;
		}
	    }
	}
    }
    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->Polylines) (pDrawable, pGC, mode, npt, pptInit);

    GC_OP_EPILOGUE (pGC);
}

static void
miSpritePolySegment(pDrawable, pGC, nseg, pSegs)
    DrawablePtr pDrawable;
    GCPtr 	pGC;
    int		nseg;
    xSegment	*pSegs;
{
    int		n;
    register xSegment *segs;
    BoxPtr	cursor;
    int		x1, y1, x2, y2;
    int		extra;

    GC_SETUP(pDrawable, pGC);

    if (nseg && GC_CHECK((WindowPtr) pDrawable))
    {
	cursor = &pSpriteScreenPriv->saved;
	extra = pGC->lineWidth >> 1;
	if (pGC->capStyle == CapProjecting)
	    extra = pGC->lineWidth;
	for (segs = pSegs, n = nseg; n--; segs++)
	{
	    x1 = segs->x1 + pDrawable->x;
	    y1 = segs->y1 + pDrawable->y;
	    x2 = segs->x2 + pDrawable->x;
	    y2 = segs->y2 + pDrawable->y;
	    LINE_SORT(x1, y1, x2, y2);
	    if (LINE_OVERLAP(cursor, x1, y1, x2, y2, extra))
	    {
		miSpriteRemoveCursor (pDrawable->pScreen);
		break;
	    }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PolySegment) (pDrawable, pGC, nseg, pSegs);

    GC_OP_EPILOGUE (pGC);
}

static void
miSpritePolyRectangle(pDrawable, pGC, nrects, pRects)
    DrawablePtr	pDrawable;
    GCPtr	pGC;
    int		nrects;
    xRectangle	*pRects;
{
    register xRectangle *rects;
    BoxPtr	cursor;
    int		lw;
    int		n;
    int		x1, y1, x2, y2;
    
    GC_SETUP (pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
    {
	lw = pGC->lineWidth >> 1;
	cursor = &pSpriteScreenPriv->saved;
	for (rects = pRects, n = nrects; n--; rects++)
	{
	    x1 = rects->x + pDrawable->x;
	    y1 = rects->y + pDrawable->y;
	    x2 = x1 + (int)rects->width;
	    y2 = y1 + (int)rects->height;
	    if (LINE_OVERLAP(cursor, x1, y1, x2, y1, lw) ||
		LINE_OVERLAP(cursor, x2, y1, x2, y2, lw) ||
		LINE_OVERLAP(cursor, x1, y2, x2, y2, lw) ||
		LINE_OVERLAP(cursor, x1, y1, x1, y2, lw))
	    {
		miSpriteRemoveCursor (pDrawable->pScreen);
		break;
	    }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PolyRectangle) (pDrawable, pGC, nrects, pRects);

    GC_OP_EPILOGUE (pGC);
}

static void
miSpritePolyArc(pDrawable, pGC, narcs, parcs)
    DrawablePtr	pDrawable;
    register GCPtr pGC;
    int		narcs;
    xArc	*parcs;
{
    BoxPtr	cursor;
    int		lw;
    int		n;
    register xArc *arcs;
    
    GC_SETUP (pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
    {
	lw = pGC->lineWidth >> 1;
	cursor = &pSpriteScreenPriv->saved;
	for (arcs = parcs, n = narcs; n--; arcs++)
	{
	    if (ORG_OVERLAP (cursor, pDrawable->x, pDrawable->y,
			     arcs->x - lw, arcs->y - lw,
			     (int) arcs->width + (int) pGC->lineWidth,
 			     (int) arcs->height + (int) pGC->lineWidth))
	    {
		miSpriteRemoveCursor (pDrawable->pScreen);
		break;
	    }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PolyArc) (pDrawable, pGC, narcs, parcs);

    GC_OP_EPILOGUE (pGC);
}

static void
miSpriteFillPolygon(pDrawable, pGC, shape, mode, count, pPts)
    register DrawablePtr pDrawable;
    register GCPtr	pGC;
    int			shape, mode;
    int			count;
    DDXPointPtr		pPts;
{
    int		x, y, minx, miny, maxx, maxy;
    register DDXPointPtr pts;
    int		n;

    GC_SETUP (pDrawable, pGC);

    if (count && GC_CHECK((WindowPtr) pDrawable))
    {
	x = pDrawable->x;
	y = pDrawable->y;
	pts = pPts;
	minx = maxx = pts->x;
	miny = maxy = pts->y;
	pts++;
	n = count - 1;

	if (mode == CoordModeOrigin)
	{
	    for (; n--; pts++)
	    {
		if (pts->x < minx)
		    minx = pts->x;
		else if (pts->x > maxx)
		    maxx = pts->x;
		if (pts->y < miny)
		    miny = pts->y;
		else if (pts->y > maxy)
		    maxy = pts->y;
	    }
	    minx += x;
	    miny += y;
	    maxx += x;
	    maxy += y;
	}
	else
	{
	    x += minx;
	    y += miny;
	    minx = maxx = x;
	    miny = maxy = y;
	    for (; n--; pts++)
	    {
		x += pts->x;
		y += pts->y;
		if (x < minx)
		    minx = x;
		else if (x > maxx)
		    maxx = x;
		if (y < miny)
		    miny = y;
		else if (y > maxy)
		    maxy = y;
	    }
	}
	if (BOX_OVERLAP(&pSpriteScreenPriv->saved,minx,miny,maxx,maxy))
	    miSpriteRemoveCursor (pDrawable->pScreen);
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->FillPolygon) (pDrawable, pGC, shape, mode, count, pPts);

    GC_OP_EPILOGUE (pGC);
}

static void
miSpritePolyFillRect(pDrawable, pGC, nrectFill, prectInit)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nrectFill; 	/* number of rectangles to fill */
    xRectangle	*prectInit;  	/* Pointer to first rectangle to fill */
{
    GC_SETUP(pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
    {
	register int	    nRect;
	register xRectangle *pRect;
	register int	    xorg, yorg;

	xorg = pDrawable->x;
	yorg = pDrawable->y;

	for (nRect = nrectFill, pRect = prectInit; nRect--; pRect++) {
	    if (ORGRECT_OVERLAP(&pSpriteScreenPriv->saved,xorg,yorg,pRect)){
		miSpriteRemoveCursor(pDrawable->pScreen);
		break;
	    }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PolyFillRect) (pDrawable, pGC, nrectFill, prectInit);

    GC_OP_EPILOGUE (pGC);
}

static void
miSpritePolyFillArc(pDrawable, pGC, narcs, parcs)
    DrawablePtr	pDrawable;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    GC_SETUP(pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
    {
	register int	n;
	BoxPtr		cursor;
	register xArc *arcs;

	cursor = &pSpriteScreenPriv->saved;

	for (arcs = parcs, n = narcs; n--; arcs++)
	{
	    if (ORG_OVERLAP(cursor, pDrawable->x, pDrawable->y,
			    arcs->x, arcs->y,
 			    (int) arcs->width, (int) arcs->height))
	    {
		miSpriteRemoveCursor (pDrawable->pScreen);
		break;
	    }
	}
    }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PolyFillArc) (pDrawable, pGC, narcs, parcs);

    GC_OP_EPILOGUE (pGC);
}

/*
 * general Poly/Image text function.  Extract glyph information,
 * compute bounding box and remove cursor if it is overlapped.
 */

static Bool
miSpriteTextOverlap (pDraw, font, x, y, n, charinfo, imageblt, w, cursorBox)
    DrawablePtr   pDraw;
    FontPtr	  font;
    int		  x, y;
    unsigned long n;
    CharInfoPtr   *charinfo;
    Bool	  imageblt;
    int		  w;
    BoxPtr	  cursorBox;
{
    ExtentInfoRec extents;

    x += pDraw->x;
    y += pDraw->y;

    if (FONTMINBOUNDS(font,characterWidth) >= 0)
    {
	/* compute an approximate (but covering) bounding box */
	if (!imageblt || (charinfo[0]->metrics.leftSideBearing < 0))
	    extents.overallLeft = charinfo[0]->metrics.leftSideBearing;
	else
	    extents.overallLeft = 0;
	if (w)
	    extents.overallRight = w - charinfo[n-1]->metrics.characterWidth;
	else
	    extents.overallRight = FONTMAXBOUNDS(font,characterWidth)
				    * (n - 1);
	if (imageblt && (charinfo[n-1]->metrics.characterWidth >
			 charinfo[n-1]->metrics.rightSideBearing))
	    extents.overallRight += charinfo[n-1]->metrics.characterWidth;
	else
	    extents.overallRight += charinfo[n-1]->metrics.rightSideBearing;
	if (imageblt && FONTASCENT(font) > FONTMAXBOUNDS(font,ascent))
	    extents.overallAscent = FONTASCENT(font);
	else
	    extents.overallAscent = FONTMAXBOUNDS(font, ascent);
	if (imageblt && FONTDESCENT(font) > FONTMAXBOUNDS(font,descent))
	    extents.overallDescent = FONTDESCENT(font);
	else
	    extents.overallDescent = FONTMAXBOUNDS(font,descent);
	if (!BOX_OVERLAP(cursorBox,
			 x + extents.overallLeft,
			 y - extents.overallAscent,
			 x + extents.overallRight,
			 y + extents.overallDescent))
	    return FALSE;
	else if (imageblt && w)
	    return TRUE;
	/* if it does overlap, fall through and compute exactly, because
	 * taking down the cursor is expensive enough to make this worth it
	 */
    }
    QueryGlyphExtents(font, charinfo, (unsigned int)n, &extents);
    if (imageblt)
    {
	if (extents.overallWidth > extents.overallRight)
	    extents.overallRight = extents.overallWidth;
	if (extents.overallWidth < extents.overallLeft)
	    extents.overallLeft = extents.overallWidth;
	if (extents.overallLeft > 0)
	    extents.overallLeft = 0;
	if (extents.fontAscent > extents.overallAscent)
	    extents.overallAscent = extents.fontAscent;
	if (extents.fontDescent > extents.overallDescent)
	    extents.overallDescent = extents.fontDescent;
    }
    return (BOX_OVERLAP(cursorBox,
			x + extents.overallLeft,
			y - extents.overallAscent,
			x + extents.overallRight,
			y + extents.overallDescent));
}

/*
 * values for textType:
 */
#define TT_POLY8   0
#define TT_IMAGE8  1
#define TT_POLY16  2
#define TT_IMAGE16 3

/*** SI ***/
static int 
miSpriteText(pDraw, pGC, x, y, count, chars, fontEncoding, textType, cursorBox)
    DrawablePtr	    pDraw;
    GCPtr	    pGC;
    int		    x,
		    y;
    unsigned long    count;
    char	    *chars;
    FontEncoding    fontEncoding;
    Bool	    textType;
    BoxPtr	    cursorBox;
{
    CharInfoPtr		*charinfo = NULL;
    register CharInfoPtr *info;
    unsigned long	n, i;
    int			newx, w;
    Bool		imageblt;
    SetupDrawableScreen(pDraw);
    si_prepareScreen(pScreen);

    newx = x;
    imageblt = (textType == TT_IMAGE8) || (textType == TT_IMAGE16);

    if (!si_havetruecursor || !si_havedlfonts) {
	charinfo = (CharInfoPtr *) ALLOCATE_LOCAL(count * sizeof(CharInfoPtr));
	if (!charinfo)
	  return(x);

	GetGlyphs(pGC->font, count, (unsigned char *)chars,
		  fontEncoding, &n, charinfo);

	w = 0;
	for (info = charinfo, i = n; i--; ++info) {
	    w += (*info)->metrics.characterWidth;
	}
	if (n == 0) {
	    DEALLOCATE_LOCAL(charinfo);
	    newx += w;
	    return(newx);
	}
    }

    if (!si_havetruecursor) {
	if (miSpriteTextOverlap(pDraw, pGC->font, x, y, n, charinfo,
				imageblt, w, cursorBox))
	  miSpriteRemoveCursor(pScreen);
    }

    /*
     * In the SI, we don't know until runtime whether we'll be drawing
     * with glyph routines or text routines.  If the SDD is supporting
     * downloaded fonts, use the text routines.
     */
    if (si_havedlfonts) {
	switch (textType) {
	  default:
	  case TT_POLY8:
	    newx = (*pGC->ops->PolyText8)(pDraw,pGC,x,y,(int)count,chars);
	    break;
	  case TT_IMAGE8:
	    (*pGC->ops->ImageText8)(pDraw,pGC,x,y,(int)count,chars);
	    break;
	  case TT_POLY16:
	    newx = (*pGC->ops->PolyText16)(pDraw,pGC,x,y,(int)count,chars);
	    break;
	  case TT_IMAGE16:
	    (*pGC->ops->ImageText16)(pDraw,pGC,x,y,(int)count,chars);
	    break;
	}
    } else {
	void (*drawFunc)();
	/*
	 * On the other hand, if the device does use GlyphBlt ultimately
	 * to do text, we don't want to slow it down by invoking the text
	 * functions and having them call GetGlyphs all over again, so we
	 * go directly to the GlyphBlt functions here.
	 */
	if (imageblt) {
	    drawFunc = pGC->ops->ImageGlyphBlt;
	} else {
	    drawFunc = pGC->ops->PolyGlyphBlt;
	    newx += w;
	}
	(*drawFunc) (pDraw, pGC, x, y, n, charinfo, FONTGLYPHS(pGC->font));
    }

    if (charinfo)
      DEALLOCATE_LOCAL(charinfo);
    return(newx);
}

static int
miSpritePolyText8(pDrawable, pGC, x, y, count, chars)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		x, y;
    int 	count;
    char	*chars;
{
    int	ret;

    GC_SETUP (pDrawable, pGC);

    GC_OP_PROLOGUE (pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
	ret = miSpriteText (pDrawable, pGC, x, y, (unsigned long)count, chars,
			    Linear8Bit, TT_POLY8, &pSpriteScreenPriv->saved);
    else
	ret = (*pGC->ops->PolyText8) (pDrawable, pGC, x, y, count, chars);

    GC_OP_EPILOGUE (pGC);
    return ret;
}

static int
miSpritePolyText16(pDrawable, pGC, x, y, count, chars)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned short *chars;
{
    int	ret;

    GC_SETUP(pDrawable, pGC);

    GC_OP_PROLOGUE (pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
      ret = miSpriteText (pDrawable, pGC, x, y, (unsigned long)count,
			  (char *)chars,
			  FONTLASTROW(pGC->font) == 0 ?
			  Linear16Bit : TwoD16Bit, TT_POLY16,
			  &pSpriteScreenPriv->saved);
    else
      ret = (*pGC->ops->PolyText16) (pDrawable, pGC, x, y, count, chars);

    GC_OP_EPILOGUE (pGC);
    return ret;
}

static void
miSpriteImageText8(pDrawable, pGC, x, y, count, chars)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		x, y;
    int		count;
    char	*chars;
{
    GC_SETUP(pDrawable, pGC);

    GC_OP_PROLOGUE (pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
      (void) miSpriteText (pDrawable, pGC, x, y, (unsigned long)count,
			   chars, Linear8Bit, TT_IMAGE8,
			   &pSpriteScreenPriv->saved);
    else
      (*pGC->ops->ImageText8) (pDrawable, pGC, x, y, count, chars);

    GC_OP_EPILOGUE (pGC);
}

static void
miSpriteImageText16(pDrawable, pGC, x, y, count, chars)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned short *chars;
{
    GC_SETUP(pDrawable, pGC);

    GC_OP_PROLOGUE (pGC);

    if (GC_CHECK((WindowPtr) pDrawable))
      (void) miSpriteText (pDrawable, pGC, x, y, (unsigned long)count,
			   (char *)chars,
			   FONTLASTROW(pGC->font) == 0 ?
			   Linear16Bit : TwoD16Bit, TT_IMAGE16,
			   &pSpriteScreenPriv->saved);
    else
      (*pGC->ops->ImageText16) (pDrawable, pGC, x, y, count, chars);

    GC_OP_EPILOGUE (pGC);
}

static void
miSpriteImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	x, y;
    unsigned long nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    pointer 	pglyphBase;	/* start of array of glyphs */
{
    GC_SETUP(pDrawable, pGC);

    GC_OP_PROLOGUE (pGC);

    if (GC_CHECK((WindowPtr) pDrawable) &&
	miSpriteTextOverlap (pDrawable, pGC->font, x, y, nglyph, ppci,
			     TRUE, 0, &pSpriteScreenPriv->saved))
      {
	  miSpriteRemoveCursor(pDrawable->pScreen);
      }
    (*pGC->ops->ImageGlyphBlt)(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);

    GC_OP_EPILOGUE (pGC);
}

static void
miSpritePolyGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int 	x, y;
    unsigned long nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    char 	*pglyphBase;	/* start of array of glyphs */
{
    GC_SETUP (pDrawable, pGC);

    GC_OP_PROLOGUE (pGC);

    if (GC_CHECK((WindowPtr) pDrawable) &&
	miSpriteTextOverlap (pDrawable, pGC->font, x, y, nglyph, ppci,
			     FALSE, 0, &pSpriteScreenPriv->saved))
      {
	  miSpriteRemoveCursor(pDrawable->pScreen);
      }
    (*pGC->ops->PolyGlyphBlt) (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);

    GC_OP_EPILOGUE (pGC);
}

static void
miSpritePushPixels(pGC, pBitMap, pDrawable, w, h, x, y)
    GCPtr	pGC;
    PixmapPtr	pBitMap;
    DrawablePtr pDrawable;
    int		w, h, x, y;
{
    GC_SETUP(pDrawable, pGC);

    if (GC_CHECK((WindowPtr) pDrawable) &&
	ORG_OVERLAP(&pSpriteScreenPriv->saved, pDrawable->x, pDrawable->y,
		    x, y, w, h))
      {
	  miSpriteRemoveCursor (pDrawable->pScreen);
      }

    GC_OP_PROLOGUE (pGC);

    (*pGC->ops->PushPixels) (pGC, pBitMap, pDrawable, w, h, x, y);

    GC_OP_EPILOGUE (pGC);
}

/*
 * I don't expect this routine will ever be called, as the GC
 * will have been unwrapped for the line drawing
 */

static void
miSpriteLineHelper()
{
    FatalError("miSpriteLineHelper called\n");
}

/*
 * miPointer interface routines
 */

/**** SI start ****/
static Bool si_curs_realize();
static Bool si_curs_unrealize();
static void si_curs_display();
static void si_curs_undisplay();
static void si_curs_move();

#define SPRITE_PAD  8

static Bool
miSpriteRealizeCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    SetupSpriteScreenPriv(pScreen);

    if (pCursor == pSpriteScreenPriv->pCursor)
	pSpriteScreenPriv->checkPixels = TRUE;
    return (*pSpriteScreenPriv->funcs->RealizeCursor) (pScreen, pCursor);
}

static Bool
miSpriteUnrealizeCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    SetupSpriteScreenPriv(pScreen);

    return (*pSpriteScreenPriv->funcs->UnrealizeCursor) (pScreen, pCursor);
}

static void
miSpriteSetCursor (pScreen, pCursor, x, y)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    SetupSpriteScreenPriv(pScreen);

    pSpriteScreenPriv->shouldBeUp = TRUE;
    if (pSpriteScreenPriv->x == x &&
	pSpriteScreenPriv->y == y &&
	pSpriteScreenPriv->pCursor == pCursor &&
	!pSpriteScreenPriv->checkPixels)
    {
	return;
    }
    if (!pCursor)
    {
    	pSpriteScreenPriv->shouldBeUp = FALSE;
    	if (pSpriteScreenPriv->isUp)
	    miSpriteRemoveCursor (pScreen);
	pSpriteScreenPriv->pCursor = 0;
	return;
    }
    pSpriteScreenPriv->x = x;
    pSpriteScreenPriv->y = y;
    pSpriteScreenPriv->pCacheWin = NullWindow;
    if (pSpriteScreenPriv->checkPixels || pSpriteScreenPriv->pCursor != pCursor)
    {
	pSpriteScreenPriv->pCursor = pCursor;
	miSpriteFindColors (pScreen);
    }
    if (pSpriteScreenPriv->isUp) {
	int	sx, sy;
	/*
	 * check to see if the old saved region
	 * encloses the new sprite, in which case we use
	 * the flicker-free MoveCursor primitive.
	 */
	sx = pSpriteScreenPriv->x - (int)pCursor->bits->xhot;
	sy = pSpriteScreenPriv->y - (int)pCursor->bits->yhot;
	if (sx + (int) pCursor->bits->width >= pSpriteScreenPriv->saved.x1 &&
	    sx < pSpriteScreenPriv->saved.x2 &&
	    sy + (int) pCursor->bits->height >= pSpriteScreenPriv->saved.y1 &&
	    sy < pSpriteScreenPriv->saved.y2 &&
	    (int) pCursor->bits->width + (2 * SPRITE_PAD) ==
		pSpriteScreenPriv->saved.x2 - pSpriteScreenPriv->saved.x1 &&
	    (int) pCursor->bits->height + (2 * SPRITE_PAD) ==
		pSpriteScreenPriv->saved.y2 - pSpriteScreenPriv->saved.y1
	    )
	{
	    pSpriteScreenPriv->isUp = FALSE;
	    if (!(sx >= pSpriteScreenPriv->saved.x1 &&
	      	  sx + (int)pCursor->bits->width < pSpriteScreenPriv->saved.x2 &&
	      	  sy >= pSpriteScreenPriv->saved.y1 &&
	      	  sy + (int)pCursor->bits->height < pSpriteScreenPriv->saved.y2))
	    {
		int oldx1, oldy1, dx, dy;

		oldx1 = pSpriteScreenPriv->saved.x1;
		oldy1 = pSpriteScreenPriv->saved.y1;
		dx = oldx1 - (sx - SPRITE_PAD);
		dy = oldy1 - (sy - SPRITE_PAD);
		pSpriteScreenPriv->saved.x1 -= dx;
		pSpriteScreenPriv->saved.y1 -= dy;
		pSpriteScreenPriv->saved.x2 -= dx;
		pSpriteScreenPriv->saved.y2 -= dy;
		(void) (*pSpriteScreenPriv->funcs->ChangeSave)
		  (pScreen, pSpriteScreenPriv->saved.x1,
		   pSpriteScreenPriv->saved.y1,
		   pSpriteScreenPriv->saved.x2 - pSpriteScreenPriv->saved.x1,
		   pSpriteScreenPriv->saved.y2 - pSpriteScreenPriv->saved.y1,
		   dx, dy);
	    }
	    (void) (*pSpriteScreenPriv->funcs->MoveCursor)
	      (pScreen, pCursor, pSpriteScreenPriv->saved.x1,
	       pSpriteScreenPriv->saved.y1,
	       pSpriteScreenPriv->saved.x2 - pSpriteScreenPriv->saved.x1,
	       pSpriteScreenPriv->saved.y2 - pSpriteScreenPriv->saved.y1,
	       sx - pSpriteScreenPriv->saved.x1,
	       sy - pSpriteScreenPriv->saved.y1,
	       pSpriteScreenPriv->colors[SOURCE_COLOR].pixel,
	       pSpriteScreenPriv->colors[MASK_COLOR].pixel);
	    pSpriteScreenPriv->isUp = TRUE;
	}
	else
	{
	    miSpriteRemoveCursor (pScreen);
	}
    }
    if (!pSpriteScreenPriv->isUp && pSpriteScreenPriv->pCursor)
	miSpriteRestoreCursor (pScreen);
}

static void
miSpriteMoveCursor (pScreen, x, y)
    ScreenPtr	pScreen;
    int		x, y;
{
    SetupSpriteScreenPriv(pScreen);

    miSpriteSetCursor (pScreen, pSpriteScreenPriv->pCursor, x, y);
}

/*
 * undraw/draw cursor
 */

static void
miSpriteRemoveCursor (pScreen)
    ScreenPtr	pScreen;
{
    SetupSpriteScreenPriv(pScreen);
    si_prepareScreen(pScreen);

    if (si_havetruecursor) {
	si_curs_undisplay(pScreen);
	return;
    }

    pSpriteScreenPriv->isUp = FALSE;
    pSpriteScreenPriv->pCacheWin = NullWindow;
    if (!(*pSpriteScreenPriv->funcs->RestoreUnderCursor)
	(pScreen, pSpriteScreenPriv->saved.x1,
	 pSpriteScreenPriv->saved.y1,
	 pSpriteScreenPriv->saved.x2 - pSpriteScreenPriv->saved.x1,
	 pSpriteScreenPriv->saved.y2 - pSpriteScreenPriv->saved.y1))
      {
	  pSpriteScreenPriv->isUp = TRUE;
      }
}

/*
 * Called from the block handler, restores the cursor
 * before waiting for something to do.
 */

static void
miSpriteRestoreCursor (pScreen)
    ScreenPtr	pScreen;
{
    int		x, y;
    CursorPtr	pCursor;
    SetupSpriteScreenPriv(pScreen);
    si_prepareScreen(pScreen);

    pCursor = pSpriteScreenPriv->pCursor;

    if (si_havetruecursor) {
	if (pSpriteScreenPriv->checkPixels)
	  miSpriteFindColors(pScreen);

	si_curs_display(pScreen, pCursor,
			pSpriteScreenPriv->colors[SOURCE_COLOR].pixel,
			pSpriteScreenPriv->colors[MASK_COLOR].pixel);
	return;
    }

    miSpriteComputeSaved (pScreen);

    x = pSpriteScreenPriv->x - (int)pCursor->bits->xhot;
    y = pSpriteScreenPriv->y - (int)pCursor->bits->yhot;
    if ((*pSpriteScreenPriv->funcs->SaveUnderCursor)
	(pScreen,
	 pSpriteScreenPriv->saved.x1,
	 pSpriteScreenPriv->saved.y1,
	 pSpriteScreenPriv->saved.x2 - pSpriteScreenPriv->saved.x1,
	 pSpriteScreenPriv->saved.y2 - pSpriteScreenPriv->saved.y1))
    {
	if (pSpriteScreenPriv->checkPixels)
	    miSpriteFindColors (pScreen);
	if ((*pSpriteScreenPriv->funcs->PutUpCursor)
	    (pScreen, pCursor, x, y,
	     pSpriteScreenPriv->colors[SOURCE_COLOR].pixel,
	     pSpriteScreenPriv->colors[MASK_COLOR].pixel))
	  pSpriteScreenPriv->isUp = TRUE;
    }
}

/*
 * compute the desired area of the screen to save
 */

static void
miSpriteComputeSaved (pScreen)
    ScreenPtr	pScreen;
{
    int		x, y, w, h;
    int		wpad, hpad;
    CursorPtr	pCursor;
    SetupSpriteScreenPriv(pScreen);
    si_prepareScreen(pScreen);

    pCursor = pSpriteScreenPriv->pCursor;
    x = pSpriteScreenPriv->x - (int)pCursor->bits->xhot;
    y = pSpriteScreenPriv->y - (int)pCursor->bits->yhot;
    w = pCursor->bits->width;
    h = pCursor->bits->height;
    wpad = SPRITE_PAD;
    hpad = SPRITE_PAD;
    pSpriteScreenPriv->saved.x1 = x - wpad;
    pSpriteScreenPriv->saved.y1 = y - hpad;
    pSpriteScreenPriv->saved.x2 = pSpriteScreenPriv->saved.x1 + w + wpad * 2;
    pSpriteScreenPriv->saved.y2 = pSpriteScreenPriv->saved.y1 + h + hpad * 2;

    if (si_havefakecursor) {
	/*
	 * We expand the cursor's box here to be aligned on whatever
	 * boundry the driver has requested.  This lets some drivers
	 * work more efficiently.
	 */
	pSpriteScreenPriv->saved.x1 &= ~si_cursormask;
	pSpriteScreenPriv->saved.y1 &= ~si_cursormask;
	pSpriteScreenPriv->saved.x2 |= si_cursormask;
	pSpriteScreenPriv->saved.y2 |= si_cursormask;
    }
}

/*==========================================================================*/
/**** SI start ****/

/* per-cursor per-screen private data */
typedef struct {
    PixmapPtr		sourceBits;	    /* source bits */
    PixmapPtr		maskBits;	    /* mask bits */
    PixmapPtr		invsrcBits;	    /* mask bits */
    int			index;		    /* SI cursor index */
} siDCCursorRec, *siDCCursorPtr;

static CursorPtr  currentCursor = NullCursor;	/* Cursor being displayed */
static SICursor	SC;
static SIbitmap	SIsrc, SIisrc, SImask;

#define GetCursorPrivate(s) (&(GetScreenPrivate(s)->hardwareCursor))
#define SetupCursor(s)	    siCursorPtr pCurPriv = GetCursorPrivate(s)

static Bool
si_curs_realize (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    siDCCursorPtr   pPriv;
    GCPtr	    pGC;
    XID		    gcvals[3];

    if (pCursor->bits->refcnt <= 1)
      pCursor->bits->devPriv[pScreen->myNum] = (pointer)NULL;
    else /* this cursor is already set up */
      return(TRUE);

    pPriv = (siDCCursorPtr) xalloc((unsigned long)sizeof(siDCCursorRec));
    if (!pPriv)
      return(FALSE);

    pPriv->index = 0;

    /*
     * Allocate the pixmaps for the source, mask, and invsrc bitmaps.
     */
    pPriv->sourceBits = (*pScreen->CreatePixmap)
      (pScreen, pCursor->bits->width, pCursor->bits->height, 1);
    if (!pPriv->sourceBits) {
	xfree ((pointer) pPriv);
	return(FALSE);
    }

    pPriv->maskBits =  (*pScreen->CreatePixmap)
      (pScreen, pCursor->bits->width, pCursor->bits->height, 1);

    if (!pPriv->maskBits) {
	(*pScreen->DestroyPixmap) (pPriv->sourceBits);
	xfree ((pointer) pPriv);
	return(FALSE);
    }

    pPriv->invsrcBits =  (*pScreen->CreatePixmap)
      (pScreen, pCursor->bits->width, pCursor->bits->height, 1);

    if (!pPriv->invsrcBits) {
	(*pScreen->DestroyPixmap) (pPriv->sourceBits);
	(*pScreen->DestroyPixmap) (pPriv->maskBits);
	xfree ((pointer) pPriv);
	return(FALSE);
    }

    pCursor->bits->devPriv[pScreen->myNum] = (pointer) pPriv;

    /* create the two sets of bits, clipping as appropriate */

    pGC = GetScratchGC (1, pScreen);
    if (!pGC) {
	(void) si_curs_unrealize(pScreen, pCursor);
	return(FALSE);
    }

    /*
     * Set up the source bitmap
     */
    ValidateGC ((DrawablePtr)pPriv->sourceBits, pGC);
    (*pGC->ops->PutImage) (pPriv->sourceBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, pCursor->bits->source);

    /*
     * Set up the mask bitmap
     */
    ValidateGC ((DrawablePtr)pPriv->sourceBits, pGC);
    (*pGC->ops->PutImage) (pPriv->maskBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, pCursor->bits->mask);

    /* 
     * Set up the inverted source bitmap
     */
    ValidateGC ((DrawablePtr)pPriv->invsrcBits, pGC);
    (*pGC->ops->PutImage) (pPriv->invsrcBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, pCursor->bits->mask);

    gcvals[0] = GXandInverted;
    ChangeGC (pGC, GCFunction, gcvals);
    ValidateGC ((DrawablePtr)pPriv->invsrcBits, pGC);
    (*pGC->ops->PutImage) (pPriv->invsrcBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, pCursor->bits->source);

    FreeScratchGC (pGC);
    return(TRUE);
}

static Bool
si_curs_unrealize (pScreen, pCursor)
  ScreenPtr	pScreen;
  CursorPtr	pCursor;
{
    siDCCursorPtr   pPriv;

    pPriv = (siDCCursorPtr) pCursor->bits->devPriv[pScreen->myNum];
    if (pPriv && (pCursor->bits->refcnt <= 1))
    {
	(*pScreen->DestroyPixmap) (pPriv->sourceBits);
	(*pScreen->DestroyPixmap) (pPriv->maskBits);
	(*pScreen->DestroyPixmap) (pPriv->invsrcBits);
	xfree ((pointer) pPriv);
	pCursor->bits->devPriv[pScreen->myNum] = (pointer)NULL;
    }
    return(TRUE);
}

static void
si_curs_display(pScreen, pCursor, fg, bg)
  ScreenPtr	pScreen;
  CursorPtr	pCursor;
  Pixel		fg;		    /* Foreground color */
  Pixel		bg;		    /* Background color */
{
    siDCCursorPtr   pPriv;
    si_prepareScreen(pScreen);

    pPriv = (siDCCursorPtr) pCursor->bits->devPriv[pScreen->myNum];
    
    SIsrc.BbitsPerPixel = SIisrc.BbitsPerPixel = SImask.BbitsPerPixel = 1;
    SIsrc.Bwidth = SIisrc.Bwidth = SImask.Bwidth = pCursor->bits->width;
    SIsrc.Bheight = SIisrc.Bheight = SImask.Bheight = pCursor->bits->height;
    SImask.Bptr = (SIArray) pPriv->maskBits->devPrivate.ptr;
    SIsrc.Bptr = (SIArray) pPriv->sourceBits->devPrivate.ptr;
    SIisrc.Bptr = (SIArray) pPriv->invsrcBits->devPrivate.ptr;

    SC.SCwidth = SIsrc.Bwidth;
    SC.SCheight = SIsrc.Bheight;
    SC.SCfg = fg;
    SC.SCbg = bg;
    SC.SCmask = &SImask;
    SC.SCsrc = &SIsrc;
    SC.SCinvsrc = &SIisrc;
    if (!si_downloadcursor(pPriv->index, &SC)) {
	FatalError ("Cannot get memory for downloading cursor.");
    };
    si_turnoncursor(pPriv->index);
    return;
}

static void
si_curs_undisplay(pScreen)
  ScreenPtr	pScreen;
{
    si_prepareScreen(pScreen);
    si_turnoffcursor(0);
}

static void
si_curs_move(pScreen, pCursor, x, y)
  ScreenPtr	pScreen;
  CursorPtr	pCursor;
  int x, y;
{
    siDCCursorPtr   pPriv;
    si_prepareScreen(pScreen);

    pPriv = (siDCCursorPtr) pCursor->bits->devPriv[pScreen->myNum];
    
    x -= (int)pCursor->bits->xhot;
    y -= (int)pCursor->bits->yhot;
    si_movecursor(pPriv->index, x, y);
}

/*==========================================================================*/
static Bool
siSpriteRealizeCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    SetupSpriteScreenPriv(pScreen);

    if (pCursor == pSpriteScreenPriv->pCursor)
      pSpriteScreenPriv->checkPixels = TRUE;
    return(si_curs_realize (pScreen, pCursor));
}

static Bool
siSpriteUnrealizeCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    return(si_curs_unrealize(pScreen, pCursor));
}


static void
siSpriteSetCursor (pScreen, pCursor, x, y)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
    int		x, y;
{
    SetupSpriteScreenPriv(pScreen);
    si_prepareScreen(pScreen);

    pSpriteScreenPriv->shouldBeUp = TRUE;
    if (pSpriteScreenPriv->x == x &&
	pSpriteScreenPriv->y == y &&
	pSpriteScreenPriv->pCursor == pCursor &&
	!pSpriteScreenPriv->checkPixels)
      {
	  return;
      }

    if (!pCursor) {
    	pSpriteScreenPriv->shouldBeUp = FALSE;
    	if (pSpriteScreenPriv->isUp)
	    miSpriteRemoveCursor (pScreen);
	pSpriteScreenPriv->pCursor = 0;
	return;
    }

    pSpriteScreenPriv->x = x;
    pSpriteScreenPriv->y = y;
    pSpriteScreenPriv->pCacheWin = NullWindow;
    if (pSpriteScreenPriv->checkPixels ||
	pSpriteScreenPriv->pCursor != pCursor)
      {
	  pSpriteScreenPriv->pCursor = pCursor;
	  miSpriteFindColors (pScreen);
      } else {
	  pSpriteScreenPriv->pCursor = pCursor;
      }

    /*
     * If there's already a cursor up, it may be different than
     * the new one to be displayed, so remove it here.
     */
    if ((pSpriteScreenPriv->isUp) || (miSpriteGeneration > 1))
      miSpriteRemoveCursor(pScreen);

    /*
     * Make sure the cursor is in the right place.
     */
    miSpriteComputeSaved(pScreen, pCursor, x, y);
    si_curs_move(pScreen, pCursor, x, y);

    /*
     * If we have a true hardware cursor, all we need to do is make
     * sure the right cursor is being displayed at the right location.
     */
    if (si_havetruecursor) {
        miSpriteRestoreCursor(pScreen);
        return;
    }

    /*
     * Otherwise, we have to manage some internal data structures first.
     */
    pSpriteScreenPriv->shouldBeUp = TRUE;
    pSpriteScreenPriv->pCacheWin = NullWindow;

    if (!pSpriteScreenPriv->isUp && pSpriteScreenPriv->pCursor)
        miSpriteRestoreCursor (pScreen);

}

static void
siSpriteMoveCursor (pScreen, x, y)
    ScreenPtr	pScreen;
    int		x, y;
{
    SetupSpriteScreenPriv(pScreen);
    si_prepareScreen(pScreen);

    if (si_havetruecursor) {
	si_curs_move(pScreen, pSpriteScreenPriv->pCursor, x, y);
	return;
    }
    miSpriteSetCursor (pScreen, pSpriteScreenPriv->pCursor, x, y);
}
