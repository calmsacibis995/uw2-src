/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/sigc.c	1.8"

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
/* $XConsortium: cfbgc.c,v 5.32.1.1 90/03/21 10:16:47 rws Exp $ */

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "miscstruct.h"
#include "dixfontstr.h"
#include "fontstruct.h"
#include "gc.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "region.h"
#include "regionstr.h"

#include "si.h"
#include "sidep.h"
#include "mistruct.h"
/* #include "mibstore.h" */

#include "simskbits.h"

#define DASHES	TRUE
/*
 * The following variables are shared between the GC routines
 * (Create/Validate) and the GS create/validate routines.
 */

static SIint32	SIDashDef[] = { 4, 4 };
static SIint32	SIDashDefCNT = 2;

/*
 *      function siZeroDashLine () declared as returning (void). 
 */
extern void siZeroDashLine ();

/*
static void siValidateGC(), siChangeGC(), siCopyGC(), siDestroyGC();
static void siChangeClip(), siDestroyClip(), siCopyClip();
static void siDestroyOps();
*/
static void siChangeGC(), siCopyGC();
static void siDestroyOps();

static GCFuncs siGCFuncs = {
    siValidateGC,
    siChangeGC,
    siCopyGC,
    siDestroyGC,
    siChangeClip,
    siDestroyClip,
    siCopyClip,
};

extern void mfbPushPixels ();
extern void siPolyFillArcSolidCopy();
/* extern void siPushPixels8 (); */
extern void miPushPixels ();
int sisetdashes();

/* 
 * SI:  We only have one set of Ops since we don't distinguish
 * between terminal emulator and other fonts.  (We check things at
 * runtime.)
 */

static GCOps siGCOps = {
    siSolidFS,
    siSetSpans,
    miPutImage,
    miCopyArea,
    miCopyPlane,
    miPolyPoint,
    miZeroLine,
    miPolySegment,
    miPolyRectangle,
    miZeroPolyArc,
    miFillPolygon,
    miPolyFillRect,
    miPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    miImageGlyphBlt,
    miPolyGlyphBlt,
    miPushPixels,
    NULL,
};

static GCOps *
matchCommon (pGC)
    GCPtr   pGC;
{
    if (pGC->lineWidth != 0)
        return 0;
    if (pGC->lineStyle != LineSolid)
        return 0;
    if (pGC->fillStyle != FillSolid)
        return 0;
    SET_PSZ(pGC->depth);
    if ((pGC->alu != GXcopy) || ((pGC->planemask & PMSK) != PMSK))
        return 0;
    return &siGCOps;
}

Bool
siCreateGC(pGC)
    register GCPtr pGC;
{
    siPrivGC *pPriv;

    pGC->clientClip = NULL;
    pGC->clientClipType = CT_NONE;

    /*
     * some of the output primitives aren't really necessary, since they
     * will be filled in ValidateGC because of dix/CreateGC() setting all
     * the change bits.  Others are necessary because although they depend
     * on being a color frame buffer, they don't change 
     */

    pGC->funcs = &siGCFuncs;
    pGC->ops = &siGCOps;


    /* si wants to translate before scan conversion */
    pGC->miTranslate = 1;
    pPriv = (siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr);
    /*pPriv->rop = pGC->alu; */
    pPriv->rop = ReduceRop(pGC->alu, pGC->fgPixel);
    pPriv->fExpose = TRUE;
    pPriv->freeCompClip = FALSE;
    pPriv->pRotatedPixmap = (PixmapPtr) NULL;

/* SI: START */

    pPriv->GStateidx = sigetnextstate();
    pPriv->GState.SGpmask = pGC->planemask;
    pPriv->GState.SGmode = pGC->alu;
    switch (pGC->fillStyle) {
    case FillSolid:
	pPriv->GState.SGfillmode = SGFillSolidFG;
		/* next line is has no meaning here */
		/* However may improve on char I/O  */
	pPriv->GState.SGstplmode = SGOPQStipple;
	break;
    case FillTiled:
	pPriv->GState.SGfillmode = SGFillTile;
		/* next line is has no meaning here */
		/* However may improve on char I/O  */
	pPriv->GState.SGstplmode = SGOPQStipple;
	break;
    case FillStippled:
	pPriv->GState.SGfillmode = SGFillStipple;
	pPriv->GState.SGstplmode = SGStipple;
	break;
    case FillOpaqueStippled:
	pPriv->GState.SGfillmode = SGFillStipple;
	pPriv->GState.SGstplmode = SGOPQStipple;
	break;
    default:
	FatalError("siCreateGC: illegal fillStyle\n");
    }
    switch (pGC->lineStyle) { /* for dashes */
      case LineSolid:
	pPriv->GState.SGlinestyle = SGLineSolid;
	break;
      case LineOnOffDash:
	pPriv->GState.SGlinestyle = SGLineDash;
	break;
      case LineDoubleDash:
	pPriv->GState.SGlinestyle = SGLineDblDash;
	break;
      default:
	FatalError("siCreateGC: illegal lineStyle\n");
    }
    pPriv->GState.SGfg = pGC->fgPixel;
    pPriv->GState.SGbg = pGC->bgPixel;
    pPriv->GState.SGcmapidx = 0;
    pPriv->GState.SGtile = &pPriv->GCtile;
    pPriv->GState.SGstipple = &pPriv->GCstpl;

    pPriv->GState.SGlineCNT = 0;
    pPriv->GState.SGline = NULL;

    pPriv->GState.SGcliplist = (SIRectP)0;
    pPriv->GState.SGclipCNT = 0;
    pPriv->GSmodified = 0;
    sivalidatestate(pPriv->GStateidx, &pPriv->GState, SI_TRUE, 0);
/* SI: END */

    return TRUE;
}

/* functions from MIT R4 */

static GCOps *
siCreateOps (prototype)
    GCOps	*prototype;
{
    GCOps	*ret;
    extern Bool	Must_have_memory;

    /* XXX */ Must_have_memory = TRUE;
    ret = (GCOps *) xalloc((unsigned long)sizeof(GCOps));
    /* XXX */ Must_have_memory = FALSE;
    if (!ret)
	return 0;
    *ret = *prototype;
    ret->devPrivate.val = 1;
    return ret;
}

static void
siDestroyOps (ops)
    GCOps   *ops;
{
    if (ops->devPrivate.val)
	Xfree((pointer) ops);
}

/*ARGSUSED*/
static void
siChangeGC(pGC, mask)
    GC              *pGC;
    BITS32          mask;
{
    return;
}

static void
siDestroyGC(pGC)
    GC                  *pGC;
{
    siPrivGC *pPriv;
    SIGStateP 	pGS;

    pPriv = (siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr);

#if 0
    /* These two may be leaking... */
    pGS = &pPriv->GState;
    if (pGS->SGcliplist &&
	pGS->SGcliplist != (SIRectP) REGION_RECTS(pPriv->pCompositeClip)) {
	Xfree((pointer) pGS->SGcliplist);
    }
    if (pGS->SGline &&
	pGS->SGline != SIDashDef) {
	Xfree((pointer) pGS->SGline);
    }
#endif

    if (pPriv->freeCompClip)
        (*pGC->pScreen->RegionDestroy)(pPriv->pCompositeClip);
    siDestroyOps (pGC->ops);
}

static void
siValidateGC(pGC, changes, pDrawable)
    register GCPtr  pGC;
    Mask	    changes;
    DrawablePtr	    pDrawable;
{
    WindowPtr   pWin;
    int         mask;		/* stateChanges */
    int         index;		/* used for stepping through bitfields */
    int         new_line, new_fillspans;
    int		new_rotate;
    /* flags for changing the proc vector */
    /* SI: START */
    int		new_tile = FALSE;
    int		new_stipple = FALSE;
    siPrivGCPtr devPriv;
    SIGStateP 	pGS;
    int		rrop, new_rrop, new_dash;
    si_prepareScreen(pGC->pScreen);
    /* SI: END */

    new_rotate = pGC->lastWinOrg.x != pDrawable->x ||
		 pGC->lastWinOrg.y != pDrawable->y;

    pGC->lastWinOrg.x = pDrawable->x;
    pGC->lastWinOrg.y = pDrawable->y;
    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	pWin = (WindowPtr) pDrawable;
    }
    else
    {
	pWin = (WindowPtr) NULL;
    }

    devPriv = ((siPrivGCPtr) (pGC->devPrivates[siGCPrivateIndex].ptr));
    pGS = &devPriv->GState;

    new_rrop = FALSE;
    new_line = FALSE;
    new_fillspans = FALSE;
    new_dash = FALSE;		/* SI */

    /*
     * if the client clip is different or moved OR the subwindowMode has
     * changed OR the window's clip has changed since the last validation
     * we need to recompute the composite clip 
     */

    if ((changes & (GCClipXOrigin|GCClipYOrigin|GCClipMask|GCSubwindowMode)) ||
	(pDrawable->serialNumber != (pGC->serialNumber & DRAWABLE_SERIAL_BITS))
	)
    {
	ScreenPtr pScreen = pGC->pScreen;

	if (pWin) {
	    RegionPtr   pregWin;
	    Bool        freeTmpClip, freeCompClip;

	    if (pGC->subWindowMode == IncludeInferiors) {
		pregWin = NotClippedByChildren(pWin);
		freeTmpClip = TRUE;
	    }
	    else {
		pregWin = &pWin->clipList;
		freeTmpClip = FALSE;
	    }
	    freeCompClip = devPriv->freeCompClip;

	    /*
	     * if there is no client clip, we can get by with just keeping
	     * the pointer we got, and remembering whether or not should
	     * destroy (or maybe re-use) it later.  this way, we avoid
	     * unnecessary copying of regions.  (this wins especially if
	     * many clients clip by children and have no client clip.) 
	     */
	    if (pGC->clientClipType == CT_NONE) {
		if (freeCompClip)
		    (*pScreen->RegionDestroy) (devPriv->pCompositeClip);
		devPriv->pCompositeClip = pregWin;
		devPriv->freeCompClip = freeTmpClip;
	    }
	    else {
		/*
		 * we need one 'real' region to put into the composite
		 * clip. if pregWin the current composite clip are real,
		 * we can get rid of one. if pregWin is real and the
		 * current composite clip isn't, use pregWin for the
		 * composite clip. if the current composite clip is real
		 * and pregWin isn't, use the current composite clip. if
		 * neither is real, create a new region. 
		 */

		(*pScreen->TranslateRegion)(pGC->clientClip,
					    pDrawable->x + pGC->clipOrg.x,
					    pDrawable->y + pGC->clipOrg.y);
						  
		if (freeCompClip)
		{
		    (*pGC->pScreen->Intersect)(devPriv->pCompositeClip,
					       pregWin, pGC->clientClip);
		    if (freeTmpClip)
			(*pScreen->RegionDestroy)(pregWin);
		}
		else if (freeTmpClip)
		{
		    (*pScreen->Intersect)(pregWin, pregWin, pGC->clientClip);
		    devPriv->pCompositeClip = pregWin;
		}
		else
		{
		    devPriv->pCompositeClip = (*pScreen->RegionCreate)(NullBox,
								       0);
		    (*pScreen->Intersect)(devPriv->pCompositeClip,
					  pregWin, pGC->clientClip);
		}
		devPriv->freeCompClip = TRUE;
		(*pScreen->TranslateRegion)(pGC->clientClip,
					    -(pDrawable->x + pGC->clipOrg.x),
					    -(pDrawable->y + pGC->clipOrg.y));
						  
	    }
	    /* SI: START */
	    /* This code the GDLIB CLIPLIST'S for sivalidategs */
            pGS->SGcliplist = (SIRectP) REGION_RECTS(devPriv->pCompositeClip);
            pGS->SGclipCNT = (SIint32) REGION_NUM_RECTS(devPriv->pCompositeClip);
            devPriv->GSmodified |= SetSGcliplist;
	    /* SI: END   */
	}			/* end of composite clip for a window */
	else {
	    BoxRec      pixbounds;

	    /* XXX should we translate by drawable.x/y here ? */
	    pixbounds.x1 = 0;
	    pixbounds.y1 = 0;
	    pixbounds.x2 = pDrawable->width;
	    pixbounds.y2 = pDrawable->height;

	    if (devPriv->freeCompClip)
		(*pScreen->RegionReset)(devPriv->pCompositeClip, &pixbounds);
	    else {
		devPriv->freeCompClip = TRUE;
		devPriv->pCompositeClip = (*pScreen->RegionCreate)(&pixbounds,
								   1);
	    }

	    if (pGC->clientClipType == CT_REGION)
	    {
		(*pScreen->TranslateRegion)(devPriv->pCompositeClip,
					    -pGC->clipOrg.x, -pGC->clipOrg.y);
		(*pScreen->Intersect)(devPriv->pCompositeClip,
				      devPriv->pCompositeClip,
				      pGC->clientClip);
		(*pScreen->TranslateRegion)(devPriv->pCompositeClip,
					    pGC->clipOrg.x, pGC->clipOrg.y);
	    }
	}			/* end of composite clip for pixmap */
    }

    mask = changes;
    while (mask) {
	index = lowbit (mask);
	mask &= ~index;

	/*
	 * this switch acculmulates a list of which procedures might have
	 * to change due to changes in the GC.  in some cases (e.g.
	 * changing one 16 bit tile for another) we might not really need
	 * a change, but the code is being paranoid. this sort of batching
	 * wins if, for example, the alu and the font have been changed,
	 * or any other pair of items that both change the same thing. 
	 */
	switch (index) {
	case GCFunction:
	    pGS->SGmode = pGC->alu;	/* SI */
	    new_rrop = TRUE;	 	/* SI */	
	    break;			/* SI */
	case GCForeground:
	    new_rrop = TRUE;	 	/* SI */	
	    pGS->SGfg = pGC->fgPixel;	/* SI */
	    break;
	case GCPlaneMask:
	    new_rrop = TRUE;	 	/* SI */	
	    pGS->SGpmask = pGC->planemask;	/* SI */
	    break;
	case GCBackground:
	    new_fillspans = TRUE;
	    new_rrop = TRUE;	 	/* SI */	
	    pGS->SGbg = pGC->bgPixel;	/* SI */
	    break;
	case GCLineStyle:
	    new_dash = TRUE;	 	/* SI */	

	    switch (pGC->lineStyle) { /* dashes */
	      case LineSolid:
		pGS->SGlinestyle = SGLineSolid;
		break;
	      case LineOnOffDash:
		pGS->SGlinestyle = SGLineDash;
		break;
	      case LineDoubleDash:
		pGS->SGlinestyle = SGLineDblDash;
		break;
	      default:
		FatalError("siValidateGC: illegal lineStyle\n");
	    }
	    /* FALLTHROUGH */
	case GCLineWidth:
	case GCJoinStyle:
	    new_line = TRUE;
	    break;
	case GCCapStyle:
	    break;
	case GCFillStyle:
	    new_fillspans = TRUE;
	    new_line = TRUE;

	    /* SI: START */
            switch (pGC->fillStyle) {
            case FillSolid:
                pGS->SGfillmode = SGFillSolidFG;
                break;
            case FillTiled:
                pGS->SGfillmode = SGFillTile;
                break;
            case FillStippled:
                pGS->SGfillmode = SGFillStipple;
                pGS->SGstplmode = SGStipple;
                break;
            case FillOpaqueStippled:
                pGS->SGfillmode = SGFillStipple;
                pGS->SGstplmode = SGOPQStipple;
                break;
            default:
                FatalError("siValidateGC: illegal fillStyle\n");
            }
	    /* SI: END */

	    break;
	case GCFillRule:
	    if (pGC->fillRule == WindingRule)
		pGS->SGfillrule = SGWindingRule;
	    else
		pGS->SGfillrule = SGEvenOddRule;
	    break;
	case GCTile:
	    new_fillspans = TRUE;
	    if (!pGC->tileIsPixel)
		    new_tile = TRUE;
	    break;

	case GCStipple:
	    if (pGC->stipple)
		    new_stipple = TRUE;
	    new_fillspans = TRUE;
	    break;

	case GCTileStipXOrigin:
	    new_rotate = TRUE;
	    break;

	case GCTileStipYOrigin:
	    new_rotate = TRUE;
	    break;

	case GCFont:
	    break;
	case GCSubwindowMode:
	    break;
	case GCGraphicsExposures:
	    break;
	case GCClipXOrigin:
	    break;
	case GCClipYOrigin:
	    break;
	case GCClipMask:
	    break;
	case GCDashOffset:
	case GCDashList:
	    new_dash = TRUE;	/* SI */
	    new_line = TRUE;	/* SI */
	    break;
	case GCArcMode:
	    break;
	default:
	    break;
	}
    }

    /*
     * If the drawable has changed,  check its depth & ensure suitable
     * entries are in the proc vector. 
     */
    if (pDrawable->serialNumber != (pGC->serialNumber & (DRAWABLE_SERIAL_BITS))) {
	new_fillspans = TRUE;	/* deal with FillSpans later */
    }

    /* deal with the changes we've collected */

    if (new_rrop || new_fillspans)
    {
	rrop = ReduceRop(pGC->alu, pGC->fgPixel);
	devPriv->rop = rrop;
	new_fillspans = TRUE;

	/* opaque stipples:
	   fg	bg	ropOpStip	fill style
	   1	0	alu		tile
	   0	1	inverseAlu	tile
	   1	1	rrop(fg, alu)	solid
	   0	0	rrop(fg, alu)	solid
	Note that rrop(fg, alu) == mfbPrivGC.rop, so we don't really need to
	compute it.
	*/
        if (pGC->fillStyle == FillOpaqueStippled)
        {
	    if (pGC->fgPixel != pGC->bgPixel)
		devPriv->ropOpStip = pGC->alu;
	    else
	        devPriv->ropOpStip = rrop;
        }
    }
    /* SI: END */

    if (new_line || new_fillspans)
    {
	GCOps	*newops;

	if (newops = matchCommon (pGC))
 	{
	    if (pGC->ops->devPrivate.val)
		siDestroyOps (pGC->ops);
	    pGC->ops = newops;
	    new_line = new_fillspans = 0;
	}
 	else
 	{
	    if (!pGC->ops->devPrivate.val)
	    {
		pGC->ops = siCreateOps (pGC->ops);
		pGC->ops->devPrivate.val = 1;
	    }
	}
    }

    if (new_line)
    {
	pGC->ops->PolySegment = miPolySegment;
	if (pGC->lineWidth == 0) {
		pGC->ops->Polylines = miZeroLine;	/* SI */
		pGC->ops->PolyArc = miZeroPolyArc;
#ifdef DASHES
		if (new_dash &&
		    (si_hasanydash(SIavail_line) ))
		{

		    /* New Style or Dash list...*/
		    if (sisetdashes(pGC)) {
			devPriv->GSmodified |= SetSGline;
		    }
		}
#endif /* DASHES */
	} else {
	    if (pGC->lineStyle == LineSolid) {
		pGC->ops->Polylines = miWideLine;
	    } else {
		pGC->ops->Polylines = miWideDash;
	    }
	    pGC->ops->PolyArc = miPolyArc;
	}
    }

/*
 * SI:  all glyphs go through the same routines, miPolyGlyphBlt and 
 * miImageGlyphBlt, so we don't need to check anything.
 */

    if (new_fillspans) {
	switch (pGC->fillStyle) {
	case FillSolid:
	    pGC->ops->FillSpans = siSolidFS; 		/* SI */
	    break;
	case FillTiled:
	    pGC->ops->FillSpans = siUnnaturalTileFS;	/* SI */
	    break;
	case FillStippled:
	    pGC->ops->FillSpans = siUnnaturalStippleFS; /* SI */
	    break;
	case FillOpaqueStippled:
	    if (pGC->fgPixel == pGC->bgPixel)
		pGC->ops->FillSpans = siSolidFS;	/* SI */
	    else
		pGC->ops->FillSpans = siUnnaturalStippleFS;	/* SI */
	    break;
	default:
	    FatalError("siValidateGC: illegal fillStyle\n");
	}
    } /* end of new_fillspans */

/* SI: START */
    if (new_rotate || new_tile || new_stipple) {
	int xrot, yrot;
	/*
	 * If we've gotten here, we're probably going to rotate the tile
	 * and/or stipple, so we have to add the pattern origin into
	 * the rotation factor, even if it hasn't changed.
	 */
	xrot = pGC->patOrg.x + pDrawable->x;
	yrot = pGC->patOrg.y + pDrawable->y;
	if (!pGC->tileIsPixel && 
	    (new_tile || (new_rotate && pGC->tile.pixmap) ||
	    (pGC->tile.pixmap && xrot != devPriv->GCtile.BorgX) ||
	    (pGC->tile.pixmap && yrot != devPriv->GCtile.BorgY))) {
	    devPriv->GCtile.BbitsPerPixel = pGC->tile.pixmap->drawable.bitsPerPixel;
	    devPriv->GCtile.Bwidth = pGC->tile.pixmap->drawable.width;
	    devPriv->GCtile.Bheight = pGC->tile.pixmap->drawable.height;
	    devPriv->GCtile.BorgX = xrot;
	    devPriv->GCtile.BorgY = yrot;
	    devPriv->GCtile.Btype = Z_BITMAP;
	    devPriv->GCtile.Bptr = (SIArray)pGC->tile.pixmap->devPrivate.ptr;
	    devPriv->GSmodified |= SetSGtile;
	}
	if (new_stipple || (new_rotate && pGC->stipple) ||
	  ((pGC->stipple) && xrot != devPriv->GCstpl.BorgX) ||
	  (pGC->stipple && yrot != devPriv->GCstpl.BorgY)) {
	    devPriv->GCstpl.BbitsPerPixel = 1;
	    devPriv->GCstpl.Bwidth = pGC->stipple->drawable.width;
	    devPriv->GCstpl.Bheight = pGC->stipple->drawable.height;
	    devPriv->GCstpl.BorgX = xrot;
	    devPriv->GCstpl.BorgY = yrot;
	    devPriv->GCstpl.Btype = Z_BITMAP;
	    devPriv->GCstpl.Bptr = (SIArray)pGC->stipple->devPrivate.ptr;
	    devPriv->GSmodified |= SetSGstipple;
	}
    }

    /*
     * If this GC has ever been used with a window with backing-store enabled,
     * we must call miVaidateBackingStore to keep the backing-store module
     * up-to-date, should this GC be used with that drawable again. In
     * addition, if the current drawable is a window and has backing-store
     * enabled, we also call miValidateBackingStore to give it a chance to get
     * its hooks in.
     */
/*
 * LOOK INTO THIS LATER 
    if (pGC->devBackingStore ||
	(pWin && (pWin->backingStore != NotUseful)))
    {
	miValidateBackingStore(pDrawable, pGC, procChanges);
    }
 */

    /*
     * Any Open Display Interface specific GC updating commands
     */
    if (pWin) {
	sivalidatestate(devPriv->GStateidx, pGS, SI_TRUE, devPriv->GSmodified);
    }
}

/*ARGSUSED*/
static void
siCopyGC (pGCSrc, changes, pGCDst)
    GCPtr       pGCSrc;
    Mask        changes;
    GCPtr       pGCDst;
{
    return;
}

/* 
 * SI Graphic State functions :
 */

/*
 * Get/Set state information subroutines
 */

/*
 * sigetnextstate - returns the next available state index.
 *	CURRENTLY this subroutine does a simple FIFO
 *	choosing. Future improvemnts should change this
 *	to a LRU scheme.
 *
 * Currently, siGC is a pointer in the SIScreenRec to the GC that
 * is currently in use by the index'ed state.  A GC only shares 1 state
 * and is only valid for one screen.
 * If the state does not contain the GC's info at validate time,
 * then the state must be redownloaded.  This NON LRU format
 * may force a lot of downloads.
 */

static SIint32
sigetnextstate()
{
    si_currentScreen();

    if ( ++siNextState >= si_GetInfoVal(SIstatecnt) )
	siNextState = 0;
    return(siNextState);
}

/*
 * sifreestate - clears up any internal info thus freeing a graphics
 * state.  (Future use in LRU management)
 */

void
sifreestate(index, pGS)
  SIint32	index;
  SIGStateP pGS;
{
    si_currentScreen();

    if (siGSCache[index].gs_ptr == pGS) {
	bzero(&siGSCache[index], sizeof(GSCache));
    }
}

/*
 * Initialize the state information tables.  For State management.
 */

void
siinitstates()
{
    SIint32	i;
    si_currentScreen();

    if ((i = si_GetInfoVal(SIstatecnt)) < 1)
	FatalError("Device Dependent Driver MUST provide 1 State");
    if (siGSCache) {
	Xfree((pointer) siGSCache);
	siNextState = -1;
    }
    siGSCache = (GSCacheP)Xalloc((unsigned long)(sizeof(GSCache)*i));
    bzero(siGSCache, sizeof(GSCache)*i);
}

/*
 * Return SI_TRUE if the download should occur.  return SI_FALSE if not.
 */

sicmptile(old, new)
  register SIbitmapP old, new;
{
    int size;

    if (old == new)
	return(SI_FALSE);

    if (new == (SIbitmapP)0)
	return(SI_FALSE);
    if (old == (SIbitmapP)0)
	return(SI_TRUE);
	/* Use new here because old may be unitialized */
    if (new->BbitsPerPixel && new->Bwidth && new->Bheight) {
	size = ( (sizeof(long)*sizeof(char)) / new->BbitsPerPixel) - 1;
	size = new->Bheight * ( (new->Bwidth + size) & ~size );
	switch(new->Bwidth) {
	case 1:
	    size >>= 3; break;
	case 2:
	    size >>= 2; break;
	case 4:
	    size >>= 1; break;
	case 8:
	    break;
	}
    } else
	return(SI_FALSE);
    if (
	(old->BbitsPerPixel == new->BbitsPerPixel) &&
	(old->Bwidth == new->Bwidth) &&
	(old->Bheight == new->Bheight) &&
	(old->BorgX == new->BorgX) &&
	(old->BorgY == new->BorgY) &&
	( (old->Bptr == new->Bptr) ||
	  (bcmp(old->Bptr, new->Bptr, size) == 0)
	)
       )
	return(SI_FALSE);
    return(SI_TRUE);
}

void
sivalidatestate(index, pGS, force, mod)
  SIint32	index;
  register SIGStateP pGS;
  SIBool force;
  SIint32 mod;
{
    SIGStateP	old;
    SIGState	new;
    SIint32	newSGflag = 0;
    SIint32	getGSflag = 0;

    si_currentScreen();

    if (force == SI_FALSE && siGSCache[index].gs_ptr == pGS)
	return;			 /* This state already validated */

    getGSflag = siGSCache[index].gs_validval;
    getGSflag = (
		GetSGpmask | GetSGmode | GetSGstplmode |
		GetSGfillmode | GetSGlinestyle | GetSGfg |
		 GetSGbg | GetSGcmapidx
		);
    old = &siGSCache[index].gs_cur;

    /* This line is here because older SDD's are not
       prepared to handle info about linestyle and should not
       be repeatedly sent any linestyle changes */
    old->SGlinestyle = pGS->SGlinestyle;

    if (si_getstateinfo(index, getGSflag, old) == SI_FAIL)
	FatalError( "Can't get State Info" );
    /* Check each value, and only change the differences */
    newSGflag = 0;
    if (old->SGpmask != pGS->SGpmask) {
	newSGflag |= SetSGpmask;
	new.SGpmask = pGS->SGpmask;
	old->SGpmask = pGS->SGpmask;
    }
    if (old->SGmode != pGS->SGmode) {
	newSGflag |= SetSGmode;
	new.SGmode = pGS->SGmode;
	old->SGmode = pGS->SGmode;
    }
    if (old->SGstplmode != pGS->SGstplmode) {
	newSGflag |= SetSGstplmode;
	new.SGstplmode = pGS->SGstplmode;
	old->SGstplmode = pGS->SGstplmode;
    }
    if (old->SGfillmode != pGS->SGfillmode) {
	newSGflag |= SetSGfillmode;
	new.SGfillmode = pGS->SGfillmode;
	old->SGfillmode = pGS->SGfillmode;
    }
    if (old->SGfg != pGS->SGfg) {
	newSGflag |= SetSGfg;
	new.SGfg = pGS->SGfg;
	old->SGfg = pGS->SGfg;
    }
    if (old->SGbg != pGS->SGbg) {
	newSGflag |= SetSGbg;
	new.SGbg = pGS->SGbg;
	old->SGbg = pGS->SGbg;
    }
    if (old->SGcmapidx != pGS->SGcmapidx) {
	newSGflag |= SetSGcmapidx;
	new.SGcmapidx = pGS->SGcmapidx;
	old->SGcmapidx = pGS->SGcmapidx;
    }
    /* Checks to determine of the tiles and stipples should be
     * downloaded need to be done here.  This is an area for
     * later improvement.  Currently always download them.
     */
    if (mod & SetSGtile) {
	/* Force BorgX and BorgY to be positive */
	while(pGS->SGtile->BorgX < 0)
		pGS->SGtile->BorgX += pGS->SGtile->Bwidth;
	while(pGS->SGtile->BorgY < 0)
		pGS->SGtile->BorgY += pGS->SGtile->Bheight;
	newSGflag |= SetSGtile;
	new.SGtile = pGS->SGtile;
	old->SGtile = pGS->SGtile;
    }
    if (mod & SetSGstipple) {
	/* Force BorgX and BorgY to be positive */
	while(pGS->SGstipple->BorgX < 0)
		pGS->SGstipple->BorgX += pGS->SGstipple->Bwidth;
	while(pGS->SGstipple->BorgY < 0)
		pGS->SGstipple->BorgY += pGS->SGstipple->Bheight;
	newSGflag |= SetSGstipple;
	new.SGstipple = pGS->SGstipple;
	old->SGstipple = pGS->SGstipple;
    }
    if ((mod & SetSGcliplist) && si_hasanycliplist) {
	register SIRectP clp, clp2;
	int	clpcnt;

	newSGflag |= SetSGcliplist;
	new.SGclipCNT = old->SGclipCNT = clpcnt = pGS->SGclipCNT;
	if (siGSCache[index].gs_clipsz == 0) {
	    clp = (SIRectP) Xalloc((unsigned long)(clpcnt*sizeof(SIRect)));
	    siGSCache[index].gs_clipsz = clpcnt;
	} else if (siGSCache[index].gs_clipsz < clpcnt) {
	    clp = (SIRectP) Xrealloc((pointer)old->SGcliplist, (unsigned long)
				     (clpcnt*sizeof(SIRect)));
	    siGSCache[index].gs_clipsz = clpcnt;
	} else {
	    clp = old->SGcliplist;
	}
	new.SGcliplist = old->SGcliplist = clp;
	clp2 = pGS->SGcliplist;
	while (--clpcnt >= 0) {		/* Update the offsets */
		*clp = *clp2++;
		clp->lr.x -= 1;
		clp->lr.y -= 1;
		clp++;
	}
    }
    if (old->SGlinestyle != pGS->SGlinestyle) { /* dashes */
	newSGflag |= SetSGlinestyle;
	new.SGlinestyle = pGS->SGlinestyle;
	old->SGlinestyle = pGS->SGlinestyle;
    }
    if (mod & SetSGline) {
	newSGflag |= SetSGline;
	new.SGlineCNT = pGS->SGlineCNT;
	new.SGline = pGS->SGline;
	old->SGlineCNT = pGS->SGlineCNT;
	old->SGline = pGS->SGline;
    }

    if (newSGflag == 0) {	/* Don't do anything.  Current is identical */
	siGSCache[index].gs_ptr = pGS;
	return;
    }
    /* Update the state to the new values */
    if (si_downloadstate(index, newSGflag, &new) == SI_FAIL)
	FatalError( "Can't set State Info" );
    siGSCache[index].gs_ptr = pGS;
    siGSCache[index].gs_validval |= getGSflag;
    return;
}


/*
 * Setup The GS used for a temp OPS.
 */

int
sisettempgs(pGS, pTile, pStpl)
  register SIGStateP pGS;
  register SIbitmapP pTile, pStpl;
{
    int idx = sigetnextstate();
    SIVisualP pVisuals;			/* SI */
    si_currentScreen();

    pVisuals = si_GetInfoVal(SIvisuals);
    pGS->SGpmask = (~0U >> (32 - pVisuals->SVdepth));
    pGS->SGmode = GXcopy;
    pGS->SGstplmode = SGOPQStipple;
    pGS->SGfillmode = SGFillSolidFG;
    pGS->SGfillrule = SGEvenOddRule;
    pGS->SGlinestyle = LineSolid;	/* added */
    pGS->SGfg = 0;
    pGS->SGbg = 1;
    pGS->SGcmapidx = 0;
    pGS->SGtile = pTile;
    pGS->SGstipple = pStpl;
    pTile->Bptr = (SIArray)0;
    pStpl->Bptr = (SIArray)0;
    pTile->Btype = Z_BITMAP;
    pStpl->Btype = Z_BITMAP;
    pGS->SGlineCNT = 0;
    pGS->SGline = NULL;
    pGS->SGcliplist = (SIRectP)0;	/* added */
    pGS->SGclipCNT = 0;			/* added */
    return(idx);
}

static void
sisetlinedashes(pGS,cnt,list)
  SIGStateP pGS;
  int cnt;
  SIint32 *list;
{
    if (pGS->SGline && pGS->SGline != list &&
	pGS->SGline != SIDashDef) {
	Xfree((pointer) pGS->SGline);
    }
    pGS->SGlineCNT = cnt;
    pGS->SGline = list;
}

/* returns 1 if we have a new dash pattern */
static int
sisetdashes(pGC)
  GCPtr   pGC;
{
    siPrivGCPtr devPriv;
    SIGStateP pGS;
    int ndash = pGC->numInDashList;
    /*
     *      (int) off  defined as (unsigned int)  as it is being assigned an
     *	(unsigned short) pGC->dashOffset
     */
    unsigned int offset = pGC->dashOffset;	
    int odd_start;
    unsigned char *pdash = pGC->dash;
    int j, i, cnt;
    SIint32 *list, *clist;

    devPriv = ((siPrivGCPtr) (pGC->devPrivates[siGCPrivateIndex].ptr));
    pGS = &devPriv->GState;

    if (pGC->lineStyle == LineSolid) {
#if 0
	sisetlinedashes(pGS,SIDashSolidCNT,SIDashSolid);
#endif
	return(0);
    }

    list = (SIint32 *) Xalloc((unsigned long)(sizeof(SIint32)*(ndash+4)));
    clist = list;

#if 0
    ErrorF("[offset=%d, ndash=%d] [ ",offset,ndash);
    for (i=0; i < ndash; ++i) ErrorF("%d ",pdash[i]);
    ErrorF("]\n");
#endif

    /* find beginning of dash pattern */
    i = 0;
    while(offset > pdash[i]) {
	offset -= pdash[i];
	if (++i >= ndash)
	  i = 0;
    }
    odd_start = i & 1;

    /* build line list */

    /* if offset left us on an ODD, pad out the EVEN */
    if (odd_start) {
	*clist++ = 0;
    }
    /* if offset has anything left, start with leftover */
    if (offset) {
	*clist++ = pdash[i] - offset;
	if (++i >= ndash)
	  i = 0;
	--ndash;
    }
    /* add the bulk of the dashlist */
    for(j=0; j < ndash; ++j) {
	*clist++ = pdash[i];
	if (++i >= ndash)
	  i = 0;
    }
    /* if we did the offset before, stick the offset on the end */
    if (offset) {
	/* get to the right EVEN/ODD place */
	if (((i & 1) && !odd_start) ||
	    (!(i & 1) && odd_start)) {
	    *clist++ = 0;
	}
	*clist++ = offset;
    }
    cnt = clist - list;

#if 0
    ErrorF("[cnt=%d] [ ",cnt);
    for (i=0; i < cnt; ++i) ErrorF("%d ",list[i]);
    ErrorF("]\n");
#endif

    /* Identity check */
    if (cnt != pGS->SGlineCNT) {
	sisetlinedashes(pGS,cnt,list);
	return(1);
    }

    for (j=0; j < cnt; ++j) {
	if (pGS->SGline[j] != list[j]) {
	    sisetlinedashes(pGS,cnt,list);
	    return(1);
	}
    }

    /* what we already had was identical,
       don't bother to download it again */
    Xfree((pointer) list);
    return(0);
}

