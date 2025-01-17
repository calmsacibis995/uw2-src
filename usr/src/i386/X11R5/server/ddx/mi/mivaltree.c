/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/mi/mivaltree.c	1.3"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/* $XConsortium: mivaltree.c,v 5.28 91/07/10 15:19:52 keith Exp $ */
/*
 * mivaltree.c --
 *	Functions for recalculating window clip lists. Main function
 *	is miValidateTree.
 *
 * Copyright 1987, 1988, 1989 by 
 * Digital Equipment Corporation, Maynard, Massachusetts,
 * and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
 * 
 *                         All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the names of Digital or MIT not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  
 * 
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * 
 ******************************************************************/

 /* 
  * Aug '86: Susan Angebranndt -- original code
  * July '87: Adam de Boor -- substantially modified and commented
  * Summer '89: Joel McCormack -- so fast you wouldn't believe it possible.
  *             In particular, much improved code for window mapping and
  *             circulating.
  *		Bob Scheifler -- avoid miComputeClips for unmapped windows,
  *				 valdata changes
  */

#include    "X.h"
#include    "scrnintstr.h"
#include    "validate.h"
#include    "windowstr.h"
#include    "mi.h"
#include    "regionstr.h"

#ifdef SHAPE
/*
 * Compute the visibility of a shaped window
 */
miShapedWindowIn (pScreen, universe, bounding, rect, x, y)
    ScreenPtr	pScreen;
    RegionPtr	universe, bounding;
    BoxPtr	rect;
    register int x, y;
{
    BoxRec  box;
    register BoxPtr  boundBox;
    int	    nbox;
    Bool    someIn, someOut;
    register int t, x1, y1, x2, y2;
    int	    (*RectIn)();

    RectIn = pScreen->RectIn;
    nbox = REGION_NUM_RECTS (bounding);
    boundBox = REGION_RECTS (bounding);
    someIn = someOut = FALSE;
    x1 = rect->x1;
    y1 = rect->y1;
    x2 = rect->x2;
    y2 = rect->y2;
    while (nbox--)
    {
	if ((t = boundBox->x1 + x) < x1)
	    t = x1;
	box.x1 = t;
	if ((t = boundBox->y1 + y) < y1)
	    t = y1;
	box.y1 = t;
	if ((t = boundBox->x2 + x) > x2)
	    t = x2;
	box.x2 = t;
	if ((t = boundBox->y2 + y) > y2)
	    t = y2;
	box.y2 = t;
	if (box.x1 > box.x2)
	    box.x2 = box.x1;
	if (box.y1 > box.y2)
	    box.y2 = box.y1;
	switch ((*RectIn) (universe, &box))
	{
	case rgnIN:
	    if (someOut)
		return rgnPART;
	    someIn = TRUE;
	    break;
	case rgnOUT:
	    if (someIn)
		return rgnPART;
	    someOut = TRUE;
	    break;
	default:
	    return rgnPART;
	}
	boundBox++;
    }
    if (someIn)
	return rgnIN;
    return rgnOUT;
}
#endif

/*-
 *-----------------------------------------------------------------------
 * miComputeClips --
 *	Recompute the clipList, borderClip, exposed and borderExposed
 *	regions for pParent and its children. Only viewable windows are
 *	taken into account.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	clipList, borderClip, exposed and borderExposed are altered.
 *	A VisibilityNotify event may be generated on the parent window.
 *
 *-----------------------------------------------------------------------
 */

static void
miComputeClips (pParent, pScreen, universe, kind, exposed)
    register WindowPtr	pParent;
    register ScreenPtr	pScreen;
    register RegionPtr	universe;
    VTKind		kind;
    RegionPtr		exposed; /* for intermediate calculations */
{
    int			dx,
			dy;
    RegionRec		childUniverse;
    register WindowPtr	pChild;
    int     	  	oldVis, newVis;
    BoxRec		borderSize;
    RegionRec		childUnion;
    Bool		overlap;
    RegionPtr		borderVisible;
    Bool		resized;
    
    /*
     * Figure out the new visibility of this window.
     * The extent of the universe should be the same as the extent of
     * the borderSize region. If the window is unobscured, this rectangle
     * will be completely inside the universe (the universe will cover it
     * completely). If the window is completely obscured, none of the
     * universe will cover the rectangle.
     */

    borderSize.x1 = pParent->drawable.x - wBorderWidth(pParent);
    borderSize.y1 = pParent->drawable.y - wBorderWidth(pParent);
    dx = (int) pParent->drawable.x + (int) pParent->drawable.width + wBorderWidth(pParent);
    if (dx > 32767)
	dx = 32767;
    borderSize.x2 = dx;
    dy = (int) pParent->drawable.y + (int) pParent->drawable.height + wBorderWidth(pParent);
    if (dy > 32767)
	dy = 32767;
    borderSize.y2 = dy;

    oldVis = pParent->visibility;
    switch ((* pScreen->RectIn) (universe, &borderSize)) 
    {
	case rgnIN:
	    newVis = VisibilityUnobscured;
	    break;
	case rgnPART:
	    newVis = VisibilityPartiallyObscured;
#ifdef SHAPE
	    {
		RegionPtr   pBounding;

		pBounding = wBoundingShape (pParent);
		if (pBounding)
		{
		    switch (miShapedWindowIn (pScreen, universe, pBounding,
					      &borderSize,
					      pParent->drawable.x,
 					      pParent->drawable.y))
		    {
		    case rgnIN:
			newVis = VisibilityUnobscured;
			break;
		    case rgnOUT:
			newVis = VisibilityFullyObscured;
			break;
		    }
		}
	    }
#endif
	    break;
	default:
	    newVis = VisibilityFullyObscured;
	    break;
    }
    pParent->visibility = newVis;
    if (oldVis != newVis &&
	((pParent->eventMask | wOtherEventMasks(pParent)) & VisibilityChangeMask))
	SendVisibilityNotify(pParent);

    dx = pParent->drawable.x - pParent->valdata->before.oldAbsCorner.x;
    dy = pParent->drawable.y - pParent->valdata->before.oldAbsCorner.y;

    /*
     * avoid computations when dealing with simple operations
     */

    switch (kind) {
    case VTMap:
    case VTStack:
    case VTUnmap:
	break;
    case VTMove:
	if ((oldVis == newVis) &&
	    ((oldVis == VisibilityFullyObscured) ||
	     (oldVis == VisibilityUnobscured)))
	{
	    pChild = pParent;
	    while (1)
	    {
		if (pChild->viewable)
		{
		    if (pChild->valdata)
		    {
			(* pScreen->RegionInit) (&pChild->valdata->after.borderExposed,
						 NullBox, 0);
			(* pScreen->RegionInit) (&pChild->valdata->after.exposed,
						 NullBox, 0);
		    }
		    if (pChild->visibility != VisibilityFullyObscured)
		    {
			(* pScreen->TranslateRegion) (&pChild->borderClip,
						      dx, dy);
			(* pScreen->TranslateRegion) (&pChild->clipList,
						      dx, dy);
			pChild->drawable.serialNumber = NEXT_SERIAL_NUMBER;
			if (pScreen->ClipNotify)
			    (* pScreen->ClipNotify) (pChild, dx, dy);

		    }
		    if (pChild->firstChild)
		    {
			pChild = pChild->firstChild;
			continue;
		    }
		}
		while (!pChild->nextSib && (pChild != pParent))
		    pChild = pChild->parent;
		if (pChild == pParent)
		    break;
		pChild = pChild->nextSib;
	    }
	    return;
	}
	/* fall through */
    default:
    	/*
     	 * To calculate exposures correctly, we have to translate the old
     	 * borderClip and clipList regions to the window's new location so there
     	 * is a correspondence between pieces of the new and old clipping regions.
     	 */
    	if (dx || dy) 
    	{
	    /*
	     * We translate the old clipList because that will be exposed or copied
	     * if gravity is right.
	     */
	    (* pScreen->TranslateRegion) (&pParent->borderClip, dx, dy);
	    (* pScreen->TranslateRegion) (&pParent->clipList, dx, dy);
    	} 
	break;
    }

    borderVisible = pParent->valdata->before.borderVisible;
    resized = pParent->valdata->before.resized;
    (* pScreen->RegionInit) (&pParent->valdata->after.borderExposed, NullBox, 0);
    (* pScreen->RegionInit) (&pParent->valdata->after.exposed, NullBox, 0);

    /*
     * Since the borderClip must not be clipped by the children, we do
     * the border exposure first...
     *
     * 'universe' is the window's borderClip. To figure the exposures, remove
     * the area that used to be exposed from the new.
     * This leaves a region of pieces that weren't exposed before.
     */

    if (HasBorder (pParent))
    {
    	if (borderVisible)
    	{
	    /*
	     * when the border changes shape, the old visible portions
	     * of the border will be saved by DIX in borderVisible --
	     * use that region and destroy it
	     */
	    (* pScreen->Subtract) (exposed, universe, borderVisible);
	    (* pScreen->RegionDestroy) (borderVisible);
    	}
    	else
    	{
	    (* pScreen->Subtract) (exposed, universe, &pParent->borderClip);
    	}
    	(* pScreen->Subtract) (&pParent->valdata->after.borderExposed,
			       exposed, &pParent->winSize);

    	(* pScreen->RegionCopy) (&pParent->borderClip, universe);
    
    	/*
     	 * To get the right clipList for the parent, and to make doubly sure
     	 * that no child overlaps the parent's border, we remove the parent's
     	 * border from the universe before proceeding.
     	 */
    
    	(* pScreen->Intersect) (universe, universe, &pParent->winSize);
    }
    else
    	(* pScreen->RegionCopy) (&pParent->borderClip, universe);
    
    pChild = pParent->firstChild;
    if (pChild && pParent->mapped)
    {
	(*pScreen->RegionInit) (&childUniverse, NullBox, 0);
	(*pScreen->RegionInit) (&childUnion, NullBox, 0);
	if ((pChild->drawable.y < pParent->lastChild->drawable.y) ||
	    ((pChild->drawable.y == pParent->lastChild->drawable.y) &&
	     (pChild->drawable.x < pParent->lastChild->drawable.x)))
	{
	    for (; pChild; pChild = pChild->nextSib)
	    {
		if (pChild->viewable)
		    (* pScreen->RegionAppend)(&childUnion, &pChild->borderSize);
	    }
	}
	else
	{
	    for (pChild = pParent->lastChild; pChild; pChild = pChild->prevSib)
	    {
		if (pChild->viewable)
		    (* pScreen->RegionAppend)(&childUnion, &pChild->borderSize);
	    }
	}
	(* pScreen->RegionValidate)(&childUnion, &overlap);

	for (pChild = pParent->firstChild;
	     pChild;
	     pChild = pChild->nextSib)
 	{
	    if (pChild->viewable) {
		/*
		 * If the child is viewable, we want to remove its extents
		 * from the current universe, but we only re-clip it if
		 * it's been marked.
		 */
		if (pChild->valdata) {
		    /*
		     * Figure out the new universe from the child's
		     * perspective and recurse.
		     */
		    (* pScreen->Intersect) (&childUniverse,
					    universe,
					    &pChild->borderSize);
		    miComputeClips (pChild, pScreen, &childUniverse, kind,
				    exposed);
		}
		/*
		 * Once the child has been processed, we remove its extents
		 * from the current universe, thus denying its space to any
		 * other sibling.
		 */
		if (overlap)
		    (* pScreen->Subtract)
			(universe, universe, &pChild->borderSize);
	    }
	}
	if (!overlap)
	    (* pScreen->Subtract) (universe, universe, &childUnion);
	(* pScreen->RegionUninit) (&childUnion);
	(* pScreen->RegionUninit) (&childUniverse);
    } /* if any children */

    /*
     * 'universe' now contains the new clipList for the parent window.
     *
     * To figure the exposure of the window we subtract the old clip from the
     * new, just as for the border.
     */

    if (oldVis == VisibilityFullyObscured ||
	oldVis == VisibilityNotViewable)
    {
	(* pScreen->RegionCopy) (&pParent->valdata->after.exposed,
				 universe);
    }
    else if (newVis != VisibilityFullyObscured &&
	     newVis != VisibilityNotViewable)
    {
    	(* pScreen->Subtract) (&pParent->valdata->after.exposed,
			       universe, &pParent->clipList);
    }

    /*
     * One last thing: backing storage. We have to try to save what parts of
     * the window are about to be obscured. We can just subtract the universe
     * from the old clipList and get the areas that were in the old but aren't
     * in the new and, hence, are about to be obscured.
     */
    if (pParent->backStorage && !resized)
    {
	(* pScreen->Subtract) (exposed, &pParent->clipList, universe);
	(* pScreen->SaveDoomedAreas)(pParent, exposed, dx, dy);
    }
    
    /* HACK ALERT - copying contents of regions, instead of regions */
    {
	RegionRec   tmp;

	tmp = pParent->clipList;
	pParent->clipList = *universe;
	*universe = tmp;
    }

#ifdef NOTDEF
    (* pScreen->RegionCopy) (&pParent->clipList, universe);
#endif

    pParent->drawable.serialNumber = NEXT_SERIAL_NUMBER;

    if (pScreen->ClipNotify)
	(* pScreen->ClipNotify) (pParent, dx, dy);
}

static void
miTreeObscured(pParent)
    register WindowPtr pParent;
{
    register WindowPtr pChild;
    register int    oldVis;

    pChild = pParent;
    while (1)
    {
	if (pChild->viewable)
	{
	    oldVis = pChild->visibility;
	    if (oldVis != (pChild->visibility = VisibilityFullyObscured) &&
		((pChild->eventMask | wOtherEventMasks(pChild)) & VisibilityChangeMask))
		SendVisibilityNotify(pChild);
	    if (pChild->firstChild)
	    {
		pChild = pChild->firstChild;
		continue;
	    }
	}
	while (!pChild->nextSib && (pChild != pParent))
	    pChild = pChild->parent;
	if (pChild == pParent)
	    break;
	pChild = pChild->nextSib;
    }
}

/*-
 *-----------------------------------------------------------------------
 * miValidateTree --
 *	Recomputes the clip list for pParent and all its inferiors.
 *
 * Results:
 *	Always returns 1.
 *
 * Side Effects:
 *	The clipList, borderClip, exposed, and borderExposed regions for
 *	each marked window are altered.
 *
 * Notes:
 *	This routine assumes that all affected windows have been marked
 *	(valdata created) and their winSize and borderSize regions
 *	adjusted to correspond to their new positions. The borderClip and
 *	clipList regions should not have been touched.
 *
 *	The top-most level is treated differently from all lower levels
 *	because pParent is unchanged. For the top level, we merge the
 *	regions taken up by the marked children back into the clipList
 *	for pParent, thus forming a region from which the marked children
 *	can claim their areas. For lower levels, where the old clipList
 *	and borderClip are invalid, we can't do this and have to do the
 *	extra operations done in miComputeClips, but this is much faster
 *	e.g. when only one child has moved...
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
miValidateTree (pParent, pChild, kind)
    WindowPtr	  	pParent;    /* Parent to validate */
    WindowPtr	  	pChild;     /* First child of pParent that was
				     * affected */
    VTKind    	  	kind;       /* What kind of configuration caused call */
{
    RegionRec	  	totalClip;  /* Total clipping region available to
				     * the marked children. pParent's clipList
				     * merged with the borderClips of all
				     * the marked children. */
    RegionRec	  	childClip;  /* The new borderClip for the current
				     * child */
    RegionRec		childUnion; /* the space covered by borderSize for
				     * all marked children */
    RegionRec		exposed;    /* For intermediate calculations */
    register ScreenPtr	pScreen;
    register WindowPtr	pWin;
    Bool		overlap;
    int			viewvals;
    Bool		forward;

    pScreen = pParent->drawable.pScreen;
    if (pChild == NullWindow)
	pChild = pParent->firstChild;

    (*pScreen->RegionInit) (&childClip, NullBox, 0);
    (*pScreen->RegionInit) (&exposed, NullBox, 0);

    /*
     * compute the area of the parent window occupied
     * by the marked children + the parent itself.  This
     * is the area which can be divied up among the marked
     * children in their new configuration.
     */
    (*pScreen->RegionInit) (&totalClip, NullBox, 0);
    viewvals = 0;
    if ((pChild->drawable.y < pParent->lastChild->drawable.y) ||
	((pChild->drawable.y == pParent->lastChild->drawable.y) &&
	 (pChild->drawable.x < pParent->lastChild->drawable.x)))
    {
	forward = TRUE;
	for (pWin = pChild; pWin; pWin = pWin->nextSib)
	{
	    if (pWin->valdata)
	    {
		(* pScreen->RegionAppend) (&totalClip, &pWin->borderClip);
		if (pWin->viewable)
		    viewvals++;
	    }
	}
    }
    else
    {
	forward = FALSE;
	pWin = pParent->lastChild;
	while (1)
	{
	    if (pWin->valdata)
	    {
		(* pScreen->RegionAppend) (&totalClip, &pWin->borderClip);
		if (pWin->viewable)
		    viewvals++;
	    }
	    if (pWin == pChild)
		break;
	    pWin = pWin->prevSib;
	}
    }
    (* pScreen->RegionValidate)(&totalClip, &overlap);

    /*
     * Now go through the children of the root and figure their new
     * borderClips from the totalClip, passing that off to miComputeClips
     * to handle recursively. Once that's done, we remove the child
     * from the totalClip to clip any siblings below it.
     */

    overlap = TRUE;
    if (kind != VTStack)
    {
	(* pScreen->Union) (&totalClip, &totalClip, &pParent->clipList);
	if (viewvals > 1)
	{
	    /*
	     * precompute childUnion to discover whether any of them
	     * overlap.  This seems redundant, but performance studies
	     * have demonstrated that the cost of this loop is
	     * lower than the cost of multiple Subtracts in the
	     * loop below.
	     */
	    (*pScreen->RegionInit) (&childUnion, NullBox, 0);
	    if (forward)
	    {
		for (pWin = pChild; pWin; pWin = pWin->nextSib)
		    if (pWin->valdata && pWin->viewable)
			(* pScreen->RegionAppend) (&childUnion,
						   &pWin->borderSize);
	    }
	    else
	    {
		pWin = pParent->lastChild;
		while (1)
		{
		    if (pWin->valdata && pWin->viewable)
			(* pScreen->RegionAppend) (&childUnion,
						   &pWin->borderSize);
		    if (pWin == pChild)
			break;
		    pWin = pWin->prevSib;
		}
	    }
	    (*pScreen->RegionValidate)(&childUnion, &overlap);
	    if (overlap)
		(*pScreen->RegionUninit) (&childUnion);
	}
    }

    for (pWin = pChild;
	 pWin != NullWindow;
	 pWin = pWin->nextSib)
    {
	if (pWin->viewable) {
	    if (pWin->valdata) {
		(* pScreen->Intersect) (&childClip,
					&totalClip,
 					&pWin->borderSize);
		miComputeClips (pWin, pScreen, &childClip, kind, &exposed);
		if (overlap)
		{
		    (* pScreen->Subtract) (&totalClip,
				       	   &totalClip,
				       	   &pWin->borderSize);
		}
	    } else if (pWin->visibility == VisibilityNotViewable) {
		miTreeObscured(pWin);
	    }
	} else {
	    if (pWin->valdata) {
		(* pScreen->RegionEmpty)(&pWin->clipList);
		(* pScreen->RegionEmpty)(&pWin->borderClip);
		pWin->valdata = (ValidatePtr)NULL;
	    }
	}
    }

    (* pScreen->RegionUninit) (&childClip);
    if (!overlap)
    {
	(*pScreen->Subtract)(&totalClip, &totalClip, &childUnion);
	(*pScreen->RegionUninit) (&childUnion);
    }

    (* pScreen->RegionInit) (&pParent->valdata->after.exposed, NullBox, 0);
    (* pScreen->RegionInit) (&pParent->valdata->after.borderExposed, NullBox, 0);

    /*
     * each case below is responsible for updating the
     * clipList and serial number for the parent window
     */

    switch (kind) {
    case VTStack:
	break;
    default:
	/*
	 * totalClip contains the new clipList for the parent. Figure out
	 * exposures and obscures as per miComputeClips and reset the parent's
	 * clipList.
	 */
	(* pScreen->Subtract) (&pParent->valdata->after.exposed,
			       &totalClip, &pParent->clipList);
	/* FALLTHROUGH */
    case VTMap:
	if (pParent->backStorage) {
	    (* pScreen->Subtract) (&exposed, &pParent->clipList, &totalClip);
	    (* pScreen->SaveDoomedAreas)(pParent, &exposed, 0, 0);
	}
	
	(* pScreen->RegionCopy) (&pParent->clipList, &totalClip);
	pParent->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	break;
    }

    (* pScreen->RegionUninit) (&totalClip);
    (* pScreen->RegionUninit) (&exposed);
    if (pScreen->ClipNotify)
	(*pScreen->ClipNotify) (pParent, 0, 0);
    return (1);
}
